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
test_makefile := external/libcxx/test/utilities/function.objects/comparisons/Android.mk

test_name := utilities/function.objects/comparisons/less
test_src := less.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/comparisons/not_equal_to
test_src := not_equal_to.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/comparisons/greater_equal
test_src := greater_equal.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/comparisons/less_equal
test_src := less_equal.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/comparisons/transparent
test_src := transparent.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/comparisons/equal_to
test_src := equal_to.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/comparisons/greater
test_src := greater.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))