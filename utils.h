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

using namespace std;

extern bool DEBUG; //				���������� �������������� ���������
extern bool ShutDown; //			���� ��������� ������

extern const int CODE_TIMER; // 	���������-������� �� �������
extern const int ND; //				id ����
extern int tik;// 					���


//��������� ��� ����������� �����
typedef struct _tdb_ms {
	string name;//	��� �����
	int pid; // 	id ��������
	int tid; // 	id �������
} tdb_ms_t;

extern tdb_ms_t TDB;

#endif /* UTILS_H_ */
