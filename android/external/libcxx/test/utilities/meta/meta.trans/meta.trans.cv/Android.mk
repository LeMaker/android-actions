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
test_makefile := external/libcxx/test/utilities/meta/meta.trans/meta.trans.cv/Android.mk

test_name := utilities/meta/meta.trans/meta.trans.cv/remove_const
test_src := remove_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.trans/meta.trans.cv/remove_volatile
test_src := remove_volatile.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.trans/meta.trans.cv/add_cv
test_src := add_cv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.trans/meta.trans.cv/remove_cv
test_src := remove_cv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.trans/meta.trans.cv/add_const
test_src := add_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.trans/meta.trans.cv/add_volatile
test_src := add_volatile.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))