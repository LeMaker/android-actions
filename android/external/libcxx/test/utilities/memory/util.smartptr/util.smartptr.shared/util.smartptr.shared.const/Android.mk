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
test_makefile := external/libcxx/test/utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/Android.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/unique_ptr
test_src := unique_ptr.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/nullptr_t_deleter_throw
test_src := nullptr_t_deleter_throw.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/weak_ptr
test_src := weak_ptr.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/pointer_throw
test_src := pointer_throw.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/shared_ptr_Y_rv
test_src := shared_ptr_Y_rv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/pointer
test_src := pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/pointer_deleter_throw
test_src := pointer_deleter_throw.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/shared_ptr_pointer
test_src := shared_ptr_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/auto_ptr
test_src := auto_ptr.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/shared_ptr_rv
test_src := shared_ptr_rv.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/shared_ptr_Y
test_src := shared_ptr_Y.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/nullptr_t_deleter_allocator
test_src := nullptr_t_deleter_allocator.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/nullptr_t_deleter
test_src := nullptr_t_deleter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/pointer_deleter
test_src := pointer_deleter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/shared_ptr
test_src := shared_ptr.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/pointer_deleter_allocator_throw
test_src := pointer_deleter_allocator_throw.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/nullptr_t_deleter_allocator_throw
test_src := nullptr_t_deleter_allocator_throw.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/nullptr_t
test_src := nullptr_t.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.const/pointer_deleter_allocator
test_src := pointer_deleter_allocator.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))