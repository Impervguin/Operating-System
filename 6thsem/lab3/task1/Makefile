obj-m += procinfo.o

CC = clang

PWD := $(CURDIR)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) CC=clang LD=ld.lld modules

clear :
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clear