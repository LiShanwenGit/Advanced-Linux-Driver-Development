KERN_DIR = /home/lsw/h616/linux-6.1.37

all:APP
	make -C $(KERN_DIR) M=$(shell pwd) modules 

clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko .*.cmd *.exe

MODULE=poll_io_demo
obj-m += $(MODULE).o
$(MODULE)-objs = poll-io-demo.o

APP:epoll-io-app.c
	aarch64-linux-gcc $< -o epoll-io-app.exe