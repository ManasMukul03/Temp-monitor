# Build the tempsensor kernel module against the running kernel.
#   make          -> builds tempsensor.ko
#   make load     -> inserts the module
#   make unload   -> removes the module
#   make clean
obj-m += tempsensor.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

load: all
	sudo insmod tempsensor.ko
	@sudo dmesg | tail -3

unload:
	sudo rmmod tempsensor
	@sudo dmesg | tail -2

.PHONY: all clean load unload
