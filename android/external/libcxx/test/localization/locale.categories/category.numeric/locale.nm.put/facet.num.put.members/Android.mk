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
test_makefile := external/libcxx/test/localization/locale.categories/category.numeric/locale.nm.put/facet.num.put.members/Android.mk

test_name := localization/locale.categories/category.numeric/locale.nm.put/facet.num.put.members/put_unsigned_long_long
test_src := put_unsigned_long_long.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.numeric/locale.nm.put/facet.num.put.members/put_long_long
test_src := put_long_long.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.numeric/locale.nm.put/facet.num.put.members/put_long_double
test_src := put_long_double.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.numeric/locale.nm.put/facet.num.put.members/put_long
test_src := put_long.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.numeric/locale.nm.put/facet.num.put.members/put_double
test_src := put_double.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.numeric/locale.nm.put/facet.num.put.members/put_unsigned_long
test_src := put_unsigned_long.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.numeric/locale.nm.put/facet.num.put.members/put_pointer
test_src := put_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.numeric/locale.nm.put/facet.num.put.members/put_bool
test_src := put_bool.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))