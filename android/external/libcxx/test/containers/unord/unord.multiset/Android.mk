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
test_makefile := external/libcxx/test/containers/unord/unord.multiset/Android.mk

test_name := containers/unord/unord.multiset/db_iterators_7
test_src := db_iterators_7.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/swap_member
test_src := swap_member.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/erase_iter_iter_db4
test_src := erase_iter_iter_db4.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/bucket_size
test_src := bucket_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/bucket_count
test_src := bucket_count.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/max_load_factor
test_src := max_load_factor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/emplace_hint
test_src := emplace_hint.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/erase_iter_iter_db1
test_src := erase_iter_iter_db1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/db_local_iterators_7
test_src := db_local_iterators_7.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/insert_hint_const_lvalue
test_src := insert_hint_const_lvalue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/erase_range
test_src := erase_range.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/find_const
test_src := find_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/max_bucket_count
test_src := max_bucket_count.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/erase_iter_iter_db3
test_src := erase_iter_iter_db3.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/erase_key
test_src := erase_key.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/erase_iter_db2
test_src := erase_iter_db2.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/insert_init
test_src := insert_init.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/erase_const_iter
test_src := erase_const_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/local_iterators
test_src := local_iterators.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/clear
test_src := clear.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/equal_range_const
test_src := equal_range_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/insert_range
test_src := insert_range.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/max_size
test_src := max_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/find_non_const
test_src := find_non_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/iterators
test_src := iterators.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/scary
test_src := scary.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/types
test_src := types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/emplace
test_src := emplace.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/reserve
test_src := reserve.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/bucket
test_src := bucket.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/insert_rvalue
test_src := insert_rvalue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/insert_hint_rvalue
test_src := insert_hint_rvalue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/erase_iter_db1
test_src := erase_iter_db1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/load_factor
test_src := load_factor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/count
test_src := count.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/db_iterators_8
test_src := db_iterators_8.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/db_local_iterators_8
test_src := db_local_iterators_8.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/erase_iter_iter_db2
test_src := erase_iter_iter_db2.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/eq
test_src := eq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/insert_const_lvalue
test_src := insert_const_lvalue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/rehash
test_src := rehash.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multiset/equal_range_non_const
test_src := equal_range_non_const.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))