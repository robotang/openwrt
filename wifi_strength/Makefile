ifeq ($(CROSS_COMPILE),)
CC = gcc
endif

all:
	${CC} main.c gps.c serial.c -Wall -g -o app

upload:
	scp app root@192.168.1.1:~/dev

clean:
	rm app *.o

.PHONY:
	all upload clean
