SER2NET = ser2net-2.8-rc2

all:
	cd $(SER2NET); \
	./configure --target=mipsel-linux --host=mipsel-linux --build=i386-pc-linux-gnu --prefix=/usr
	cd $(SER2NET); \
	make

download:
	wget http://sourceforge.net/projects/ser2net/files/$(SER2NET).tar.gz
	tar xvf $(SER2NET).tar.gz
	
upload:
	cd $(SER2NET); \
	scp ser2net root@192.168.1.1:~/

clean:
	cd $(SER2NET); \
	make clean

realclean:
	rm -rf $(SER2NET)
	rm $(SER2NET).tar.gz 

.PHONY:
	all download upload clean realclean
