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
test_makefile := external/libcxx/test/numerics/numarray/template.gslice.array/gslice.array.comp.assign/Android.mk

test_name := numerics/numarray/template.gslice.array/gslice.array.comp.assign/and
test_src := and.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.gslice.array/gslice.array.comp.assign/multiply
test_src := multiply.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.gslice.array/gslice.array.comp.assign/addition
test_src := addition.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.gslice.array/gslice.array.comp.assign/subtraction
test_src := subtraction.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.gslice.array/gslice.array.comp.assign/divide
test_src := divide.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.gslice.array/gslice.array.comp.assign/modulo
test_src := modulo.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.gslice.array/gslice.array.comp.assign/shift_right
test_src := shift_right.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.gslice.array/gslice.array.comp.assign/xor
test_src := xor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.gslice.array/gslice.array.comp.assign/shift_left
test_src := shift_left.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.gslice.array/gslice.array.comp.assign/or
test_src := or.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))