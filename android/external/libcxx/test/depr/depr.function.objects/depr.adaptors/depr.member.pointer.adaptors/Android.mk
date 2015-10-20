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
test_makefile := external/libcxx/test/depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/Android.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/const_mem_fun_t
test_src := const_mem_fun_t.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/mem_fun_ref1
test_src := mem_fun_ref1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/const_mem_fun1
test_src := const_mem_fun1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/mem_fun
test_src := mem_fun.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/const_mem_fun
test_src := const_mem_fun.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/const_mem_fun1_t
test_src := const_mem_fun1_t.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/const_mem_fun_ref
test_src := const_mem_fun_ref.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/mem_fun_ref_t
test_src := mem_fun_ref_t.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/mem_fun_ref
test_src := mem_fun_ref.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/const_mem_fun1_ref_t
test_src := const_mem_fun1_ref_t.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/const_mem_fun_ref_t
test_src := const_mem_fun_ref_t.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/mem_fun1_ref_t
test_src := mem_fun1_ref_t.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/const_mem_fun_ref1
test_src := const_mem_fun_ref1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/mem_fun1
test_src := mem_fun1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/mem_fun_t
test_src := mem_fun_t.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.member.pointer.adaptors/mem_fun1_t
test_src := mem_fun1_t.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))