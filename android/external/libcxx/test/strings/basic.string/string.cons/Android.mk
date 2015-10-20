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
test_makefile := external/libcxx/test/strings/basic.string/string.cons/Android.mk

test_name := strings/basic.string/string.cons/alloc
test_src := alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/move_noexcept
test_src := move_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/default_noexcept
test_src := default_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/char_assignment
test_src := char_assignment.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/move
test_src := move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/move_assign_noexcept
test_src := move_assign_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/size_char_alloc
test_src := size_char_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/copy_alloc
test_src := copy_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/copy_assignment
test_src := copy_assignment.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/substr
test_src := substr.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/iter_alloc
test_src := iter_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/initializer_list
test_src := initializer_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/pointer_alloc
test_src := pointer_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/pointer_assignment
test_src := pointer_assignment.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/pointer_size_alloc
test_src := pointer_size_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/dtor_noexcept
test_src := dtor_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/initializer_list_assignment
test_src := initializer_list_assignment.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/move_assignment
test_src := move_assignment.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.cons/move_alloc
test_src := move_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))