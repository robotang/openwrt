SPCASERV = spcaserv

all:
	cd $(SPCASERV); \
	make

download:
	wget http://robotang.co.nz/assets/Uploads/openwrt/$(SPCASERV).tar.gz
	tar xvf $(SPCASERV).tar.gz
	patch -p0 < spcaserv-patch
	
upload:
	cd $(SPCASERV); \
	scp spcaserv root@192.168.1.1:~/dev

clean:
	cd $(SPCASERV); \
	make clean

realclean:
	rm -rf $(SPCASERV)
	rm $(SPCASERV).tar.gz 

.PHONY:
	all download upload clean realclean
