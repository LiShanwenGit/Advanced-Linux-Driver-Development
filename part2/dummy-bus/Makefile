KERN_DIR = /home/lsw/h616/linux-6.1.37
all:
	make -C $(KERN_DIR) M=$(shell pwd) modules 

clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko  .*.cmd

MODULE_BUS=dummy-bus
MODULE_CTRL_DRV=dummy-ctrl
MODULE_LED_DRV=dummy-dev

obj-m += $(MODULE_BUS).o
$(MODULE_BUS)-objs=dummy-bus-init.o

obj-m += $(MODULE_CTRL_DRV).o
$(MODULE_CTRL_DRV)-objs=dummy-ctrl-driver.o

obj-m += $(MODULE_LED_DRV).o
$(MODULE_LED_DRV)-objs=dummy-dev-driver.o
