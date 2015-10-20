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
test_makefile := external/libcxx/test/utilities/time/Android.mk

test_name := utilities/time/milliseconds
test_src := milliseconds.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/hours
test_src := hours.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/minutes
test_src := minutes.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/version
test_src := version.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/nanoseconds
test_src := nanoseconds.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/seconds
test_src := seconds.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/microseconds
test_src := microseconds.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))