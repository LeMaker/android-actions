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
test_makefile := external/libcxx/test/utilities/allocator.adaptor/allocator.adaptor.types/Android.mk

test_name := utilities/allocator.adaptor/allocator.adaptor.types/inner_allocator_type
test_src := inner_allocator_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/allocator.adaptor/allocator.adaptor.types/propagate_on_container_copy_assignment
test_src := propagate_on_container_copy_assignment.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/allocator.adaptor/allocator.adaptor.types/propagate_on_container_swap
test_src := propagate_on_container_swap.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/allocator.adaptor/allocator.adaptor.types/allocator_pointers
test_src := allocator_pointers.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/allocator.adaptor/allocator.adaptor.types/propagate_on_container_move_assignment
test_src := propagate_on_container_move_assignment.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))