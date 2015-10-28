#
# Copyright (C) 2011 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# These two variables are set first, so they can be overridden
# by BoardConfigVendor.mk
BOARD_USES_GENERIC_AUDIO := true
USE_CAMERA_STUB := true

# Use the non-open-source parts, if they're present
#-include vendor/ti/panda/BoardConfigVendor.mk
TARGET_ARCH := arm
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_SMP := true
TARGET_ARCH_VARIANT := armv7-a-neon
#TARGET_ARCH_VARIANT := armv7-a
ARCH_ARM_HAVE_TLS_REGISTER := true
ARCH_ARM_HAVE_32_BYTE_CACHE_LINES := true

TARGET_CPU_VARIANT := cortex-a9
# Realsil added
BOARD_HAVE_BLUETOOTH := true
#Realtek add start
BOARD_HAVE_BLUETOOTH_RTK := true
BOARD_HAVE_BLUETOOTH_RTK_COEX := false
BLUETOOTH_HCI_USE_RTK_H5 := true
#Realtek add end


TARGET_NO_BOOTLOADER := false
TARGET_NO_RECOVERY := false

TARGET_NO_KERNEL := false
BOARD_KERNEL_BASE := 0x00000000
#BOARD_KERNEL_CMDLINE := androidboot.console=ttyS0

BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR=device/generic/common/bluetooth

TARGET_NO_RADIOIMAGE := true
TARGET_BOARD_PLATFORM := S500
TARGET_BOOTLOADER_BOARD_NAME := lemaker_guitar_bbb

BOARD_EGL_CFG := device/actions/lemaker_guitar_bbb/egl.cfg
TARGET_BOARD_INFO_FILE := device/actions/lemaker_guitar_bbb/board-info.txt

BOARD_USE_LEGACY_UI := true
VSYNC_EVENT_PHASE_OFFSET_NS := 0

#BOARD_USES_HGL := true
#BOARD_USES_OVERLAY := true
BUILD_EMULATOR_OPENGL := true
USE_OPENGL_RENDERER := true

TARGET_USERIMAGES_USE_EXT4 := true
ifeq ($(R_GMS_TYPE),full)
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1610612736
else
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1073741824
endif
#BOARD_SYSTEMIMAGE_PARTITION_SIZE := 16777216
BOARD_USERDATAIMAGE_PARTITION_SIZE := 671088640
BOARD_FLASH_BLOCK_SIZE := 4096
#OVERRIDE_RS_DRIVER := libPVRRS.so

BOARD_CHARGER_ENABLE_SUSPEND := true
TARGET_PROVIDES_INIT_RC := true
#TARGET_USERIMAGES_SPARSE_EXT_DISABLED := true

TARGET_SYSTEM_IMG_VOLUME_LABEL := "-LSYSTEM" 

ifeq ($(strip $(TARGET_BUILD_VARIANT)), eng)
WITH_DEXPREOPT :=false
else
WITH_DEXPREOPT :=true
endif
#WITH_DEXPREOPT_BOOT_IMG_ONLY := true

-include device/actions/lemaker_guitar_bbb/wifi.mk
#
BOARD_SEPOLICY_DIRS += device/actions/lemaker_guitar_bbb/sepolicy
BOARD_SEPOLICY_UNION := \
        genfs_contexts \
        zygote.te \
        healthd.te \
        installd.te \
        file.te \
        init.te \
        vold.te \
        pvrsrvctl.te \
        servicemanager.te \
        surfaceflinger.te \
        mediaserver.te \
        sdcardd.te \
        kernel.te \
        system_app.te \
        rild.te \
        recovery.te \
        service.te \
        uncrypt.te \
        service_contexts

BOARD_SEPOLICY_REPLACE := \
        netd.te \
        domain.te \
        system_server.te \
        untrusted_app.te \
        app.te

# Include an expanded selection of fonts
EXTENDED_FONT_FOOTPRINT := true
