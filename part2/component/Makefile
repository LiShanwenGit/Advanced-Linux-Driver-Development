KERN_DIR = /home/lsw/h616/linux-6.1.37
all:
	make -C $(KERN_DIR) M=$(shell pwd) modules 

clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko  .*.cmd

MODULE1=master_comp
obj-m += $(MODULE1).o
$(MODULE1)-objs = master-comp.o

MODULE2=sub_comp1
obj-m += $(MODULE2).o
$(MODULE2)-objs = sub-comp1.o

MODULE3=sub_comp2
obj-m += $(MODULE3).o
$(MODULE3)-objs = sub-comp2.o
