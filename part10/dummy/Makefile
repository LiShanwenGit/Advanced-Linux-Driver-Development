KERN_DIR = /home/lsw/h616/linux-6.1.37
all:
	make -C $(KERN_DIR) M=$(shell pwd) modules 

clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko .*.cmd

MODULE_DEV=dummy_i2cdev
obj-m += $(MODULE_DEV).o
$(MODULE_DEV)-objs=dummy-i2cdev.o

MODULE_MASTER=dummy_i2cadapter
obj-m += $(MODULE_MASTER).o
$(MODULE_MASTER)-objs=dummy-i2cadapter.o
