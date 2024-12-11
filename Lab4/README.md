Порядок запуска программы:
1. cd Lab4/src
2. make
3. ./program1 
4. ./program2

Как работает program1:
1 A deltaX – вызывает Derivative из уже подлинкованной библиотеки.
2 A B – вызывает Square из подлинкованной библиотеки.
0 – сообщает, что переключение невозможно.

Как работает program1:
1 A deltaX – вызывает Derivative из уже подлинкованной библиотеки.
2 A B – вызывает Square из подлинкованной библиотеки.
0 – закрывает текущие библиотеки (dlclose), затем открывает другой набор (libderivative2.so и libsquare2.so).

Получение данных от strace (только после компиляции программы шагом выше! Так же всё выполняем в Lab3/src):
1. apt install strace (потому что докером не устанавливал я)
2. strace -f -o straced.txt ./program1
Теперь данные strace можно посмотреть в файле straced.txt

Получение выделенных данных от strace:
1. strace -f -e trace=fork,execve,waitpid,exit,pipe,dup2,open,close -o bold_straced.txt ./main
