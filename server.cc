/*
 * server.cc
 *
 *  Created on: 18.06.2024
 *      Author: qnx
 */
#include "server.h"
#include "utils.h"

#define REG_CHAN 	"RTS_registration_channel"

const int REG_TYPE = 101; //	тип сообщения для регистрации

typedef struct _pulse msg_header_t; // 		абстрактный тип для заголовка сообщения как у импульса
typedef struct _registration_hadnle { // 	структура сообщения с данными регистрируемой СУБТД
	msg_header_t hdr;
	string name; //	имя СУБТД
	int pid; //		id процесса
	int tid; //		id нити
	int nd;//		дискриптор узла
} registration_handle_t;

tdb_map_buffer_t tdb_map; //	Буфер для запоминания СУБТД

/*
 * Нить сервера регистрации СУБТД
 */
void* server(void*) {
	cout << "- - - - Server: 	запуск сервера регистрации" << endl;
	name_attach_t* attach;
	registration_handle_t msg;
	int rcvid;

	//	Создание именованного канала
	if ((attach = name_attach(NULL, REG_CHAN, 0)) == NULL) {
		cout << "- - - - Server: 	ошибка создания именованного канала" << endl;
		exit(EXIT_FAILURE);
	}

	//	Цикл ожидания поступления сообщений в именованный канал
	while (!shutDown) {
		cout << "- - - - Server: 	ожидание запроса подключения" << endl;
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);
		cout << "- - - - Server ПОЛУЧЕН RCVID = " << rcvid << endl;

		if (rcvid == -1) { /* Ошибка */
			cout << "- - - - Server: 	поулчен rcvid = " << rcvid << endl;
			continue;
		}

		if (rcvid == 0) {/* Получен импульс */
			// обработка системных сигналов
			switch (msg.hdr.code) {
			case _PULSE_CODE_DISCONNECT:
				ConnectDetach(msg.hdr.scoid); /* Уничтожить соединение ядра с каналом сервера*/
				name_detach(attach, 0); /* Удалить имя процесса */
				printf("Server receive _PULSE_CODE_DISCONNECT and terminated");
				break;
			case _PULSE_CODE_UNBLOCK:
				cout << "- - - - Server _PULSE_CODE_UNBLOCK\n";
				break;
			default:
				cout << "- - - - Server default\n";
				break;
			}
			continue;
		}
		/* Полученное сообщение не импульс */
		// обработка системных сообщений
		if (msg.hdr.type == _IO_CONNECT) {
			cout << "- - - - Server: recieve name_open()\n";
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
			cout << "- - - - Server: полчено сообщение REG_TYPE \n";
			regTDB(msg.name, msg.pid, msg.tid, msg.nd);

			// TODO: обработка успешности ответа

			MsgReply(rcvid, EOK, 0, 0);
			continue;
		}
	}

	printf("- - - - Server: Server is shutting down \n");
	cout << "- - - - Server: ShutDown = " << shutDown << endl;
	return NULL;
}

/*
 * Функция регистрации
 */
bool regTDB(string name, int pid, int tid, int nd) {

	if (tdb_map.buf.count(name) > 0) {
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
		cout << "Server: " << endl;
		cout << "----------TDB_MS REGISTRATION INFO---------" << endl;
		cout << "				SUCCESS						" << endl;
		cout << "-------------------------------------------" << endl << endl;
		return true;
	}
	return false;
}
