#pragma once
#include <cstddef>
#include <iostream>
#include <vector>

// Упрощённый вариант «блоки по 2^n»:
// - Допустим, что минимальный блок = 2^MIN_POW, максимальный = 2^MAX_POW
// - Храним массив списков свободных блоков: freeLists[i] для блоков размера 2^(i)
// - При alloc находим 2^k >= (block_size + заголовок), выдаём блок из freeLists[k]
//   (если там пусто — попробуйте больший k).
// - При free возвращаем блок обратно в соответствующий список (без слияния).
// - Реальное объединение/деление для 2^n обычно приводит к buddy-аллокатору.
//   Здесь, для простоты, «без объединения», иначе это будет почти Buddy :)

class Allocator_Pow2
{
    struct Header {
        size_t sizePow;   // степень двойки, которая описывает размер этого блока
        bool   used;      // метка занятости (для отладки, не обязательно)
    };

private:
    static const int MIN_POW = 4;  // минимальный блок 16 байт (можно настроить)
    static const int MAX_POW = 30; // верхняя граница ~1 Гб, чтобы не вылезти
    // Массив списков (в виде односвязных цепочек)
    // freeLists[k] хранит указатели на блоки размером 2^k
    void* freeLists[MAX_POW + 1];

    // Указатель на начало всей области
    char* bufferStart;
    size_t totalSize;

public:
    Allocator_Pow2(void* realMemory, size_t memory_size);

    void* alloc(size_t block_size);
    void  free(void* ptr);

    size_t get_free_memory();

private:
    // Вспомогательные методы
    int   findPow2(size_t needSize);
    void  initMemory();
    size_t blockSizeFromPow(int p) { return (size_t(1) << p); }
};
