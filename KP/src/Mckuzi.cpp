#include "Mckuzi.h"

Allocator_McKuzi::Allocator_McKuzi(void* realMemory, size_t memory_size)
{
    // Инициализируем один большой блок
    head = reinterpret_cast<Block*>(realMemory);
    head->size = memory_size - sizeof(Block);
    head->free = true;
    head->next = nullptr;
}

void* Allocator_McKuzi::alloc(size_t block_size)
{
    // Ищем блок с минимальным «избытком» (best fit)
    Block *bestBlock = nullptr;
    Block *prevBest  = nullptr; // предыдущий для bestBlock
    Block *cur       = head;
    Block *prev      = nullptr;

    size_t minDiff = (size_t)-1; // очень большое число

    // 1) Найдём блок best fit
    while (cur != nullptr)
    {
        if (cur->free && cur->size >= block_size)
        {
            size_t diff = cur->size - block_size;
            if (diff < minDiff)
            {
                minDiff = diff;
                bestBlock = cur;
                prevBest  = prev;
            }
        }
        prev = cur;
        cur  = cur->next;
    }

    // Если подходящего блока не нашлось
    if (!bestBlock)
    {
        return nullptr;
    }

    // 2) Попробуем разделить (если достаточно места для заголовка + данных)
    if (bestBlock->size >= block_size + sizeof(Block) + 8)
    {
        // Создаём новый блок «хвост» из избытка
        char* basePtr = reinterpret_cast<char*>(bestBlock);
        Block *newBlock = reinterpret_cast<Block*>( basePtr + sizeof(Block) + block_size );
        newBlock->size = bestBlock->size - block_size - sizeof(Block);
        newBlock->free = true;
        newBlock->next = bestBlock->next;

        // Изменяем размер у блока, который отдаём
        bestBlock->size = block_size;
        bestBlock->free = false;
        bestBlock->next = newBlock;
    }
    else
    {
        // Не делим — просто делаем блок занятым
        bestBlock->free = false;
    }

    // 3) Возвращаем указатель на «полезную» область (после заголовка)
    return reinterpret_cast<void*>(bestBlock + 1);
}

void Allocator_McKuzi::free(void* ptr)
{
    if (!ptr) return;
    // Получаем заголовок
    Block *block = reinterpret_cast<Block*>(ptr) - 1;
    block->free = true;

    // Попробуем объединять блоки (coalescing) с соседями
    // Для этого пройдёмся с начала списка
    Block *cur = head;
    Block *prev = nullptr;

    while (cur)
    {
        if (cur == block)
        {
            // Попробуем слить с предыдущим
            if (prev && prev->free)
            {
                // Объединяем prev и cur
                prev->size += sizeof(Block) + cur->size;
                prev->next = cur->next;
                cur = prev; // текущий блок расширен
            }
            // Попробуем слить с следующим
            if (cur->next && cur->next->free)
            {
                Block *tmp = cur->next;
                cur->size += sizeof(Block) + tmp->size;
                cur->next = tmp->next;
            }
            break; // закончили объединение
        }
        prev = cur;
        cur  = cur->next;
    }
}

size_t Allocator_McKuzi::get_free_memory()
{
    size_t totalFree = 0;
    Block *cur = head;
    while (cur)
    {
        if (cur->free)
        {
            totalFree += cur->size;
        }
        cur = cur->next;
    }
    return totalFree;
}
