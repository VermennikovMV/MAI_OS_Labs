Порядок запуска программы:
1. cd Lab1/src
2. g++ main.cpp -o main
3. g++ child.cpp -o child
4. ./main

Получение данных от strace (только после компиляции программы шагом выше! Так же всё выполняем в Lab1/src):
1. apt install strace (потому что докером не устанавливал я)
2. strace -f -o straced.txt ./main
Теперь данные strace можно посмотреть в файле straced.txt

Получение выделенных данных от strace:
1. strace -f -e trace=fork,execve,waitpid,exit,pipe,dup2,open,close -o bold_straced.txt ./main
