################################################################################
DDK_ROOT=$(shell pwd)
PROJECT_ROOT=$(DDK_ROOT)/../../../../../..

# Macros.
export TARGET_PRODUCT=gs705a
include $(PROJECT_ROOT)/build/$(TARGET_PRODUCT)/config/build_config.cfg
export ANDROID_ROOT=$(ANDROID_PATH)
export KERNELDIR=$(KERNEL_BUILD_PATH) 
export ARCH=$(R_ARCH)
export CROSS_COMPILE=$(ANDROID_ROOT)/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.7/bin/arm-linux-androideabi-
export KERNEL_CROSS_COMPILE=$(R_CROSS_COMPILE)
export PATH:="$(PATH):$(ANDROID_ROOT)/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.7/bin:$(ANDROID_ROOT)/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin"

#release debug timing
BUILD = release
PDUMP = 0
PVR_ANDROID_RS     = 1
PVR_ANDROID_OPENCL = 1
SUPPORT_SGX_HWPERF = 1
SUPPORT_HW_RECOVERY = 1
SUPPORT_ACTIVE_POWER_MANAGEMENT = 1
SUPPORT_PVRSRV_ANDROID_SYSTRACE = 0

################################################################################
# Directories setting.
################################################################################
km_target_dir = $(DDK_ROOT)/eurasia_km/eurasiacon/binary2_owl_android_$(BUILD)/target
prebuilt_dir  = $(ANDROID_ROOT)/device/actions/prebuilt/gpu

################################################################################
BUILD_FLAGS += BUILD=$(BUILD)
BUILD_FLAGS += PDUMP=$(PDUMP)
BUILD_FLAGS += PVR_ANDROID_RS=$(PVR_ANDROID_RS)
BUILD_FLAGS += PVR_ANDROID_OPENCL=$(PVR_ANDROID_OPENCL)
BUILD_FLAGS += SUPPORT_SGX_HWPERF=$(SUPPORT_SGX_HWPERF)
BUILD_FLAGS += SUPPORT_HW_RECOVERY=$(SUPPORT_HW_RECOVERY)
BUILD_FLAGS += SUPPORT_ACTIVE_POWER_MANAGEMENT=$(SUPPORT_ACTIVE_POWER_MANAGEMENT)
BUILD_FLAGS += SUPPORT_PVRSRV_ANDROID_SYSTRACE=$(SUPPORT_PVRSRV_ANDROID_SYSTRACE)

# Do not compile camerahal and unittests by default.
BUILD_FLAGS += EXCLUDED_APIS="camerahal unittests"

all:
	@echo "-- build km gpu driver --"
	make -j20 $(BUILD_FLAGS) -C $(DDK_ROOT)/eurasia_km/eurasiacon/build/linux2/owl_android
	@echo "-- install km gpu driver --"
	cp $(km_target_dir)/pvrsrvkm.ko                        $(PREBUILT_MODULES_DIR)/pvrsrvkm.ko
	cp $(km_target_dir)/dc_owl.ko                          $(PREBUILT_MODULES_DIR)/dc_owl.ko
	@echo "-- install completed --"
clean:
	@echo "-- clean km gpu driver --"
	make clobber BUILD=$(BUILD) -C $(DDK_ROOT)/eurasia_km/eurasiacon/build/linux2/owl_android
	@echo "-- clean completed --"

.PHONY: all     \
		clean   
