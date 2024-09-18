#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int main() {
    int fd1[2], fd2[2];
    if (pipe(fd1) == -1 || pipe(fd2) == -1) {
        std::cout << "Ошибка при создании pipe";
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cout << "Ошибка при создании процесса";
        return 1;
    }

    if (pid > 0) { // Родительский процесс
        close(fd1[0]); // Закрываем чтение из первого pipe
        close(fd2[1]); // Закрываем запись во второй pipe

        std::cout << "Введите числа в формате: «число число число<endline>»." << std::endl;

        std::vector<float> numbers;
        float number;
        while (std::cin >> number) {
            numbers.push_back(number);
            if (std::cin.peek() == '\n') break; //Сравнивается с помощью cin.peek() последующий символ, не извлекаясь. То есть если пользователь нажал enter, то ввод завершён
        }

        // Передача чисел дочернему процессу
        int len = numbers.size();
        if (write(fd1[1], &len, sizeof(int)) == -1) {
            std::cout << "Ошибка при записи длины";
            return 1;
        }
        if (write(fd1[1], numbers.data(), len * sizeof(float)) == -1) {
            std::cout << "Ошибка при записи чисел";
            return 1;
        }

        // Ожидание завершения дочернего процесса
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 1) {
            std::cerr << "Деление на 0. Завершение работы." << std::endl;
            return 1;
        }

        // Чтение результата
        int res_len = len - 1;
        std::vector<float> results(res_len);
        if (read(fd2[0], results.data(), res_len * sizeof(float)) == -1) {
            std::cout << "Ошибка при чтении результатов";
            return 1;
        }

        std::cout << "Результат деления: ";
        std::cout << results.back() << " ";
        std::cout << std::endl;

        close(fd1[1]);
        close(fd2[0]);
    } else { // Дочерний процесс
        close(fd1[1]); // Закрываем запись в первый pipe
        close(fd2[0]); // Закрываем чтение из второго pipe

        // Перенаправление stdin и stdout на pipe
        if (dup2(fd1[0], STDIN_FILENO) == -1) {
            std::cout << "Ошибка при перенаправлении stdin";
            return 1;
        }
        if (dup2(fd2[1], STDOUT_FILENO) == -1) {
            std::cout << "Ошибка при перенаправлении stdout";
            return 1;
        }

        // Запуск дочернего процесса
        execl("./child", "child", NULL);
        std::cout << "Ошибка при вызове execl";
        return 1;
    }

    return 0;
}
