/*
 * utils.h
 *
 *  Created on: 18.06.2024
 *      Author: qnx
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <iostream>// 		�������������
#include <errno.h> // 		��� EOK
#include <unistd.h>// 		sleep
#include <sys/dispatch.h>// ����������� ������
#include <string>//			������
#include <map>

using namespace std;

extern bool DEBUG; //				���������� �������������� ���������
extern bool shutDown; //			���� ��������� ������

extern const int CODE_TIMER; // 	���������-������� �� �������
extern const int ND; //				id ����
extern int TICK;// 					���

//��������� ��� ����������� �����
typedef struct _tdb_ms {
	int pid; // 		id ��������
	pthread_t tid; // 	id ����
	int nd;  // 		���������� ����
} tdb_ms_t;

typedef struct _tdb_map_buffer{
	pthread_mutex_t Mutex;
	map<string, tdb_ms_t> buf;
} tdb_map_buffer_t;

extern tdb_map_buffer_t tdb_map;;

#endif /* UTILS_H_ */
