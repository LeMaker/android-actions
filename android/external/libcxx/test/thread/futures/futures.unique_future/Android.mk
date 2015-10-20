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
test_makefile := external/libcxx/test/thread/futures/futures.unique_future/Android.mk

test_name := thread/futures/futures.unique_future/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.unique_future/move_ctor
test_src := move_ctor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.unique_future/dtor
test_src := dtor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.unique_future/wait_until
test_src := wait_until.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.unique_future/share
test_src := share.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.unique_future/get
test_src := get.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.unique_future/wait
test_src := wait.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.unique_future/move_assign
test_src := move_assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.unique_future/wait_for
test_src := wait_for.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))