KERN_DIR = /home/lsw/h616/linux-6.1.37
KBUILD_EXTRA_SYMBOLS += $(@D)/../bus-test/Module.symvers

all:
	make -C $(KERN_DIR) M=$(shell pwd) modules 

clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko  .*.cmd

MODULE=device_test
obj-m += $(MODULE).o
$(MODULE)-objs = device-test.o