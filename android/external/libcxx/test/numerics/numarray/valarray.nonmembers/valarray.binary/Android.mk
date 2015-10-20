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
test_makefile := external/libcxx/test/numerics/numarray/valarray.nonmembers/valarray.binary/Android.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/divide_value_valarray
test_src := divide_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/and_valarray_value
test_src := and_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/xor_valarray_valarray
test_src := xor_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/divide_valarray_value
test_src := divide_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/modulo_valarray_value
test_src := modulo_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/shift_right_value_valarray
test_src := shift_right_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/or_value_valarray
test_src := or_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/minus_valarray_value
test_src := minus_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/shift_left_value_valarray
test_src := shift_left_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/xor_valarray_value
test_src := xor_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/minus_value_valarray
test_src := minus_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/times_valarray_value
test_src := times_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/minus_valarray_valarray
test_src := minus_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/plus_value_valarray
test_src := plus_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/modulo_valarray_valarray
test_src := modulo_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/plus_valarray_valarray
test_src := plus_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/divide_valarray_valarray
test_src := divide_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/shift_right_valarray_valarray
test_src := shift_right_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/shift_left_valarray_valarray
test_src := shift_left_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/shift_left_valarray_value
test_src := shift_left_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/times_valarray_valarray
test_src := times_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/and_valarray_valarray
test_src := and_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/modulo_value_valarray
test_src := modulo_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/shift_right_valarray_value
test_src := shift_right_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/or_valarray_valarray
test_src := or_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/times_value_valarray
test_src := times_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/and_value_valarray
test_src := and_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/xor_value_valarray
test_src := xor_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/or_valarray_value
test_src := or_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.binary/plus_valarray_value
test_src := plus_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))