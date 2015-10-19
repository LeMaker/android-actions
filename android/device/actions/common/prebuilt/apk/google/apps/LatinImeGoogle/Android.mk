###############################################################################
# LatinImeGoogle
LOCAL_PATH := $(call my-dir)

my_archs := arm x86 arm64 x86_64
my_src_arch := $(call get-prebuilt-src-arch, $(my_archs))

include $(CLEAR_VARS)
LOCAL_MODULE := LatinImeGoogle
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_TAGS := optional
LOCAL_BUILT_MODULE_STEM := package.apk
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
#LOCAL_PRIVILEGED_MODULE :=
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_OVERRIDES_PACKAGES := LatinIME
ifeq ($(my_src_arch),arm)
  LOCAL_SRC_FILES := $(LOCAL_MODULE).apk
else ifeq ($(my_src_arch),x86)
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_x86.apk
else ifeq ($(my_src_arch),arm64)
  LOCAL_SRC_FILES := $(LOCAL_MODULE).apk
else ifeq ($(my_src_arch),x86_64)
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_x86.apk
endif
#LOCAL_REQUIRED_MODULES :=
LOCAL_PREBUILT_JNI_LIBS := \
    lib/$(my_src_arch)/libjni_latinimegoogle.so
LOCAL_MODULE_TARGET_ARCH := $(my_src_arch)
include $(BUILD_PREBUILT)
