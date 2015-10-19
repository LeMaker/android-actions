ALSPDIR := $(shell ../../../config.make)
include $(ALSPDIR)/ALSP.mk
include $(ALSPDIR)/Config
TOPDIR  := ../..

CUR_DIR := .

RANLIB = $(COMPILERPREFIX)ranlib

CAM_SRC := $(CUR_DIR)/src
V4L2HAL := $(CUR_DIR)/v4l2camera
CAM_INC := $(CUR_DIR)/include
COMMON_INC := ../../omx-include
BASE_INC := ../../base
COM_INC := ../..

CFLAGS =    -Wno-unknown-pragmas -Wall -msoft-float 
CFLAGS += -fPIC -ldl -L$(ALSPLIBMMMDIR)
CFLAGS += -g -O2 -I$(CAM_INC) -I$(COMMON_INC) -I$(ALSPINC) -I$(BASE_INC) -I$(COM_INC)
#CFLAGS += -mtune=24kec -mdsp 


SRC_C = $(CAM_SRC)/library_entry_point.c $(CAM_SRC)/omx_base_camera_video_port.c $(CAM_SRC)/omx_camera_component.c 
SRC_HAL_C = $(V4L2HAL)/base_v4l2_module.c $(V4L2HAL)/isp_camera.c $(V4L2HAL)/v4l2_camera.c

LDFLAGS = 
#libomxBellagio_base.a
all: clean
	$(CC) $(CFLAGS) $(ALSPRECYCLEDIR)/libomxBellagio_base.a $(SRC_C) -shared -o libOMX.Action.Video.Camera.so
	$(CC) $(CFLAGS) $(SRC_HAL_C) -shared -o libACT_V4L2HAL.so
	mv libOMX.Action.Video.Camera.so $(ALSPLIBMMMDIR)
	mv libACT_V4L2HAL.so $(ALSPLIBMMMDIR)
#	mv libOMX.Action.Video.Encoder.so $(ALSPLIBDIR)
	@echo ----------------------------------------
	@echo ¹§Ï²Äú,±àÒë³É¹¦£¡£¨Ð¡¶Ë£©
	
clean:
	$(RM) $(ALSPLIBMMMDIR)/libOMX.Action.Video.Camera.so $(ALSPLIBMMMDIR)/libACT_V4L2HAL.so

.PHONY: clean