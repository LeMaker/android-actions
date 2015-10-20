#ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	src/pppoe.c \
	src/if.c \
	src/debug.c \
	src/common.c \
	src/ppp.c \
	src/discovery.c

#LOCAL_SHARED_LIBRARIES := \
#	libcutils libcrypto
#

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/src

LOCAL_CFLAGS := -DANDROID_CHANGES -DCHAPMS=1 -DMPPE=1 -DPPPD_PATH="/system/bin/pppd" \
				-DVERSION="3.10"

LOCAL_MODULE_TAGS:= eng

LOCAL_MODULE:= pppoe

include $(BUILD_EXECUTABLE)

#endif
