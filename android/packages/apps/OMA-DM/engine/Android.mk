LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
# DM Engine directories
DM_GLOBAL_SUBDIRS_A := \
 dmlib/dmengine/dm_ua/src \
 dmlib/dmengine/dm_persist/src \
 dmlib/dmengine/dm_tnm/src \
 dmlib/dmengine/dm_util/src \
 dmlib/dmtapi/native/src \
 dmlib/plugin/src \
 dmlib/notification_agent/src 

DM_GLOBAL_SUBDIRS_B := \
 dmlib/dmengine/dm_security/src \
 dmlib/dmengine/dm_ssession/src \
 dmlib/dmengine/oma_toolkit/src 
 

ifdef DM_NATIVE_HTTP
    DM_GLOBAL_SUBDIRS_B +=  dmlib/dmengine/dm_transport/src
endif

DM_OMA_TOOLKIT_INCLUDES := \
 dmlib/dmengine/oma_toolkit/hdr \
 dmlib/dmengine/oma_toolkit/sml/xpt/hdr \
 dmlib/dmengine/oma_toolkit/sml/mgr/hdr \
 dmlib/dmengine/oma_toolkit/sml/ghdr \
 dmlib/dmengine/oma_toolkit/sml/lib/hdr \
 dmlib/dmengine/oma_toolkit/sml/wsm/hdr \
 dmlib/dmengine/oma_toolkit/sml/xlt/src \
 dmlib/dmengine/oma_toolkit/sml/xlt/hdr 

DM_API_INCLUDES := \
    dmlib/api/common \
    dmlib/api/native \
    dmlib/api/native/plugin 

# DM Engine source files
DM_ENGINE_SRC_FILES:= $(foreach dir,$(DM_GLOBAL_SUBDIRS_A),$(wildcard $(LOCAL_PATH)/$(dir)/*.*))
DM_ENGINE_SRC_FILES:= $(subst $(LOCAL_PATH)/,,$(DM_ENGINE_SRC_FILES))
DM_ENGINE_SRC_FILES += xpl/src/xpl_dm_Notifications.cpp 

# DM Session source files
DM_SESSION_SRC_FILES:= $(foreach dir,$(DM_GLOBAL_SUBDIRS_B),$(wildcard $(LOCAL_PATH)/$(dir)/*.*))
DM_SESSION_SRC_FILES:= $(subst $(LOCAL_PATH)/,,$(DM_SESSION_SRC_FILES))

# DM XPL source files
DM_XPL_SRC_FILES := \
  xpl/src/xpl_Alert.cc \
  xpl/src/xpl_File.cc \
  xpl/src/xpl_Lib.cc \
  xpl/src/xpl_Memory.cc \
  xpl/src/xpl_Regex.cc \
  xpl/src/xpl_Time.cc \
  xpl/src/xpl_dm_Manager.cc \
  xpl/src/xpl_dm_Notifications.cc \
  xpl/src/dmAllocatedPointersPool.cc \
  xpl/src/dmMemory.cc \
  xpl/src/dmNewDataTypesValidation.cc \
  xpl/src/dmThreadHelper.cc \
  xpl/src/dmThreadQueue.cc \
  xpl/src/dmprofile.cc \

ifdef DM_NATIVE_HTTP
    DM_XPL_SRC_FILES += xpl/src/xpl_HTTP_socket.cc \
                        xpl/src/dmSocketConnector.cc
else
    DM_JNI_SRC_FILES:= jni/DMServiceConnection.cc  \
                       jni/DMServiceAlert.cc \
                       jni/DMServiceMain.cc  \
                       dmlib/dmengine/dm_transport/src/dm_tpt_utils.c \
                       jni/DMTreeManager.cc
    LOCAL_SRC_FILES += $(DM_JNI_SRC_FILES)
endif


# DM source files
LOCAL_SRC_FILES += $(DM_ENGINE_SRC_FILES)
LOCAL_SRC_FILES += $(DM_SESSION_SRC_FILES)
LOCAL_SRC_FILES += $(DM_XPL_SRC_FILES)

#############################################
# DM include files
DM_ENGINE_INCLUDES = $(foreach dir1,$(DM_GLOBAL_SUBDIRS_A), \
  $(subst /src,/hdr,$(dir1)) )
DM_ENGINE_INCLUDES += $(foreach dir1,$(DM_GLOBAL_SUBDIRS_B), \
  $(subst /src,/hdr,$(dir1)) )

DM_ENGINE_INCLUDES += dmlib/dmengine/dm_transport/hdr
DM_ENGINE_INCLUDES += jni

DM_ENGINE_INCLUDES += $(DM_API_INCLUDES)
DM_ENGINE_INCLUDES += $(DM_OMA_TOOLKIT_INCLUDES)

DM_ENGINE_INCLUDES := $(addprefix $(LOCAL_PATH)/, $(DM_ENGINE_INCLUDES))
DM_XPL_INCLUDES := $(LOCAL_PATH)/xpl/hdr 


LOCAL_C_INCLUDES += $(DM_ENGINE_INCLUDES)
LOCAL_C_INCLUDES += $(DM_XPL_INCLUDES)
LOCAL_C_INCLUDES += $(JNI_H_INCLUDE)

############################################
# general DM flags
DM_CFLAGS :=  -DVRTXMC \
              -DDM_ATOMIC_SUPPORTED \
              -DLOB_SUPPORT \
              -DXPL_LOG_LEVEL=XPL_LOG_Warn \
              -DDM_NO_LOCKING \
              -DDM_NO_SESSION_LIB \
              -DTNDS_SUPPORT

# for old ARM platform builds
#DM_ARM_CFLAGS := -DPLATFORM_EZX \
#                 -DDMSyncMLLibVersion=\"03.00.00\"

# for x86/ARM builds on Android
DM_X86_CFLAGS = -DEZX_PORT -DPLATFORM_X86 -DPLATFORM_ANDROID \
                -DSYNCML_DM_DBG_USING_XML \
                -DDM_SUPPORT_AUTHPREF \
                -DNO_CAF \
                -DNO_OTHER_PLUGIN

# saved from LJ for future reference
#DM_CFLAGS += -DFEAT_DM_VERSION_FLEX -DDM_PERFORMANCE_ENABLED

# compile flags
LOCAL_CFLAGS += $(DM_CFLAGS)
LOCAL_CFLAGS += $(DM_X86_CFLAGS)

ifdef FTR_OMADM_SDMSERVICES
LOCAL_CFLAGS += -DDM_SDMSERVICES
endif

ifdef DM_NATIVE_HTTP
LOCAL_CFLAGS += -DDM_NATIVE_HTTP
endif

ifeq ($(TARGET_BUILD_TYPE),release)
LOCAL_CFLAGS += -DDM_UNITEST
endif

LOCAL_SHARED_LIBRARIES += \
    libandroid_runtime \
    liblog \
    libdl

LOCAL_MODULE := libdmengine
LOCAL_PRELINK_MODULE := false
#TARGET_BUILD_TYPE=debug
LOCAL_STRIP_MODULE := true

LOCAL_REQUIRED_MODULES := dmt_data
LOCAL_CPP_EXTENSION := .cc

LOCAL_MULTILIB := 32

include $(BUILD_SHARED_LIBRARY)

ifeq ($(TARGET_BUILD_TYPE),release)
include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH)/../unitest)
endif

# This finds and builds the test apk as well, so a single make does both.
include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))
