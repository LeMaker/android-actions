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
test_makefile := external/libcxx/test/strings/basic.string/string.access/Android.mk

test_name := strings/basic.string/string.access/db_front
test_src := db_front.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.access/at
test_src := at.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.access/db_cback
test_src := db_cback.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.access/front
test_src := front.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.access/back
test_src := back.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.access/db_cindex
test_src := db_cindex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.access/index
test_src := index.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.access/db_back
test_src := db_back.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.access/db_cfront
test_src := db_cfront.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.access/db_index
test_src := db_index.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))