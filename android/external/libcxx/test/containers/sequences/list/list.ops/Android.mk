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
test_makefile := external/libcxx/test/containers/sequences/list/list.ops/Android.mk

test_name := containers/sequences/list/list.ops/sort
test_src := sort.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.ops/sort_comp
test_src := sort_comp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.ops/merge
test_src := merge.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.ops/unique_pred
test_src := unique_pred.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.ops/splice_pos_list_iter
test_src := splice_pos_list_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.ops/splice_pos_list
test_src := splice_pos_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.ops/merge_comp
test_src := merge_comp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.ops/splice_pos_list_iter_iter
test_src := splice_pos_list_iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.ops/reverse
test_src := reverse.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.ops/unique
test_src := unique.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.ops/remove
test_src := remove.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/list/list.ops/remove_if
test_src := remove_if.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))