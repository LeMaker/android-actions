LOCAL_PATH:= $(call my-dir)

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES := \
  AttrVisitor.inc \
  DiagnosticCommonKinds.inc \
  DiagnosticDriverKinds.inc \
  DiagnosticSemaKinds.inc \
  Options.inc \
  CC1Options.inc

clang_driver_SRC_FILES := \
  Action.cpp \
  Compilation.cpp \
  Driver.cpp \
  DriverOptions.cpp \
  Job.cpp \
  Multilib.cpp \
  Phases.cpp \
  SanitizerArgs.cpp \
  Tool.cpp \
  ToolChain.cpp \
  ToolChains.cpp \
  Tools.cpp \
  Types.cpp \
  WindowsToolChain.cpp

LOCAL_SRC_FILES := $(clang_driver_SRC_FILES)

LOCAL_MODULE := libclangDriver
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(CLANG_VERSION_INC_MK)
include $(BUILD_HOST_STATIC_LIBRARY)
