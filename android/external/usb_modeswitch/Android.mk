LOCAL_PATH:= $(call my-dir)
LINUX_KERNEL_INCLUDE = external/libusb-compat/libusb/
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
	usb_modeswitch.c

LOCAL_CFLAGS := -O2 -g
LOCAL_CFLAGS += -DHAVE_CONFIG_H -D_U_="__attribute__((unused))"

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(LINUX_KERNEL_INCLUDE)

LOCAL_SHARED_LIBRARIES += libssl libcrypto libusb libusb-compat

LOCAL_STATIC_LIBRARIES += libpcap

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_MODULE_TAGS := eng optional

LOCAL_MODULE := usb_modeswitch

#LOCAL_POST_PROCESS_COMMAND := $(shell cp -rf $(LOCAL_PATH)/usb_modeswitch.d  $(TARGET_OUT)/etc/)

include $(BUILD_EXECUTABLE)


# define target for usb_modeswitch.d

define _add-usbmodeswitch-file
include $$(CLEAR_VARS)
LOCAL_MODULE := usb_modeswitch_$(notdir $(1))
LOCAL_MODULE_STEM := $(notdir $(1))
_files_modules += $$(LOCAL_MODULE)
LOCAL_SRC_FILES := $1
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $$(TARGET_OUT_ETC)/usb_modeswitch.d
include $$(BUILD_PREBUILT)
endef

_files_modules :=
_files :=
$(foreach _file, $(call find-subdir-files, usb_modeswitch.d/*), \
  $(eval $(call _add-usbmodeswitch-file,$(_file))))

include $(CLEAR_VARS)
LOCAL_MODULE := usb_modeswitch.d
LOCAL_MODULE_TAGS := eng optional
LOCAL_REQUIRED_MODULES := $(_files_modules)
include $(BUILD_PHONY_PACKAGE)

_add-usbmodeswitch-file :=
_files_modules :=
