Перед использованием cmake нужно установить библиотеку:
apt-get install libzmq3-dev

Чтобы проверить работоспособность heartbit, то нужно сделать следующее после cmske:
1. ./control
2. create 10 -1
Запоминаем pid, который написан после "Ok"
3. Открываем другую консоль bash и пишем в ней kill -9 (pid дочернего узла 10)
4. Заходим в исходную консоль bash, ждём 8 секунд и видим "Heartbit: node 10 is unavailable now"