CC = gcc
DEBUG = -g
CFLAGS = -Wall -lpthread -pthread -c $(DEBUG)
LFLAGS = -Wall -lpthread -pthread $(DEBUG)

all: sftp server


server: server.o chatroom_utils.o log.o
	$(CC) $(LFLAGS) server.o chatroom_utils.o log.o -o server

server.o: server.c chatroom_utils.h log.h
	$(CC) $(CFLAGS) server.c

sftp: sftp.o chatroom_utils.o log.o
	$(CC) $(LFLAGS) sftp.o chatroom_utils.o log.o -o sftp

sftp.o: sftp.c chatroom_utils.h log.h
	$(CC) $(CFLAGS) sftp.c

chatroom_utils.o: chatroom_utils.h chatroom_utils.c
	$(CC) $(CFLAGS) chatroom_utils.c

log.o: log.h log.c
	$(CC) $(CFLAGS) log.c

clean:
	rm -rf *.o *~ sftp server
