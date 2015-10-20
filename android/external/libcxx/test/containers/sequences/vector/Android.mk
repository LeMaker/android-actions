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
test_makefile := external/libcxx/test/containers/sequences/vector/Android.mk

test_name := containers/sequences/vector/db_iterators_7
test_src := db_iterators_7.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/db_front
test_src := db_front.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/db_iterators_5
test_src := db_iterators_5.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/db_cback
test_src := db_cback.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/db_iterators_3
test_src := db_iterators_3.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/version
test_src := version.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/iterators
test_src := iterators.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/types
test_src := types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/db_iterators_2
test_src := db_iterators_2.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/db_cindex
test_src := db_cindex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/const_value_type
test_src := const_value_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/db_iterators_8
test_src := db_iterators_8.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/db_back
test_src := db_back.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/db_cfront
test_src := db_cfront.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/db_iterators_4
test_src := db_iterators_4.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/db_index
test_src := db_index.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/db_iterators_6
test_src := db_iterators_6.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))