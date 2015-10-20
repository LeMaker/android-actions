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
test_makefile := external/libcxx/test/localization/locale.categories/category.ctype/locale.codecvt/Android.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/codecvt_base
test_src := codecvt_base.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/types_wchar_t
test_src := types_wchar_t.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/types_char32_t
test_src := types_char32_t.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/types_char
test_src := types_char.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/ctor_char32_t
test_src := ctor_char32_t.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/ctor_char16_t
test_src := ctor_char16_t.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/ctor_char
test_src := ctor_char.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/ctor_wchar_t
test_src := ctor_wchar_t.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/types_char16_t
test_src := types_char16_t.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))