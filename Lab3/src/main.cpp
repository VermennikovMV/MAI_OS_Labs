#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>

struct SharedData {
    int len;        // Кол-во чисел
    int error_flag; // Флаг ошибки: 0 - нет ошибки, 1 - деление на ноль
    // Далее идут числа и результат. Структура памяти:
    // [int len][int error_flag][float numbers[len]][float results[len-1]]
    // однако мы не можем заранее задать массив переменной длины в структуре,
    // поэтому будем обращаться к ним по смещению
};

int main() {
    std::cout << "Введите числа в формате: «число число число<endline>»." << std::endl;

    std::vector<float> numbers;
    float number;
    while (std::cin >> number) {
        numbers.push_back(number);
        if (std::cin.peek() == '\n') break;
    }

    int len = (int)numbers.size();
    if (len < 2) {
        std::cerr << "Нужно как минимум 2 числа." << std::endl;
        return 1;
    }

    // Рассчитаем размер, который нам нужен под mmap:
    // SharedData + numbers[len] + results[len-1]
    // Размер: sizeof(SharedData) + len*sizeof(float) + (len-1)*sizeof(float)
    size_t size = sizeof(SharedData) + len * sizeof(float) + (len - 1) * sizeof(float);

    const char* filename = "shared.bin";
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        std::cerr << "Ошибка при открытии файла для mmap" << std::endl;
        return 1;
    }

    if (ftruncate(fd, size) == -1) {
        std::cerr << "Ошибка при изменении размера файла" << std::endl;
        close(fd);
        return 1;
    }

    void* addr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        std::cerr << "Ошибка при mmap" << std::endl;
        close(fd);
        return 1;
    }

    close(fd); // Дескриптор файла больше не нужен, т.к. мы уже сделали mmap

    // Запишем данные в общую память
    SharedData* shared = (SharedData*)addr;
    shared->len = len;
    shared->error_flag = 0;

    // Область в памяти после SharedData для чисел и результатов
    float* numbers_ptr = (float*)((char*)addr + sizeof(SharedData));
    float* results_ptr = numbers_ptr + len; 

    // Записываем числа
    memcpy(numbers_ptr, numbers.data(), len * sizeof(float));

    // Создаем дочерний процесс
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Ошибка при создании процесса" << std::endl;
        munmap(addr, size);
        return 1;
    }

    if (pid > 0) { 
        // Родительский процесс
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            if (shared->error_flag == 1) {
                std::cerr << "Деление на 0. Завершение работы." << std::endl;
            } else {
                // Читаем результат
                int res_len = len - 1;
                std::vector<float> results(res_len);
                memcpy(results.data(), results_ptr, res_len * sizeof(float));

                std::cout << "Результат деления: " << results.back() << std::endl;
            }
        } else {
            std::cerr << "Дочерний процесс завершился нештатно." << std::endl;
        }

        munmap(addr, size);
    } else { 
        // Дочерний процесс: запускаем child
        execl("./child", "child", filename, (char*)NULL);
        std::cerr << "Ошибка при вызове execl" << std::endl;
        munmap(addr, size);
        return 1;
    }

    return 0;
}
