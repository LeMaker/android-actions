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
test_makefile := external/libcxx/test/re/re.regex/re.regex.assign/Android.mk

test_name := re/re.regex/re.regex.assign/string
test_src := string.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.assign/assign_ptr_size_flag
test_src := assign_ptr_size_flag.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.assign/il
test_src := il.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.assign/assign_ptr_flag
test_src := assign_ptr_flag.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.assign/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.assign/assign
test_src := assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.assign/assign_iter_iter_flag
test_src := assign_iter_iter_flag.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.assign/ptr
test_src := ptr.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.assign/assign.il
test_src := assign.il.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.assign/assign_string_flag
test_src := assign_string_flag.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))