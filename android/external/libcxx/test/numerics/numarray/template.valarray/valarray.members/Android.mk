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
test_makefile := external/libcxx/test/numerics/numarray/template.valarray/valarray.members/Android.mk

test_name := numerics/numarray/template.valarray/valarray.members/apply_value
test_src := apply_value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.members/shift
test_src := shift.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.members/sum
test_src := sum.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.members/swap
test_src := swap.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.members/min
test_src := min.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.members/cshift
test_src := cshift.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.members/size
test_src := size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.members/resize
test_src := resize.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.members/max
test_src := max.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.members/apply_cref
test_src := apply_cref.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))