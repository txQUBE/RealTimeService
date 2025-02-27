/*
 * server.cc
 *
 *  Created on: 18.06.2024
 *      Author: Грац Д.Д.
 *
 *  Нить сервера регистрации СУБТД в СРВ
 */
#include "server.h"
#include "utils.h"

#define REG_CHAN "RTS_registration_channel" // имя канала регистрации
const int REG_TYPE = 101; //	тип сообщения для регистрации

typedef struct _pulse msg_header_t; // 		абстрактный тип для заголовка сообщения как у импульса
typedef struct _registration_hadnle { // 	структура сообщения с данными регистрируемой СУБТД
	msg_header_t hdr;
	string name; //		имя СУБТД
	int pid; //			id процесса
	pthread_t tid; //	id нити
	int nd;//			дискриптор узла
} registration_handle_t;

tdb_map_buffer_t tdb_map; //	Буфер для запоминания СУБТД

// Прототипы функций
// функция регистрации СУБТД в СРВ
bool reg_tdb(string name, int pid, pthread_t tid, int nd);
// Функция проверки существования СУБТД в буфере сервера
bool isRegistered(string name);

/*
 * Нить сервера регистрации СУБТД
 * Открывает канал для регистрации
 * Обрабатывает системные сообщения, импульсы и сообщения для регистрации СУБТД
 */
void* server(void*) {
	cout << "- - - - Server: starting..." << endl;
	name_attach_t* attach;
	registration_handle_t msg;
	int rcvid;

	//	Создание именованного канала
	if ((attach = name_attach(NULL, REG_CHAN, 0)) == NULL) {
		cerr << "- - - - Server: 	error name_attach(). errno:" << errno << endl;
		exit(EXIT_FAILURE);
	}

	//	Цикл ожидания поступления сообщений в именованный канал
	while (!shutDown) {
		cout << "- - - - Server: wait for msg..." << endl;
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);

		if (rcvid == -1) { /* Ошибка */
			if (errno == ENOTCONN) {
				// Клиент отключился
				printf("- - - - Server: Client disconected.\n");
				continue;
			} else {
				perror("MsgReceive");
				break;
			}
		}

		if (rcvid == 0) {/* Получен импульс */
			// обработка системных сигналов
			switch (msg.hdr.code) {
			case _PULSE_CODE_DISCONNECT:
				ConnectDetach(msg.hdr.scoid); /* Уничтожить соединение ядра с каналом сервера*/
				printf("- - - - Server:  _PULSE_CODE_DISCONNECT\n");
				break;
			case _PULSE_CODE_UNBLOCK:
				cout << "- - - - Server: _PULSE_CODE_UNBLOCK\n";
				break;
			default:
				cout << "- - - - Server: default\n";
				break;
			}
			continue;
		}
		/* Полученное сообщение не импульс */
		// обработка системных сообщений
		if (msg.hdr.type == _IO_CONNECT) {
			MsgReply(rcvid, EOK, NULL, 0);
			continue;
		}
		if (msg.hdr.type > _IO_BASE && msg.hdr.type <= _IO_MAX) { //Получено IO-сообщение от ядра
			cout << "_IO_BASE < hdt.type << _IO_MAX\n";
			MsgError(rcvid, ENOSYS );
			continue;
		}
		// обработка сообщения о регистрации
		if (msg.hdr.code == REG_TYPE) {
			bool success = reg_tdb(msg.name, msg.pid, msg.tid, msg.nd);
			if (success) {
			    MsgReply(rcvid, EOK, 0, 0); // Успешная регистрация
			} else {
			    MsgReply(rcvid, EINVAL, 0, 0); // Ошибка регистрации
			}
			continue;
		}
	}

	printf("- - - - Server: Server is shutting down \n");
	name_detach(attach, 0); /* Удалить имя процесса */
	return NULL;
}

/*
 * Функция регистрации
 * Формирует структуру для хранения данных СУБТД и сохраняет в буфер
 */
bool reg_tdb(string name, int pid, pthread_t tid, int nd) {

	if (isRegistered(name)) {
		return false; // регистрация не выполнена, СУБТД с таким именем уже зарегистрирована
	}

	// структура для хранения данных СУБТД
	tdb_ms_t tdb;
	tdb.pid = pid;
	tdb.tid = tid;
	tdb.nd = nd;
	tdb_map.buf[name] = tdb;

	bool isRegistered;
	isRegistered = tdb_map.buf.count(name) > 0;
	// проверка успеха регистрации
	if (isRegistered) {
		cout << "- - - - Server: "<< name << " REGISTRATION SUCCESS" << endl;
		return true;
	}
	return false;
}

/*
 * Функция проверки регистрации
 * Проверяет существование СУБТД в буфере по её имени
 */
bool isRegistered(string name) {
	return tdb_map.buf.count(name) > 0;
}
