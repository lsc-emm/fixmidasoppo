obj-m += midas_fix.o

KDIR ?= /path/to/gki/kernel/out/android12-5.10/common
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
