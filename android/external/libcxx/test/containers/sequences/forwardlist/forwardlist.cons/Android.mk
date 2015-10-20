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
test_makefile := external/libcxx/test/containers/sequences/forwardlist/forwardlist.cons/Android.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/assign_size_value
test_src := assign_size_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/alloc
test_src := alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/move_noexcept
test_src := move_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/default_noexcept
test_src := default_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/assign_op_init
test_src := assign_op_init.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/size_value
test_src := size_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/init
test_src := init.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/assign_copy
test_src := assign_copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/assign_range
test_src := assign_range.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/move
test_src := move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/move_assign_noexcept
test_src := move_assign_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/copy_alloc
test_src := copy_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/size
test_src := size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/range_alloc
test_src := range_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/default_recursive
test_src := default_recursive.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/range
test_src := range.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/size_value_alloc
test_src := size_value_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/dtor_noexcept
test_src := dtor_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/assign_move
test_src := assign_move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/assign_init
test_src := assign_init.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/init_alloc
test_src := init_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.cons/move_alloc
test_src := move_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))