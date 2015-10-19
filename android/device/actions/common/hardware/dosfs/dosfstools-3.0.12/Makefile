#!/bin/sh
#Biuld dosfsck
CC    = arm-none-linux-gnueabi-gcc
CFLAG = -O2 -mfloat-abi=softfp -mfpu=neon -static
DUMP  = arm-none-linux-gnueabi-objdump

LD=arm-none-linux-gnueabi-ld -nostdlib --dynamic-linker /system/bin/linker -rpath /system/lib -rpath /path/to/android_lib -L /path/to/android_lib -lc 

OBJ_FILE  = boot.o check.o common.o fat.o file.o io.o lfn.o dosfsck.o

dosfsck: $(OBJ_FILE)
	$(LD) -o dosfsck $(OBJ_FILE)
	$(DUMP) -D dosfsck > udelay.list

	
boot.o : boot.c
	$(CC) $(CFLAG) -c -o boot.o boot.c
	
check.o : check.c
	$(CC) $(CFLAG) -c -o check.o check.c

common.o : common.c
	$(CC) $(CFLAG) -c -o common.o common.c
	
fat.o : fat.c
	$(CC) $(CFLAG) -c -o fat.o fat.c
	
file.o : file.c
	$(CC) $(CFLAG) -c -o file.o file.c			

io.o : io.c
	$(CC) $(CFLAG) -c -o io.o io.c	
	
lfn.o : lfn.c
	$(CC) $(CFLAG) -c -o lfn.o lfn.c		

dosfsck.o : dosfsck.c
	$(CC) $(CFLAG) -c -o dosfsck.o dosfsck.c		
			
make clean:
	rm *.o dosfsck dosfsck.list

