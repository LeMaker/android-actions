ALSPDIR := $(shell ../../config.make)
include $(ALSPDIR)/ALSP.mk
include $(ALSPDIR)/Config

CUR_DIR := .

RANLIB = $(COMPILERPREFIX)ranlib

COMMON_INC := ../omx-include
BASE_INC := ../base
COM_INC := ..

CFLAGS =    -Wno-unknown-pragmas -Wall -msoft-float 
CFLAGS += -fPIC -ldl -L$(ALSPLIBMMMDIR) -lexpat
CFLAGS += -g -O2 -I$(COMMON_INC) -I$(ALSPINC) -I$(BASE_INC) -I$(COM_INC)
#CFLAGS += -mtune=24kec -mdsp 


SRC_C = $(CUR_DIR)/omxcore.c

all: clean
	$(CC) $(CFLAGS) $(SRC_C) -shared -o libOMX_Core.so
	mv libOMX_Core.so $(ALSPLIBMMMDIR)/
	
clean:
	$(RM) $(ALSPLIBMMMDIR)/libOMX_Core.so

.PHONY: clean