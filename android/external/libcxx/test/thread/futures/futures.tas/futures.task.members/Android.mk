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
test_makefile := external/libcxx/test/thread/futures/futures.tas/futures.task.members/Android.mk

test_name := thread/futures/futures.tas/futures.task.members/operator
test_src := operator.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.tas/futures.task.members/get_future
test_src := get_future.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.tas/futures.task.members/dtor
test_src := dtor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.tas/futures.task.members/ctor_func_alloc
test_src := ctor_func_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.tas/futures.task.members/swap
test_src := swap.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.tas/futures.task.members/make_ready_at_thread_exit
test_src := make_ready_at_thread_exit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.tas/futures.task.members/ctor_func
test_src := ctor_func.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.tas/futures.task.members/reset
test_src := reset.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.tas/futures.task.members/ctor_move
test_src := ctor_move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.tas/futures.task.members/assign_move
test_src := assign_move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := thread/futures/futures.tas/futures.task.members/ctor_default
test_src := ctor_default.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))