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
test_makefile := external/libcxx/test/re/re.submatch/re.submatch.members/Android.mk

test_name := re/re.submatch/re.submatch.members/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.submatch/re.submatch.members/compare_string_type
test_src := compare_string_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.submatch/re.submatch.members/length
test_src := length.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.submatch/re.submatch.members/operator_string
test_src := operator_string.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.submatch/re.submatch.members/compare_value_type_ptr
test_src := compare_value_type_ptr.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.submatch/re.submatch.members/compare_sub_match
test_src := compare_sub_match.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.submatch/re.submatch.members/str
test_src := str.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))