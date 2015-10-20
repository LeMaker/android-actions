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
test_makefile := external/libcxx/test/utilities/utility/pairs/pairs.pair/Android.mk

test_name := utilities/utility/pairs/pairs.pair/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/utility/pairs/pairs.pair/const_first_const_second
test_src := const_first_const_second.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/utility/pairs/pairs.pair/copy_ctor
test_src := copy_ctor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/utility/pairs/pairs.pair/assign_rv_pair
test_src := assign_rv_pair.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/utility/pairs/pairs.pair/swap
test_src := swap.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/utility/pairs/pairs.pair/assign_const_pair_U_V
test_src := assign_const_pair_U_V.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/utility/pairs/pairs.pair/const_pair_U_V
test_src := const_pair_U_V.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/utility/pairs/pairs.pair/assign_rv_pair_U_V
test_src := assign_rv_pair_U_V.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/utility/pairs/pairs.pair/rv_pair_U_V
test_src := rv_pair_U_V.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/utility/pairs/pairs.pair/piecewise
test_src := piecewise.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/utility/pairs/pairs.pair/types
test_src := types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/utility/pairs/pairs.pair/U_V
test_src := U_V.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))