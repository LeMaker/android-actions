###############################################################################
# GmsCore
LOCAL_PATH := $(call my-dir)

my_archs := arm x86 arm64 x86_64
my_src_arch := $(call get-prebuilt-src-arch, $(my_archs))
ifeq ($(my_src_arch),arm)
my_src_abi := armeabi-v7a
else ifeq ($(my_src_arch),x86)
my_src_abi := x86
else ifeq ($(my_src_arch),arm64)
my_src_abi := arm64-v8a
my_src_32bit_abi := armeabi-v7a
else ifeq ($(my_src_arch),x86_64)
my_src_abi := x86_64
my_src_32bit_abi := x86
endif

include $(CLEAR_VARS)
LOCAL_MODULE := GmsCore
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_TAGS := optional
LOCAL_BUILT_MODULE_STEM := package.apk
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_PRIVILEGED_MODULE := true
LOCAL_CERTIFICATE := PRESIGNED
#LOCAL_OVERRIDES_PACKAGES :=
ifeq ($(my_src_arch),arm)
ifneq (,$(filter xxxhdpi,$(PRODUCT_AAPT_PREF_CONFIG)))
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_xxhdpi.apk
else ifneq (,$(filter xxhdpi,$(PRODUCT_AAPT_PREF_CONFIG)))
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_xxhdpi.apk
else ifneq (,$(filter xhdpi,$(PRODUCT_AAPT_PREF_CONFIG)))
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_xhdpi.apk
else ifneq (,$(filter hdpi,$(PRODUCT_AAPT_PREF_CONFIG)))
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_hdpi.apk
else ifneq (,$(filter mdpi,$(PRODUCT_AAPT_PREF_CONFIG)))
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_hdpi.apk
else
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_alldpi.apk
endif
else
ifneq (,$(filter xxxhdpi,$(PRODUCT_AAPT_PREF_CONFIG)))
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_$(my_src_arch)_xxhdpi.apk
else ifneq (,$(filter xxhdpi,$(PRODUCT_AAPT_PREF_CONFIG)))
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_$(my_src_arch)_xxhdpi.apk
else ifneq (,$(filter xhdpi,$(PRODUCT_AAPT_PREF_CONFIG)))
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_$(my_src_arch)_xhdpi.apk
else ifneq (,$(filter hdpi,$(PRODUCT_AAPT_PREF_CONFIG)))
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_$(my_src_arch)_hdpi.apk
else ifneq (,$(filter mdpi,$(PRODUCT_AAPT_PREF_CONFIG)))
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_$(my_src_arch)_hdpi.apk
else
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_$(my_src_arch)_alldpi.apk
endif
endif
#LOCAL_REQUIRED_MODULES :=
ifeq ($(my_src_arch),arm)
LOCAL_PREBUILT_JNI_LIBS := \
    @lib/$(my_src_abi)/libAppDataSearch.so \
    @lib/$(my_src_abi)/libWhisper.so \
    @lib/$(my_src_abi)/libconscrypt_gmscore_jni.so \
    @lib/$(my_src_abi)/libgames_rtmp_jni.so \
    @lib/$(my_src_abi)/libgcastv2_base.so \
    @lib/$(my_src_abi)/libgcastv2_support.so \
    @lib/$(my_src_abi)/libgms-ocrclient.so \
    @lib/$(my_src_abi)/libgmscore.so \
    @lib/$(my_src_abi)/libjgcastservice.so \
    @lib/$(my_src_abi)/libsslwrapper_jni.so
else ifeq ($(my_src_arch),x86)
LOCAL_PREBUILT_JNI_LIBS := \
    @lib/$(my_src_abi)/libAppDataSearch.so \
    @lib/$(my_src_abi)/libWhisper.so \
    @lib/$(my_src_abi)/libconscrypt_gmscore_jni.so \
    @lib/$(my_src_abi)/libgames_rtmp_jni.so \
    @lib/$(my_src_abi)/libgcastv2_base.so \
    @lib/$(my_src_abi)/libgcastv2_support.so \
    @lib/$(my_src_abi)/libgms-ocrclient.so \
    @lib/$(my_src_abi)/libgmscore.so \
    @lib/$(my_src_abi)/libjgcastservice.so \
    @lib/$(my_src_abi)/libsslwrapper_jni.so
else ifeq ($(my_src_arch),arm64)
LOCAL_PREBUILT_JNI_LIBS_arm64 := \
    @lib/$(my_src_abi)/libAppDataSearch.so \
    @lib/$(my_src_abi)/libWhisper.so \
    @lib/$(my_src_abi)/libconscrypt_gmscore_jni.so \
    @lib/$(my_src_abi)/libgames_rtmp_jni.so \
    @lib/$(my_src_abi)/libgcastv2_base.so \
    @lib/$(my_src_abi)/libgcastv2_support.so \
    @lib/$(my_src_abi)/libgms-ocrclient.so \
    @lib/$(my_src_abi)/libgmscore.so \
    @lib/$(my_src_abi)/libjgcastservice.so \
    @lib/$(my_src_abi)/libsslwrapper_jni.so
LOCAL_PREBUILT_JNI_LIBS_arm := \
    @lib/$(my_src_32bit_abi)/libconscrypt_gmscore_jni.so \
    @lib/$(my_src_32bit_abi)/libgmscore.so
LOCAL_MULTILIB := both
else ifeq ($(my_src_arch),x86_64)
LOCAL_PREBUILT_JNI_LIBS_x86_64 := \
    @lib/$(my_src_abi)/libAppDataSearch.so \
    @lib/$(my_src_abi)/libWhisper.so \
    @lib/$(my_src_abi)/libconscrypt_gmscore_jni.so \
    @lib/$(my_src_abi)/libgames_rtmp_jni.so \
    @lib/$(my_src_abi)/libgcastv2_base.so \
    @lib/$(my_src_abi)/libgcastv2_support.so \
    @lib/$(my_src_abi)/libgms-ocrclient.so \
    @lib/$(my_src_abi)/libgmscore.so \
    @lib/$(my_src_abi)/libjgcastservice.so \
    @lib/$(my_src_abi)/libsslwrapper_jni.so
LOCAL_PREBUILT_JNI_LIBS_x86 := \
    @lib/$(my_src_32bit_abi)/libconscrypt_gmscore_jni.so \
    @lib/$(my_src_32bit_abi)/libgmscore.so
LOCAL_MULTILIB := both
endif
LOCAL_MODULE_TARGET_ARCH := $(my_src_arch)
include $(BUILD_PREBUILT)
