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
test_makefile := external/libcxx/test/numerics/complex.number/complex.member.ops/Android.mk

test_name := numerics/complex.number/complex.member.ops/divide_equal_complex
test_src := divide_equal_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.member.ops/minus_equal_scalar
test_src := minus_equal_scalar.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.member.ops/minus_equal_complex
test_src := minus_equal_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.member.ops/plus_equal_scalar
test_src := plus_equal_scalar.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.member.ops/times_equal_complex
test_src := times_equal_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.member.ops/times_equal_scalar
test_src := times_equal_scalar.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.member.ops/assignment_complex
test_src := assignment_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.member.ops/assignment_scalar
test_src := assignment_scalar.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.member.ops/divide_equal_scalar
test_src := divide_equal_scalar.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.member.ops/plus_equal_complex
test_src := plus_equal_complex.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))