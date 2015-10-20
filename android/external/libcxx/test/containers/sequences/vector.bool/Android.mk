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
test_makefile := external/libcxx/test/containers/sequences/vector.bool/Android.mk

test_name := containers/sequences/vector.bool/construct_default
test_src := construct_default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/vector_bool
test_src := vector_bool.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/construct_size
test_src := construct_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/move_noexcept
test_src := move_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/insert_iter_iter_iter
test_src := insert_iter_iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/find
test_src := find.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/initializer_list_alloc
test_src := initializer_list_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/construct_size_value
test_src := construct_size_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/emplace_back
test_src := emplace_back.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/shrink_to_fit
test_src := shrink_to_fit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/default_noexcept
test_src := default_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/insert_iter_initializer_list
test_src := insert_iter_initializer_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/swap
test_src := swap.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/swap_noexcept
test_src := swap_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/assign_copy
test_src := assign_copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/move
test_src := move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/insert_iter_value
test_src := insert_iter_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/construct_iter_iter
test_src := construct_iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/move_assign_noexcept
test_src := move_assign_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/assign_initializer_list
test_src := assign_initializer_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/resize_size_value
test_src := resize_size_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/copy_alloc
test_src := copy_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/capacity
test_src := capacity.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/construct_iter_iter_alloc
test_src := construct_iter_iter_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/iterators
test_src := iterators.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/construct_size_value_alloc
test_src := construct_size_value_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/types
test_src := types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/emplace
test_src := emplace.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/reserve
test_src := reserve.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/insert_iter_size_value
test_src := insert_iter_size_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/push_back
test_src := push_back.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/erase_iter
test_src := erase_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/erase_iter_iter
test_src := erase_iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/initializer_list
test_src := initializer_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/op_equal_initializer_list
test_src := op_equal_initializer_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/dtor_noexcept
test_src := dtor_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/assign_move
test_src := assign_move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/resize_size
test_src := resize_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector.bool/move_alloc
test_src := move_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))