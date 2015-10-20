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
test_makefile := external/libcxx/test/localization/locales/locale/locale.cons/Android.mk

test_name := localization/locales/locale/locale.cons/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale/locale.cons/string
test_src := string.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale/locale.cons/locale_string_cat
test_src := locale_string_cat.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale/locale.cons/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale/locale.cons/assign
test_src := assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale/locale.cons/char_pointer
test_src := char_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale/locale.cons/locale_facetptr
test_src := locale_facetptr.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale/locale.cons/locale_locale_cat
test_src := locale_locale_cat.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale/locale.cons/locale_char_pointer_cat
test_src := locale_char_pointer_cat.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))