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
test_makefile := external/libcxx/test/thread/thread.threads/thread.thread.class/thread.thread.id/Android.mk

test_name := thread/thread.threads/thread.thread.class/thread.thread.id/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.threads/thread.thread.class/thread.thread.id/thread_id
test_src := thread_id.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.threads/thread.thread.class/thread.thread.id/lt
test_src := lt.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.threads/thread.thread.class/thread.thread.id/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.threads/thread.thread.class/thread.thread.id/assign
test_src := assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.threads/thread.thread.class/thread.thread.id/stream
test_src := stream.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.threads/thread.thread.class/thread.thread.id/eq
test_src := eq.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))