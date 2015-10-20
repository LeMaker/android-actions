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
test_makefile := external/libcxx/test/strings/string.conversions/Android.mk

test_name := strings/string.conversions/stod
test_src := stod.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/string.conversions/stold
test_src := stold.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/string.conversions/to_wstring
test_src := to_wstring.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/string.conversions/stoul
test_src := stoul.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/string.conversions/stol
test_src := stol.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/string.conversions/stoll
test_src := stoll.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/string.conversions/to_string
test_src := to_string.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/string.conversions/stoull
test_src := stoull.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/string.conversions/stof
test_src := stof.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/string.conversions/stoi
test_src := stoi.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))