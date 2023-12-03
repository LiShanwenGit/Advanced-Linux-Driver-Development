KERN_DIR = /home/lsw/h616/linux-6.1.37
all: TEST_APP
	make -C $(KERN_DIR) M=$(shell pwd) modules 

clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko .*.cmd test_app.exe

obj-m += dummy-mmap.o

# test application
TEST_APP: test_app.c
	aarch64-linux-gcc  test_app.c -o test_app.exe
