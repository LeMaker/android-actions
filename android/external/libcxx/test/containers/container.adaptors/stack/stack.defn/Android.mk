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
test_makefile := external/libcxx/test/containers/container.adaptors/stack/stack.defn/Android.mk

test_name := containers/container.adaptors/stack/stack.defn/push
test_src := push.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.defn/top_const
test_src := top_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.defn/top
test_src := top.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.defn/push_rv
test_src := push_rv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.defn/swap
test_src := swap.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.defn/pop
test_src := pop.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.defn/assign_copy
test_src := assign_copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.defn/size
test_src := size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.defn/types
test_src := types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.defn/emplace
test_src := emplace.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.defn/empty
test_src := empty.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.defn/assign_move
test_src := assign_move.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))