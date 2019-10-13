
obj-m := shmq.o

KERNELDIR=/var/userspace/konstantin/linux_vanilla/linux-2.6.34.13/

PWD:= $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm *o modules.order Module.symvers *mod.c
