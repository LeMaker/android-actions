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
test_makefile := external/libcxx/test/input.output/iostreams.base/fpos/fpos.operations/Android.mk

test_name := input.output/iostreams.base/fpos/fpos.operations/offset
test_src := offset.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/fpos/fpos.operations/addition
test_src := addition.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/fpos/fpos.operations/subtraction
test_src := subtraction.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/fpos/fpos.operations/streamsize
test_src := streamsize.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/fpos/fpos.operations/eq_int
test_src := eq_int.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/fpos/fpos.operations/ctor_int
test_src := ctor_int.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/fpos/fpos.operations/difference
test_src := difference.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))