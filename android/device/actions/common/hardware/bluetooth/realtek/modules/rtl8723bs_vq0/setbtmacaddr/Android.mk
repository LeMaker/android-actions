ifeq ($(R_BT_TYPE), rtl8723bs_vq0)
LOCAL_PATH:= $(call my-dir)
# RTK bt mac
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := libcutils
LOCAL_LDLIBS        += -Idl

LOCAL_SRC_FILES     := setbtmacaddr.c

LOCAL_MODULE := setbtmacaddr


include $(CLEAR_VARS)
LOCAL_SRC_FILES := setbtmacaddr.c 
LOCAL_MODULE := setbtmacaddr
LOCAL_MODULE_TAGS := optional setbtmacaddr
LOCAL_SHARED_LIBRARIES := liblog libcutils
include $(BUILD_EXECUTABLE)

endif
