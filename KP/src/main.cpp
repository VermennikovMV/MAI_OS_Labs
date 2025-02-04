#include <sys/mman.h>
#include <chrono>
#include <vector>
#include <thread>
#include <random>
#include <iostream>

// Подключаем наши заголовки с двумя реализациями
#include "Mckuzi.h"
#include "Pow2Allocator.h"

int main()
{
    std::cout << "Creating allocators..\n";

    // Общий объём памяти, отведённый под оба аллокатора
    size_t size = 1024 * 1024 * 1024; // 1 Гб, как в примере

    // MAP_PRIVATE | MAP_ANONYMOUS: анонимная память, изменения видны только текущему процессу
    void *mem1 = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    void *mem2 = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // Создаём объекты аллокаторов
    Allocator_McKuzi   al1(mem1, size);
    Allocator_Pow2     al2(mem2, size);

    // Первичная проверка свободной памяти
    std::cout << "All memory: " 
              << al1.get_free_memory() << " (McKuzi), "
              << al2.get_free_memory() << " (Pow2)\n";
    std::cout << "Created\n\n";

    // -------------------
    // TEST 1: Много маленьких блоков одного размера
    // -------------------
    {
        std::cout << "TEST 1 -- allocate lots of memory of a single little size\n";
        int cycl_count = 5;
        std::cout << "Cycles count = " << cycl_count << "\n\n";
        int blocks_count = 10000;

        // В примере: [1, 4, 8, 16, 32, 64], 
        // но мы можем сделать аналогично с «1 << t1_sz[i]» 
        // (т.е. на самом деле это размеры 2^1, 2^4, 2^8 ...).
        // Для наглядности оставим так же, как в примере:
        int t1_sz[5] = {1,4,8,16,32};

        for (int i = 0; i < cycl_count; i++)
        {
            std::cout << "Block size = 2^" << t1_sz[i]
                      << " (=" << (1 << t1_sz[i]) << ")"
                      << ", Block count = " << blocks_count << std::endl;

            // -- Выделяем blocks_count блоков аллокатором al1
            std::vector<void*> ptrs1(blocks_count);
            auto start = std::chrono::high_resolution_clock::now();
            for (int j = 0; j < blocks_count; j++)
            {
                ptrs1[j] = al1.alloc(1 << t1_sz[i]);
            }
            auto end = std::chrono::high_resolution_clock::now();
            auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            // -- Аналогично al2
            std::vector<void*> ptrs2(blocks_count);
            start = std::chrono::high_resolution_clock::now();
            for (int j = 0; j < blocks_count; j++)
            {
                ptrs2[j] = al2.alloc(1 << t1_sz[i]);
            }
            end = std::chrono::high_resolution_clock::now();
            auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            // -- Освобождаем в аллокаторе al1
            start = std::chrono::high_resolution_clock::now();
            for (int j = 0; j < blocks_count; j++)
            {
                al1.free(ptrs1[j]);
            }
            end = std::chrono::high_resolution_clock::now();
            auto duration3 = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            // -- И в аллокаторе al2
            start = std::chrono::high_resolution_clock::now();
            for (int j = 0; j < blocks_count; j++)
            {
                al2.free(ptrs2[j]);
            }
            end = std::chrono::high_resolution_clock::now();
            auto duration4 = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            std::cout << "Result Allocate:\n"
                      << " McKuzi: " << duration1.count() << " us, "
                      << " Pow2:   " << duration2.count() << " us,  ratio="
                      << (duration2.count() ? double(duration1.count())/duration2.count() : 0.0)
                      << "\n";
            std::cout << "Result Free:\n"
                      << " McKuzi: " << duration3.count() << " us, "
                      << " Pow2:   " << duration4.count() << " us,  ratio="
                      << (duration4.count() ? double(duration3.count())/duration4.count() : 0.0)
                      << "\n\n";
        }
    }

    // -------------------
    // TEST 2: Выделение памяти при сильной фрагментации
    // -------------------
    {
        std::cout << "TEST 2 -- allocate a lot of memory in the face of high fragmentation\n";
        int blocks_count = 10000;
        std::cout << "Allocate " << blocks_count 
                  << " blocks of size=100 (McKuzi & Pow2)\n";

        // -- аллокация для al1
        std::vector<void *> frament_blocks1(blocks_count);
        for (int i = 0; i < blocks_count; ++i)
        {
            frament_blocks1[i] = al1.alloc(100);
        }
        std::cout << "Free all even blocks (al1)\n";
        for (int i = 0; i < blocks_count; ++i)
        {
            if (i % 2 == 0) // Освобождаем чётные
                al1.free(frament_blocks1[i]);
        }

        // -- аналогично для al2
        std::vector<void *> frament_blocks2(blocks_count);
        for (int i = 0; i < blocks_count; ++i)
        {
            frament_blocks2[i] = al2.alloc(100);
        }
        std::cout << "Free all even blocks (al2)\n";
        for (int i = 0; i < blocks_count; ++i)
        {
            if (i % 2 == 0)
                al2.free(frament_blocks2[i]);
        }

        std::cout << "Fragmentation completed\n";

        // Теперь пробуем выделить (blocks_count/2) блоков по 80
        int half = blocks_count / 2;
        std::cout << "Block size=80, Block count=" << half << std::endl;

        // -- alloc
        std::vector<void*> ptrs1(half);
        auto start = std::chrono::high_resolution_clock::now();
        for (int j = 0; j < half; j++)
        {
            ptrs1[j] = al1.alloc(80);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::vector<void*> ptrs2(half);
        start = std::chrono::high_resolution_clock::now();
        for (int j = 0; j < half; j++)
        {
            ptrs2[j] = al2.alloc(80);
        }
        end = std::chrono::high_resolution_clock::now();
        auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        // -- free
        start = std::chrono::high_resolution_clock::now();
        for (int j = 0; j < half; j++)
        {
            al1.free(ptrs1[j]);
        }
        end = std::chrono::high_resolution_clock::now();
        auto duration3 = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        start = std::chrono::high_resolution_clock::now();
        for (int j = 0; j < half; j++)
        {
            al2.free(ptrs2[j]);
        }
        end = std::chrono::high_resolution_clock::now();
        auto duration4 = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "Result Allocate:\n"
                  << " McKuzi: " << duration1.count() << " us, "
                  << " Pow2:   " << duration2.count() << " us, ratio="
                  << (duration2.count() ? double(duration1.count())/duration2.count() : 0.0)
                  << "\n";
        std::cout << "Result Free:\n"
                  << " McKuzi: " << duration3.count() << " us, "
                  << " Pow2:   " << duration4.count() << " us, ratio="
                  << (duration4.count() ? double(duration3.count())/duration4.count() : 0.0)
                  << "\n\n";

        // Наконец, полностью освобождаем неосвобождённые блоки
        for (int i = 0; i < blocks_count; ++i)
        {
            if (i % 2 != 0) // Освобождаем только нечетные (чётные мы уже освободили)
            {
                al1.free(frament_blocks1[i]);
                al2.free(frament_blocks2[i]);
            }
        }
    }

    // -------------------
    // TEST 3: Случайные размеры блоков
    // -------------------
    {
        std::cout << "TEST 3 -- allocate random blocks\n";
        int num_iterations = 5;
        int num_blocks     = 10000; // Use a lot of blocks
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(4, 1024); // 4..1024

        for (int iter = 0; iter < num_iterations; ++iter)
        {
            std::vector<size_t> sizes(num_blocks);
            std::vector<void*> ptrs1(num_blocks);
            std::vector<void*> ptrs2(num_blocks);

            // генерируем размеры
            for (int i = 0; i < num_blocks; ++i)
            {
                sizes[i] = distrib(gen);
            }

            // -- alloc McKuzi
            auto start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < num_blocks; ++i)
            {
                ptrs1[i] = al1.alloc(sizes[i]);
            }
            auto end = std::chrono::high_resolution_clock::now();
            auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            // -- alloc Pow2
            start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < num_blocks; ++i)
            {
                ptrs2[i] = al2.alloc(sizes[i]);
            }
            end = std::chrono::high_resolution_clock::now();
            auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            // -- free McKuzi
            start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < num_blocks; ++i)
            {
                al1.free(ptrs1[i]);
            }
            end = std::chrono::high_resolution_clock::now();
            auto duration3 = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            // -- free Pow2
            start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < num_blocks; ++i)
            {
                al2.free(ptrs2[i]);
            }
            end = std::chrono::high_resolution_clock::now();
            auto duration4 = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            std::cout << "Iteration " << (iter+1) << ":\n"
                      << " Allocate => McKuzi: " << duration1.count() << " us, "
                      << "Pow2: " << duration2.count() << " us, ratio="
                      << (duration2.count() ? double(duration1.count())/duration2.count() : 0.0) << "\n"
                      << " Free     => McKuzi: " << duration3.count() << " us, "
                      << "Pow2: " << duration4.count() << " us, ratio="
                      << (duration4.count() ? double(duration3.count())/duration4.count() : 0.0) << "\n\n";
        }
    }

    // -------------------
    // TEST 4: Usage factor (McKuzi)
    // -------------------
    {
        std::cout << "TEST 4 -- Usage factor (McKuzi)\n";
        const int num_requests = 10000;
        const size_t max_block_size = 512;
        std::uniform_int_distribution<size_t> dist(16, max_block_size);
        std::random_device rd;
        std::mt19937 gen(rd());

        size_t total_allocated = 0;
        size_t memory_size = al1.get_free_memory();
        std::cout << "All memory (McKuzi) = " << memory_size << '\n';

        std::vector<void*> ptrs(num_requests);
        for (int i = 0; i < num_requests; ++i)
        {
            size_t req_size = dist(gen);
            void* p = al1.alloc(req_size);
            ptrs[i] = p;
            if (p) {
                total_allocated += req_size;
            }
        }

        double utilization_factor = 0.0;
        size_t free_mem_now = al1.get_free_memory();
        if ((memory_size - free_mem_now) != 0)
        {
            utilization_factor = double(total_allocated) / double(memory_size - free_mem_now);
        }

        std::cout << "Total allocated: " << total_allocated << " bytes\n"
                  << "Free memory:     " << free_mem_now    << " bytes\n"
                  << "Utilization factor: " << utilization_factor << std::endl;

        // Освобождаем
        for (int i = 0; i < num_requests; ++i)
        {
            al1.free(ptrs[i]);
        }
    }

    // -------------------
    // TEST 5: Usage factor (Pow2)
    // -------------------
    {
        std::cout << "TEST 5 -- Usage factor (Pow2)\n";
        const int num_requests = 10000;
        const size_t max_block_size = 512;
        std::uniform_int_distribution<size_t> dist(16, max_block_size);
        std::random_device rd;
        std::mt19937 gen(rd());

        size_t total_allocated = 0;
        size_t memory_size = al2.get_free_memory();
        std::cout << "All memory (Pow2) = " << memory_size << '\n';

        std::vector<void*> ptrs(num_requests);
        for (int i = 0; i < num_requests; ++i)
        {
            size_t req_size = dist(gen);
            void* p = al2.alloc(req_size);
            ptrs[i] = p;
            if (p) {
                total_allocated += req_size;
            }
        }

        double utilization_factor = 0.0;
        size_t free_mem_now = al2.get_free_memory();
        if ((memory_size - free_mem_now) != 0)
        {
            utilization_factor = double(total_allocated) / double(memory_size - free_mem_now);
        }

        std::cout << "Total allocated: " << total_allocated << " bytes\n"
                  << "Free memory:     " << free_mem_now    << " bytes\n"
                  << "Utilization factor: " << utilization_factor << std::endl;

        // Освобождаем
        for (int i = 0; i < num_requests; ++i)
        {
            al2.free(ptrs[i]);
        }
    }

    // Освобождаем выделенную под аллокаторы память
    munmap(mem1, size);
    munmap(mem2, size);

    return 0;
}
