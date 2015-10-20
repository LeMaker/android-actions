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
test_makefile := external/libcxx/test/thread/thread.threads/thread.thread.class/thread.thread.member/Android.mk

test_name := thread/thread.threads/thread.thread.class/thread.thread.member/swap
test_src := swap.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.threads/thread.thread.class/thread.thread.member/native_handle
test_src := native_handle.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.threads/thread.thread.class/thread.thread.member/detach
test_src := detach.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.threads/thread.thread.class/thread.thread.member/join
test_src := join.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.threads/thread.thread.class/thread.thread.member/get_id
test_src := get_id.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/thread.threads/thread.thread.class/thread.thread.member/joinable
test_src := joinable.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))