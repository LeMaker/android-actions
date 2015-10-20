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
test_makefile := external/libcxx/test/utilities/memory/util.smartptr/util.smartptr.shared.atomic/Android.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared.atomic/atomic_compare_exchange_strong_explicit
test_src := atomic_compare_exchange_strong_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared.atomic/atomic_exchange
test_src := atomic_exchange.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared.atomic/atomic_compare_exchange_weak_explicit
test_src := atomic_compare_exchange_weak_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared.atomic/atomic_is_lock_free
test_src := atomic_is_lock_free.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared.atomic/atomic_store
test_src := atomic_store.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared.atomic/atomic_load
test_src := atomic_load.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared.atomic/atomic_load_explicit
test_src := atomic_load_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared.atomic/atomic_store_explicit
test_src := atomic_store_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared.atomic/atomic_compare_exchange_weak
test_src := atomic_compare_exchange_weak.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared.atomic/atomic_compare_exchange_strong
test_src := atomic_compare_exchange_strong.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/memory/util.smartptr/util.smartptr.shared.atomic/atomic_exchange_explicit
test_src := atomic_exchange_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))