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
test_makefile := external/libcxx/test/utilities/time/time.duration/time.duration.arithmetic/Android.mk

test_name := utilities/time/time.duration/time.duration.arithmetic/op_times=
test_src := op_times=.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/time.duration/time.duration.arithmetic/op_+
test_src := op_+.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/time.duration/time.duration.arithmetic/op_-
test_src := op_-.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/time.duration/time.duration.arithmetic/op_--
test_src := op_--.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/time.duration/time.duration.arithmetic/op_++
test_src := op_++.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/time.duration/time.duration.arithmetic/op_++int
test_src := op_++int.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/time.duration/time.duration.arithmetic/op_+=
test_src := op_+=.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/time.duration/time.duration.arithmetic/op_-=
test_src := op_-=.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/time.duration/time.duration.arithmetic/op_mod=rep
test_src := op_mod=rep.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/time.duration/time.duration.arithmetic/op_mod=duration
test_src := op_mod=duration.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/time.duration/time.duration.arithmetic/op_--int
test_src := op_--int.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/time/time.duration/time.duration.arithmetic/op_divide=
test_src := op_divide=.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))