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
test_makefile := external/libcxx/test/thread/thread.condition/thread.condition.condvar/Android.mk

test_name := thread/thread.condition/thread.condition.condvar/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.condition/thread.condition.condvar/notify_one
test_src := notify_one.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.condition/thread.condition.condvar/wait_until
test_src := wait_until.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.condition/thread.condition.condvar/destructor
test_src := destructor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.condition/thread.condition.condvar/wait
test_src := wait.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.condition/thread.condition.condvar/native_handle
test_src := native_handle.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.condition/thread.condition.condvar/wait_pred
test_src := wait_pred.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.condition/thread.condition.condvar/wait_until_pred
test_src := wait_until_pred.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.condition/thread.condition.condvar/notify_all
test_src := notify_all.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.condition/thread.condition.condvar/wait_for
test_src := wait_for.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.condition/thread.condition.condvar/wait_for_pred
test_src := wait_for_pred.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))