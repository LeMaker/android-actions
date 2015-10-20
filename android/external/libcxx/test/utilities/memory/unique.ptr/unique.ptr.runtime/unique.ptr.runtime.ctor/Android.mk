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
test_makefile := external/libcxx/test/utilities/memory/unique.ptr/unique.ptr.runtime/unique.ptr.runtime.ctor/Android.mk

test_name := utilities/memory/unique.ptr/unique.ptr.runtime/unique.ptr.runtime.ctor/pointer01
test_src := pointer01.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.runtime/unique.ptr.runtime.ctor/pointer_deleter03
test_src := pointer_deleter03.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.runtime/unique.ptr.runtime.ctor/pointer_deleter04
test_src := pointer_deleter04.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.runtime/unique.ptr.runtime.ctor/move02
test_src := move02.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.runtime/unique.ptr.runtime.ctor/pointer02
test_src := pointer02.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.runtime/unique.ptr.runtime.ctor/move01
test_src := move01.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.runtime/unique.ptr.runtime.ctor/pointer_deleter02
test_src := pointer_deleter02.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.runtime/unique.ptr.runtime.ctor/default01
test_src := default01.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.runtime/unique.ptr.runtime.ctor/default02
test_src := default02.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.runtime/unique.ptr.runtime.ctor/nullptr
test_src := nullptr.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.runtime/unique.ptr.runtime.ctor/pointer_deleter01
test_src := pointer_deleter01.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))