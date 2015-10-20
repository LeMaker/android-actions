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
test_makefile := external/libcxx/test/atomics/atomics.flag/Android.mk

test_name := atomics/atomics.flag/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.flag/atomic_flag_clear
test_src := atomic_flag_clear.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.flag/atomic_flag_test_and_set_explicit
test_src := atomic_flag_test_and_set_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.flag/atomic_flag_test_and_set
test_src := atomic_flag_test_and_set.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.flag/init
test_src := init.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.flag/clear
test_src := clear.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.flag/test_and_set
test_src := test_and_set.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.flag/atomic_flag_clear_explicit
test_src := atomic_flag_clear_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))