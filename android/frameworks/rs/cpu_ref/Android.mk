
LOCAL_PATH:=$(call my-dir)

rs_base_CFLAGS := -Werror -Wall -Wno-unused-parameter -Wno-unused-variable -fno-exceptions
ifeq ($(TARGET_BUILD_PDK), true)
  rs_base_CFLAGS += -D__RS_PDK__
endif

ifneq ($(OVERRIDE_RS_DRIVER),)
  rs_base_CFLAGS += -DOVERRIDE_RS_DRIVER=$(OVERRIDE_RS_DRIVER)
endif

include $(CLEAR_VARS)
ifneq ($(HOST_OS),windows)
LOCAL_CLANG := true
endif
LOCAL_MODULE := libRSCpuRef
LOCAL_MODULE_TARGET_ARCH := arm mips mips64 x86 x86_64 arm64

ifeq ($(HOST_OS), darwin)
LOCAL_CFLAGS += -no-integrated-as
LOCAL_ASFLAGS += -no-integrated-as
endif

LOCAL_SRC_FILES:= \
	rsCpuCore.cpp \
	rsCpuScript.cpp \
	rsCpuRuntimeMath.cpp \
	rsCpuRuntimeStubs.cpp \
	rsCpuScriptGroup.cpp \
	rsCpuIntrinsic.cpp \
	rsCpuIntrinsic3DLUT.cpp \
	rsCpuIntrinsicBlend.cpp \
	rsCpuIntrinsicBlur.cpp \
	rsCpuIntrinsicColorMatrix.cpp \
	rsCpuIntrinsicConvolve3x3.cpp \
	rsCpuIntrinsicConvolve5x5.cpp \
	rsCpuIntrinsicHistogram.cpp \
	rsCpuIntrinsicResize.cpp \
	rsCpuIntrinsicLUT.cpp \
	rsCpuIntrinsicYuvToRGB.cpp

LOCAL_CFLAGS_arm64 += -DARCH_ARM_USE_INTRINSICS -DARCH_ARM64_USE_INTRINSICS -DARCH_ARM64_HAVE_NEON

ifeq ($(RS_DISABLE_A53_WORKAROUND),true)
LOCAL_CFLAGS_arm64 += -DDISABLE_A53_WORKAROUND
endif

LOCAL_SRC_FILES_arm64 += \
    rsCpuIntrinsics_advsimd_3DLUT.S \
    rsCpuIntrinsics_advsimd_Convolve.S \
    rsCpuIntrinsics_advsimd_Blur.S \
    rsCpuIntrinsics_advsimd_ColorMatrix.S \
    rsCpuIntrinsics_advsimd_YuvToRGB.S
#    rsCpuIntrinsics_advsimd_Blend.S \

ifeq ($(ARCH_ARM_HAVE_NEON),true)
    LOCAL_CFLAGS_arm += -DARCH_ARM_HAVE_NEON
endif

ifeq ($(ARCH_ARM_HAVE_VFP),true)
    LOCAL_CFLAGS_arm += -DARCH_ARM_HAVE_VFP -DARCH_ARM_USE_INTRINSICS
    LOCAL_SRC_FILES_arm += \
    rsCpuIntrinsics_neon_3DLUT.S \
    rsCpuIntrinsics_neon_Blend.S \
    rsCpuIntrinsics_neon_Blur.S \
    rsCpuIntrinsics_neon_Convolve.S \
    rsCpuIntrinsics_neon_ColorMatrix.S \
    rsCpuIntrinsics_neon_YuvToRGB.S \

    LOCAL_ASFLAGS_arm := -mfpu=neon
endif

ifeq ($(ARCH_X86_HAVE_SSSE3),true)
    LOCAL_CFLAGS += -DARCH_X86_HAVE_SSSE3
    LOCAL_SRC_FILES+= \
    rsCpuIntrinsics_x86.c
endif

LOCAL_SHARED_LIBRARIES += libRS libcutils libutils liblog libsync libc++

# these are not supported in 64-bit yet
LOCAL_SHARED_LIBRARIES += libbcc libbcinfo


LOCAL_C_INCLUDES += frameworks/compile/libbcc/include
LOCAL_C_INCLUDES += frameworks/rs

ifneq ($(HOST_OS),windows)
include external/libcxx/libcxx.mk
endif
include frameworks/compile/libbcc/libbcc-targets.mk

LOCAL_CFLAGS += $(rs_base_CFLAGS)

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
