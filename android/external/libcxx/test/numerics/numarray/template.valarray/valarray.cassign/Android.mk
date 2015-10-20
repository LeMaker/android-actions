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
test_makefile := external/libcxx/test/numerics/numarray/template.valarray/valarray.cassign/Android.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/shift_right_valarray
test_src := shift_right_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/times_valarray
test_src := times_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/modulo_value
test_src := modulo_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/minus_value
test_src := minus_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/times_value
test_src := times_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/xor_valarray
test_src := xor_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/or_value
test_src := or_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/xor_value
test_src := xor_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/minus_valarray
test_src := minus_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/and_value
test_src := and_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/or_valarray
test_src := or_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/shift_left_valarray
test_src := shift_left_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/plus_valarray
test_src := plus_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/modulo_valarray
test_src := modulo_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/divide_valarray
test_src := divide_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/and_valarray
test_src := and_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/divide_value
test_src := divide_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/plus_value
test_src := plus_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/shift_right_value
test_src := shift_right_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cassign/shift_left_value
test_src := shift_left_value.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))