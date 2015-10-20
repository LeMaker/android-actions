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
test_makefile := external/libcxx/test/iterators/predef.iterators/move.iterators/move.iter.ops/move.iter.op.comp/Android.mk

test_name := iterators/predef.iterators/move.iterators/move.iter.ops/move.iter.op.comp/op_lte
test_src := op_lte.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := iterators/predef.iterators/move.iterators/move.iter.ops/move.iter.op.comp/op_eq
test_src := op_eq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := iterators/predef.iterators/move.iterators/move.iter.ops/move.iter.op.comp/op_neq
test_src := op_neq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := iterators/predef.iterators/move.iterators/move.iter.ops/move.iter.op.comp/op_gt
test_src := op_gt.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := iterators/predef.iterators/move.iterators/move.iter.ops/move.iter.op.comp/op_lt
test_src := op_lt.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := iterators/predef.iterators/move.iterators/move.iter.ops/move.iter.op.comp/op_gte
test_src := op_gte.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))