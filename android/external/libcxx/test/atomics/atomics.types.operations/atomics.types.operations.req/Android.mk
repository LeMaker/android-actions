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
test_makefile := external/libcxx/test/atomics/atomics.types.operations/atomics.types.operations.req/Android.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_or
test_src := atomic_fetch_or.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_compare_exchange_strong_explicit
test_src := atomic_compare_exchange_strong_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_exchange
test_src := atomic_exchange.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_compare_exchange_weak_explicit
test_src := atomic_compare_exchange_weak_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_or_explicit
test_src := atomic_fetch_or_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_add_explicit
test_src := atomic_fetch_add_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_is_lock_free
test_src := atomic_is_lock_free.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_sub_explicit
test_src := atomic_fetch_sub_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_init
test_src := atomic_init.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_store
test_src := atomic_store.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_load
test_src := atomic_load.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_load_explicit
test_src := atomic_load_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_store_explicit
test_src := atomic_store_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_and
test_src := atomic_fetch_and.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_add
test_src := atomic_fetch_add.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_sub
test_src := atomic_fetch_sub.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_compare_exchange_weak
test_src := atomic_compare_exchange_weak.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_var_init
test_src := atomic_var_init.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_compare_exchange_strong
test_src := atomic_compare_exchange_strong.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_exchange_explicit
test_src := atomic_exchange_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_xor
test_src := atomic_fetch_xor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_and_explicit
test_src := atomic_fetch_and_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_xor_explicit
test_src := atomic_fetch_xor_explicit.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))