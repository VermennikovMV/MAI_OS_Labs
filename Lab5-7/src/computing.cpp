#include "lib.h"
#include <vector>       // нужно для std::vector
#include <string>
#include <list>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <unistd.h>
#include <cstring>

// Вспомогательная функция поиска ВСЕХ вхождений pattern в text
static std::vector<int> findAllOccurrences(const std::string &text,
                                           const std::string &pattern)
{
    std::vector<int> positions;
    if (pattern.empty()) {
        return positions;
    }
    size_t start = 0;
    while (true) {
        size_t pos = text.find(pattern, start);
        if (pos == std::string::npos) {
            break;
        }
        positions.push_back((int)pos);
        start = pos + 1; // ищем далее
    }
    return positions;
}

int main(int argc, char *argv[])
{
    // Мой id
    int my_id = std::atoi(argv[1]);

    // Создаём узел (is_child = true => zmq_connect)
    Node self = createNode(my_id, true);

    // Храним список своих детей
    std::list<Node> children;

    // Период heartbeat (мс), если >0, шлём стук родителю
    int heartbit_interval_ms = 0;
    // Запомним время, когда в последний раз отстукивали
    auto last_hb_time = std::chrono::steady_clock::now();

    while (true) {
        // 1. Проверяем сообщения от детей
        for (auto &child : children) {
            while (true) {
                message cm = get_mes(child);
                if (cm.command == None) {
                    break; // нет сообщений
                }
                // Всё, что приходит от детей, пробрасываем родителю
                send_mes(self, cm);
            }
        }

        // 2. Проверяем сообщение от родителя
        while (true) {
            message m = get_mes(self);
            if (m.command == None) {
                break; // нет сообщений
            }
            switch (m.command) {
            case Create:
                if (m.id == my_id) {
                    // Создаём нового ребёнка
                    Node ch = createProcess(m.num);
                    children.push_back(ch);
                    // Отправляем родителю подтверждение
                    message ack(Create, ch.id, ch.pid);
                    send_mes(self, ack);
                } else {
                    // Пробрасываем дальше детям
                    for (auto &ch : children) {
                        send_mes(ch, m);
                    }
                }
                break;

            case Ping:
                if (m.id == my_id) {
                    // Ответ
                    send_mes(self, m);
                } else {
                    // Проброс
                    for (auto &ch : children) {
                        send_mes(ch, m);
                    }
                }
                break;

            case ExecSub:
                if (m.id == my_id) {
                    // Разбираем text + '\n' + pattern
                    std::string combined(m.st);
                    auto pos_nl = combined.find('\n');
                    if (pos_nl == std::string::npos) {
                        // Неверный формат => шлём ошибку (ErrorResp)
                        message err(ErrorResp, my_id, 0, "Bad exec format");
                        send_mes(self, err);
                        break;
                    }
                    std::string text_str    = combined.substr(0, pos_nl);
                    std::string pattern_str = combined.substr(pos_nl + 1);

                    // Поиск
                    auto occ = findAllOccurrences(text_str, pattern_str);
                    if (occ.empty()) {
                        // Шлём "-1"
                        message ans(ExecSub, my_id, 0, "-1");
                        send_mes(self, ans);
                    } else {
                        // Формируем "0;3;5" и т.д.
                        std::string res;
                        for (size_t i = 0; i < occ.size(); i++) {
                            if (i > 0) res += ";";
                            res += std::to_string(occ[i]);
                        }
                        message ans(ExecSub, my_id, 0, res.c_str());
                        send_mes(self, ans);
                    }
                } else {
                    // Пробрасываем детям
                    for (auto &ch : children) {
                        send_mes(ch, m);
                    }
                }
                break;

            case Heartbit:
                // Родитель сказал "стучи" каждые m.num мс
                heartbit_interval_ms = m.num;
                break;

            case ErrorResp:
                // Можно как-то обработать
                std::cerr << "Error from parent: " << m.st << std::endl;
                break;

            default:
                // Проброс дальше детям
                for (auto &ch : children) {
                    send_mes(ch, m);
                }
                break;
            }
        }

        // 3. Отправляем heartbeat (если нужно)
        if (heartbit_interval_ms > 0) {
            auto now = std::chrono::steady_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_hb_time).count();
            if (diff >= heartbit_interval_ms) {
                message hb(Heartbit, my_id, 0);
                send_mes(self, hb);
                last_hb_time = std::chrono::steady_clock::now();
            }
        }

        usleep(50000);
    }

    return 0;
}
