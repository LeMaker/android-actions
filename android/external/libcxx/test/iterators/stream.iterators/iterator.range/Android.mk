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
test_makefile := external/libcxx/test/iterators/stream.iterators/iterator.range/Android.mk

test_name := iterators/stream.iterators/iterator.range/begin_array
test_src := begin_array.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := iterators/stream.iterators/iterator.range/begin_const
test_src := begin_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := iterators/stream.iterators/iterator.range/end_non_const
test_src := end_non_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := iterators/stream.iterators/iterator.range/end_array
test_src := end_array.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := iterators/stream.iterators/iterator.range/end_const
test_src := end_const.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := iterators/stream.iterators/iterator.range/begin_non_const
test_src := begin_non_const.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))