KERN_DIR = /home/lsw/linux
all:
	make -C $(KERN_DIR) M=$(shell pwd) modules 

clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko .*.cmd

obj-m += drm-st7796.o
