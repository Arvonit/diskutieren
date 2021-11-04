CC=clang
CFLAGS=-Wall -Werror -Wextra -Wno-unused-parameter -Wno-unused-variable -std=c99

all: client server

debug: CFLAGS += -g
debug: client server

client: client.o util.o
	$(CC) $(CFLAGS) -o client client.o util.o

server: server.o util.o
	$(CC) $(CFLAGS) -o server server.o util.o

util.o: util.c util.h
	$(CC) $(CFLAGS) -c util.c

client.o: client.c client.h
	$(CC) $(CFLAGS) -c client.c

server.o: server.c client.h
	$(CC) $(CFLAGS) -c server.c

clean:
	rm -f client server a.out *.dSYM *.o
