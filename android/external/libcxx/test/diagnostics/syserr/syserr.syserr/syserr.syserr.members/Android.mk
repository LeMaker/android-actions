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
test_makefile := external/libcxx/test/diagnostics/syserr/syserr.syserr/syserr.syserr.members/Android.mk

test_name := diagnostics/syserr/syserr.syserr/syserr.syserr.members/ctor_error_code_string
test_src := ctor_error_code_string.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := diagnostics/syserr/syserr.syserr/syserr.syserr.members/ctor_error_code_const_char_pointer
test_src := ctor_error_code_const_char_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := diagnostics/syserr/syserr.syserr/syserr.syserr.members/ctor_int_error_category_const_char_pointer
test_src := ctor_int_error_category_const_char_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := diagnostics/syserr/syserr.syserr/syserr.syserr.members/ctor_error_code
test_src := ctor_error_code.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := diagnostics/syserr/syserr.syserr/syserr.syserr.members/ctor_int_error_category_string
test_src := ctor_int_error_category_string.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := diagnostics/syserr/syserr.syserr/syserr.syserr.members/ctor_int_error_category
test_src := ctor_int_error_category.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))