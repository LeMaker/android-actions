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
test_makefile := external/libcxx/test/strings/char.traits/char.traits.specializations/char.traits.specializations.wchar.t/Android.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.wchar.t/length
test_src := length.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.wchar.t/compare
test_src := compare.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.wchar.t/find
test_src := find.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.wchar.t/lt
test_src := lt.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.wchar.t/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.wchar.t/move
test_src := move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.wchar.t/not_eof
test_src := not_eof.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.wchar.t/assign3
test_src := assign3.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.wchar.t/assign2
test_src := assign2.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.wchar.t/types
test_src := types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.wchar.t/to_int_type
test_src := to_int_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.wchar.t/eq_int_type
test_src := eq_int_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.wchar.t/to_char_type
test_src := to_char_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.wchar.t/eq
test_src := eq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.wchar.t/eof
test_src := eof.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))