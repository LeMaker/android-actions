

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE :=Superuser
LOCAL_SRC_FILES :=app/Superuser.apk
LOCAL_MODULE_TAGS :=optional
LOCAL_MODULE_CLASS :=APPS
LOCAL_CERTIFICATE := PRESIGNED
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE :=supersu
LOCAL_SRC_FILES :=xbin/su
LOCAL_MODULE_STEM := su
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS :=EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT)/xbin/
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE :=supolicy
LOCAL_SRC_FILES :=xbin/supolicy
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS :=EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT)/xbin/
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE :=sugote-mksh
LOCAL_SRC_FILES :=xbin/sugote-mksh
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS :=EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT)/xbin/
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE :=libsupol
LOCAL_SRC_FILES   := lib/libsupol.so
LOCAL_MODULE_STEM := libsupol.so
LOCAL_MODULE_TAGS :=optional
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
include $(BUILD_PREBUILT)

DAEMON_SU := $(TARGET_OUT)/xbin/daemonsu
$(DAEMON_SU): $(LOCAL_PATH)/xbin/su | $(ACP)
	$(copy-file-to-new-target)

HIDDEN_SU := $(TARGET_OUT)/bin/.ext/.su
$(HIDDEN_SU): $(LOCAL_PATH)/xbin/su | $(ACP)
	$(copy-file-to-new-target)

GOTE_SU := $(TARGET_OUT)/xbin/sugote
$(GOTE_SU): $(LOCAL_PATH)/xbin/su | $(ACP)
	$(copy-file-to-new-target)

ALL_MODULES.$(LOCAL_MODULE).INSTALLED := \
    $(ALL_MODULES.$(LOCAL_MODULE).INSTALLED) $(DAEMON_SU)  $(HIDDEN_SU) $(GOTE_SU)

include $(CLEAR_VARS)
LOCAL_MODULE :=install-recovery.sh
LOCAL_SRC_FILES :=install-recovery.sh
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS :=EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin/
include $(BUILD_PREBUILT)

#utils related
superuser_PRODUCT_PACKAGES += \
    	supersu \
    	supolicy \
    	sugote-mksh \
    	libsupol \
    	Superuser  \
        install-recovery.sh


include $(CLEAR_VARS)
LOCAL_MODULE := superuser_prebuilt
LOCAL_MODULE_TAGS := eng optional
LOCAL_REQUIRED_MODULES := $(superuser_PRODUCT_PACKAGES)
include $(BUILD_PHONY_PACKAGE)
