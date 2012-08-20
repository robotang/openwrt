all:
	${CC} capture.c camera.c -Wall -o capture

upload:
	scp capture root@192.168.1.1:~/dev

clean:
	rm capture

.PHONY:
	all upload clean
