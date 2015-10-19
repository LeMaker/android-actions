#定义当前的程序名
PROG := libOMX.Action.Video.Decoder.so
include $(TOPDIR)/Config
COMMONLIBDIR := $(TOPDIR)/lib/

VIDEC_INC := $(CURDIR)/../inc
ACTION_COMMON_INC := $(TOPDIR)/system/src/openmax_il/common/inc
OMXCORE_INC := $(TOPDIR)/system/src/openmax_il/omx_core/inc
ALSP_INC := $(TOPDIR)/../../inc
SYS_INC := $(TOPDIR)/../../lib/5202/include
LIB_DIR = $(TOPDIR)/../../lib/5202/


CFLAGS += -g -O3 -I$(VIDEC_INC) -I$(ALSP_INC) -I$(OMXCORE_INC) -I$(SYS_INC) -I$(ACTION_COMMON_INC)


SOURCES_C := $(wildcard *.c)
OBJS_C := $(patsubst %.c, %.o,$(SOURCES_C))

all: clean  $(PROG)
	cp $(PROG) $(COMMONLIBDIR)
	cp $(PROG) $(LIB_DIR)


$(PROG): $(OBJS_C)
	$(CC) -shared -o $@ -L$(COMMONLIBDIR) -ldl -lpthread $(OBJS_C)
	
$(OBJS_C) : %.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	$(RM) *.o $(PROG) $(PROG).exe $(PROG).bin

.PHONY: all clean mkdir 
.PHONY: $(SUBDIRS)



