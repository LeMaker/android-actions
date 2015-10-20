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
test_makefile := external/libcxx/test/numerics/numarray/valarray.nonmembers/valarray.comparison/Android.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/greater_equal_valarray_valarray
test_src := greater_equal_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/and_valarray_value
test_src := and_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/equal_value_valarray
test_src := equal_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/not_equal_value_valarray
test_src := not_equal_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/less_valarray_valarray
test_src := less_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/or_value_valarray
test_src := or_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/less_equal_valarray_valarray
test_src := less_equal_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/not_equal_valarray_valarray
test_src := not_equal_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/equal_valarray_value
test_src := equal_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/less_value_valarray
test_src := less_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/less_equal_value_valarray
test_src := less_equal_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/greater_equal_valarray_value
test_src := greater_equal_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/greater_valarray_valarray
test_src := greater_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/and_valarray_valarray
test_src := and_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/not_equal_valarray_value
test_src := not_equal_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/or_valarray_valarray
test_src := or_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/greater_equal_value_valarray
test_src := greater_equal_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/less_equal_valarray_value
test_src := less_equal_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/and_value_valarray
test_src := and_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/less_valarray_value
test_src := less_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/greater_valarray_value
test_src := greater_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/greater_value_valarray
test_src := greater_value_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/or_valarray_value
test_src := or_valarray_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/valarray.nonmembers/valarray.comparison/equal_valarray_valarray
test_src := equal_valarray_valarray.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))