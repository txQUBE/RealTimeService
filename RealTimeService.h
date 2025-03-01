/*
 * RealTimeService.h
 *
 *  Created on: 18.06.2024
 *      Author: qnx
 */

#ifndef REALTIMESERVICE_H_
#define REALTIMESERVICE_H_

#include <pthread.h> // 	pthread_create

void* server(void*);
void* notification(void*);

#endif /* REALTIMESERVICE_H_ */
