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
test_makefile := external/libcxx/test/numerics/complex.number/complex.ops/Android.mk

test_name := numerics/complex.number/complex.ops/scalar_divide_complex
test_src := scalar_divide_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/complex_plus_complex
test_src := complex_plus_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/complex_times_scalar
test_src := complex_times_scalar.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/complex_not_equals_scalar
test_src := complex_not_equals_scalar.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/complex_equals_scalar
test_src := complex_equals_scalar.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/unary_plus
test_src := unary_plus.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/stream_input
test_src := stream_input.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/complex_divide_scalar
test_src := complex_divide_scalar.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/scalar_plus_complex
test_src := scalar_plus_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/scalar_times_complex
test_src := scalar_times_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/complex_divide_complex
test_src := complex_divide_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/complex_not_equals_complex
test_src := complex_not_equals_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/complex_minus_scalar
test_src := complex_minus_scalar.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/stream_output
test_src := stream_output.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/complex_minus_complex
test_src := complex_minus_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/complex_equals_complex
test_src := complex_equals_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/unary_minus
test_src := unary_minus.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/complex_times_complex
test_src := complex_times_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/scalar_not_equals_complex
test_src := scalar_not_equals_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/complex_plus_scalar
test_src := complex_plus_scalar.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/scalar_minus_complex
test_src := scalar_minus_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.ops/scalar_equals_complex
test_src := scalar_equals_complex.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))