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
test_makefile := external/libcxx/test/containers/sequences/dynarray/dynarray.overview/Android.mk

test_name := containers/sequences/dynarray/dynarray.overview/at
test_src := at.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/dynarray/dynarray.overview/front_back
test_src := front_back.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/dynarray/dynarray.overview/capacity
test_src := capacity.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/dynarray/dynarray.overview/indexing
test_src := indexing.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/sequences/dynarray/dynarray.overview/begin_end
test_src := begin_end.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))