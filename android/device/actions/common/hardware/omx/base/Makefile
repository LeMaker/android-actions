ALSPDIR := $(shell ../../config.make)
include $(ALSPDIR)/ALSP.mk
include $(ALSPDIR)/Config
TOPDIR  := ..

RANLIB = $(COMPILERPREFIX)ranlib

CAM_BASE:= $(TOPDIR)/base
CAM_COMMON := $(TOPDIR)
CAM_INC := $(TOPDIR)/omx-include
COMMON_INC := ../include

CFLAGS =    -Wno-unknown-pragmas -Wall -msoft-float 
CFLAGS += -fPIC -g -O2 -I$(CAM_BASE) -I$(CAM_COMMON) -I$(CAM_INC) -I$(ALSPINC)

DDIR := $(TOPDIR)/release

SRC_C = $(wildcard *.c) 
SRC_C += $(wildcard ../*.c) 

 
LDFLAGS = 

OBJS := $(patsubst %.c,$(DDIR)/%.o,$(SRC_C))

  
libomxBellagio_base.a:clean $(OBJS)  
	$(LD) -r -o $(DDIR)/$@ $(DDIR)/*.o $(CAM_COMMON)/*.o
	mv $(DDIR)/libomxBellagio_base.a $(ALSPRECYCLEDIR)
#	mv libOMX.Action.Video.Encoder.so $(ALSPLIBDIR)
	@echo ----------------------------------------
	@echo ¹§Ï²Äú,±àÒë³É¹¦£¡£¨Ð¡¶Ë£©
	
$(OBJS): $(DDIR)/%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(SUBDIRS):
	$(MAKE) -C $@
			
clean:
	$(RM) $(DDIR)/*.o $(CAM_COMMON)/*.o $(ALSPRECYCLEDIR)/libomxBellagio_base.a

.PHONY: clean
.PHONY: $(SUBDIRS)