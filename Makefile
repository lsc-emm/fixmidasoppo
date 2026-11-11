obj-m += midas_fix.o

# KDIR 指向 GKI 内核源码输出目录
KDIR ?= /path/to/gki/kernel/out/android12-5.10/common
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
