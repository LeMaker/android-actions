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
test_makefile := external/libcxx/test/utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/Android.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/move_convert04
test_src := move_convert04.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/pointer01
test_src := pointer01.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/move_convert07
test_src := move_convert07.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/pointer_deleter03
test_src := pointer_deleter03.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/pointer03
test_src := pointer03.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/pointer_deleter04
test_src := pointer_deleter04.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/move02
test_src := move02.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/pointer02
test_src := pointer02.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/move_convert05
test_src := move_convert05.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/move01
test_src := move01.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/move_convert01
test_src := move_convert01.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/move_convert06
test_src := move_convert06.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/pointer_deleter02
test_src := pointer_deleter02.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/default01
test_src := default01.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/pointer_deleter05
test_src := pointer_deleter05.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/default02
test_src := default02.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/auto_pointer
test_src := auto_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/pointer_deleter06
test_src := pointer_deleter06.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/move_convert03
test_src := move_convert03.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/nullptr
test_src := nullptr.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/move_convert02
test_src := move_convert02.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/unique.ptr/unique.ptr.single/unique.ptr.single.ctor/pointer_deleter01
test_src := pointer_deleter01.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))