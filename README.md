 g++ -g -c app.cpp -o app.o
 g++ -g app.o -o app.exe -L CommandParser/TcpServer -lnw -L CommandParser -lcli -lpthread -lrt
