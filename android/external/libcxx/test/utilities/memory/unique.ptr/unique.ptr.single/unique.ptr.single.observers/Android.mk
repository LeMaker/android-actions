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
test_makefile := external/libcxx/test/utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.observers/Android.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.observers/dereference
test_src := dereference.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.observers/get
test_src := get.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.observers/explicit_bool
test_src := explicit_bool.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.observers/op_arrow
test_src := op_arrow.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.observers/get_deleter
test_src := get_deleter.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))