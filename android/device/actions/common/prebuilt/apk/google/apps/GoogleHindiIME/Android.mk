###############################################################################
# GoogleHindiIME
LOCAL_PATH := $(call my-dir)

my_archs := arm x86 arm64 x86_64
my_src_arch := $(call get-prebuilt-src-arch, $(my_archs))
ifeq ($(my_src_arch),arm)
my_src_abi := armeabi-v7a
else ifeq ($(my_src_arch),x86)
my_src_abi := x86
else ifeq ($(my_src_arch),arm64)
my_src_abi := arm64-v8a
else ifeq ($(my_src_arch),x86_64)
my_src_abi := x86_64
endif

include $(CLEAR_VARS)
LOCAL_MODULE := GoogleHindiIME
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_TAGS := optional
LOCAL_BUILT_MODULE_STEM := package.apk
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
#LOCAL_PRIVILEGED_MODULE :=
LOCAL_CERTIFICATE := PRESIGNED
#LOCAL_OVERRIDES_PACKAGES :=
ifeq ($(my_src_arch),arm)
  LOCAL_SRC_FILES := $(LOCAL_MODULE).apk
else ifeq ($(my_src_arch),x86)
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_x86.apk
else ifeq ($(my_src_arch),arm64)
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_arm64.apk
else ifeq ($(my_src_arch),x86_64)
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_x86_64.apk
endif
#LOCAL_REQUIRED_MODULES :=
LOCAL_PREBUILT_JNI_LIBS := \
    @lib/$(my_src_abi)/libgnustl_shared.so \
    @lib/$(my_src_abi)/libhindi_data_bundle_preload.so \
    @lib/$(my_src_abi)/libjni_hmm_shared_engine.so
LOCAL_MODULE_TARGET_ARCH := $(my_src_arch)
include $(BUILD_PREBUILT)
