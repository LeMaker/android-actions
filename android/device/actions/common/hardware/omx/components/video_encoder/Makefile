ALSPDIR := $(shell ../../../config.make)
include $(ALSPDIR)/ALSP.mk
include $(ALSPDIR)/Config
TOPDIR  := ../..

CUR_DIR := .

RANLIB = $(COMPILERPREFIX)ranlib

VCE_SRC := $(CUR_DIR)/src
VCE_INC := $(CUR_DIR)/include
COMMON_INC := ../../omx-include
BASE_INC := ../../base
COM_INC := ../..

CFLAGS =    -Wno-unknown-pragmas -Wall -msoft-float 
CFLAGS += -fPIC -ldl -L$(ALSPLIBMMMDIR)
CFLAGS += -g -O2 -I$(VCE_INC) -I$(COMMON_INC) -I$(BASE_INC) -I$(COM_INC) -I$(ALSPINC)


SRC_C = $(VCE_SRC)/face_engine.c $(VCE_SRC)/frame_mng.c $(VCE_SRC)/omx_malloc.c $(VCE_SRC)/omx_videoenc_component.c  $(VCE_SRC)/omx_videoenc_port.c $(VCE_SRC)/library_entry_point.c

LDFLAGS = 
#libomxBellagio_base.a
all: clean
	$(CC) $(CFLAGS) $(ALSPRECYCLEDIR)/libomxBellagio_base.a $(SRC_C) -shared -o libOMX.Action.Video.Encoder.so
	mv libOMX.Action.Video.Encoder.so $(ALSPLIBDIR)
	@echo ----------------------------------------
	@echo ¹§Ï²Äú,±àÒë³É¹¦£¡£¨Ð¡¶Ë£©
	
clean:
	$(RM) $(ALSPLIBMMMDIR)/libOMX.Action.Video.Encoder.so

.PHONY: clean