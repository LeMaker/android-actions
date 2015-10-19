# Copyright 2012, The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

phone_common_dir := ../PhoneCommon
src_dirs := src $(phone_common_dir)/src
res_dirs := res $(phone_common_dir)/res

LOCAL_SRC_FILES := $(call all-java-files-under, $(src_dirs))
LOCAL_RESOURCE_DIR := $(addprefix $(LOCAL_PATH)/, $(res_dirs))

LOCAL_AAPT_FLAGS := \
    --auto-add-overlay \
    --extra-packages com.android.phone.common

LOCAL_STATIC_JAVA_LIBRARIES := \
    com.android.vcard \
    guava \
    android-common \
    android-support-v13 \
    android-support-v4 \
    libphonenumber \
    libgeocoding

LOCAL_PACKAGE_NAME := com.android.contacts.common

LOCAL_PROGUARD_ENABLED := disabled
LOCAL_PROGUARD_FLAG_FILES := proguard.flags

include $(BUILD_PACKAGE)

include $(CLEAR_VARS)

# Open-source libphonenumber libraries as found in code.google.com/p/libphonenumber
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := \
   libphonenumber:libs/libphonenumber-6.2.jar \
   libgeocoding:libs/geocoder-2.9.jar

include $(BUILD_MULTI_PREBUILT)

# Use the following include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))
