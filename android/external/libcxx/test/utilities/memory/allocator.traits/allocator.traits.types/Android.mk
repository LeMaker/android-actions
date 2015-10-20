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
test_makefile := external/libcxx/test/utilities/memory/allocator.traits/allocator.traits.types/Android.mk

test_name := utilities/memory/allocator.traits/allocator.traits.types/size_type
test_src := size_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/allocator.traits/allocator.traits.types/difference_type
test_src := difference_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/allocator.traits/allocator.traits.types/const_pointer
test_src := const_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/allocator.traits/allocator.traits.types/pointer
test_src := pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/allocator.traits/allocator.traits.types/propagate_on_container_copy_assignment
test_src := propagate_on_container_copy_assignment.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/allocator.traits/allocator.traits.types/propagate_on_container_swap
test_src := propagate_on_container_swap.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/allocator.traits/allocator.traits.types/rebind_alloc
test_src := rebind_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/allocator.traits/allocator.traits.types/void_pointer
test_src := void_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/allocator.traits/allocator.traits.types/propagate_on_container_move_assignment
test_src := propagate_on_container_move_assignment.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/allocator.traits/allocator.traits.types/const_void_pointer
test_src := const_void_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))