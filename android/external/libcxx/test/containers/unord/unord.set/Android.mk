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
test_makefile := external/libcxx/test/containers/unord/unord.set/Android.mk

test_name := containers/unord/unord.set/db_iterators_7
test_src := db_iterators_7.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/swap_member
test_src := swap_member.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/erase_iter_iter_db4
test_src := erase_iter_iter_db4.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/bucket_size
test_src := bucket_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/bucket_count
test_src := bucket_count.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/max_load_factor
test_src := max_load_factor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/emplace_hint
test_src := emplace_hint.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/erase_iter_iter_db1
test_src := erase_iter_iter_db1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/db_local_iterators_7
test_src := db_local_iterators_7.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/insert_hint_const_lvalue
test_src := insert_hint_const_lvalue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/erase_range
test_src := erase_range.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/find_const
test_src := find_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/max_bucket_count
test_src := max_bucket_count.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/erase_iter_iter_db3
test_src := erase_iter_iter_db3.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/erase_key
test_src := erase_key.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/erase_iter_db2
test_src := erase_iter_db2.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/version
test_src := version.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/insert_init
test_src := insert_init.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/erase_const_iter
test_src := erase_const_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/local_iterators
test_src := local_iterators.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/clear
test_src := clear.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/equal_range_const
test_src := equal_range_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/insert_range
test_src := insert_range.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/max_size
test_src := max_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/find_non_const
test_src := find_non_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/iterators
test_src := iterators.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/types
test_src := types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/emplace
test_src := emplace.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/reserve
test_src := reserve.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/bucket
test_src := bucket.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/insert_rvalue
test_src := insert_rvalue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/insert_hint_rvalue
test_src := insert_hint_rvalue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/erase_iter_db1
test_src := erase_iter_db1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/load_factor
test_src := load_factor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/count
test_src := count.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/db_iterators_8
test_src := db_iterators_8.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/db_local_iterators_8
test_src := db_local_iterators_8.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/erase_iter_iter_db2
test_src := erase_iter_iter_db2.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/eq
test_src := eq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/insert_const_lvalue
test_src := insert_const_lvalue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/rehash
test_src := rehash.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.set/equal_range_non_const
test_src := equal_range_non_const.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))