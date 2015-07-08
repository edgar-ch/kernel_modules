obj-m := gpio_irq.o

KDIR := /home/user/ti-processor-sdk-linux-am335x-evm-01.00.00.00/board-support/linux-3.14.26-g2489c02
PWD := $(shell pwd)
ARCH_TYPE := arm
TOOLCHAIN := arm-linux-gnueabihf-

default:
	$(MAKE) ARCH=$(ARCH_TYPE) CROSS_COMPILE=$(TOOLCHAIN) -C $(KDIR) M=$(PWD) modules