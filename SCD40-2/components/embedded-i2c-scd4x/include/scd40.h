/*
 * scd40.h
 *
 *  Created on: Aug 25, 2023
 *      Author: dig
 */

#ifndef COMPONENTS_EMBEDDED_I2C_SCD4X_INCLUDE_SCD40_H_
#define COMPONENTS_EMBEDDED_I2C_SCD4X_INCLUDE_SCD40_H_
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { SCD40_OK, SCD40_INVALID_SAMPLE, SCD40_NOT_READY, SCD40_ERROR } SCD40Status_t;

typedef union  {
	uint64_t serial;
	uint16_t serial_0;
	uint16_t serial_1;
	uint16_t serial_2;
}SCD40serial_t;


typedef struct {
	uint16_t co2;
	float temperature;
	float humidity;
}SCD40measValues_t;

SCD40Status_t SCD40Init(i2c_port_t i2c_master_port);
SCD40Status_t SCD40ReadSerial(SCD40serial_t * serial);
SCD40Status_t SCD40Read( SCD40measValues_t * values);
SCD40Status_t SCD40StartPeriodicMeasurement( );

#ifdef __cplusplus
}
#endif


#endif /* COMPONENTS_EMBEDDED_I2C_SCD4X_INCLUDE_SCD40_H_ */
