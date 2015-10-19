LOCAL_PATH := $(call my-dir)
# =============================================================
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
		$(call all-java-files-under, core)\
		$(call all-java-files-under, services)
		
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := actions

#LOCAL_JAVA_LIBRARIES := services
LOCAL_MODULE_CLASS := JAVA_LIBRARIES	

# AIDL
LOCAL_AIDL_INCLUDES +=$(LOCAL_PATH)/core/java
LOCAL_SRC_FILES += \
	core/java/com/actions/hardware/ICableStatusListener.aidl \
	core/java/com/actions/hardware/IDisplayService.aidl\
	core/java/com/actions/hardware/IPerformanceService.aidl

framework_built := $(call java-lib-deps,framework)
LOCAL_PRELINK_MODULE := false 
include $(BUILD_JAVA_LIBRARY)

# The JNI component
include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))
 

