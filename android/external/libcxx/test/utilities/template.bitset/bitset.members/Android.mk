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
test_makefile := external/libcxx/test/utilities/template.bitset/bitset.members/Android.mk

test_name := utilities/template.bitset/bitset.members/right_shift
test_src := right_shift.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/flip_all
test_src := flip_all.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/op_and_eq
test_src := op_and_eq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/op_or_eq
test_src := op_or_eq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/to_ullong
test_src := to_ullong.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/index_const
test_src := index_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/set_one
test_src := set_one.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/right_shift_eq
test_src := right_shift_eq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/to_ulong
test_src := to_ulong.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/left_shift
test_src := left_shift.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/any
test_src := any.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/op_eq_eq
test_src := op_eq_eq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/test
test_src := test.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/size
test_src := size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/reset_all
test_src := reset_all.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/set_all
test_src := set_all.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/flip_one
test_src := flip_one.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/all
test_src := all.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/none
test_src := none.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/to_string
test_src := to_string.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/reset_one
test_src := reset_one.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/index
test_src := index.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/left_shift_eq
test_src := left_shift_eq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/count
test_src := count.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/op_xor_eq
test_src := op_xor_eq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/template.bitset/bitset.members/not_all
test_src := not_all.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))