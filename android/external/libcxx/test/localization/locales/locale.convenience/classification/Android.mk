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
test_makefile := external/libcxx/test/localization/locales/locale.convenience/classification/Android.mk

test_name := localization/locales/locale.convenience/classification/iscntrl
test_src := iscntrl.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/classification/isspace
test_src := isspace.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/classification/isalpha
test_src := isalpha.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/classification/isalnum
test_src := isalnum.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/classification/ispunct
test_src := ispunct.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/classification/isupper
test_src := isupper.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/classification/isxdigit
test_src := isxdigit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/classification/isprint
test_src := isprint.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/classification/islower
test_src := islower.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/classification/isgraph
test_src := isgraph.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/classification/isdigit
test_src := isdigit.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))