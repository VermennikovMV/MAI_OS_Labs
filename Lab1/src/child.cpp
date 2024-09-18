#include <unistd.h>
#include <vector>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <cstdio>

int main() {
    // Чтение чисел из stdin
    int len;
    if (read(STDIN_FILENO, &len, sizeof(int)) == -1) {
        std::cout <<  "Ошибка при чтении длины";
        return 1;
    }
    std::vector<float> numbers(len);
    if (read(STDIN_FILENO, numbers.data(), len * sizeof(float)) == -1) {
        std::cout <<  "Ошибка при чтении чисел";
        return 1;
    }

    // Открытие файла для записи
    int fd = open("result.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        std::cout << "Ошибка при открытии файла";
        return 1;
    }

    // Выполнение деления и запись результатов
    float first_number = numbers[0];
    int res_len = len - 1;
    std::vector<float> results(res_len);

    for (int i = 1; i < len; ++i) {
        if (numbers[i] == 0) {
            close(fd);
            exit(1);  // Деление на 0
        }
        results[i - 1] = first_number / numbers[i];

        if (i == len - 1) {
            // Форматирование результата в строку
            char buffer[64];
            int n = snprintf(buffer, sizeof(buffer), "%f\n", results[i - 1]);
            if (n <= 0) {
                std::cout << "Ошибка при форматировании строки";
                close(fd);
                return 1;
            }

            // Запись строки в файл
            if (write(fd, buffer, n) == -1) {
                std::cout << "Ошибка при записи в файл";
                close(fd);
                return 1;
            }
        }
    }

    close(fd);

    // Передача результатов родительскому процессу через stdout
    if (write(STDOUT_FILENO, results.data(), res_len * sizeof(float)) == -1) {
        std::cout << "Ошибка при записи результатов";
        return 1;
    }

    return 0;
}
