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
test_makefile := external/libcxx/test/strings/basic.string/string.capacity/Android.mk

test_name := strings/basic.string/string.capacity/length
test_src := length.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.capacity/resize_size_char
test_src := resize_size_char.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.capacity/shrink_to_fit
test_src := shrink_to_fit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.capacity/clear
test_src := clear.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.capacity/size
test_src := size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.capacity/max_size
test_src := max_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.capacity/capacity
test_src := capacity.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.capacity/reserve
test_src := reserve.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.capacity/empty
test_src := empty.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.capacity/resize_size
test_src := resize_size.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))