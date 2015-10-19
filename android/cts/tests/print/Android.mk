# Copyright (C) 2014 The Android Open Source Project
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

LOCAL_PATH:= $(call my-dir)

##################################################
# Build the print instrument library
##################################################
include $(CLEAR_VARS)
LOCAL_MODULE := CtsPrintInstrument
LOCAL_SRC_FILES := $(call all-subdir-java-files) \
    src/android/print/cts/IPrivilegedOperations.aidl
LOCAL_MODULE_TAGS := optional
LOCAL_DEX_PREOPT := false

include $(BUILD_JAVA_LIBRARY)

# Copy the shell script to run the print instrument Jar to the CTS out folder.
$(CTS_TESTCASES_OUT)/$(LOCAL_MODULE).jar : $(LOCAL_BUILT_MODULE) | $(ACP) 
	$(copy-file-to-target)

# Copy the built print instrument library Jar to the CTS out folder.
$(CTS_TESTCASES_OUT)/print-instrument : $(LOCAL_PATH)/print-instrument | $(ACP)
	$(copy-file-to-target)

