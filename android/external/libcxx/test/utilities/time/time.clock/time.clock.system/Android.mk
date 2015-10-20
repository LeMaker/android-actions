#
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
#
LOCAL_PATH := $(call my-dir)
test_makefile := external/libcxx/test/utilities/time/time.clock/time.clock.system/Android.mk

test_name := utilities/time/time.clock/time.clock.system/now
test_src := now.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/time.clock/time.clock.system/consistency
test_src := consistency.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/time.clock/time.clock.system/rep_signed
test_src := rep_signed.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/time.clock/time.clock.system/to_time_t
test_src := to_time_t.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/time.clock/time.clock.system/from_time_t
test_src := from_time_t.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))