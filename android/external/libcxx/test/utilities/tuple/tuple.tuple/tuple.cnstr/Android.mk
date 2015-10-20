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
test_makefile := external/libcxx/test/utilities/tuple/tuple.tuple/tuple.cnstr/Android.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/convert_copy
test_src := convert_copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/alloc
test_src := alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/alloc_UTypes
test_src := alloc_UTypes.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/alloc_const_Types
test_src := alloc_const_Types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/alloc_move_pair
test_src := alloc_move_pair.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/alloc_move
test_src := alloc_move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/UTypes
test_src := UTypes.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/convert_move
test_src := convert_move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/move
test_src := move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/const_pair
test_src := const_pair.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/alloc_convert_copy
test_src := alloc_convert_copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/alloc_copy
test_src := alloc_copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/const_Types
test_src := const_Types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/alloc_convert_move
test_src := alloc_convert_move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/move_pair
test_src := move_pair.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/tuple/tuple.tuple/tuple.cnstr/alloc_const_pair
test_src := alloc_const_pair.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))