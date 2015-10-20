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
test_makefile := external/libcxx/test/containers/associative/multimap/multimap.cons/Android.mk

test_name := containers/associative/multimap/multimap.cons/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/compare
test_src := compare.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/alloc
test_src := alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/iter_iter
test_src := iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/move_noexcept
test_src := move_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/default_noexcept
test_src := default_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/iter_iter_comp
test_src := iter_iter_comp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/move
test_src := move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/move_assign_noexcept
test_src := move_assign_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/assign_initializer_list
test_src := assign_initializer_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/copy_alloc
test_src := copy_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/move_assign
test_src := move_assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/initializer_list_compare
test_src := initializer_list_compare.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/iter_iter_comp_alloc
test_src := iter_iter_comp_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/copy_assign
test_src := copy_assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/compare_alloc
test_src := compare_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/initializer_list
test_src := initializer_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/initializer_list_compare_alloc
test_src := initializer_list_compare_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/dtor_noexcept
test_src := dtor_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multimap/multimap.cons/move_alloc
test_src := move_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))