KERN_DIR = /home/lsw/h616/linux-6.1.37
all:
	make -C $(KERN_DIR) M=$(shell pwd) modules 

clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko .*.cmd

obj-m += drm-hotplug-poll.o
