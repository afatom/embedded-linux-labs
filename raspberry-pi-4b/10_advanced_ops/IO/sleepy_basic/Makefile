obj-m += sleepy_basic.o
 

# The KDIR depends on your specific RPi kernel version
KDIR := $(shell readlink -f /lib/modules/$(shell uname -r)/build)
# KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	# Run bear to intercept the compilation and update the JSON
	bear --append -- make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f compile_commands.json