LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#LOCAL_ARM_MODE := arm
LOCAL_CFLAGS := -O2 -DNDEBUG  -fstrict-aliasing

LOCAL_SRC_FILES := \
	isp_param/isp_param.c

LOCAL_SHARED_LIBRARIES := liblog

LOCAL_MODULE_TAGS := eng optional

#LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := libACT_ISP_PARM

include $(BUILD_SHARED_LIBRARY)

######################################################################

include $(CLEAR_VARS)

#LOCAL_ARM_MODE := arm
LOCAL_CFLAGS := -O2 -DNDEBUG  -fstrict-aliasing

LOCAL_SRC_FILES := \
	imx_param/isp_imx_param.c

LOCAL_SHARED_LIBRARIES := liblog

LOCAL_MODULE_TAGS := eng optional

#LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := libACT_IMX_PARM

include $(BUILD_SHARED_LIBRARY)

