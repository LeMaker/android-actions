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
test_makefile := external/libcxx/test/algorithms/alg.sorting/alg.min.max/Android.mk

test_name := algorithms/alg.sorting/alg.min.max/max_element
test_src := max_element.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/minmax_init_list_comp
test_src := minmax_init_list_comp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/min_element_comp
test_src := min_element_comp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/min_comp
test_src := min_comp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/minmax_element
test_src := minmax_element.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/minmax_init_list
test_src := minmax_init_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/min
test_src := min.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/minmax
test_src := minmax.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/max_comp
test_src := max_comp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/max_init_list
test_src := max_init_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/minmax_comp
test_src := minmax_comp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/max_init_list_comp
test_src := max_init_list_comp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/min_init_list_comp
test_src := min_init_list_comp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/min_element
test_src := min_element.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/min_init_list
test_src := min_init_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/max
test_src := max.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/max_element_comp
test_src := max_element_comp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.sorting/alg.min.max/minmax_element_comp
test_src := minmax_element_comp.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))