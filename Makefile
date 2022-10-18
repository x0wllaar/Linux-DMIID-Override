obj-m += dmi_override.o 
ccflags-y := -std=gnu2x -Wno-declaration-after-statement

PWD := $(CURDIR)
all: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 
clean: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
