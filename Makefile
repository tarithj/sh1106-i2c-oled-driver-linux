obj-m += sh1106.o

sh1106-objs := src/sh1106.o src/fb.o src/sysfs.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install_mod: all
	sudo insmod sh1106.ko

remove_mod:
	sudo rmmod sh1106

test: all
	-echo 0x3c | sudo tee /sys/bus/i2c/devices/i2c-3/delete_device
	-sudo rmmod sh1106
	-sudo insmod sh1106.ko
	-echo SH1106 0x3c | sudo tee /sys/bus/i2c/devices/i2c-3/new_device
