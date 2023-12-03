KERN_DIR = /home/lsw/h616/linux-6.1.37
all:
	make -C $(KERN_DIR) M=$(shell pwd) modules 

clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko  .*.cmd

MODULE=memfs_drv

obj-m += $(MODULE).o
$(MODULE)-objs=memfs.o
