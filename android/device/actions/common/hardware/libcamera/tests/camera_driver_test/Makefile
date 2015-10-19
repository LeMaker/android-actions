
PWD       := $(shell pwd)
#KERNEL_DIR ?= $(PWD)/../../../build/gs702a/out/kernel
#INCLUDE_DIR += $(PWD)/../../include
LIBDIR += /usr/lib




OUT_NAME = test_case_1
CONFIG_NAME = config_1
OTHER = display.o buf_op.o device_op.o interface.o video_dev.o ion.o





 #CC = arm-none-linux-gnueabi-gcc
CC =  arm-none-linux-gnueabi-gcc   -static  -lpthread   

$(OUT_NAME) : $(OUT_NAME).o  $(OTHER) $(CONFIG_NAME).o
	$(CC) -o $(OUT_NAME) $(OUT_NAME).o  $(CONFIG_NAME).o $(OTHER)  libpthread.a


.PHONY: clean
clean : 
	-rm *.o $(OUT_NAME) *.bak