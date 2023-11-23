/*
 *
 *  Created on: Feb 8, 2018
 *      Author: dig
 */

#include "include/averager.h"
#include <stdlib.h>

#ifdef USE_FREERTOS
#include "freertos/FreeRTOS.h"
#endif

Averager::Averager( uint32_t averages) {
	bufWriteIndex = 0;
	bufValues = 0;
	bufSize = averages;
	pBuffer = (int32_t *) malloc ( bufSize * 4);
}

void Averager::clear( ){
	bufWriteIndex = 0;
	bufValues = 0;
}

void * Averager::setAverages( uint32_t averages){
	if ( averages > 0 ) {
		bufSize = averages;
		bufWriteIndex = 0;
		bufValues = 0;
		free (pBuffer);
		pBuffer = (int32_t *) malloc ( bufSize * 4);
		return pBuffer;
	}
	return NULL;
}

// write cyclic buffer
int32_t  Averager::write( int32_t value){
	if ( pBuffer == NULL){
		return -1;
	}
	else {
		if ( bufValues < bufSize)
			bufValues++;

		*(pBuffer + bufWriteIndex) = value;
		bufWriteIndex++;
		if( bufWriteIndex == bufSize)
			bufWriteIndex = 0;
	}
	return 0;
}


float Averager::average( void){
	float result=0;
	int64_t averageAccu = 0;
	int32_t highest = INT32_MIN;
	int32_t lowest = INT32_MAX;

	int32_t * p = (int32_t *) pBuffer;
	int32_t value;
	if (bufValues > 0) {
		for ( int n = 0; n < bufValues;n++){
			value = *p++;
			averageAccu += value;
			if (value > highest)
				highest = value;
			if (value < lowest)
				lowest = value;
		}
		if ( bufValues > 2 ) {
			averageAccu -= lowest;
			averageAccu -= highest;
			result = (float) averageAccu/(bufValues-2);
		}
		else
			result = (float) averageAccu/bufValues;
	}
	return result;
}

