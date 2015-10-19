
PWD=$(shell pwd)
PREFIX=arm-none-linux-gnueabi-
DIRECTHEADER=./lib/lib32/directfb/include/directfb
DIRECTLIBS=./lib/lib32/directfb/lib
LIBDIR=./lib/lib32
CFLAGS= -L$(DIRECTLIBS) -L$(LIBDIR) -ldirectfb -ldirect -lencpp -lalc -ltinyalsa -Wl,-rpath $(DIRECTLIBS)
TARGETDIR=$(PWD)/../initramfs/bin/

CC=$(PREFIX)gcc
LD=$(PREFIX)ld

SOURCES := $(wildcard *.c)
OBJ := $(patsubst %.c,%.o,$(SOURCES))
PRG := pcba_linux

all: $(PRG)

$(OBJ):%.o:%.c
	$(CC) -c -D__GS705A__ -I$(DIRECTHEADER) -I./include $< -o $@

$(PRG): $(OBJ)
	$(CC) *.o -o $@ $(CFLAGS)
	cp pcba_linux $(TARGETDIR) 
test:
	gcc init_case.c -I$(DIRECTHEADER) -I./include -o init_case
	
clean: 
	rm *.o pcba_linux 

