/*
 * updateSpiffsTask.h
 *
 *  Created on: May 24, 2023
 *      Author: dig
 */

#ifndef COMPONENTS_SPIFFSOTA_INCLUDE_UPDATESPIFFSTASK_H_
#define COMPONENTS_SPIFFSOTA_INCLUDE_UPDATESPIFFSTASK_H_

#define MAX_STORAGEVERSIONSIZE 16

void updateSpiffsTask(void *pvParameter);
extern volatile bool spiffsUpdateFinised;


#endif /* COMPONENTS_SPIFFSOTA_INCLUDE_UPDATESPIFFSTASK_H_ */
