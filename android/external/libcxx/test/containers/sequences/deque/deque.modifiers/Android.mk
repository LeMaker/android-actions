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
test_makefile := external/libcxx/test/containers/sequences/deque/deque.modifiers/Android.mk

test_name := containers/sequences/deque/deque.modifiers/push_front_rvalue
test_src := push_front_rvalue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/push_back_exception_safety
test_src := push_back_exception_safety.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/pop_back
test_src := pop_back.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/emplace_back
test_src := emplace_back.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/insert_iter_iter
test_src := insert_iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/insert_iter_initializer_list
test_src := insert_iter_initializer_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/insert_size_value
test_src := insert_size_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/push_front_exception_safety
test_src := push_front_exception_safety.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/emplace
test_src := emplace.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/insert_value
test_src := insert_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/insert_rvalue
test_src := insert_rvalue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/push_back
test_src := push_back.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/erase_iter
test_src := erase_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/erase_iter_iter
test_src := erase_iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/push_back_rvalue
test_src := push_back_rvalue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/emplace_front
test_src := emplace_front.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/pop_front
test_src := pop_front.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.modifiers/push_front
test_src := push_front.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))