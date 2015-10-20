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
test_makefile := external/libcxx/test/input.output/iostreams.base/ios/iostate.flags/Android.mk

test_name := input.output/iostreams.base/ios/iostate.flags/bool
test_src := bool.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/iostate.flags/good
test_src := good.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/iostate.flags/not
test_src := not.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/iostate.flags/setstate
test_src := setstate.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/iostate.flags/rdstate
test_src := rdstate.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/iostate.flags/clear
test_src := clear.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/iostate.flags/exceptions_iostate
test_src := exceptions_iostate.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/iostate.flags/bad
test_src := bad.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/iostate.flags/fail
test_src := fail.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/iostate.flags/exceptions
test_src := exceptions.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/iostate.flags/eof
test_src := eof.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))