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
#define TICKPAS_SIG SIGRTMIN // сигнал о прошедшем тике
#define TICKUPDATE_SIG SIGRTMIN + 1 // сигнал об обновлении параметров тика
#define TICKUPDATE_CODE 2 // код сообщения с данными параметров таймера

// Структура для хранения параметров таймера
struct Clock {
	long tick_nsec; // 	Длительность одного тика в наносекундах
	int tick_sec;// 	Длительность одного тика в секундах
	timer_t periodicTimer;// дескриптор таймера
	struct itimerspec periodicTick;// интервал срабатывания относительного таймера в 1 тик
	int Time; // 		Номер текущего тика часов ПРВ
};

extern Clock timer; // Буфер таймера
extern pthread_barrier_t notif_barrier;
extern bool tick_changed; // флаг-статус изменения тика таймера

typedef struct _pulse msg_header_t; // 		абстрактный тип для заголовка сообщения как у импульса
//Структура для отправки сообщения с параметрами таймера
struct local_time_msg_t {
	msg_header_t hdr;
	long tick_nsec; // 	Длительность одного тика, составляющая в наносекундах
	int tick_sec;// 	Длительность одного тика, составляющая в секундах
	int Time; // 		Номер текущего тика часов ПРВ
};

//Для приема испульса от таймера
union TimerSig {
	_pulse M_pulse;
} tsig;

//Для хранения момента времени каждого тика
time_t now;

//Прототипы функций
void sendNotifTickPassed(); // функция отправки сигнала о прошедшем тике
void sendTickParam(int coid); // функция отправки актуального значения тика таймера
void sendTickUpdate();// функция отправки обновлений параметров таймера
void gotAPulse(void);//		Функиця обработки сигнала таймера

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

		//проверить статус изменения параметров тика таймера
		if (tick_changed) {
			sendTickUpdate();
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
	pthread_mutex_lock(&tdb_map.mutex);

	if (tdb_map.buf.empty()) {
		cout << "- Notif: no registered TDB_MS" << endl;
	} else {
		cout << "- Notif: send SIGUSR1" << endl << endl;
		// итерация буфера с зарегистрированными СУБТД
		for (map<string, tdb_ms_t>::iterator it = tdb_map.buf.begin(); it
				!= tdb_map.buf.end(); ++it) {
			// Послать сигнал о прошедшем тике В СУБТД
			if (SignalKill(0, it->second.pid, it->second.tid, TICKPAS_SIG,
					SI_USER, 0) == -1) {
				if (errno == ESRCH) {
					// Удалить из буфера неактивный процесс
					cout << "- Notif: erase " << it->first << endl;
					tdb_map.buf.erase(it->first);
				} else
					cerr << "- Notif: error SignalKill errno: " << strerror(
							errno) << endl;
			}
		}
	}
	pthread_mutex_unlock(&tdb_map.mutex);
}

void sendTickUpdate() {
	pthread_mutex_lock(&tdb_map.mutex);

	for (map<string, tdb_ms_t>::iterator it = tdb_map.buf.begin(); it
			!= tdb_map.buf.end(); ++it) {
		// Послать сигнал об изменении тика таймера и отправить сообщение с актуальным тиком
		if (SignalKill(0, it->second.pid, it->second.tid, TICKUPDATE_SIG,
				SI_USER, 0) != -1) {
			int coid;
			//подключиться к именованному каналу СУБТД для отправки сообщения
			if ((coid = name_open(it->first.c_str(), 0)) == -1) {
				cerr << "- Notif: error sendTickUpdate " << it->first<<" errno: "
						<< errno << endl;
			}
			//отправить параметры таймера
			sendTickParam(coid);
			cout << "- Notif: sent TICKUPDATE to "<< it->first.c_str() << endl;
			name_close(coid);
		}
	}
	pthread_mutex_unlock(&tdb_map.mutex);
}

void sendTickParam(int coid) {
	local_time_msg_t msg;
	msg.hdr.type = 0x00;
	msg.hdr.subtype = 0x00;
	msg.hdr.code = TICKUPDATE_CODE;
	msg.Time = timer.Time;
	msg.tick_nsec = timer.tick_nsec;
	msg.tick_sec = timer.tick_sec;

	int reply = MsgSend(coid, &msg, sizeof(msg), NULL, 0);
	if (reply != EOK){
		cerr << "- - - - Server: MsgSend error, errno: " << errno << endl;
	}
}

