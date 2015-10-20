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
test_makefile := external/libcxx/test/localization/locales/locale.convenience/conversions/conversions.buffer/Android.mk

test_name := localization/locales/locale.convenience/conversions/conversions.buffer/state
test_src := state.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/conversions/conversions.buffer/overflow
test_src := overflow.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/conversions/conversions.buffer/rdbuf
test_src := rdbuf.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/conversions/conversions.buffer/test
test_src := test.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/conversions/conversions.buffer/pbackfail
test_src := pbackfail.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/conversions/conversions.buffer/underflow
test_src := underflow.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/conversions/conversions.buffer/ctor
test_src := ctor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := localization/locales/locale.convenience/conversions/conversions.buffer/seekoff
test_src := seekoff.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))