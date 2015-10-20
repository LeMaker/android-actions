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
test_makefile := external/libcxx/test/numerics/complex.number/complex.transcendentals/Android.mk

test_name := numerics/complex.number/complex.transcendentals/cosh
test_src := cosh.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/exp
test_src := exp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/log
test_src := log.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/atanh
test_src := atanh.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/pow_complex_scalar
test_src := pow_complex_scalar.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/log10
test_src := log10.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/asin
test_src := asin.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/acos
test_src := acos.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/tan
test_src := tan.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/pow_complex_complex
test_src := pow_complex_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/cos
test_src := cos.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/sinh
test_src := sinh.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/sin
test_src := sin.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/pow_scalar_complex
test_src := pow_scalar_complex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/sqrt
test_src := sqrt.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/acosh
test_src := acosh.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/asinh
test_src := asinh.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/atan
test_src := atan.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.transcendentals/tanh
test_src := tanh.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))