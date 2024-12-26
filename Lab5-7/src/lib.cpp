#include "lib.h"

#include <sys/select.h>
#include <cstring>

bool inputAvailable() {
    // Неблокирующая проверка ввода
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    return (FD_ISSET(STDIN_FILENO, &fds));
}

std::time_t t_now() {
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

Node createNode(int id, bool is_child) {
    Node node;
    node.id = id;
    node.pid = getpid();

    node.context = zmq_ctx_new();
    node.socket  = zmq_socket(node.context, ZMQ_DEALER);

    // Порт: 5555 + id (пример)
    node.address = "tcp://127.0.0.1:" + std::to_string(5555 + id);

    if (is_child) {
        // Узел-коннект
        zmq_connect(node.socket, node.address.c_str());
    } else {
        // Узел-бинд
        zmq_bind(node.socket, node.address.c_str());
    }
    return node;
}

Node createProcess(int id) {
    pid_t pid = fork();
    if (pid == 0) {
        // Дочерний процесс
        execl("./computing", "computing", std::to_string(id).c_str(), NULL);
        std::cerr << "execl failed" << std::endl;
        _exit(1);
    } else if (pid == -1) {
        // Ошибка при fork
        std::cerr << "Fork failed" << std::endl;
        _exit(1);
    }
    // Родитель
    Node node = createNode(id, false);
    node.pid  = pid;
    return node;
}

void send_mes(Node &node, const message &m) {
    zmq_msg_t request;
    zmq_msg_init_size(&request, sizeof(m));
    std::memcpy(zmq_msg_data(&request), &m, sizeof(m));
    zmq_msg_send(&request, node.socket, ZMQ_DONTWAIT);
    zmq_msg_close(&request);
}

message get_mes(Node &node) {
    zmq_msg_t msg;
    zmq_msg_init(&msg);
    int rc = zmq_msg_recv(&msg, node.socket, ZMQ_DONTWAIT);
    if (rc == -1) {
        // Нет сообщения
        zmq_msg_close(&msg);
        return message(None, -1, -1);
    }
    message m;
    std::memcpy(&m, zmq_msg_data(&msg), sizeof(message));
    zmq_msg_close(&msg);
    return m;
}
