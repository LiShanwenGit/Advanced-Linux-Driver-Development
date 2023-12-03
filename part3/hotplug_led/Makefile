KERN_DIR = /home/lsw/h616/linux-6.1.37
all:
	make -C $(KERN_DIR) M=$(shell pwd) modules 

clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko  .*.cmd

MODULE_BUS=hotplug-led-bus
MODULE_CTRL_DRV=hotplug-led-ctl
MODULE_LED_DRV=hotplug-led

obj-m += $(MODULE_BUS).o
$(MODULE_BUS)-objs=hotplug-led-bus-init.o

obj-m += $(MODULE_CTRL_DRV).o
$(MODULE_CTRL_DRV)-objs=hotplug-led-ctl-driver.o

obj-m += $(MODULE_LED_DRV).o
$(MODULE_LED_DRV)-objs=hotplug-led-device.o
