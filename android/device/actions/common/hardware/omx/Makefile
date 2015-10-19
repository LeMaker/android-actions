ALSPDIR := $(shell ../config.make)

include $(ALSPDIR)/ALSP.mk
include $(ALSPDIR)/Config
TOPDIR  := $(shell /bin/pwd)
export TOPDIR ALSPDIR

COMMONLIBDIR := $(TOPDIR)/lib

RANLIB = $(COMPILERPREFIX)ranlib

CFLAGS = -Wno-unknown-pragmas -Wall -g -msoft-float 
CFLAGS += -fPIC  
CFLAGS += -g -O3 -I$(OMXCORE_INC)

all:
#BISP_HAL := $(TOPDIR)/isphal
#V4L2_HAL := $(TOPDIR)/v4l2camera
#CAM_COMP := $(TOPDIR)/camera_component
#CAM_TEST := $(TOPDIR)/test_cam
#OMX_CORE := $(TOPDIR)/omx_core
OMX_BASE := $(TOPDIR)/base
CAM_COMP := $(TOPDIR)/components/camera
OMX_CORE := $(TOPDIR)/omx_core

#$(CAM_COMP) $(V4L2_HAL) $(CAM_TEST) $(BISP_HAL) $(V4L2_HAL) $(OMX_BASE) $(OMX_BASE) $(CAM_COMP) 
all:clean  $(OMX_BASE) $(OMX_CORE) $(CAM_COMP)
	@echo ----------------------------------------
	@echo ¹§Ï²Äú,±àÒë³É¹¦£¡£¨Ð¡¶Ë£©
base : $(OMX_BASE)
core : $(OMX_CORE)
camera : $(CAM_COMP)
	
	
$(BISP_HAL):
	make -C $@
	
$(V4L2_HAL):
	make -C $@

$(CAM_COMP):
	make -C $@
	
$(CAM_TEST):
	make -C $@
	
$(OMX_CORE):
	make -C $@
	
$(OMX_BASE):
	make -C $@
			
clean:
	$(RM) $(COMMONLIBDIR)/*.exe $(COMMONLIBDIR)/*.so $(COMMONLIBDIR)/*.a	
	
mkdir:
	if ! [ -d $(COMMONLIBDIR) ];\
	then echo "mkdir " $(COMMONLIBDIR) && mkdir $(COMMONLIBDIR);\
	fi
	
.PHONY: $(BISP_HAL)	
.PHONY: $(V4L2_HAL)
.PHONY: $(CAM_COMP)	
.PHONY: $(CAM_TEST)
.PHONY: $(OMX_CORE)
.PHONY: $(OMX_BASE)
.PHONY: base camera core