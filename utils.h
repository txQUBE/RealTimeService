/*
 * utils.h
 *
 *  Created on: 18.06.2024
 *      Author: qnx
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <iostream>// 		проепроцессор
#include <errno.h> // 		для EOK
#include <unistd.h>// 		sleep
#include <sys/dispatch.h>// именованные каналы
#include <string>//			строки
using namespace std;

extern bool DEBUG; //				показывать дополнительные сообщения
extern bool shutDown; //			флаг завершить работу

extern const int CODE_TIMER; // 	сообщение-импульс от таймера
extern const int ND; //				id узла
extern int TICK;// 					ТИК

//Структура для запоминания СУБТД
typedef struct _tdb_ms {
	pthread_mutex_t Mutex;
	string name;//	имя СУБТД
	int pid; // 	id процесса
	int tid; // 	id нити
} tdb_ms_t;

extern tdb_ms_t registered_tdb;

#endif /* UTILS_H_ */
