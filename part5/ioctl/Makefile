KERN_DIR = /home/lsw/h616/linux-6.1.37

all:APP
	make -C $(KERN_DIR) M=$(shell pwd) modules 

clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko .*.cmd *.exe

MODULE=ioctl-io
obj-m += $(MODULE).o
$(MODULE)-objs = ioctl.o

APP:test_app.c
	aarch64-linux-gcc $< -o test_app.exe