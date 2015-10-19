BUILD_CTSAutoSetting_APK := $(R_BUILD_CTSAutoSetting_APK)
$(info BUILD_CTSAutoSetting_APK=$(BUILD_CTSAutoSetting_APK))

ifeq ($(BUILD_CTSAutoSetting_APK), true)
	LOCAL_PATH:= $(call my-dir)
	include $(CLEAR_VARS)

	LOCAL_JAVA_LIBRARIES := bouncycastle conscrypt telephony-common actions
	LOCAL_STATIC_JAVA_LIBRARIES := android-support-v4 android-support-v13 jsr305

	LOCAL_MODULE_TAGS := optional

	LOCAL_SRC_FILES := \
        	$(call all-java-files-under, src) 

	LOCAL_AAPT_FLAGS := \
		--auto-add-overlay 
        
	LOCAL_PACKAGE_NAME := CtsAutoSetting
	LOCAL_CERTIFICATE := platform
	LOCAL_PRIVILEGED_MODULE := true
	LOCAL_DEX_PREOPT := false

	#LOCAL_PROGUARD_FLAG_FILES := proguard.flags

	LOCAL_AAPT_FLAGS += -c zz_ZZ

	include $(BUILD_PACKAGE)

	# Use the folloing include to make our test apk.
	include $(call all-makefiles-under,$(LOCAL_PATH))

endif