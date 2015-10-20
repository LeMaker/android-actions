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
test_makefile := external/libcxx/test/containers/sequences/vector/vector.modifiers/Android.mk

test_name := containers/sequences/vector/vector.modifiers/erase_iter_iter_db4
test_src := erase_iter_iter_db4.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/push_back_exception_safety
test_src := push_back_exception_safety.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/insert_iter_iter_iter
test_src := insert_iter_iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/pop_back
test_src := pop_back.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/erase_iter_iter_db1
test_src := erase_iter_iter_db1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/emplace_back
test_src := emplace_back.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/insert_iter_initializer_list
test_src := insert_iter_initializer_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/erase_iter_iter_db3
test_src := erase_iter_iter_db3.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/erase_iter_db2
test_src := erase_iter_db2.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/insert_iter_value
test_src := insert_iter_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/emplace
test_src := emplace.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/emplace_extra
test_src := emplace_extra.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/insert_iter_size_value
test_src := insert_iter_size_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/insert_iter_rvalue
test_src := insert_iter_rvalue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/push_back
test_src := push_back.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/erase_iter
test_src := erase_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/erase_iter_db1
test_src := erase_iter_db1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/erase_iter_iter
test_src := erase_iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/push_back_rvalue
test_src := push_back_rvalue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/vector/vector.modifiers/erase_iter_iter_db2
test_src := erase_iter_iter_db2.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))