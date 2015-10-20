
#      
# usb dongle use libusb-compat
#ActionsCode(author:ywwang, type:new_method)
#
  

#ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	libusb/core.c

LOCAL_SYSTEM_SHARED_LIBRARIES := libc

LOCAL_SHARED_LIBRARIES := \
	libcrypto libz libcutils libusb

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/libusb  \
	external/libusb \
	external/libusb/libusb

#/opt/arm-2011.09/arm-none-linux-gnueabi/libc/usr/include

LOCAL_CFLAGS := -DANDROID_CHANGES -DCHAPMS=1 -DMPPE=1 -Iexternal/openssl/include -DHAVE_CONFIG_H

LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE:= libusb-compat

include $(BUILD_SHARED_LIBRARY)

#endif
