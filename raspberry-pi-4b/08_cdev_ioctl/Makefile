obj-m += cdev_ioctl_inclusive.o

all: module testapp

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm a.out

testapp: test_cdev_ioctl.c
	gcc test_cdev_ioctl.c