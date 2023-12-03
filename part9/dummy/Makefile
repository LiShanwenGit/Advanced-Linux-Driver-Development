KERN_DIR = /home/lsw/h616/linux-6.1.37
all:
	make -C $(KERN_DIR) M=$(shell pwd) modules 

clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko .*.cmd

MODULE_DEV=dummy_spidev
obj-m += $(MODULE_DEV).o
$(MODULE_DEV)-objs=dummy-spidev.o

MODULE_MASTER=dummy_spimaster
obj-m += $(MODULE_MASTER).o
$(MODULE_MASTER)-objs=dummy-spimaster.o
