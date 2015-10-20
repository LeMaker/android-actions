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
test_makefile := external/libcxx/test/numerics/numarray/template.valarray/valarray.cons/Android.mk

test_name := numerics/numarray/template.valarray/valarray.cons/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cons/indirect_array
test_src := indirect_array.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cons/pointer_size
test_src := pointer_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cons/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cons/move
test_src := move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cons/size
test_src := size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cons/initializer_list
test_src := initializer_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cons/gslice_array
test_src := gslice_array.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cons/mask_array
test_src := mask_array.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cons/slice_array
test_src := slice_array.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/numarray/template.valarray/valarray.cons/value_size
test_src := value_size.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))