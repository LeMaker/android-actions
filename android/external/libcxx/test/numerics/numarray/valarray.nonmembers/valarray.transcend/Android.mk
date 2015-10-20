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
test_makefile := external/libcxx/test/numerics/numarray/valarray.nonmembers/valarray.transcend/Android.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/sinh_valarray
test_src := sinh_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/log10_valarray
test_src := log10_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/abs_valarray
test_src := abs_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/pow_valarray_valarray
test_src := pow_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/cosh_valarray
test_src := cosh_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/sqrt_valarray
test_src := sqrt_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/pow_value_valarray
test_src := pow_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/log_valarray
test_src := log_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/cos_valarray
test_src := cos_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/asin_valarray
test_src := asin_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/pow_valarray_value
test_src := pow_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/acos_valarray
test_src := acos_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/atan2_valarray_valarray
test_src := atan2_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/tan_valarray
test_src := tan_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/atan2_valarray_value
test_src := atan2_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/atan2_value_valarray
test_src := atan2_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/sin_valarray
test_src := sin_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/atan_valarray
test_src := atan_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/exp_valarray
test_src := exp_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.transcend/tanh_valarray
test_src := tanh_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))