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
test_makefile := external/libcxx/test/numerics/complex.number/complex.special/Android.mk

test_name := numerics/complex.number/complex.special/long_double_float_implicit
test_src := long_double_float_implicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.special/long_double_double_implicit
test_src := long_double_double_implicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.special/float_double_explicit
test_src := float_double_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.special/long_double_double_explicit
test_src := long_double_double_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.special/double_long_double_explicit
test_src := double_long_double_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.special/double_float_implicit
test_src := double_float_implicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.special/float_long_double_explicit
test_src := float_long_double_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.special/double_float_explicit
test_src := double_float_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/complex.number/complex.special/long_double_float_explicit
test_src := long_double_float_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))