obj-m += test-irq-latency.o

CROSS = arm-linux-gnueabi-

KDIR = /home/jwn/DES/kernel/kernel

PWD := $(shell pwd)

all:
	make ARCH=arm -C $(KDIR) M=$(PWD) CROSS_COMPILE=$(CROSS) modules
	scp test-irq-latency.ko root@192.168.7.2:/home/root
clean:
	make -C $(KDIR) M=$(PWD) CROSS_COMPILE=$(CROSS) clean
