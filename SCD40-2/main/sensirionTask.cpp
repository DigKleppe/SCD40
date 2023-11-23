/*
 * sensirionTask.cpp
 *
 *  Created on: Jan 5, 2022
 *      Author: dig
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "mdns.h"

#include "sensirionTask.h"
#include "udpClient.h"
#include "settings.h"
#include "wifiConnect.h"

#include "scd40.h"
int16_t scd4x_start_periodic_measurement();
extern bool inLowPowerMode;

//#include "scd4x_i2c.h"
//#include "sensirion_common.h"
//#include "sensirion_i2c_hal.h"

#include "cgiScripts.h"

#include <math.h>
#include <string.h>
#include "averager.h"


#define MAXRETRIES 	5
#define SCD30_TIMEOUT 600

static const char *TAG = "sensirionTask";

bool sensorDataIsSend = false;

TaskHandle_t SensirionTaskh;

extern int scriptState;
extern SemaphoreHandle_t I2CSemaphore;  // used by lvgl, shares the same bus

Averager temperatureAvg(AVGERAGESAMPLES);
Averager humAvg(AVGERAGESAMPLES);
Averager CO2Avg(AVGERAGESAMPLES);

typedef struct {
	int32_t timeStamp;
	float temperature;
	float hum;
	float co2;
} log_t;

static log_t tLog[ MAXLOGVALUES];
static log_t lastVal;
//static log_t rawLastVal;
static int timeStamp = 0;

static int logTxIdx;
static int logRxIdx;

void testLog(void) {
//	logTxIdx = 0;
	for (int p = 0; p < 20; p++) {
		tLog[logTxIdx].timeStamp = timeStamp++;
		tLog[logTxIdx].temperature = 10 + (float) p / 10.0;
		tLog[logTxIdx].hum = 20 + (float) p / 3.0;
		tLog[logTxIdx].co2 = 200 + p;
		logTxIdx++;
		if (logTxIdx >= MAXLOGVALUES)
			logTxIdx = 0;
	}
//
//	scriptState = 0;
//	do {
//		len = getLogScript(buf, 50);
//		buf[len] = 0;
//		printf("%s\r",buf);
//	} while (len);
//
//	for (int p = 0; p < 5; p++) {
//
//		tLog[logTxIdx].timeStamp = timeStamp++;
//		for (int n = 0; n < NR_NTCS; n++) {
//
//			tLog[logTxIdx].temperature[n] = p + n;
//		}
//		tLog[logTxIdx].refTemperature = tmpTemperature; // from I2C TMP117
//		logTxIdx++;
//		if (logTxIdx >= MAXLOGVALUES )
//			logTxIdx = 0;
//	}
//	do {
//		len = getNewMeasValuesScript(buf, 50);
//		buf[len] = 0;
//		printf("%s\r",buf);
//	} while (len);
//
//	printf("\r\n *************\r\n");
}

float getTemperature(void) {
	return lastVal.temperature;
}
extern bool connected;

void sensirionTask(void *pvParameter) {
	i2c_port_t I2CmasterPort = *(i2c_port_t*) pvParameter;
	time_t now = 0;
	struct tm timeinfo;
	int lastminute = -1;
	int logPrescaler = 1;
	char str[64];
	int sensirionTimeoutTimer = SCD30_TIMEOUT;
	bool sensirionError = false;
	SCD40measValues_t rawValues;
	SCD40Status_t status;
	SCD40serial_t serial;
	int16_t error = 0;

	ESP_LOGI(TAG, "Starting task");

	time(&now);

#ifdef SIMULATE
	float x = 0;
	serial.serial = 0x12345678;

	ESP_LOGI(TAG, "Simulating Air sensor");
	while (1) {
		vTaskDelay(1000);
		lastVal.timeStamp = timeStamp++;
		lastVal.co2 = 500 + 100 * sin(x) - userSettings.CO2offset;
		lastVal.hum = 200 + 100 * cos(x) - userSettings.RHoffset;
		lastVal.temperature = 25 + 10 * sin(x + 1);
		x += 0.01;
		if (x > 1)
			x = 0;

		//	ESP_LOGI(TAG, "t: %1.2f RH:%1.1f co2:%f", lastVal.temperature, lastVal.hum, lastVal.co2);

		sprintf(str, "3:%d\n\r", (int)lastVal.co2);
		UDPsendMssg(UDPTXPORT, str, strlen(str));

		sprintf(str, "S: %s t:%1.2f RH:%1.1f co2:%d", userSettings.moduleName, lastVal.temperature, lastVal.hum, (int) lastVal.co2);
		UDPsendMssg(UDPTX2PORT, str, strlen(str));

		ESP_LOGI(TAG, "%s %d", str, logTxIdx);

		if (connected) {
			sensorDataIsSend = true;
			vTaskDelay(500 / portTICK_PERIOD_MS); // wait for UDP to send
		}

		temperatureAvg.write((int) (lastVal.temperature* 100.0));
		humAvg.write((int) ((int)lastVal.hum));
		CO2Avg.write((int) (lastVal.co2));
		lastVal.temperature = temperatureAvg.average()/ 100.0;
		lastVal.hum = humAvg.average();
		lastVal.co2 = (int) CO2Avg.average();

//		time(&now);
//		localtime_r(&now, &timeinfo);
//		if (lastminute != timeinfo.tm_min) {
//			lastminute = timeinfo.tm_min;   // every minute
		tLog[logTxIdx] = lastVal;
		logTxIdx++;
		if (logTxIdx >= MAXLOGVALUES)
			logTxIdx = 0;
//		}
	}
}

#else
	do {
		ESP_LOGI(TAG, "init");  // todo remove
		vTaskDelay(100 / portTICK_PERIOD_MS);


		if (SCD40Init(I2CmasterPort) != SCD40_OK) {
			ESP_LOGE(TAG, "SCD40 Not found");
			vTaskDelay(100 / portTICK_PERIOD_MS);
			error = 1;
		} else {
			error = SCD40ReadSerial(&serial);
		}
		if (error) {
			ESP_LOGE(TAG, "Eror reading serial SCD40");
			vTaskDelay(100 / portTICK_PERIOD_MS);
		}
	} while (error);

//	ESP_LOGI(TAG, "SCD40 serial: 0x%04x%04x%04x\n", serial.serial_0, serial.serial_1, serial.serial_2);

	SCD40StartPeriodicMeasurement();
	sensirionTimeoutTimer = SCD30_TIMEOUT;
	//testLog();

	while (1) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		status = SCD40Read(&rawValues);
		if (status == SCD40_OK) {
			if (inLowPowerMode)   // only 1 reading
				scd4x_stop_periodic_measurement();

			sensirionTimeoutTimer = SCD30_TIMEOUT;
			lastVal.temperature = rawValues.temperature - userSettings.temperatureOffset;
			lastVal.co2 = rawValues.co2 - userSettings.CO2offset;
			lastVal.hum = rawValues.humidity - userSettings.RHoffset;
			lastVal.timeStamp = timeStamp++;

			ESP_LOGI(TAG, "t: %1.2f rh:%1.1f co2:%d", lastVal.temperature, lastVal.hum, (int) lastVal.co2);
			if (connectStatus ==  IP_RECEIVED) {
				sprintf(str, "3:%d\n", (int) lastVal.co2);
				UDPsendMssg(UDPTXPORT, str, strlen(str));
				UDPsendMssg(UDPTXPORT, str, strlen(str));
				sprintf(str, "S: %s t:%1.2f rh:%1.1f co2:%d\n", userSettings.moduleName, lastVal.temperature, lastVal.hum, (int) lastVal.co2);
				UDPsendMssg(UDPTX2PORT, str, strlen(str));
				UDPsendMssg(UDPTX2PORT, str, strlen(str));

				if (inLowPowerMode) {  // only 1 reading
				//	scd4x_stop_periodic_measurement();
					vTaskDelay(300 / portTICK_PERIOD_MS); // wait for UDP to send
					sensorDataIsSend = true;
					while (1)
						vTaskDelay(500 / portTICK_PERIOD_MS);
				}
			}
			temperatureAvg.write((int) (rawValues.temperature * 100.0));  // avarage rawvalues
			humAvg.write((int) ((int) rawValues.humidity));
			CO2Avg.write((int) (rawValues.co2));
		}
		if (!inLowPowerMode) {
			if (sensirionTimeoutTimer-- == 0) {
				ESP_LOGE(TAG, "Air sensor timeout");
				sensirionError = true;
			} else
				sensirionError = false;
			time(&now);
			localtime_r(&now, &timeinfo);  // no use in low power mode
			if (lastminute != timeinfo.tm_min) {
				lastminute = timeinfo.tm_min;   // every minute
				if (logPrescaler-- == 0) {
					logPrescaler = LOGINTERVAL;
					lastVal.temperature = temperatureAvg.average() / 100.0; // log rawvalues
					lastVal.hum = humAvg.average();
					lastVal.co2 = (int) CO2Avg.average();

					tLog[logTxIdx] = lastVal;
					logTxIdx++;
					if (logTxIdx >= MAXLOGVALUES)
						logTxIdx = 0;
				}
			}
		}
	}
}

#endif

// CGI stuff

calValues_t calValues = { NOCAL, NOCAL, NOCAL };
// @formatter:off
char tempName[MAX_STRLEN];

const CGIdesc_t writeVarDescriptors[] = {
		{ "Temperatuur", &calValues.temperature, FLT, 1 },
		{ "RH", &calValues.RH, FLT, 1 },
		{ "CO2", &calValues.CO2, FLT, 1 },
		{ "moduleName",tempName, STR, 1 }
};

#define NR_CALDESCRIPTORS (sizeof (writeVarDescriptors)/ sizeof (CGIdesc_t))
// @formatter:on

int getRTMeasValuesScript(char *pBuffer, int count) {
	int len = 0;

	switch (scriptState) {
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "%ld,", lastVal.timeStamp); // send offset corrected values
		len += sprintf(pBuffer + len, "%3.2f,", lastVal.temperature);
		len += sprintf(pBuffer + len, "%3.2f,", lastVal.hum);
		len += sprintf(pBuffer + len, "%3.0f,", lastVal.co2);
		return len;
		break;
	default:
		break;
	}
	return 0;
}

int getSensorNameScript(char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "Actueel,Nieuw\n");
		len += sprintf(pBuffer + len, "%s\n", userSettings.moduleName);
		return len;
		break;
	default:
		break;
	}
	return 0;
}

int getInfoValuesScript(char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "%s\n", "Meting,Actueel,Offset");
		len += sprintf(pBuffer + len, "%s,%3.2f,%3.2f\n", "Temperatuur", lastVal.temperature, userSettings.temperatureOffset); // send offset corrected values
		len += sprintf(pBuffer + len, "%s,%3.1f,%3.1f\n", "RH", lastVal.hum, userSettings.RHoffset);
		len += sprintf(pBuffer + len, "%s,%3.0f,%3.0f\n", "CO2", lastVal.co2, userSettings.CO2offset);
		return len;
		break;
	default:
		break;
	}
	return 0;
}

// only build javascript table

int getCalValuesScript(char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "%s\n", "Meting,Referentie,Stel in,Herstel");
		len += sprintf(pBuffer + len, "%s\n", "Temperatuur\n RH\n CO2");
		return len;
		break;
	default:
		break;
	}
	return 0;
}

int saveSettingsScript(char *pBuffer, int count) {
	saveSettings();
	return 0;
}

int cancelSettingsScript(char *pBuffer, int count) {
	loadSettings();
	return 0;
}

// these functions only work for one user!

int getNewMeasValuesScript(char *pBuffer, int count) {

	int left, len = 0;
	if (logRxIdx != (logTxIdx)) {  // something to send?
		do {
			len += sprintf(pBuffer + len, "%ld,", tLog[logRxIdx].timeStamp);
			len += sprintf(pBuffer + len, "%3.2f,", tLog[logRxIdx].temperature - userSettings.temperatureOffset);
			len += sprintf(pBuffer + len, "%3.1f,", tLog[logRxIdx].hum - userSettings.RHoffset);
			len += sprintf(pBuffer + len, "%3.0f,", tLog[logRxIdx].co2 - userSettings.CO2offset);
			logRxIdx++;
			if (logRxIdx > MAXLOGVALUES)
				logRxIdx = 0;
			left = count - len;

		} while ((logRxIdx != logTxIdx) && (left > 40));

	}
	return len;
}
// reads all avaiable data from log
// issued as first request.

int getLogScript(char *pBuffer, int count) {
	static int oldTimeStamp = 0;
	static int logsToSend = 0;
	int left, len = 0;
	int n;

	ESP_LOGI(TAG, "getLogScript %d ", scriptState);

	if (scriptState == 0) { // find oldest value in cyclic logbuffer
		logRxIdx = 0;
		oldTimeStamp = 0;
		for (n = 0; n < MAXLOGVALUES; n++) {
			if (tLog[logRxIdx].timeStamp < oldTimeStamp)
				break;
			else {
				oldTimeStamp = tLog[logRxIdx++].timeStamp;
			}
		}
		if (tLog[logRxIdx].timeStamp == 0) { // then log not full
// not written yet?
			logRxIdx = 0;
			logsToSend = n;
		} else
			logsToSend = MAXLOGVALUES;
		scriptState++;
	}
	if (scriptState == 1) { // send complete buffer
		//	ESP_LOGI(TAG, "Sending %d logs", logsToSend);
		if (logsToSend) {
			do {
				len += sprintf(pBuffer + len, "%ld,", tLog[logRxIdx].timeStamp);
				len += sprintf(pBuffer + len, "%3.2f,", tLog[logRxIdx].temperature - userSettings.temperatureOffset);
				len += sprintf(pBuffer + len, "%3.1f,", tLog[logRxIdx].hum - userSettings.RHoffset);
				len += sprintf(pBuffer + len, "%3.0f\n", tLog[logRxIdx].co2 - userSettings.CO2offset);
				logRxIdx++;
				if (logRxIdx >= MAXLOGVALUES)
					logRxIdx = 0;
				left = count - len;
				logsToSend--;

			} while (logsToSend && (left > 40));
		}
	}
	ESP_LOGI(TAG, "Sending logs %d bytes", len);
	return len;
}

void parseCGIWriteData(char *buf, int received) {
	if (strncmp(buf, "setCal:", 7) == 0) {
		if (readActionScript(&buf[7], writeVarDescriptors, NR_CALDESCRIPTORS)) {
			lastVal.temperature = temperatureAvg.average() / 100.0; // calibrate against averaged values
			lastVal.hum = humAvg.average();
			lastVal.co2 = (int) CO2Avg.average();

			if (calValues.temperature != NOCAL) { // then real temperature received
				userSettings.temperatureOffset = lastVal.temperature - calValues.temperature;
				calValues.temperature = NOCAL;
			}
			if (calValues.CO2 != NOCAL) { // then real temperature received
				userSettings.CO2offset = lastVal.co2 - calValues.CO2;
				calValues.CO2 = NOCAL;
			}
			if (calValues.RH != NOCAL) { // then real temperature received
				userSettings.RHoffset = lastVal.hum - calValues.RH;
				calValues.RH = NOCAL;
			}
		}
	} else {
		if (strncmp(buf, "setName:", 8) == 0) {
			if (readActionScript(&buf[8], writeVarDescriptors, NR_CALDESCRIPTORS)) {
				if (strcmp(tempName, userSettings.moduleName) != 0) {
					strcpy(userSettings.moduleName, tempName);
					ESP_ERROR_CHECK(mdns_hostname_set(userSettings.moduleName));
					ESP_LOGI(TAG, "Hostname set to %s", userSettings.moduleName);
					saveSettings();
				}
			}
		}
	}
}
