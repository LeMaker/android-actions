# Copyright 2007-2008 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

# DM plugin source files
DM_PLUGIN_SRC_FILES := \
 src/DmtJavaPlugin.cc \
 src/DmtJavaPluginManager.cc \
 src/DmtJavaPluginTree.cc  \
 src/DmtJavaPluginNode.cc

#javaplugin.h
#javapluginmgr.h

DM_PLUGIN_INCLUDES := \
 $(LOCAL_PATH)/../../dmlib/api/common \
 $(LOCAL_PATH)/../../dmlib/api/native \
 $(LOCAL_PATH)/../../dmlib/api/native/plugin \
 $(LOCAL_PATH)/../../xpl/hdr \
 $(JNI_H_INCLUDE)

############################################
# DM source files
LOCAL_SRC_FILES += $(DM_PLUGIN_SRC_FILES)


############################################
# DM unittest include files
LOCAL_C_INCLUDES += $(DM_PLUGIN_INCLUDES)

############################################
# general DM flags
DM_CFLAGS := \
-DLOB_SUPPORT

# compile flags
LOCAL_CFLAGS += $(DM_CFLAGS)

LOCAL_SHARED_LIBRARIES += \
  liblog \
  libdmengine \
  libandroid_runtime \
  libnativehelper

LOCAL_MODULE := libdmjavaplugin
LOCAL_PRELINK_MODULE := false
#TARGET_BUILD_TYPE=debug
LOCAL_STRIP_MODULE=false

LOCAL_CPP_EXTENSION := .cc

LOCAL_MULTILIB := 32

include $(BUILD_SHARED_LIBRARY)

