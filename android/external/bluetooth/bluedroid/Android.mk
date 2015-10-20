ifeq ($(strip $(R_BT_TYPE)), rtl8723au)
R_BT_VENDOR := realtek
else ifeq ($(strip $(R_BT_TYPE)), rtl8723as)
R_BT_VENDOR := realtek
else ifeq ($(strip $(R_BT_TYPE)), rtl8723bu)
R_BT_VENDOR := realtek
else ifeq ($(strip $(R_BT_TYPE)), rtl8723bs)
R_BT_VENDOR := realtek
else ifeq ($(strip $(R_BT_TYPE)), rtl8723bs_vq0)
R_BT_VENDOR := realtek
else ifeq ($(strip $(R_BT_TYPE)), ap6210)
R_BT_VENDOR := broadcom
else ifeq ($(strip $(R_BT_TYPE)), ap6212)
R_BT_VENDOR := broadcom
else ifeq ($(strip $(R_BT_TYPE)), ap6335)
R_BT_VENDOR := broadcom
else ifeq ($(strip $(R_BT_TYPE)), ap6330)
R_BT_VENDOR := broadcom
else ifeq ($(strip $(R_BT_TYPE)), ap6476)
R_BT_VENDOR := broadcom
endif


ifeq ($(strip $(R_BT_VENDOR)), broadcom)
LOCAL_PATH := $(call my-dir)

bdroid_CFLAGS := -Wno-unused-parameter

# Setup bdroid local make variables for handling configuration
ifneq ($(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR),)
  bdroid_C_INCLUDES := $(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR)
  bdroid_CFLAGS += -DHAS_BDROID_BUILDCFG
else
  bdroid_C_INCLUDES :=
  bdroid_CFLAGS += -DHAS_NO_BDROID_BUILDCFG
endif

bdroid_CFLAGS += -Wall -Werror

ifneq ($(BOARD_BLUETOOTH_BDROID_HCILP_INCLUDED),)
  bdroid_CFLAGS += -DHCILP_INCLUDED=$(BOARD_BLUETOOTH_BDROID_HCILP_INCLUDED)
endif

include $(call all-subdir-makefiles)

# Cleanup our locals
bdroid_C_INCLUDES :=
bdroid_CFLAGS :=
endif
