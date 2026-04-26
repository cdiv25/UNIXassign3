CC=gcc
CFLAGS=-Wall -g
LIBS=-lc

all: server client

# server stuff
server: server.o shared.o
	$(CC) $(LIBS) server.o shared.o -o server

server.o: iserver.c shared.h QuizDB.h
	$(CC) -c $(CFLAGS) iserver.c -o server.o

# client stuff
client: client.o shared.o
	$(CC) $(LIBS) client.o shared.o -o client

client.o: iclient.c shared.h QuizDB.h
	$(CC) -c $(CFLAGS) iclient.c -o client.o

# other
clean:
	rm *.o server client