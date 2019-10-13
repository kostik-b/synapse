
obj-m := packet_capture.o

KERNELDIR=/var/userspace/konstantin/linux_vanilla/linux-2.6.34.13/

EXTRA_CFLAGS += -I$(PWD)/../

PWD:= $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
