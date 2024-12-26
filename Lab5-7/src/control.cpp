#include "lib.h"
#include <sstream>
#include <limits>
#include <algorithm>

// Если хотим хранить время последнего heartbit от узлов:
static std::map<int, std::time_t> last_heartbeat;
static int heartbit_interval_ms = 0; // 0, если не используем heartbeat

int main() {
    // Множество всех известных id
    std::unordered_set<int> all_id;
    all_id.insert(-1);

    // Список дочерних узлов (прямые дети управляющего)
    std::list<Node> children;

    // Список «висящих» сообщений
    std::list<message> saved_mes;

    std::string input_line;
    while (true) {
        // 1. Приём сообщений от детей
        for (auto &child : children) {
            while (true) {
                message m = get_mes(child);
                if (m.command == None) {
                    break; // Нет новых сообщений
                }
                switch (m.command) {
                case Create:
                    // Подтверждение "Ok: pid"
                    all_id.insert(m.id);
                    std::cout << "Ok: " << m.num << std::endl;
                    // Убираем из saved_mes
                    {
                        auto it = std::find_if(saved_mes.begin(), saved_mes.end(),
                                               [&](const message &mm) {
                                                   return mm.command == Create && mm.num == m.id;
                                               });
                        if (it != saved_mes.end()) {
                            saved_mes.erase(it);
                        }
                    }
                    break;

                case Ping:
                    // "Ok: X is available"
                    std::cout << "Ok: " << m.id << " is available" << std::endl;
                    {
                        auto it = std::find_if(saved_mes.begin(), saved_mes.end(),
                                               [&](const message &mm) {
                                                   return mm.command == Ping && mm.id == m.id;
                                               });
                        if (it != saved_mes.end()) {
                            saved_mes.erase(it);
                        }
                    }
                    break;

                case ExecSub:
                    // Ответ на поиск подстроки: m.st = "0;3;..." или "-1"
                    std::cout << "Ok: " << m.id << ": " << m.st << std::endl;
                    {
                        auto it = std::find_if(saved_mes.begin(), saved_mes.end(),
                                               [&](const message &mm) {
                                                   return mm.command == ExecSub && mm.id == m.id;
                                               });
                        if (it != saved_mes.end()) {
                            saved_mes.erase(it);
                        }
                    }
                    break;

                case Heartbit:
                    // Узел m.id стукнул
                    last_heartbeat[m.id] = t_now();
                    break;

                case ErrorResp:
                    // Узел прислал ошибку
                    std::cerr << "Error from node " << m.id << ": " << m.st << std::endl;
                    break;

                default:
                    break;
                }
            }
        }

        // 2. Проверка «просроченных» сообщений (таймаут 30с)
        for (auto it = saved_mes.begin(); it != saved_mes.end();) {
            double diff_sec = std::difftime(t_now(), it->sent_time);
            if (diff_sec > 30.0) {
                // Считаем узел недоступным
                switch (it->command) {
                case Create:
                    std::cout << "Error: cannot create child with parent "
                              << it->id << " (parent unavailable)" << std::endl;
                    break;
                case Ping:
                    std::cout << "Error: node " << it->id << " is unavailable" << std::endl;
                    break;
                case ExecSub:
                    std::cout << "Error: node " << it->id
                              << " is unavailable (no exec response)" << std::endl;
                    break;
                default:
                    break;
                }
                it = saved_mes.erase(it);
            } else {
                ++it;
            }
        }

        // 3. Проверка heartbeat (если установлено heartbit_interval_ms > 0)
        if (heartbit_interval_ms > 0) {
            auto now_t = t_now();
            for (auto nid : all_id) {
                if (nid == -1) continue; // Управляющий
                // Когда последний раз стучал?
                if (!last_heartbeat.count(nid)) {
                    continue; // Ещё ни разу не пришёл стук
                }
                double ms_diff = std::difftime(now_t, last_heartbeat[nid]) * 1000.0;
                if (ms_diff > 4.0 * heartbit_interval_ms) {
                    std::cout << "Heartbit: node " << nid << " is unavailable now" << std::endl;
                    // Можно убрать из last_heartbeat, чтобы не повторять
                    last_heartbeat.erase(nid);
                }
            }
        }

        // 4. Читаем пользовательский ввод (не блокируясь)
        if (!inputAvailable()) {
            usleep(50000);
            continue;
        }

        // Считываем строку целиком
        if (!std::getline(std::cin, input_line)) {
            // EOF или ошибка — можно сделать break или continue
            continue;
        }
        if (input_line.empty()) {
            // Пустая строка
            continue;
        }

        // Разбираем первую команду
        std::istringstream iss(input_line);
        std::string command;
        iss >> command;

        if (command == "create") {
            int child_id, parent_id;
            if (!(iss >> child_id >> parent_id)) {
                std::cout << "Error: invalid usage of create" << std::endl;
                continue;
            }
            if (all_id.count(child_id)) {
                std::cout << "Error: Node with id " << child_id << " already exists" << std::endl;
                continue;
            }
            if (!all_id.count(parent_id)) {
                std::cout << "Error: Parent with id " << parent_id << " not found" << std::endl;
                continue;
            }
            if (parent_id == -1) {
                // Создаём локально
                Node ch = createProcess(child_id);
                children.push_back(ch);
                all_id.insert(child_id);
                std::cout << "Ok: " << ch.pid << std::endl;
            } else {
                // Посылаем команду Create -> parent_id, child_id
                message m(Create, parent_id, child_id);
                saved_mes.push_back(m);
                for (auto &ch : children) {
                    send_mes(ch, m);
                }
            }
        }
        else if (command == "ping") {
            int id;
            if (!(iss >> id)) {
                std::cout << "Error: invalid usage of ping" << std::endl;
                continue;
            }
            if (!all_id.count(id)) {
                std::cout << "Error: Node with id " << id << " doesn't exist" << std::endl;
                continue;
            }
            message m(Ping, id, 0);
            saved_mes.push_back(m);
            for (auto &ch : children) {
                send_mes(ch, m);
            }
        }
        else if (command == "heartbit") {
            int ms;
            if (!(iss >> ms)) {
                std::cout << "Error: invalid usage of heartbit" << std::endl;
                continue;
            }
            if (ms <= 0) {
                std::cout << "Error: interval must be positive" << std::endl;
                continue;
            }
            heartbit_interval_ms = ms;
            std::cout << "Ok" << std::endl;
            // Рассылаем команду Heartbit, чтобы узлы начали стучать
            message m(Heartbit, -1, ms);
            for (auto &ch : children) {
                send_mes(ch, m);
            }
        }
        else if (command == "exec") {
            // Формат: exec <node_id>
            int node_id;
            if (!(iss >> node_id)) {
                std::cout << "Error: invalid usage of exec" << std::endl;
                continue;
            }
            if (!all_id.count(node_id)) {
                std::cout << "Error: Node with id " << node_id << " doesn't exist" << std::endl;
                continue;
            }
            // Далее ждём 2 строки: текст и паттерн
            std::string text_line;
            if (!std::getline(std::cin, text_line) || text_line.empty()) {
                std::cout << "Error: no text string for exec" << std::endl;
                continue;
            }
            std::string pattern_line;
            if (!std::getline(std::cin, pattern_line) || pattern_line.empty()) {
                std::cout << "Error: no pattern string for exec" << std::endl;
                continue;
            }
            // Формируем "text_line\npattern_line"
            std::string combined = text_line + "\n" + pattern_line;

            message m(ExecSub, node_id, 0, combined.c_str());
            saved_mes.push_back(m);
            for (auto &ch : children) {
                send_mes(ch, m);
            }
        }
        else {
            std::cout << "Error: Command doesn't exist!" << std::endl;
        }

        usleep(50000);
    }

    return 0;
}
