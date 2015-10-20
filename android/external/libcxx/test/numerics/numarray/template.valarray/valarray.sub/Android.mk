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
test_makefile := external/libcxx/test/numerics/numarray/template.valarray/valarray.sub/Android.mk

test_name := numerics/numarray/template.valarray/valarray.sub/slice_const
test_src := slice_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.sub/valarray_bool_non_const
test_src := valarray_bool_non_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.sub/gslice_const
test_src := gslice_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.sub/indirect_array_const
test_src := indirect_array_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.sub/slice_non_const
test_src := slice_non_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.sub/valarray_bool_const
test_src := valarray_bool_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.sub/gslice_non_const
test_src := gslice_non_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.sub/indirect_array_non_const
test_src := indirect_array_non_const.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))