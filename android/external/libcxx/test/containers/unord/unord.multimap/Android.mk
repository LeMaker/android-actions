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
test_makefile := external/libcxx/test/containers/unord/unord.multimap/Android.mk

test_name := containers/unord/unord.multimap/db_iterators_7
test_src := db_iterators_7.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/swap_member
test_src := swap_member.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/bucket_size
test_src := bucket_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/bucket_count
test_src := bucket_count.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/max_load_factor
test_src := max_load_factor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/db_local_iterators_7
test_src := db_local_iterators_7.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/find_const
test_src := find_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/max_bucket_count
test_src := max_bucket_count.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/local_iterators
test_src := local_iterators.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/equal_range_const
test_src := equal_range_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/max_size
test_src := max_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/find_non_const
test_src := find_non_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/iterators
test_src := iterators.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/scary
test_src := scary.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/types
test_src := types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/reserve
test_src := reserve.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/bucket
test_src := bucket.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/load_factor
test_src := load_factor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/count
test_src := count.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/db_iterators_8
test_src := db_iterators_8.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/db_local_iterators_8
test_src := db_local_iterators_8.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/eq
test_src := eq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/rehash
test_src := rehash.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.multimap/equal_range_non_const
test_src := equal_range_non_const.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))