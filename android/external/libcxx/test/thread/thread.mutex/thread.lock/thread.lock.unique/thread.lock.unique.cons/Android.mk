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
test_makefile := external/libcxx/test/thread/thread.mutex/thread.lock/thread.lock.unique/thread.lock.unique.cons/Android.mk

test_name := thread/thread.mutex/thread.lock/thread.lock.unique/thread.lock.unique.cons/mutex_adopt_lock
test_src := mutex_adopt_lock.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.mutex/thread.lock/thread.lock.unique/thread.lock.unique.cons/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.mutex/thread.lock/thread.lock.unique/thread.lock.unique.cons/move_ctor
test_src := move_ctor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.mutex/thread.lock/thread.lock.unique/thread.lock.unique.cons/mutex_try_to_lock
test_src := mutex_try_to_lock.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.mutex/thread.lock/thread.lock.unique/thread.lock.unique.cons/mutex_duration
test_src := mutex_duration.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.mutex/thread.lock/thread.lock.unique/thread.lock.unique.cons/mutex_time_point
test_src := mutex_time_point.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.mutex/thread.lock/thread.lock.unique/thread.lock.unique.cons/move_assign
test_src := move_assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.mutex/thread.lock/thread.lock.unique/thread.lock.unique.cons/mutex
test_src := mutex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.mutex/thread.lock/thread.lock.unique/thread.lock.unique.cons/mutex_defer_lock
test_src := mutex_defer_lock.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))