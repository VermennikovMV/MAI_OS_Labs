Порядок запуска программы:
1. cd Lab2/src
2. g++ -pthread -o kmeans kmeans.cpp
3. ./kmeans 3 i

Получение данных от strace (только после компиляции программы шагом выше! Так же всё выполняем в Lab1/src):
1. apt install strace (потому что докером не устанавливал я)
2. strace -f -o straced.txt ./kmeans
Теперь данные strace можно посмотреть в файле straced.txt

Получение выделенных данных от strace:
1. strace -f -e trace=fork,execve,waitpid,exit,pipe,dup2,open,close -o bold_straced.txt ./kmeans
