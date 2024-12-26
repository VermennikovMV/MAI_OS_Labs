#ifndef MY_LIB_H
#define MY_LIB_H

#include <iostream>
#include <list>
#include <unordered_set>
#include <chrono>
#include <ctime>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include "zmq.h"
#include <sys/select.h>
#include <map>

// Проверка консольного ввода (не блокируясь).
bool inputAvailable();

// Текущее время (time_t).
std::time_t t_now();

// ------------------------------------------------------------
// Перечисление команд (соответствует нашему решению)
//
//   None      = 0    — отсутствие команды
//   Create    = 1    — создать новый узел
//   Ping      = 2    — проверка доступности узла ("ping")
//   ExecSub   = 3    — поиск подстроки (вариант 4)
//   Heartbit  = 4    — «стук» для проверки доступности (вариант 3)
//   ErrorResp = 5    — дополнительная команда для сообщений об ошибке
// ------------------------------------------------------------
enum com : char {
    None      = 0,
    Create    = 1,
    Ping      = 2,
    ExecSub   = 3,
    Heartbit  = 4,
    ErrorResp = 5
};

// Структура сообщения
class message {
public:
    message() {}

    // Конструктор без строки
    message(com command, int id, int num)
        : command(command), id(id), num(num), sent_time(t_now())
    {
        std::memset(st, 0, sizeof(st));
    }

    // Конструктор со строкой
    message(com command, int id, int num, const char s[])
        : command(command), id(id), num(num), sent_time(t_now())
    {
        std::memset(st, 0, sizeof(st));
        strncpy(st, s, sizeof(st) - 1);
    }

    bool operator==(const message &other) const {
        return (command == other.command &&
                id == other.id &&
                num == other.num &&
                sent_time == other.sent_time &&
                std::strcmp(st, other.st) == 0);
    }

    com command;      // Команда
    int id;           // ID узла (или родителя), кому адресовано / от кого
    int num;          // Доп. числовое поле (PID, или время heartbeat, и пр.)
    std::time_t sent_time; // время отправки (для таймаута)
    char st[100];     // Строка (текст, паттерн и т.д.)
};

// Класс, описывающий узел
class Node {
public:
    int id;          // id узла
    pid_t pid;       // pid процесса
    void *context;
    void *socket;
    std::string address;

    bool operator==(const Node &other) const {
        return (id == other.id && pid == other.pid);
    }
};

// Создание сокета (бинд или коннект)
Node createNode(int id, bool is_child);

// Создание нового процесса (fork + execl("./computing", ...))
Node createProcess(int id);

// Отправка / приём сообщений (ZeroMQ)
void send_mes(Node &node, const message &m);
message get_mes(Node &node);

#endif // MY_LIB_H
