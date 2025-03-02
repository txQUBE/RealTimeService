#include <pthread.h>
/*
 * notif.h
 *
 *  Created on: 18.06.2024
 *      Author: qnx
 */

#ifndef NOTIF_H_
#define NOTIF_H_

// ��������� ��� �������� ���������� �������
typedef struct {
	long tick_nsec; // 	������������ ������ ���� � ������������
	int tick_sec;// 	������������ ������ ���� � ��������
	timer_t periodicTimer;// ���������� �������
	struct itimerspec periodicTick;// �������� ������������ �������������� ������� � 1 ���
	int count; // 		����� �������� ���� ����� ���
} Clock;

#endif /* NOTIF_H_ */
