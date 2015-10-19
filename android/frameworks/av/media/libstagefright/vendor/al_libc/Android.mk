LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:=               \
    al_libc.c 					\
    al_uconv.c 					\
    al_detect.cpp 				\
	al_xml.cpp					\
    SinoDetect.cpp

LOCAL_C_INCLUDES := \
        $(call include-path-for, alsp) \
        $(TOP)/external/icu/icu4c/source/common \
		$(TOP)/external/tinyxml\
		$(TOP)/system/core/libion/include


LOCAL_SHARED_LIBRARIES := 	\
    libutils 				\
    libcutils 				\
    libicuuc 				\
    libion					\
	libtinyxml
                
LOCAL_MODULE_TAGS := eng
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE:= libalc

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:=               \
    al_libc.c

LOCAL_C_INCLUDES := \
    $(call include-path-for, alsp) \
	$(TOP)/system/core/libion/include\
	$(TOP)/system/core/libion/kernel-headers

LOCAL_MODULE:= libalc
include $(BUILD_STATIC_LIBRARY)
