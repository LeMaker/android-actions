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
test_makefile := external/libcxx/test/utilities/memory/pointer.traits/Android.mk

test_name := utilities/memory/pointer.traits/rebind
test_src := rebind.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/pointer.traits/difference_type
test_src := difference_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/pointer.traits/pointer
test_src := pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/pointer.traits/element_type
test_src := element_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/pointer.traits/pointer_to
test_src := pointer_to.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))