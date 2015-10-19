
LOCAL_PATH:=$(call my-dir)

rs_base_CFLAGS := -Werror -Wall -Wno-unused-parameter -Wno-unused-variable -fno-exceptions
ifeq ($(TARGET_BUILD_PDK), true)
  rs_base_CFLAGS += -D__RS_PDK__
endif

ifneq ($(OVERRIDE_RS_DRIVER),)
  rs_base_CFLAGS += -DOVERRIDE_RS_DRIVER=$(OVERRIDE_RS_DRIVER)
endif

ifneq ($(DISABLE_RS_64_BIT_DRIVER),)
  rs_base_CFLAGS += -DDISABLE_RS_64_BIT_DRIVER
endif

ifeq ($(RS_FIND_OFFSETS), true)
  rs_base_CFLAGS += -DRS_FIND_OFFSETS
endif

include $(CLEAR_VARS)
ifneq ($(HOST_OS),windows)
LOCAL_CLANG := true
endif
LOCAL_MODULE := libRSDriver
LOCAL_MODULE_TARGET_ARCH_WARN := arm mips mips64 x86 x86_64 arm64

LOCAL_SRC_FILES:= \
	driver/rsdAllocation.cpp \
	driver/rsdBcc.cpp \
	driver/rsdCore.cpp \
	driver/rsdElement.cpp \
	driver/rsdFrameBuffer.cpp \
	driver/rsdFrameBufferObj.cpp \
	driver/rsdGL.cpp \
	driver/rsdMesh.cpp \
	driver/rsdMeshObj.cpp \
	driver/rsdPath.cpp \
	driver/rsdProgram.cpp \
	driver/rsdProgramRaster.cpp \
	driver/rsdProgramStore.cpp \
	driver/rsdRuntimeStubs.cpp \
	driver/rsdSampler.cpp \
	driver/rsdScriptGroup.cpp \
	driver/rsdShader.cpp \
	driver/rsdShaderCache.cpp \
	driver/rsdType.cpp \
	driver/rsdVertexArray.cpp


LOCAL_SHARED_LIBRARIES += libRS libRSCpuRef libc++
LOCAL_SHARED_LIBRARIES += liblog libcutils libutils libEGL libGLESv1_CM libGLESv2
LOCAL_SHARED_LIBRARIES += libui libgui libsync

LOCAL_SHARED_LIBRARIES += libbcc libbcinfo libLLVM

LOCAL_C_INCLUDES += frameworks/compile/libbcc/include
LOCAL_C_INCLUDES += frameworks/rs/cpu_ref/linkloader/include
LOCAL_C_INCLUDES += external/libcxx/include

LOCAL_CFLAGS += $(rs_base_CFLAGS)
LOCAL_CPPFLAGS += -fno-exceptions

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

# Build rsg-generator ====================
include $(CLEAR_VARS)

LOCAL_MODULE := rsg-generator

# These symbols are normally defined by BUILD_XXX, but we need to define them
# here so that local-intermediates-dir works.

LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_CLASS := EXECUTABLES
intermediates := $(local-intermediates-dir)

LOCAL_SRC_FILES:= \
    spec.l \
    rsg_generator.c

include $(BUILD_HOST_EXECUTABLE)

# TODO: This should go into build/core/config.mk
RSG_GENERATOR:=$(LOCAL_BUILT_MODULE)

include $(CLEAR_VARS)
ifneq ($(HOST_OS),windows)
LOCAL_CLANG := true
endif
LOCAL_MODULE := libRS
LOCAL_MODULE_TARGET_ARCH_WARN := arm mips mips64 x86 x86_64 arm64

LOCAL_MODULE_CLASS := SHARED_LIBRARIES
generated_sources:= $(local-generated-sources-dir)

# Generate custom headers

GEN := $(addprefix $(generated_sources)/, \
            rsgApiStructs.h \
            rsgApiFuncDecl.h \
        )

$(GEN) : PRIVATE_PATH := $(LOCAL_PATH)
$(GEN) : PRIVATE_CUSTOM_TOOL = cat $(PRIVATE_PATH)/rs.spec $(PRIVATE_PATH)/rsg.spec $(PRIVATE_PATH)/rs_native.spec | $(RSG_GENERATOR) $< $@
$(GEN) : $(RSG_GENERATOR) $(LOCAL_PATH)/rs.spec $(LOCAL_PATH)/rsg.spec $(LOCAL_PATH)/rs_native.spec
$(GEN): $(generated_sources)/%.h : $(LOCAL_PATH)/%.h.rsg
	$(transform-generated-source)

# used in jni/Android.mk
rs_generated_source += $(GEN)
LOCAL_GENERATED_SOURCES += $(GEN)

# Generate custom source files

GEN := $(addprefix $(generated_sources)/, \
            rsgApi.cpp \
            rsgApiReplay.cpp \
        )

$(GEN) : PRIVATE_PATH := $(LOCAL_PATH)
$(GEN) : PRIVATE_CUSTOM_TOOL = cat $(PRIVATE_PATH)/rs.spec $(PRIVATE_PATH)/rsg.spec $(PRIVATE_PATH)/rs_native.spec | $(RSG_GENERATOR) $< $@
$(GEN) : $(RSG_GENERATOR) $(LOCAL_PATH)/rs.spec $(LOCAL_PATH)/rsg.spec $(LOCAL_PATH)/rs_native.spec
$(GEN): $(generated_sources)/%.cpp : $(LOCAL_PATH)/%.cpp.rsg
	$(transform-generated-source)

# used in jni/Android.mk
rs_generated_source += $(GEN)

LOCAL_GENERATED_SOURCES += $(GEN)

LOCAL_SRC_FILES:= \
	rsAdapter.cpp \
	rsAllocation.cpp \
	rsAnimation.cpp \
	rsComponent.cpp \
	rsContext.cpp \
	rsCppUtils.cpp \
	rsDevice.cpp \
	rsElement.cpp \
	rsFBOCache.cpp \
	rsFifoSocket.cpp \
	rsFileA3D.cpp \
	rsFont.cpp \
	rsGrallocConsumer.cpp \
	rsObjectBase.cpp \
	rsMatrix2x2.cpp \
	rsMatrix3x3.cpp \
	rsMatrix4x4.cpp \
	rsMesh.cpp \
	rsMutex.cpp \
	rsPath.cpp \
	rsProgram.cpp \
	rsProgramFragment.cpp \
	rsProgramStore.cpp \
	rsProgramRaster.cpp \
	rsProgramVertex.cpp \
	rsSampler.cpp \
	rsScript.cpp \
	rsScriptC.cpp \
	rsScriptC_Lib.cpp \
	rsScriptC_LibGL.cpp \
	rsScriptGroup.cpp \
	rsScriptIntrinsic.cpp \
	rsSignal.cpp \
	rsStream.cpp \
	rsThreadIO.cpp \
	rsType.cpp

LOCAL_SHARED_LIBRARIES += liblog libcutils libutils libEGL libGLESv1_CM libGLESv2 libc++
LOCAL_SHARED_LIBRARIES += libgui libsync libdl libui
LOCAL_SHARED_LIBRARIES += libft2 libpng libz

LOCAL_SHARED_LIBRARIES += libbcc libbcinfo libLLVM

LOCAL_C_INCLUDES += external/freetype/include
LOCAL_C_INCLUDES += frameworks/compile/libbcc/include
LOCAL_C_INCLUDES += external/libcxx/include

LOCAL_CFLAGS += $(rs_base_CFLAGS)

LOCAL_CPPFLAGS += -fno-exceptions

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

# Now build a host version for serialization
include $(CLEAR_VARS)
LOCAL_MODULE:= libRS
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_IS_HOST_MODULE := true

intermediates := $(call local-generated-sources-dir)

# Generate custom headers

GEN := $(addprefix $(intermediates)/, \
            rsgApiStructs.h \
            rsgApiFuncDecl.h \
        )

$(GEN) : PRIVATE_PATH := $(LOCAL_PATH)
$(GEN) : PRIVATE_CUSTOM_TOOL = cat $(PRIVATE_PATH)/rs.spec $(PRIVATE_PATH)/rsg.spec $(PRIVATE_PATH)/rs_native.spec | $(RSG_GENERATOR) $< $@
$(GEN) : $(RSG_GENERATOR) $(LOCAL_PATH)/rs.spec $(LOCAL_PATH)/rsg.spec $(LOCAL_PATH)/rs_native.spec
$(GEN): $(intermediates)/%.h : $(LOCAL_PATH)/%.h.rsg
	$(transform-generated-source)

LOCAL_GENERATED_SOURCES += $(GEN)

# Generate custom source files

GEN := $(addprefix $(intermediates)/, \
            rsgApi.cpp \
            rsgApiReplay.cpp \
        )

$(GEN) : PRIVATE_PATH := $(LOCAL_PATH)
$(GEN) : PRIVATE_CUSTOM_TOOL = cat $(PRIVATE_PATH)/rs.spec $(PRIVATE_PATH)/rsg.spec $(PRIVATE_PATH)/rs_native.spec | $(RSG_GENERATOR) $< $@
$(GEN) : $(RSG_GENERATOR) $(LOCAL_PATH)/rs.spec $(LOCAL_PATH)/rsg.spec $(LOCAL_PATH)/rs_native.spec
$(GEN): $(intermediates)/%.cpp : $(LOCAL_PATH)/%.cpp.rsg
	$(transform-generated-source)

LOCAL_GENERATED_SOURCES += $(GEN)

LOCAL_CFLAGS += $(rs_base_CFLAGS)
LOCAL_CFLAGS += -DANDROID_RS_SERIALIZE
LOCAL_CFLAGS += -fPIC
LOCAL_CPPFLAGS += -fno-exceptions

LOCAL_SRC_FILES:= \
	rsAdapter.cpp \
	rsAllocation.cpp \
	rsAnimation.cpp \
	rsComponent.cpp \
	rsContext.cpp \
	rsDevice.cpp \
	rsElement.cpp \
	rsFBOCache.cpp \
	rsFifoSocket.cpp \
	rsFileA3D.cpp \
	rsFont.cpp \
	rsObjectBase.cpp \
	rsMatrix2x2.cpp \
	rsMatrix3x3.cpp \
	rsMatrix4x4.cpp \
	rsMesh.cpp \
	rsMutex.cpp \
	rsPath.cpp \
	rsProgram.cpp \
	rsProgramFragment.cpp \
	rsProgramStore.cpp \
	rsProgramRaster.cpp \
	rsProgramVertex.cpp \
	rsSampler.cpp \
	rsScript.cpp \
	rsScriptC.cpp \
	rsScriptC_Lib.cpp \
	rsScriptC_LibGL.cpp \
	rsScriptGroup.cpp \
	rsScriptIntrinsic.cpp \
	rsSignal.cpp \
	rsStream.cpp \
	rsThreadIO.cpp \
	rsType.cpp

LOCAL_STATIC_LIBRARIES := libcutils libutils liblog

LOCAL_CLANG := true

include $(BUILD_HOST_STATIC_LIBRARY)

LLVM_ROOT_PATH := external/llvm

#=============================================================================
# android librsloader for libbcc (Device)
#-----------------------------------------------------------------------------

rsloader_SRC_FILES := \
  cpu_ref/linkloader/android/librsloader.cpp \
  cpu_ref/linkloader/lib/ELFHeader.cpp \
  cpu_ref/linkloader/lib/ELFSymbol.cpp \
  cpu_ref/linkloader/lib/ELFSectionHeader.cpp \
  cpu_ref/linkloader/lib/ELFTypes.cpp \
  cpu_ref/linkloader/lib/GOT.cpp \
  cpu_ref/linkloader/lib/MemChunk.cpp \
  cpu_ref/linkloader/lib/StubLayout.cpp \
  cpu_ref/linkloader/utils/helper.cpp \
  cpu_ref/linkloader/utils/raw_ostream.cpp \
  cpu_ref/linkloader/utils/rsl_assert.cpp

include $(CLEAR_VARS)


LOCAL_MODULE := librsloader
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(rsloader_SRC_FILES)

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_CFLAGS += $(rs_base_CFLAGS)
LOCAL_CPPFLAGS += -fno-exceptions

LOCAL_C_INCLUDES := \
  $(LOCAL_PATH)/cpu_ref/linkloader \
  $(LOCAL_PATH)/cpu_ref/linkloader/include \
  external/libcxx/include \
  $(LOCAL_C_INCLUDES)

include $(LLVM_ROOT_PATH)/llvm-device-build.mk
include $(BUILD_STATIC_LIBRARY)

#=============================================================================
# android librsloader for libbcc (Host)
#-----------------------------------------------------------------------------

include $(CLEAR_VARS)

LOCAL_MODULE := librsloader
ifneq ($(HOST_OS),windows)
LOCAL_CLANG := true
endif

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(rsloader_SRC_FILES)

ifdef USE_MINGW
LOCAL_SRC_FILES += cpu_ref/linkloader/lib/mmanWindows.cpp
endif

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_CFLAGS += $(rs_base_CFLAGS)
LOCAL_CFLAGS += -D__HOST__
LOCAL_CPPFLAGS += -fno-exceptions

ifeq ($(HOST_OS),windows)
LOCAL_C_INCLUDES := \
  $(LOCAL_PATH)/cpu_ref/linkloader \
  $(LOCAL_PATH)/cpu_ref/linkloader/include \
  $(LOCAL_C_INCLUDES)
else
LOCAL_C_INCLUDES := \
  $(LOCAL_PATH)/cpu_ref/linkloader \
  $(LOCAL_PATH)/cpu_ref/linkloader/include \
  external/libcxx/include \
  $(LOCAL_C_INCLUDES)
endif

include $(LLVM_ROOT_PATH)/llvm-host-build.mk
include $(BUILD_HOST_STATIC_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
