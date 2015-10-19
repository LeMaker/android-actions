
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := OWLPlayer
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES   := OWLPlayer.apk
LOCAL_BUILT_MODULE_STEM := $(LOCAL_SRC_FILES)
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_MODULE_TARGET_ARCH := arm
LOCAL_PREBUILT_JNI_LIBS := \
  @lib/armeabi-v7a/libavcodec-1.1.git.so \
  @lib/armeabi-v7a/libavformat-1.1.git.so \
  @lib/armeabi-v7a/libavutil-1.1.git.so \
  @lib/armeabi-v7a/libctrlpt_owl.so \
  @lib/armeabi-v7a/libdmr_owl.so \
  @lib/armeabi-v7a/libdms_owl.so \
  @lib/armeabi-v7a/libowlplayer_jni.so \
  @lib/armeabi-v7a/libsubtitle.so \
  @lib/armeabi-v7a/libsubtitle_jni.so \
  @lib/armeabi-v7a/libvinit.so
  
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE := AdobeFlashPlayer
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_TAGS := eng optional
LOCAL_SRC_FILES := AdobeFlashPlayer.apk
LOCAL_MODULE_STEM := $(LOCAL_SRC_FILES)
#LOCAL_BUILT_MODULE_STEM := $(LOCAL_SRC_FILES)
#LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
#LOCAL_MODULE_TARGET_ARCH := arm
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_PREBUILT_JNI_LIBS := \
  @lib/armeabi-v7a/libysshared.so \
  @lib/armeabi-v7a/libflashplayer.so
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE := webkit42_xml
LOCAL_SRC_FILES := android.webkit42.xml
LOCAL_MODULE_STEM := $(LOCAL_SRC_FILES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE := webkit42_jar
LOCAL_SRC_FILES := webkit42.jar
LOCAL_MODULE_STEM := $(LOCAL_SRC_FILES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/framework
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE := webcore_ttf
LOCAL_SRC_FILES := webcore.ttf
LOCAL_MODULE_STEM := $(LOCAL_SRC_FILES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/fonts/
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libwebcore.so libactmd5.so
LOCAL_MODULE_TAGS := eng optional
include $(BUILD_MULTI_PREBUILT)

