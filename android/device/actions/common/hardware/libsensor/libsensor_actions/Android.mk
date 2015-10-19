# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#ifeq ($(TARGET_BOARD_PLATFORM),ATM900X)

#BOARD_SENSOR_VENDOR := $(R_SENSOR_TYPE)
#$(info R_SENSOR_TYPE=$(BOARD_SENSOR_VENDOR))

#ifeq ($(BOARD_SENSOR_VENDOR), actions)

LOCAL_PATH := $(call my-dir)

define _remove_temp_actions_codec
rm -rf $(TARGET_OUT_INTERMEDIATE_LIBRARIES)/$(1).so
rm -rf $(TARGET_OUT_INTERMEDIATES)/SHARED_LIBRARIES/$(1)_intermediates
endef

$(shell $(call _remove_temp_actions_codec,sensors.$(TARGET_BOARD_PLATFORM))) 


# HAL module implemenation, not prelinked, and stored in
# hw/<SENSORS_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)

LOCAL_MODULE := sensors.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"

LOCAL_SRC_FILES := 						\
				sensors.cpp 			\
				SensorBase.cpp			\
				AccelerationSensor.cpp         \
				LightSensor.cpp         \
				CompassSensor.cpp         \
				GyroSensor.cpp         \
				TemperatureSensor.cpp         \
	           InputEventReader.cpp

LOCAL_SHARED_LIBRARIES := liblog libcutils libdl
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

#endif
#endif
