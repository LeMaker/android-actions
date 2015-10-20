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
test_makefile := external/libcxx/test/strings/char.traits/char.traits.specializations/char.traits.specializations.char/Android.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.char/length
test_src := length.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.char/compare
test_src := compare.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.char/find
test_src := find.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.char/lt
test_src := lt.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.char/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.char/move
test_src := move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.char/not_eof
test_src := not_eof.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.char/assign3
test_src := assign3.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.char/assign2
test_src := assign2.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.char/types
test_src := types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.char/to_int_type
test_src := to_int_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.char/eq_int_type
test_src := eq_int_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.char/to_char_type
test_src := to_char_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.char/eq
test_src := eq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/char.traits/char.traits.specializations/char.traits.specializations.char/eof
test_src := eof.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))