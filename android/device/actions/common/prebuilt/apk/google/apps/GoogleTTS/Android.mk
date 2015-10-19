###############################################################################
# GoogleTTS
LOCAL_PATH := $(call my-dir)

my_archs := arm x86
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
LOCAL_MODULE := GoogleTTS
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_TAGS := optional
LOCAL_BUILT_MODULE_STEM := package.apk
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
#LOCAL_PRIVILEGED_MODULE :=
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_OVERRIDES_PACKAGES := PicoTts
_googletts_lib_version_suffix :=
ifeq ($(my_src_arch),arm)
  LOCAL_SRC_FILES := $(LOCAL_MODULE).apk
  _googletts_lib_version_suffix := 210303130
else ifeq ($(my_src_arch),x86)
  LOCAL_SRC_FILES := $(LOCAL_MODULE)_x86.apk
  _googletts_lib_version_suffix := 210303131
endif
#LOCAL_REQUIRED_MODULES :=
LOCAL_PREBUILT_JNI_LIBS := \
    @lib/$(my_src_abi)/libpatts_engine_jni_api_ub.$(_googletts_lib_version_suffix).so \
    @lib/$(my_src_abi)/libspeexwrapper_ub.$(_googletts_lib_version_suffix).so
_googletts_lib_version_suffix :=
LOCAL_MODULE_TARGET_ARCH := $(my_src_arch)
include $(BUILD_PREBUILT)
