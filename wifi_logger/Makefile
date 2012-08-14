ifeq ($(CROSS_COMPILE),)
CC = gcc
endif

CPATH=${TOPDIR}/build_dir/target-mips_r2_uClibc-0.9.33/wireless_tools.29

all:
	${CC} wifi_logger.c gps.c serial.c wifi_scan.c ini.c -Wall -g -liw -o wifi_logger

upload:
	scp wifi_logger wifi_logger.ini root@192.168.1.2:~/dev

clean:
	rm wifi_logger *.o

.PHONY:
	all upload clean
