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
#include <map>

using namespace std;

extern bool DEBUG; //				показывать дополнительные сообщения
extern bool shutDown; //			флаг завершить работу

extern const int CODE_TIMER; // 	сообщение-импульс от таймера
extern const int ND; //				id узла
extern int TICK;// 					ТИК

//Структура для запоминания СУБТД
typedef struct _tdb_ms {
	int pid; // 		id процесса
	pthread_t tid; // 	id нити
	int nd;  // 		дескриптор узла
} tdb_ms_t;

typedef struct _tdb_map_buffer{
	pthread_mutex_t Mutex;
	map<string, tdb_ms_t> buf;
} tdb_map_buffer_t;

extern tdb_map_buffer_t tdb_map;;

#endif /* UTILS_H_ */
