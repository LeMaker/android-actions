LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += $(bdroid_CFLAGS)

LOCAL_SRC_FILES := \
	src/bt_hci_bdroid.c \
	src/btsnoop.c \
	src/btsnoop_net.c \
	src/lpm.c \
	src/utils.c \
	src/vendor.c

LOCAL_CFLAGS := -Wno-unused-parameter

$(info BLUETOOTH_HCI_USE_MCT:$(BLUETOOTH_HCI_USE_MCT))
$(info BLUETOOTH_HCI_USE_RTK_HN:$(BLUETOOTH_HCI_USE_RTK_HN))

#three choice:
#1,MCT,rtk don't use
ifeq ($(BLUETOOTH_HCI_USE_MCT),true)
$(info rtk don't use MCT)
LOCAL_CFLAGS += -DHCI_USE_MCT
LOCAL_SRC_FILES += \
	src/hci_mct.c \
	src/userial_mct.c

#2,RTK_H5:rtl8723vq0
else
LOCAL_SRC_FILES += \
	   src/hci_h4.c \
       src/hci_h5.c \
       src/userial.c \
       src/bt_skbuff.c \
       src/bt_list.c

LOCAL_CFLAGS += -std=c99

ifeq ($(BOARD_HAVE_BLUETOOTH_RTK_COEX),true)
LOCAL_CFLAGS += -DBLUETOOTH_RTK_COEX
LOCAL_SRC_FILES += \
        src/rtk_parse.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/../stack/include \
        $(LOCAL_PATH)/../gki/ulinux
endif
endif

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../osi/include \
	$(LOCAL_PATH)/../utils/include \
        $(bdroid_C_INCLUDES)

LOCAL_MODULE := libbt-hci
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES

include $(BUILD_STATIC_LIBRARY)
