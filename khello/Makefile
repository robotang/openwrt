obj-m := hello.o

default:
	$(MAKE) -C $(KERNELDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) SUBDIRS=$(PWD) modules

upload:
	scp hello.ko root@192.168.1.1:~/dev

clean:
	rm -rf *.o *.ko *.mod.* Module.* .hello* .tmp* modules.order

.PHONY:
	clean upload
