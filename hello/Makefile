all:
	${CC} hello.c -Wall -o hello

upload:
	scp hello root@192.168.1.1:~/dev

clean:
	rm hello

.PHONY:
	all upload clean
