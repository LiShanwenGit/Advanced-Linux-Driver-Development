KERN_DIR = /home/lsw/h616/linux-6.1.37

all:
	make -C $(KERN_DIR) M=$(shell pwd) modules 

clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko .*.cmd *.exe

MODULE=workqueue-demo
obj-m += $(MODULE).o
$(MODULE)-objs = workqueue_demo.o
