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
test_makefile := external/libcxx/test/utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.con/Android.mk

test_name := utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.con/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.con/alloc
test_src := alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.con/F_incomplete
test_src := F_incomplete.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.con/F_assign
test_src := F_assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.con/F
test_src := F.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.con/alloc_rfunction
test_src := alloc_rfunction.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.con/nullptr_t_assign
test_src := nullptr_t_assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.con/alloc_nullptr
test_src := alloc_nullptr.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.con/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.con/copy_assign
test_src := copy_assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.con/alloc_function
test_src := alloc_function.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.con/alloc_F
test_src := alloc_F.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.con/nullptr_t
test_src := nullptr_t.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))