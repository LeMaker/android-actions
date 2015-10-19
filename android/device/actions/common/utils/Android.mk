LOCAL_PATH := $(call my-dir)

#include $(CLEAR_VARS)
#LOCAL_MODULE := ActionsSerialWrite
#LOCAL_SRC_FILES := ./bin/ActionsSerialWrite
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_CLASS := EXECUTABLES
#LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
#include $(BUILD_PREBUILT)
#


#====================copy e2fsck to root/sbin/====================
#have no choice to do like this, because the LOCAL_MODULE=e2fsck 
#already exist in external/e2fsprogs/e2fsck/Android.mk

define _add_e2fsck_file
include $$(CLEAR_VARS)
LOCAL_MODULE := boot_e2fsck_dep_$(notdir $(1))
LOCAL_MODULE_STEM := $(notdir $(1))
_dep_modules += $$(LOCAL_MODULE)
LOCAL_SRC_FILES := $1 
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $$(TARGET_ROOT_OUT)/sbin
include $$(BUILD_PREBUILT)
endef

_dep_modules :=
$(eval $(call _add_e2fsck_file, bin/e2fsck))

include $(CLEAR_VARS)
LOCAL_MODULE := boot_e2fsck
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(_dep_modules)
include $(BUILD_PHONY_PACKAGE)


#====================copy *.ko to root/lib/modules/====================
define _add_driver_file
include $$(CLEAR_VARS)
LOCAL_MODULE := boot_driver_dep_$(notdir $(1))
LOCAL_MODULE_STEM := $(notdir $(1))
_dep_modules += $$(LOCAL_MODULE)
LOCAL_SRC_FILES := $1
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $$(TARGET_ROOT_OUT)/lib/modules
include $$(BUILD_PREBUILT)
endef

_dep_modules :=
$(foreach ko_file, $(call find-subdir-subdir-files, "driver", "*.ko"), \
  $(eval $(call _add_driver_file,$(ko_file))))

include $(CLEAR_VARS)
LOCAL_MODULE := boot_driver 
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(_dep_modules)
include $(BUILD_PHONY_PACKAGE)


#====================copy bin files to system/xbin/====================
define _add_bin_file
include $$(CLEAR_VARS)
LOCAL_MODULE := system_xbin_dep_$(notdir $(1))
LOCAL_MODULE_STEM := $(notdir $(1))
_dep_modules += $$(LOCAL_MODULE)
LOCAL_SRC_FILES := $1
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $$(TARGET_OUT_OPTIONAL_EXECUTABLES)
include $$(BUILD_PREBUILT)
endef

_dep_modules :=
$(foreach bin_file, $(call find-subdir-subdir-files, bin/*, "*"), \
  $(eval $(call _add_bin_file,$(bin_file))))

include $(CLEAR_VARS)
LOCAL_MODULE := system_xbin 
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(_dep_modules)
include $(BUILD_PHONY_PACKAGE)


#====================copy bin files to system/etc/====================
define _add_etc_file
include $$(CLEAR_VARS)
LOCAL_MODULE := system_etc_dep_$(notdir $(1))
LOCAL_MODULE_STEM := $(notdir $(1))
_dep_modules += $$(LOCAL_MODULE)
LOCAL_SRC_FILES := $1
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $$(TARGET_OUT_ETC)
include $$(BUILD_PREBUILT)
endef

_dep_modules :=
$(foreach etc_file, $(call find-subdir-subdir-files, "etc", "*.xml"), \
  $(eval $(call _add_etc_file,$(etc_file))))

include $(CLEAR_VARS)
LOCAL_MODULE := system_etc
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(_dep_modules)
include $(BUILD_PHONY_PACKAGE)


_add_e2fsck_file :=
_add_driver_file :=
_add_bin_file :=
_add_etc_file :=
_dep_modules :=

