#include <pthread.h>
/*
 * notif.h
 *
 *  Created on: 18.06.2024
 *      Author: qnx
 */

#ifndef NOTIF_H_
#define NOTIF_H_

// Структура для хранения параметров таймера
typedef struct {
	long tick_nsec; // 	Длительность одного тика в наносекундах
	int tick_sec;// 	Длительность одного тика в секундах
	timer_t periodicTimer;// дескриптор таймера
	struct itimerspec periodicTick;// интервал срабатывания относительного таймера в 1 тик
	int count; // 		Номер текущего тика часов ПРВ
} Clock;

#endif /* NOTIF_H_ */
