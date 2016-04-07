ifeq (true, false)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_JAVA_LIBRARIES := bouncycastle conscrypt telephony-common actions
LOCAL_STATIC_JAVA_LIBRARIES := android-support-v4 android-support-v13 jsr305


LOCAL_MODULE_TAGS := eng optional
LOCAL_SRC_FILES := \
        $(call all-java-files-under, src)

LOCAL_AAPT_FLAGS := \
	--auto-add-overlay 
        
LOCAL_PACKAGE_NAME := ActduinoTest

LOCAL_AAPT_FLAGS += -c zz_ZZ

include $(BUILD_PACKAGE)
include $(LOCAL_PATH)/jni/Android.mk
LOCAL_JNI_SHARED_LIBRARIES :=  libactduino_test
#Use the folloing include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))
endif
