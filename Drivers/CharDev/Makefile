obj-m += chardev.o

CROSS = arm-linux-gnueabi-

KDIR = /home/ilya/Documents/RES/kernel/kernel

PWD := $(shell pwd)

all:
	make ARCH=arm -C $(KDIR) M=$(PWD) CROSS_COMPILE=$(CROSS) modules
	scp chardev.ko root@192.168.7.2:/home/root/Drivers
clean:
	make -C $(KDIR) M=$(PWD) CROSS_COMPILE=$(CROSS) clean
