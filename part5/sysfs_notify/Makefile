KERN_DIR = /home/lsw/h616/linux-6.1.37

all:APP
	make -C $(KERN_DIR) M=$(shell pwd) modules 

clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko .*.cmd *.exe

MODULE=sysfs_notify_demo
obj-m += $(MODULE).o
$(MODULE)-objs = sysfs-notify-demo.o

APP:key_app.c
	aarch64-linux-gcc $< -o key_app.exe