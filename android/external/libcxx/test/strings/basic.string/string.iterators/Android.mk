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
test_makefile := external/libcxx/test/strings/basic.string/string.iterators/Android.mk

test_name := strings/basic.string/string.iterators/db_iterators_7
test_src := db_iterators_7.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.iterators/cend
test_src := cend.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.iterators/begin
test_src := begin.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.iterators/db_iterators_5
test_src := db_iterators_5.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.iterators/db_iterators_3
test_src := db_iterators_3.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.iterators/crend
test_src := crend.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.iterators/cbegin
test_src := cbegin.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.iterators/crbegin
test_src := crbegin.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.iterators/rend
test_src := rend.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.iterators/iterators
test_src := iterators.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.iterators/db_iterators_2
test_src := db_iterators_2.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.iterators/rbegin
test_src := rbegin.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.iterators/db_iterators_8
test_src := db_iterators_8.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.iterators/end
test_src := end.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.iterators/db_iterators_4
test_src := db_iterators_4.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.iterators/db_iterators_6
test_src := db_iterators_6.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))