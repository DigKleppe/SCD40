/*
 * sensirionTask.h
 *
 *  Created on: Apr 18, 2022
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_SENSIRIONTASK_H_
#define MAIN_INCLUDE_SENSIRIONTASK_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cgiScripts.h"
#include "settings.h"


//#define SIMULATE

#define MEASINTERVAL			 	5  // interval for sensiron sensor in seconds
#define LOGINTERVAL					5  //minutes
#define AVGERAGESAMPLES				((LOGINTERVAL * 60)/MEASINTERVAL)

#define MAXLOGVALUES				((24*60)/LOGINTERVAL)

#define NOCAL 						99999

#define UDPTXPORT	5001 // brink co2 only
#define UDPTX2PORT	5002 // all data and serial nr

typedef struct {
	float temperature;
	float RH;
	float CO2;
} calValues_t;

extern calValues_t calValues;
extern bool sensorDataIsSend; // set when UDP is send
extern char tempName[MAX_STRLEN]; // when received from cgi

extern const CGIdesc_t calibrateDescriptors[];
extern TaskHandle_t SensirionTaskh;


float getTemperature (void);

void sensirionTask(void *pvParameter);
int getRTMeasValuesScript(char *pBuffer, int count) ;
int getNewMeasValuesScript(char *pBuffer, int count);
int getLogScript(char *pBuffer, int count);
int getInfoValuesScript (char *pBuffer, int count);
int getCalValuesScript (char *pBuffer, int count);
int saveSettingsScript (char *pBuffer, int count);
int cancelSettingsScript (char *pBuffer, int count);
int calibrateRespScript(char *pBuffer, int count);
int getSensorNameScript (char *pBuffer, int count);

void sensirionTask(void *pvParameter);


#endif /* MAIN_INCLUDE_SENSIRIONTASK_H_ */
