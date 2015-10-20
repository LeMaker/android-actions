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
test_makefile := external/libcxx/test/containers/unord/unord.map/Android.mk

test_name := containers/unord/unord.map/db_iterators_7
test_src := db_iterators_7.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/compare
test_src := compare.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/swap_member
test_src := swap_member.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/bucket_size
test_src := bucket_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/bucket_count
test_src := bucket_count.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/max_load_factor
test_src := max_load_factor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/db_local_iterators_7
test_src := db_local_iterators_7.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/find_const
test_src := find_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/max_bucket_count
test_src := max_bucket_count.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/version
test_src := version.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/local_iterators
test_src := local_iterators.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/equal_range_const
test_src := equal_range_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/max_size
test_src := max_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/find_non_const
test_src := find_non_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/iterators
test_src := iterators.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/types
test_src := types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/reserve
test_src := reserve.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/bucket
test_src := bucket.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/load_factor
test_src := load_factor.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/count
test_src := count.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/db_iterators_8
test_src := db_iterators_8.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/db_local_iterators_8
test_src := db_local_iterators_8.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/eq
test_src := eq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/rehash
test_src := rehash.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/unord/unord.map/equal_range_non_const
test_src := equal_range_non_const.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))