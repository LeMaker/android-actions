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
test_makefile := external/libcxx/test/localization/locale.categories/category.monetary/locale.moneypunct/locale.moneypunct.members/Android.mk

test_name := localization/locale.categories/category.monetary/locale.moneypunct/locale.moneypunct.members/neg_format
test_src := neg_format.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.monetary/locale.moneypunct/locale.moneypunct.members/pos_format
test_src := pos_format.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.monetary/locale.moneypunct/locale.moneypunct.members/grouping
test_src := grouping.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.monetary/locale.moneypunct/locale.moneypunct.members/frac_digits
test_src := frac_digits.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.monetary/locale.moneypunct/locale.moneypunct.members/negative_sign
test_src := negative_sign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.monetary/locale.moneypunct/locale.moneypunct.members/thousands_sep
test_src := thousands_sep.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.monetary/locale.moneypunct/locale.moneypunct.members/positive_sign
test_src := positive_sign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.monetary/locale.moneypunct/locale.moneypunct.members/decimal_point
test_src := decimal_point.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.monetary/locale.moneypunct/locale.moneypunct.members/curr_symbol
test_src := curr_symbol.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))