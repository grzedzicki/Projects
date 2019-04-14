Inter-process communication using SHM
./server
./client

gcc server.c -o server -lrt -pthread
gcc client.c -o client -lrt -pthread
