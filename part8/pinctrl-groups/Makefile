KERN_DIR = /home/lsw/h616/linux-6.1.37
all:
	make -C $(KERN_DIR) M=$(shell pwd) modules

clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko .*.cmd

MODULE_CTRL=dummy_pinctrl
obj-m += $(MODULE_CTRL).o
$(MODULE_CTRL)-objs=dummy-pinctrl.o

MODULE_CLIENT=dummy_pinctrl_dev
obj-m += $(MODULE_CLIENT).o
$(MODULE_CLIENT)-objs=dummy-pinctrl-dev.o
