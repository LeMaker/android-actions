ALSPDIR := $(shell ../../../config.make)
include $(ALSPDIR)/ALSP.mk
include $(ALSPDIR)/Config
TOPDIR  := ../..

CUR_DIR := .

RANLIB = $(COMPILERPREFIX)ranlib

AUDIO_SRC := $(CUR_DIR)/src
AUDIO_INC := $(CUR_DIR)/include
COMMON_INC := ../../omx-include
BASE_INC := ../../base
COM_INC := ../..

CFLAGS =    -Wno-unknown-pragmas -Wall -msoft-float 
CFLAGS += -fPIC -ldl -L$(ALSPLIBMMMDIR)
CFLAGS += -g -O2 -I$(AUDIO_INC) -I$(COMMON_INC) -I$(ALSPINC) -I$(BASE_INC) -I$(COM_INC)



SRC_C = $(AUDIO_SRC)/library_entry_point.c $(AUDIO_SRC)/omx_maddec_component.c $(AUDIO_SRC)/omx_maddec_audio_port.c  $(AUDIO_SRC)/downmix.c 

LDFLAGS = 
#libomxBellagio_base.a
all: clean
	$(CC) $(CFLAGS) $(ALSPRECYCLEDIR)/libomxBellagio_base.a $(SRC_C) -shared -o libOMX.Action.Audio.Decoder.so
	mv libOMX.Action.Audio.Decoder.so $(ALSPLIBMMMDIR)
#	mv libOMX.Action.Video.Encoder.so $(ALSPLIBDIR)
	@echo ----------------------------------------
	@echo ¹§Ï²Äú,±àÒë³É¹¦£¡£¨Ð¡¶Ë£©
	
clean:
	$(RM) $(ALSPLIBMMMDIR)/libOMX.Action.Audio.Decoder.so

.PHONY: clean