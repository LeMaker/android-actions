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
test_makefile := external/libcxx/test/thread/futures/futures.promise/Android.mk

test_name := thread/futures/futures.promise/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/set_rvalue_at_thread_exit
test_src := set_rvalue_at_thread_exit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/move_ctor
test_src := move_ctor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/set_rvalue
test_src := set_rvalue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/set_lvalue_at_thread_exit
test_src := set_lvalue_at_thread_exit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/get_future
test_src := get_future.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/dtor
test_src := dtor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/set_lvalue
test_src := set_lvalue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/swap
test_src := swap.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/set_value_const
test_src := set_value_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/set_value_at_thread_exit_void
test_src := set_value_at_thread_exit_void.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/uses_allocator
test_src := uses_allocator.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/set_value_at_thread_exit_const
test_src := set_value_at_thread_exit_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/set_exception
test_src := set_exception.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/move_assign
test_src := move_assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/alloc_ctor
test_src := alloc_ctor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/set_value_void
test_src := set_value_void.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.promise/set_exception_at_thread_exit
test_src := set_exception_at_thread_exit.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))