LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# BTIF
LOCAL_SRC_FILES += \
    btif/src/btif_core.c \
    btif/src/bluetooth.c \
    btif/src/bt_mp_base.c \
    btif/src/bt_mp_build.c \
    btif/src/bt_mp_device_base.c \
    btif/src/bt_mp_device_general.c \
    btif/src/bt_mp_device_rtl8723a.c \
    btif/src/bt_mp_device_skip.c \
    btif/src/bt_mp_module_base.c \
    btif/src/bt_user_func.c \
    btif/src/foundation.c \
    btif/src/bt_mp_api.c \
    btif/src/bt_mp_transport.c

# GKI
LOCAL_SRC_FILES += \
    gki/ulinux/gki_ulinux.c \
    gki/common/gki_debug.c \
    gki/common/gki_time.c \
    gki/common/gki_buffer.c

ifeq ($(BLUETOOTH_HCI_USE_RTK_H5),true)
LOCAL_CFLAGS := -DHCI_USE_RTK_H5
# HCI UART H5
LOCAL_SRC_FILES += \
    hci/src/hci_h5.c \
    hci/src/userial.c \
    hci/src/bt_skbuff.c \
    hci/src/bt_list.c   \
    hci/src/bt_hci_bdroid.c \
    hci/src/bt_hw.c \
    hci/src/btsnoop.c \
    hci/src/utils.c
else
# HCI USB
LOCAL_SRC_FILES += \
    hci/src/hci_h4.c \
    hci/src/userial.c \
    hci/src/bt_hci_bdroid_usb.c \
    hci/src/bt_hw.c \
    hci/src/btsnoop.c \
    hci/src/utils.c
endif

# MAIN
LOCAL_SRC_FILES += \
    main/bte_main.c \
    main/bte_logmsg.c \
    main/bte_conf.c

# STACK
LOCAL_SRC_FILES += \
    stack/btu/btu_hcif.c \
    stack/btu/btu_task.c \
    stack/hcic/hcicmds.c \
    stack/hcic/hciblecmds.c

ifeq ($(BLUETOOTH_HCI_USE_RTK_H5),true)
# UART LIBBT
LOCAL_SRC_FILES += \
    libbt/src/bt_vendor_rtk.c \
    libbt/src/hardware.c \
    libbt/src/userial_vendor.c \
    libbt/src/upio.c
else
# LIBBT
LOCAL_SRC_FILES += \
    libbt/src/bt_vendor_rtk_usb.c
endif

# UTILS
LOCAL_SRC_FILES += \
   utils/src/bt_utils.c


LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/include \
    $(BDROID_DIR)/hci/include

# BTIF
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/btif/include

# GKI
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/gki/common \
    $(LOCAL_PATH)/gki/ulinux

# HCI
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/hci/include

# STACK
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/stack/include

# LIBBT
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/libbt/include

# UTILS
LOCAL_C_INCLUDES += \
   $(LOCAL_PATH)/utils/include

LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    libnativehelper \
    libcutils \
    libutils \
    libhardware \
    libc \
    libpower


LOCAL_MODULE := bluetoothmp.default
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

include $(BUILD_SHARED_LIBRARY)
