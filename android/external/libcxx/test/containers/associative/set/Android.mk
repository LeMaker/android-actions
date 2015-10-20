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
test_makefile := external/libcxx/test/containers/associative/set/Android.mk

test_name := containers/associative/set/insert_cv
test_src := insert_cv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/insert_rv
test_src := insert_rv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/find
test_src := find.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/emplace_hint
test_src := emplace_hint.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/insert_initializer_list
test_src := insert_initializer_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/equal_range
test_src := equal_range.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/lower_bound
test_src := lower_bound.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/insert_iter_iter
test_src := insert_iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/erase_key
test_src := erase_key.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/version
test_src := version.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/iterator
test_src := iterator.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/clear
test_src := clear.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/insert_iter_rv
test_src := insert_iter_rv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/size
test_src := size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/max_size
test_src := max_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/types
test_src := types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/emplace
test_src := emplace.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/upper_bound
test_src := upper_bound.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/erase_iter
test_src := erase_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/erase_iter_iter
test_src := erase_iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/count
test_src := count.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/empty
test_src := empty.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/set/insert_iter_cv
test_src := insert_iter_cv.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))