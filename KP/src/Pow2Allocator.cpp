#include "Pow2Allocator.h"
#include <cmath>
#include <cstring>

// Конструктор
Allocator_Pow2::Allocator_Pow2(void* realMemory, size_t memory_size)
{
    bufferStart = reinterpret_cast<char*>(realMemory);
    totalSize   = memory_size;

    // Изначально обнуляем все списки
    for(int i = 0; i <= MAX_POW; i++) {
        freeLists[i] = nullptr;
    }

    // Раз уж мы «разбиваем» всё пространство на блоки 2^n,
// упрощённо — проинициализируем его целиком списками по максимально большому блоку,
// затем можем «нарезать» вручную. Но для примера можно всё отдать как один большой блок
// ровно 2^P, где P — ближайшая вниз степень, чтобы не усложнять логику разбиения.

    // 1) Находим самую большую степень двойки, которая <= memory_size:
    int bigP = MAX_POW;
    while( bigP >= MIN_POW && ( (size_t(1)<<bigP) > memory_size ) ) {
        bigP--;
    }
    // Если уж совсем ничего не влезает, останавливаемся
    if(bigP < MIN_POW) bigP = MIN_POW;

    // Создаём один большой блок 2^bigP
    Header* h = reinterpret_cast<Header*>(bufferStart);
    h->sizePow = bigP;
    h->used    = false;
    // Следующий «указатель» в односвязном списке хранить будем в самом блоке
    // просто через reinterpret_cast<Header**>(...) и т.п.
    // Но для простоты пока сделаем так, что «адрес следующего» лежит после заголовка.
    // freeLists[bigP] = (void*)((char*)h + sizeof(Header)); // вариант 1
    freeLists[bigP] = h; // вариант 2: будем трактовать сам Header* как голову списка
}

int Allocator_Pow2::findPow2(size_t needSize)
{
    // Ищем степень p, чтобы 2^p >= needSize
    // Можно сделать быстро через built-in __builtin_clz и т.д., здесь сделаем цикл
    int p = MIN_POW;
    while(p <= MAX_POW)
    {
        if( (size_t(1)<<p) >= needSize )
            return p;
        p++;
    }
    return -1; // не нашли подходящий размер
}

void* Allocator_Pow2::alloc(size_t block_size)
{
    // + заголовок
    size_t totalNeeded = block_size + sizeof(Header);
    int p = findPow2(totalNeeded);
    if(p < 0) {
        return nullptr; // слишком большой запрос
    }
    // Ищем любую свободную «цепочку» начиная с p и выше
    int tryP = p;
    for(; tryP <= MAX_POW; tryP++)
    {
        if(freeLists[tryP] != nullptr)
        {
            // Берём голову списка
            Header* blockHead = reinterpret_cast<Header*>( freeLists[tryP] );

            // Убираем из freeLists[tryP]
            freeLists[tryP] = nullptr; // т.к. мы хранить «один блок» для примера
            // (в реальном коде, если список из многих, надо брать первый элемент
            //  и перенастраивать указатель списка на следующий)

            blockHead->used = true; // пометим, что занято

            // Если блок слишком большой и нужно расколоть — в упрощённом варианте
            // мы можем не делать этого (или сделать на уровне buddy). 
            // Для демонстрации не будем.  
            // Просто вернём этот большой блок. Реально: тут бы раскалывать вниз
            // до нужного p, но это уже buddy-аллокатор.
            
            return (void*)(blockHead + 1); 
        }
    }
    // не нашли свободных блоков подходящего размера
    return nullptr;
}

void Allocator_Pow2::free(void* ptr)
{
    if(!ptr) return;
    Header* h = reinterpret_cast<Header*>(ptr) - 1;
    h->used = false;

    // Возвращаем в список freeLists[h->sizePow], 
    // но у нас сейчас хранится «один блок» на список. Для наглядности:
    // Если там что-то уже есть, мы его потеряем… В реальном коде надо делать из этого
    // Linked list. Здесь — сверх-упрощённый вариант.
    if(freeLists[h->sizePow] == nullptr)
    {
        freeLists[h->sizePow] = h;
    }
    else
    {
        freeLists[h->sizePow] = h;
    }
}

size_t Allocator_Pow2::get_free_memory()
{
    // Пройдёмся по всем freeLists[] и суммируем
    size_t totalFree = 0;
    for(int i = MIN_POW; i <= MAX_POW; i++)
    {
        if(freeLists[i] != nullptr)
        {
            // Если хотим полноценный список — надо бежать по цепочке
            // Здесь считаем, что в каждом freeLists[i] может лежать только один блок
            Header* h = reinterpret_cast<Header*>(freeLists[i]);
            if(!h->used)
            {
                // размер блока = 2^i
                totalFree += (size_t(1) << i);
            }
        }
    }
    return totalFree;
}
