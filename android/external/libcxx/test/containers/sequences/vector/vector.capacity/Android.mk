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
test_makefile := external/libcxx/test/containers/sequences/vector/vector.capacity/Android.mk

test_name := containers/sequences/vector/vector.capacity/shrink_to_fit
test_src := shrink_to_fit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.capacity/swap
test_src := swap.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.capacity/resize_size_value
test_src := resize_size_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.capacity/capacity
test_src := capacity.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.capacity/reserve
test_src := reserve.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.capacity/resize_size
test_src := resize_size.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))