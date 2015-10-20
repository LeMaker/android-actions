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
test_makefile := external/libcxx/test/containers/sequences/forwardlist/forwardlist.modifiers/Android.mk

test_name := containers/sequences/forwardlist/forwardlist.modifiers/insert_after_range
test_src := insert_after_range.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.modifiers/insert_after_rv
test_src := insert_after_rv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.modifiers/insert_after_size_value
test_src := insert_after_size_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.modifiers/push_front_rv
test_src := push_front_rv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.modifiers/emplace_after
test_src := emplace_after.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.modifiers/erase_after_one
test_src := erase_after_one.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.modifiers/insert_after_init
test_src := insert_after_init.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.modifiers/resize_size_value
test_src := resize_size_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.modifiers/clear
test_src := clear.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.modifiers/insert_after_const
test_src := insert_after_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.modifiers/push_front_exception_safety
test_src := push_front_exception_safety.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.modifiers/erase_after_many
test_src := erase_after_many.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.modifiers/push_front_const
test_src := push_front_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.modifiers/emplace_front
test_src := emplace_front.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.modifiers/pop_front
test_src := pop_front.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/forwardlist/forwardlist.modifiers/resize_size
test_src := resize_size.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))