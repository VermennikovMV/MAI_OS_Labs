Порядок запуска программы:
1. cd KP/src
2. cmake CMakeLists.txt
3. make
4. ./allocator_compare

Получение данных от strace (только после компиляции программы шагом выше! Так же всё выполняем в KP/src):
1. apt install strace (потому что докером не устанавливал я)
2. strace -f -o straced.txt ./allocator_compare
Теперь данные strace можно посмотреть в файле straced.txt