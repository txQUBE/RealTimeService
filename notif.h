/*
 * notif.h
 *
 *  Created on: 18.06.2024
 *      Author: qnx
 */

#ifndef NOTIF_H_
#define NOTIF_H_

void gotAPulse(void);//		������� ��������� ������� �������
struct Clock {
	long tick_nsec; // ������������ ������ ���� � ������������
	int Time; // ����� �������� ���� ����� ���
};

extern Clock timer; // ����� �������

#endif /* NOTIF_H_ */
