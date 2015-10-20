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
test_makefile := external/libcxx/test/localization/locale.categories/category.monetary/locale.money.put/locale.money.put.members/Android.mk

test_name := localization/locale.categories/category.monetary/locale.money.put/locale.money.put.members/put_long_double_fr_FR
test_src := put_long_double_fr_FR.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.monetary/locale.money.put/locale.money.put.members/put_long_double_ru_RU
test_src := put_long_double_ru_RU.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.monetary/locale.money.put/locale.money.put.members/put_long_double_en_US
test_src := put_long_double_en_US.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.monetary/locale.money.put/locale.money.put.members/put_long_double_zh_CN
test_src := put_long_double_zh_CN.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.monetary/locale.money.put/locale.money.put.members/put_string_en_US
test_src := put_string_en_US.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))