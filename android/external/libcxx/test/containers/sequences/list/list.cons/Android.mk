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
test_makefile := external/libcxx/test/containers/sequences/list/list.cons/Android.mk

test_name := containers/sequences/list/list.cons/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/size_type
test_src := size_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/move_noexcept
test_src := move_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/initializer_list_alloc
test_src := initializer_list_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/default_noexcept
test_src := default_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/assign_copy
test_src := assign_copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/move
test_src := move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/move_assign_noexcept
test_src := move_assign_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/assign_initializer_list
test_src := assign_initializer_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/copy_alloc
test_src := copy_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/initializer_list
test_src := initializer_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/default_stack_alloc
test_src := default_stack_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/op_equal_initializer_list
test_src := op_equal_initializer_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/size_value_alloc
test_src := size_value_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/dtor_noexcept
test_src := dtor_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/assign_move
test_src := assign_move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/input_iterator
test_src := input_iterator.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.cons/move_alloc
test_src := move_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))