CURRENT = $(shell uname -r)
KDIR = /lib/modules/$(CURRENT)/build
PWD = $(CURDIR)
TARGET1 = md1
TARGET2 = md2
TARGET3 = md3
obj-m := $(TARGET1).o $(TARGET2).o $(TARGET3).o 
CC = clang
default: 
	$(MAKE) -C $(KDIR) M=$(PWD) CC=clang LD=ld.lld modules 
clean: 
	@rm -f *.o .*.cmd .*.flags *.mod.c *.order 
	@rm -f .*.*.cmd *~ *.*~ TODO.* 
	@rm -fR .tmp* 
	@rm -rf .tmp_versions 
disclean: clean 
	@rm *.ko *.symvers