#include <unistd.h>
#include <vector>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <cstdio>
#include <sys/mman.h>
#include <cstring>
#include <cstdlib>

struct SharedData {
    int len;
    int error_flag; 
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Не передано имя файла" << std::endl;
        return 1;
    }

    const char* filename = argv[1];
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        std::cerr << "Ошибка при открытии файла" << std::endl;
        return 1;
    }

    // Определим размер файла
    struct stat st;
    if (fstat(fd, &st) == -1) {
        std::cerr << "Ошибка fstat" << std::endl;
        close(fd);
        return 1;
    }
    size_t size = st.st_size;

    void* addr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        std::cerr << "Ошибка при mmap в дочернем процессе" << std::endl;
        close(fd);
        return 1;
    }

    close(fd);

    SharedData* shared = (SharedData*)addr;
    int len = shared->len;
    float* numbers_ptr = (float*)((char*)addr + sizeof(SharedData));
    float* results_ptr = numbers_ptr + len;

    std::vector<float> numbers(len);
    memcpy(numbers.data(), numbers_ptr, len * sizeof(float));

    float first_number = numbers[0];
    int res_len = len - 1;
    std::vector<float> results(res_len);

    for (int i = 1; i < len; ++i) {
        if (numbers[i] == 0) {
            // Деление на ноль
            shared->error_flag = 1;
            munmap(addr, size);
            exit(0);
        }
        results[i - 1] = first_number / numbers[i];
    }

    // Записываем результаты обратно
    memcpy(results_ptr, results.data(), res_len * sizeof(float));

    munmap(addr, size);
    return 0;
}
