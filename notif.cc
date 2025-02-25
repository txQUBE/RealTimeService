/*
 * notif.cc
 *
 *  Created on: 18.06.2024
 *      Author: qnx
 */

#include "notif.h"
#include "utils.h"

#define NOTIF_CHAN 	"notification_channel"

extern pthread_barrier_t notif_barrier;

//Для хранения момента времени каждого тика
time_t now;

//Для приема испульса от таймера
union TimerSig {
	_pulse M_pulse;
} tsig;

//-----------------------------------------------------------------
//*	Нить уведомлений
//* Принимает импульсы от таймера
//*	Отправляет сигнал по проществии тика в зарегестрирование СУБД
//----------------------------------------------------------------
void* notification(void*) {

	cout << "- Notif: запуск канала уведомлений" << endl;
	name_attach_t *attach;
	TimerSig tsig;

	//	Создание именованного канала
	if ((attach = name_attach(NULL, NOTIF_CHAN, 0)) == NULL) {
		cout << "- Notif: ошибка создания канала уведомлений" << endl;
		exit(EXIT_FAILURE);
	}

	pthread_barrier_wait(&notif_barrier); //можно подключаться к каналу

	//	Приём сообщений таймера
	for (;;) {
		cout << endl << "- Notif: ожидание сигнала таймера" << endl;
		int rcvid = MsgReceive(attach->chid, &tsig, sizeof(tsig), NULL);
		if (rcvid == 0) { // получен импульс от таймера
			gotAPulse();
			if (registered_tdb.pid == 0) {
				cout << "- Notif: нет зарегистрированных СУБТД" << endl;
			} else {
				cout << "- Notif: отправка SIGUSR1" << endl << endl;
				//	Послать сигнал о прошедшем тике В СУБТД
				if (SignalKill(ND, registered_tdb.pid, registered_tdb.tid, SIGUSR1, SI_USER, 0) == -1) {
					cout << "- Notif: ошибка SignalKill " << endl;
				}
			}
		}
	}
	return NULL;
}

//-----------------------------------------------------------------
//* Функиця обработки сигнала таймера
//-----------------------------------------------------------------
void gotAPulse(void) {

	time(&now);//Получаем текущий момент времени
	//Печать строки времени
	cout << endl << "Получен сигнал таймера " << ctime(&now);
	cout << "CODE_TIMER = " << (int) tsig.M_pulse.code << endl;
	cout << "Data: " << tsig.M_pulse.value.sival_int << endl;
}

