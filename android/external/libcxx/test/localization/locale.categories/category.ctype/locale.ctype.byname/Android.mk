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
test_makefile := external/libcxx/test/localization/locale.categories/category.ctype/locale.ctype.byname/Android.mk

test_name := localization/locale.categories/category.ctype/locale.ctype.byname/widen_many
test_src := widen_many.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.ctype.byname/scan_is
test_src := scan_is.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.ctype.byname/narrow_many
test_src := narrow_many.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.ctype.byname/scan_not
test_src := scan_not.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.ctype.byname/toupper_many
test_src := toupper_many.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.ctype.byname/toupper_1
test_src := toupper_1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.ctype.byname/widen_1
test_src := widen_1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.ctype.byname/tolower_1
test_src := tolower_1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.ctype.byname/types
test_src := types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.ctype.byname/is_many
test_src := is_many.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.ctype.byname/is_1
test_src := is_1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.ctype.byname/tolower_many
test_src := tolower_many.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.ctype.byname/narrow_1
test_src := narrow_1.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))