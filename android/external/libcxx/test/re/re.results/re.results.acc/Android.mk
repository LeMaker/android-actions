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
test_makefile := external/libcxx/test/re/re.results/re.results.acc/Android.mk

test_name := re/re.results/re.results.acc/length
test_src := length.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.results/re.results.acc/cbegin_cend
test_src := cbegin_cend.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.results/re.results.acc/suffix
test_src := suffix.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.results/re.results.acc/position
test_src := position.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.results/re.results.acc/index
test_src := index.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.results/re.results.acc/prefix
test_src := prefix.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.results/re.results.acc/begin_end
test_src := begin_end.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.results/re.results.acc/str
test_src := str.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))