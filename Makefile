obj-m += led7gpio.o

KDIR := /home/lephuongnam/rpi3/outsource/linux/
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
