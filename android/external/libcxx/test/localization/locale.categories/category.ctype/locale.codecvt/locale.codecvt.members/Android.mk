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
test_makefile := external/libcxx/test/localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/Android.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char_unshift
test_src := char_unshift.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char16_t_encoding
test_src := char16_t_encoding.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char32_t_out
test_src := char32_t_out.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char_length
test_src := char_length.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char_out
test_src := char_out.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char16_t_always_noconv
test_src := char16_t_always_noconv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/wchar_t_out
test_src := wchar_t_out.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char16_t_length
test_src := char16_t_length.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/wchar_t_always_noconv
test_src := wchar_t_always_noconv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/wchar_t_encoding
test_src := wchar_t_encoding.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char16_t_max_length
test_src := char16_t_max_length.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char16_t_out
test_src := char16_t_out.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char16_t_in
test_src := char16_t_in.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/wchar_t_unshift
test_src := wchar_t_unshift.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char32_t_unshift
test_src := char32_t_unshift.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char_max_length
test_src := char_max_length.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/wchar_t_length
test_src := wchar_t_length.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char32_t_length
test_src := char32_t_length.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char16_t_unshift
test_src := char16_t_unshift.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char32_t_always_noconv
test_src := char32_t_always_noconv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/wchar_t_max_length
test_src := wchar_t_max_length.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char_encoding
test_src := char_encoding.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char32_t_encoding
test_src := char32_t_encoding.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char_in
test_src := char_in.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char_always_noconv
test_src := char_always_noconv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char32_t_max_length
test_src := char32_t_max_length.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/char32_t_in
test_src := char32_t_in.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/wchar_t_in
test_src := wchar_t_in.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locale.categories/category.ctype/locale.codecvt/locale.codecvt.members/utf_sanity_check
test_src := utf_sanity_check.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))