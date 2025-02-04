#pragma once
#include <cstddef>  // size_t
#include <iostream>

// Простейший вариант «Мак–Кьюзи–Кэрелс» можно сделать как “best fit” на списке
// или как несколько списков (segregated lists) — здесь для наглядности взят
// один список + поиск лучшего блока (best fit). Это не строгое каноническое
// воплощение Мак–Кьюзи–Кэрелса, но иллюстрирует подход.

// Структура описывает управляющий блок и память
class Allocator_McKuzi
{
    struct Block
    {
        size_t size;  // размер данного блока (без учёта заголовка)
        bool free;     // свободен ли блок
        Block* next;   // односвязный список
    };

private:
    Block *head; // Голова списка

public:
    // Конструктор: realMemory — указатель на область памяти (mmap),
    // memory_size — её полный размер
    Allocator_McKuzi(void* realMemory, size_t memory_size);

    // Выделение памяти
    void* alloc(size_t block_size);

    // Освобождение памяти
    void free(void* ptr);

    // Подсчитать сколько всего свободно байт
    size_t get_free_memory();
};
