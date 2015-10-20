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
test_makefile := external/libcxx/test/numerics/complex.number/complex.value.ops/Android.mk

test_name := numerics/complex.number/complex.value.ops/polar
test_src := polar.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.value.ops/conj
test_src := conj.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.value.ops/abs
test_src := abs.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.value.ops/imag
test_src := imag.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.value.ops/proj
test_src := proj.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.value.ops/real
test_src := real.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.value.ops/arg
test_src := arg.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.value.ops/norm
test_src := norm.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))