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
test_makefile := external/libcxx/test/numerics/numarray/template.valarray/valarray.assign/Android.mk

test_name := numerics/numarray/template.valarray/valarray.assign/indirect_array_assign
test_src := indirect_array_assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.assign/mask_array_assign
test_src := mask_array_assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.assign/initializer_list_assign
test_src := initializer_list_assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.assign/move_assign
test_src := move_assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.assign/gslice_array_assign
test_src := gslice_array_assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.assign/copy_assign
test_src := copy_assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.assign/value_assign
test_src := value_assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.assign/slice_array_assign
test_src := slice_array_assign.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))