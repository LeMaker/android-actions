#定义当前的程序名
PROG := auddec_test.exe

COMMONLIBDIR := $(TOPDIR)/lib

OMXCORE_INC := $(TOPDIR)/system/src/openmax_il/omx_core/inc
CODA_INC := $(TOPDIR)/video/src/coda/inc
INC := $(TOPDIR)/audio/src/openmax_il/audio_decoder/inc
ACTION_COMMON_INC := $(TOPDIR)/system/src/openmax_il/common/inc
ALSPLIBDIR := $(TOPDIR)/../../../lib/gsbase
#ALSPRECYCLEDIR := $(TOPDIR)/../../../lib/gsbase/recycle
#COMPILERPREFIX = /opt/tools-2.6.27/bin/mipsel-linux-gnu-
COMPILERPREFIX = mipsel-linux-gnu-

CC = $(COMPILERPREFIX)gcc
LD = $(COMPILERPREFIX)ld
AR = $(COMPILERPREFIX)ar
OBJCOPY = $(COMPILERPREFIX)objcopy
OBJDUMP = $(COMPILERPREFIX)objdump
RM = rm -r -f

CFLAGS = -Wno-unknown-pragmas -mips32r2 -Wall -g -msoft-float 
CFLAGS += -fPIC -pipe 
CFLAGS += -g -O0 -I$(OMXCORE_INC) -I$(CODA_INC) -I$(INC) -I./ 
CFLAGS += -mtune=24kec -mdsp 

#LFLAGS+= --eh-frame-hdr -m elf_i386 --hash-style=gnu -dynamic-linker /lib/ld-linux.so.2 
#LFLAGS+= /usr/lib/gcc/i386-redhat-linux/4.1.1/../../../crt1.o /usr/lib/gcc/i386-redhat-linux/4.1.1/../../../crti.o /usr/lib/gcc/i386-redhat-linux/4.1.1/crtbegin.o
#LFLAGS+= --no-as-needed /usr/lib/gcc/i386-redhat-linux/4.1.1/crtend.o /usr/lib/gcc/i386-redhat-linux/4.1.1/../../../crtn.o

#LFLAGS += -ldl -L$(COMMONLIBDIR) -lrt -lOMX_Core -ldma_mem -lOMX.Action.Audio.Decoder
#LFLAGS += -L$(ALSPLIBDIR) -lalc -L$(ALSPRECYCLEDIR) -lstorage
SOURCES_C := $(wildcard *.c)
OBJS_C := $(patsubst %.c, %.o,$(SOURCES_C))
#LIBS := libaudio.dec.test.so
	
#all: clean $(LIBS)
all: clean $(PROG)
#	mv $(LIBS) $(COMMONLIBDIR)
#	mv $(LIBS) $(ALSPLIBDIR)
#	mv $(OBJS_C) $(COMMONLIBDIR)
	cp $(PROG) $(COMMONLIBDIR)

#创建目标文件, 并拷贝到lib目录下
$(PROG): $(OBJS_C)
	$(CC) $(OBJS_C) $(LFLAGS) -ldl -L$(COMMONLIBDIR) -lrt -ldma_mem -lalc -lOMX_Core -lOMX.Action.Audio.Decoder -o $(PROG)

#$(LIBS): $(OBJS_C)
#	$(CC) $(CFLAGS) $(OBJS_C)  -shared -o libaudio.dec.test.so
#	$(OBJDUMP) -D $@ > $@.lst
#	cp libaudio.dec.test.so $(COMMONLIBDIR)
#	$(AR) $(CFLAGS) -cr $@ $<
	
$(OBJS_C) : %.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	$(RM) *.o $(PROG) $(PROG).exe $(PROG).bin $(LIBS)

.PHONY: all clean mkdir 
.PHONY: $(SUBDIRS)



