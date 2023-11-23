#include "esp_log.h"
#include "driver/i2c.h"
#include "scd40.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "scd4x_i2c.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"
#include "scd40.h"

static const char *TAG = "scd40";

int testSCD40(i2c_port_t I2CmasterPort) {
	int16_t error = 0;
	SCD40serial_t serial;
	uint16_t serial_0;
	uint16_t serial_1;
	uint16_t serial_2;


//	sensirion_i2c_hal_init();
//	sensirion_i2c_hal_select_bus(I2CmasterPort);
//
//	// Clean up potential SCD40 states
//	scd4x_wake_up();
//	scd4x_stop_periodic_measurement();
//	scd4x_reinit();
//
//	uint16_t serial_0;
//	uint16_t serial_1;
//	uint16_t serial_2;
//	error = scd4x_get_serial_number(&serial_0, &serial_1, &serial_2);
//	if (error) {
//		printf("Error executing scd4x_get_serial_number(): %i\n", error);
//	} else {
//		printf("serial: 0x%04x%04x%04x\n", serial_0, serial_1, serial_2);
//	}
	sensirion_i2c_hal_select_bus(I2CmasterPort);
	do {
		error = SCD40Init(I2CmasterPort);
		if (error != SCD40_OK) {
			ESP_LOGE(TAG, "SCD40 Not found");
			vTaskDelay(100 / portTICK_PERIOD_MS);
		}
	} while (error != SCD40_OK);

	do {
		error = SCD40ReadSerial(&serial);
		if (error != SCD40_OK) {
			ESP_LOGE(TAG, "Eror reading serial SCD40");
			vTaskDelay(100 / portTICK_PERIOD_MS);
		}
	} while (error != SCD40_OK);

	// Start Measurement
	error = SCD40StartPeriodicMeasurement();
	if (error) {
		printf("Error executing scd4x_start_periodic_measurement(): %i\n", error);
	}
	printf("Waiting for first measurement... (5 sec)\n");

	for (;;) {
		// Read Measurement
		sensirion_i2c_hal_sleep_usec(100000);
		bool data_ready_flag = false;
		error = scd4x_get_data_ready_flag(&data_ready_flag);
		if (error) {
			printf("Error executing scd4x_get_data_ready_flag(): %i\n", error);
			continue;
		}
		if (!data_ready_flag) {
			continue;
		}

		uint16_t co2;
		int32_t temperature;
		int32_t humidity;

		error = scd4x_read_measurement(&co2, &temperature, &humidity);
		if (error) {
			printf("Error executing scd4x_read_measurement(): %i\n", error);
		} else if (co2 == 0) {
			printf("Invalid sample detected, skipping.\n");
		} else {
			printf("CO2: %u\n", co2);
			printf("Temperature: %2.2f °C\n", (float) temperature / 1000.0);
			printf("Humidity: %2.1f RH\n", (float) humidity / 1000.0);
		}
	}
	return 0;
}

SCD40Status_t SCD40Init(i2c_port_t i2c_master_port) {
	int16_t error = 0;
	sensirion_i2c_hal_init();
	sensirion_i2c_hal_select_bus(i2c_master_port);

	scd4x_wake_up();
	error = scd4x_stop_periodic_measurement();
	if (error)
		return SCD40_ERROR;
	error = scd4x_reinit();
	if (error)
		return SCD40_ERROR;

	return SCD40_OK;
}

SCD40Status_t SCD40ReadSerial(SCD40serial_t *serial) {
	int16_t error = 0;

	error = scd4x_get_serial_number(&serial->serial_0, &serial->serial_1, &serial->serial_2);
	if (error) {
		ESP_LOGE(TAG, "Error executing scd4x_get_serial_number(): %i\n", error);
		return SCD40_ERROR;

	} else {
//		ESP_LOGI(TAG, "serial: 0x%04x%04x%04x\n", serial->serial_0, serial->serial_1, serial->serial_2);
		return SCD40_OK;
	}
}

SCD40Status_t SCD40Read(SCD40measValues_t *values) {
	int16_t error = 0;

	bool data_ready_flag = false;

	error = scd4x_get_data_ready_flag(&data_ready_flag);
	if (error) {
		//printf("Error executing scd4x_get_data_ready_flag(): %i\n", error);
		ESP_LOGE(TAG, "Error executing scd4x_get_data_ready_flag(): %i\n", error);
		return SCD40_ERROR;
	} else {
		if (!data_ready_flag) {
			return SCD40_NOT_READY;
		} else {
			uint16_t co2;
			int32_t temperature;
			int32_t humidity;

			error = scd4x_read_measurement(&co2, &temperature, &humidity);
			if (error) {
				ESP_LOGE(TAG, "Error executing scd4x_read_measurement(): %i\n", error);
				return SCD40_ERROR;
			} else {
				if (co2 == 0) {
					ESP_LOGE(TAG, "Invalid sample detected, skipping.\n");
					return SCD40_INVALID_SAMPLE;
				} else {
					values->co2 = co2;
					values->humidity = (float) humidity / 1000.0;
					values->temperature = (float) temperature / 1000.0;

					ESP_LOGI(TAG, "CO2: %u\n", co2);
					ESP_LOGI(TAG, "Temperature: %2.2f °C\n", values->temperature);
					ESP_LOGI(TAG, "Humidity: %2.1f RH\n", values->humidity);
					return SCD40_OK;
				}
			}
		}
	}
}

SCD40Status_t SCD40StartPeriodicMeasurement( ) {

	if (scd4x_start_periodic_measurement() == NO_ERROR )
		return SCD40_OK;
	else
		return SCD40_ERROR;
}


