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
test_makefile := external/libcxx/test/localization/locale.categories/category.time/locale.time.get/locale.time.get.members/Android.mk

test_name := localization/locale.categories/category.time/locale.time.get/locale.time.get.members/get_monthname
test_src := get_monthname.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.time/locale.time.get/locale.time.get.members/get_weekday_wide
test_src := get_weekday_wide.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.time/locale.time.get/locale.time.get.members/get_one
test_src := get_one.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.time/locale.time.get/locale.time.get.members/get_time
test_src := get_time.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.time/locale.time.get/locale.time.get.members/get_weekday
test_src := get_weekday.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.time/locale.time.get/locale.time.get.members/get_date_wide
test_src := get_date_wide.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.time/locale.time.get/locale.time.get.members/get_date
test_src := get_date.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.time/locale.time.get/locale.time.get.members/get_time_wide
test_src := get_time_wide.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.time/locale.time.get/locale.time.get.members/date_order
test_src := date_order.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.time/locale.time.get/locale.time.get.members/get_monthname_wide
test_src := get_monthname_wide.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.time/locale.time.get/locale.time.get.members/get_year
test_src := get_year.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.time/locale.time.get/locale.time.get.members/get_many
test_src := get_many.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))