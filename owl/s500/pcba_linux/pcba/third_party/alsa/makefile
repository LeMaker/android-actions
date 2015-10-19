PREFIX=/opt/gcc-linaro-aarch64-linux-gnu-4.8-2013.12_linux/bin/aarch64-linux-gnu-
#PREFIX=/opt/arm-2011.09/bin/arm-none-linux-gnueabi-
CC=$(PREFIX)gcc
LD=$(PREFIX)ld

HEADER=../lib/include/directfb

SOURCES := $(wildcard *.c)
OBJ := $(patsubst %.c,%.o,$(SOURCES))
PRG := libtinyalsa.so
PLAY := tinyplay
MIX := tinymix
INFO := tinypcminfo
HDMI := tinyplay_hdmi
CAP := tinycap

all: $(PRG) $(PLAY) $(MIX) $(INFO) $(HDMI) $(CAP)

$(OBJ):%.o:%.c
	$(CC) -c -I./include $< -o $@

$(PRG): pcm.o mixer.o
	$(LD) pcm.o mixer.o -fPIC -shared -o $@

$(PLAY): tinyplay.o $(PRG)
	$(CC) $<  -L./ -ltinyalsa -o $@
	
$(MIX): tinymix.o $(PRG)
	$(CC) $<  -L./ -ltinyalsa -o $@

$(INFO): tinypcminfo.o $(PRG)
	$(CC) $<  -L./ -ltinyalsa -o $@
$(HDMI): tinyplay_hdmi.o $(PRG)
	$(CC) $<  -L./ -ltinyalsa -o $@
$(CAP): tinycap.o $(PRG)
	$(CC) $<  -L./ -ltinyalsa -o $@
clean: 
	rm *.o $(PRG) -f

