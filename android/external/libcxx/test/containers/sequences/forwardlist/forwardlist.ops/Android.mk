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
test_makefile := external/libcxx/test/containers/sequences/forwardlist/forwardlist.ops/Android.mk

test_name := containers/sequences/forwardlist/forwardlist.ops/sort
test_src := sort.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.ops/merge_pred
test_src := merge_pred.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.ops/merge
test_src := merge.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.ops/unique_pred
test_src := unique_pred.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.ops/splice_after_flist
test_src := splice_after_flist.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.ops/sort_pred
test_src := sort_pred.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.ops/splice_after_one
test_src := splice_after_one.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.ops/reverse
test_src := reverse.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.ops/unique
test_src := unique.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.ops/remove
test_src := remove.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.ops/remove_if
test_src := remove_if.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.ops/splice_after_range
test_src := splice_after_range.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))