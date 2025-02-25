/*
 * notif.h
 *
 *  Created on: 18.06.2024
 *      Author: qnx
 */

#ifndef NOTIF_H_
#define NOTIF_H_

void gotAPulse(void);//		Функиця обработки сигнала таймера
struct Clock {
	long tick_nsec; // Длительность одного тика в наносекундах
	int Time; // Номер текущего тика часов ПРВ
};

extern Clock timer; // Буфер таймера

#endif /* NOTIF_H_ */
