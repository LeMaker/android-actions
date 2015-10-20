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
test_makefile := external/libcxx/test/utilities/meta/meta.unary/meta.unary.prop/Android.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_move_constructible
test_src := is_move_constructible.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_default_constructible
test_src := is_default_constructible.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_trivially_move_assignable
test_src := is_trivially_move_assignable.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_nothrow_copy_assignable
test_src := is_nothrow_copy_assignable.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_trivially_move_constructible
test_src := is_trivially_move_constructible.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_nothrow_default_constructible
test_src := is_nothrow_default_constructible.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_trivially_constructible
test_src := is_trivially_constructible.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/__has_operator_addressof
test_src := __has_operator_addressof.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_nothrow_move_assignable
test_src := is_nothrow_move_assignable.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_final
test_src := is_final.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_copy_assignable
test_src := is_copy_assignable.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_nothrow_constructible
test_src := is_nothrow_constructible.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_pod
test_src := is_pod.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_nothrow_copy_constructible
test_src := is_nothrow_copy_constructible.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_empty
test_src := is_empty.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_signed
test_src := is_signed.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_trivially_destructible
test_src := is_trivially_destructible.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_polymorphic
test_src := is_polymorphic.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/has_virtual_destructor
test_src := has_virtual_destructor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_constructible
test_src := is_constructible.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_trivially_copy_assignable
test_src := is_trivially_copy_assignable.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_nothrow_assignable
test_src := is_nothrow_assignable.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_move_assignable
test_src := is_move_assignable.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_trivial
test_src := is_trivial.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_standard_layout
test_src := is_standard_layout.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_trivialially_copyable
test_src := is_trivialially_copyable.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_nothrow_destructible
test_src := is_nothrow_destructible.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_unsigned
test_src := is_unsigned.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_volatile
test_src := is_volatile.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_literal_type
test_src := is_literal_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_trivially_copy_constructible
test_src := is_trivially_copy_constructible.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_copy_constructible
test_src := is_copy_constructible.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_destructible
test_src := is_destructible.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_abstract
test_src := is_abstract.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_assignable
test_src := is_assignable.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_nothrow_move_constructible
test_src := is_nothrow_move_constructible.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_const
test_src := is_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_trivially_default_constructible
test_src := is_trivially_default_constructible.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.unary/meta.unary.prop/is_trivially_assignable
test_src := is_trivially_assignable.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))