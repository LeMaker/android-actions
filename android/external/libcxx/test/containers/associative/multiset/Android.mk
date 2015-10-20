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
test_makefile := external/libcxx/test/containers/associative/multiset/Android.mk

test_name := containers/associative/multiset/insert_cv
test_src := insert_cv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/insert_rv
test_src := insert_rv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/find
test_src := find.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/emplace_hint
test_src := emplace_hint.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/insert_initializer_list
test_src := insert_initializer_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/equal_range
test_src := equal_range.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/lower_bound
test_src := lower_bound.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/insert_iter_iter
test_src := insert_iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/erase_key
test_src := erase_key.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/iterator
test_src := iterator.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/clear
test_src := clear.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/insert_iter_rv
test_src := insert_iter_rv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/size
test_src := size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/max_size
test_src := max_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/scary
test_src := scary.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/types
test_src := types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/emplace
test_src := emplace.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/upper_bound
test_src := upper_bound.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/erase_iter
test_src := erase_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/erase_iter_iter
test_src := erase_iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/count
test_src := count.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/empty
test_src := empty.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/associative/multiset/insert_iter_cv
test_src := insert_iter_cv.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))