#ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	libusb/sync.c \
	libusb/io.c \
	libusb/descriptor.c \
	libusb/core.c \
	libusb/os/linux_usbfs.c

LOCAL_SYSTEM_SHARED_LIBRARIES := libc

LOCAL_SHARED_LIBRARIES := \
	libcrypto libz libcutils

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/libusb \
	$(LOCAL_PATH)/libusb/os 

#/opt/arm-2011.09/arm-none-linux-gnueabi/libc/usr/include

LOCAL_CFLAGS := -DANDROID_CHANGES -DCHAPMS=1 -DMPPE=1 -Iexternal/openssl/include -DHAVE_CONFIG_H

LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE:= libusb

include $(BUILD_SHARED_LIBRARY)

#endif
