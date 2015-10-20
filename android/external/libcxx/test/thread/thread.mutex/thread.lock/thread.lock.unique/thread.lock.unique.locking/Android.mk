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
test_makefile := external/libcxx/test/thread/thread.mutex/thread.lock/thread.lock.unique/thread.lock.unique.locking/Android.mk

test_name := thread/thread.mutex/thread.lock/thread.lock.unique/thread.lock.unique.locking/lock
test_src := lock.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.mutex/thread.lock/thread.lock.unique/thread.lock.unique.locking/unlock
test_src := unlock.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.mutex/thread.lock/thread.lock.unique/thread.lock.unique.locking/try_lock_until
test_src := try_lock_until.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.mutex/thread.lock/thread.lock.unique/thread.lock.unique.locking/try_lock_for
test_src := try_lock_for.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.mutex/thread.lock/thread.lock.unique/thread.lock.unique.locking/try_lock
test_src := try_lock.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))