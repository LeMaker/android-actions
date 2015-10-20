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
test_makefile := external/libcxx/test/localization/locales/locale.convenience/conversions/conversions.string/Android.mk

test_name := localization/locales/locale.convenience/conversions/conversions.string/from_bytes
test_src := from_bytes.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/conversions/conversions.string/state
test_src := state.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/conversions/conversions.string/ctor_codecvt
test_src := ctor_codecvt.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/conversions/conversions.string/converted
test_src := converted.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/conversions/conversions.string/to_bytes
test_src := to_bytes.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/conversions/conversions.string/types
test_src := types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/conversions/conversions.string/ctor_err_string
test_src := ctor_err_string.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/conversions/conversions.string/ctor_codecvt_state
test_src := ctor_codecvt_state.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))