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
test_makefile := external/libcxx/test/containers/sequences/deque/deque.special/Android.mk

test_name := containers/sequences/deque/deque.special/copy_backward
test_src := copy_backward.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.special/move_backward
test_src := move_backward.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.special/swap
test_src := swap.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.special/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.special/swap_noexcept
test_src := swap_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/deque/deque.special/move
test_src := move.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))