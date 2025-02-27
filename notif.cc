/*
 * notif.cc
 *
 *  Created on: 18.06.2024
 *      Author: Грац Д.Д.
 *
 *  Нить канала уведомлений об истечении тика таймера
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

//Прототипы функций
void sendNotifTickPassed(); // функция отправки сигнала о прошедшем тике

/*
 * Нить уведомлений
 * Принимает импульсы от таймера
 * Отправляет сигнал по проществии тика в зарегестрирование СУБД
 */
void* notification(void*) {

	cout << "- Notif: запуск канала уведомлений" << endl;
	name_attach_t *attach;
	TimerSig tsig;

	//	Создание именованного канала
	if ((attach = name_attach(NULL, NOTIF_CHAN, 0)) == NULL) {
		cout << "- Notif: ошибка создания канала уведомлений" << endl;
		exit(EXIT_FAILURE);
	}

	pthread_barrier_wait(&notif_barrier); // ожидание подключения таймера к каналу

	//	Приём сообщений таймера
	for (;;) {
		cout << endl << "- Notif: ожидание сигнала таймера" << endl;
		int rcvid = MsgReceive(attach->chid, &tsig, sizeof(tsig), NULL);
		if (rcvid == 0) { // получен импульс от таймера
			gotAPulse();
			sendNotifTickPassed();
		}
	}
	return NULL;
}

/*
 * Функиця обработки сигнала таймера
 */
void gotAPulse(void) {

	time(&now);//Получаем текущий момент времени
	//Печать строки времени
	cout << endl << "Timer signal received " << ctime(&now);
	cout << "CODE_TIMER = " << (int) tsig.M_pulse.code << endl;
	cout << "Data: " << tsig.M_pulse.value.sival_int << endl;
}

/*
 * функция отправки сигнала о прошедшем тике
 */
void sendNotifTickPassed() {
	// захват буфера с зарегистрированными СУБТД
	pthread_mutex_lock(&tdb_map.Mutex);

	if (tdb_map.buf.empty()) {
		cout << "- Notif: no registered TDB_MS" << endl;
	} else {
		cout << "- Notif: send SIGUSR1" << endl << endl;
		// итерация буфера с зарегистрированными СУБТД
		for (map<string, tdb_ms_t>::iterator it = tdb_map.buf.begin(); it
				!= tdb_map.buf.end(); ++it) {
			// Послать сигнал о прошедшем тике В СУБТД
			if (SignalKill(0, it->second.pid, it->second.tid, SIGUSR1,
					SI_USER, 0) == -1) {
				if(errno == ESRCH){
					// Удалить из буфера неактивный процесс
					cout << "- Notif: erase " << it->first << endl;
					tdb_map.buf.erase(it->first);

				} else
				cerr << "- Notif: error SignalKill errno: "<< strerror(errno) << endl;

			}
		}
	}

	pthread_mutex_unlock(&tdb_map.Mutex);
}

