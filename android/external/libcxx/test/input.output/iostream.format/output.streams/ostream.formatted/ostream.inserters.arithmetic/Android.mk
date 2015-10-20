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
test_makefile := external/libcxx/test/input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.arithmetic/Android.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.arithmetic/bool
test_src := bool.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.arithmetic/unsigned_short
test_src := unsigned_short.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.arithmetic/pointer
test_src := pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.arithmetic/int
test_src := int.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.arithmetic/long_double
test_src := long_double.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.arithmetic/short
test_src := short.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.arithmetic/unsigned_int
test_src := unsigned_int.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.arithmetic/unsigned_long
test_src := unsigned_long.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.arithmetic/long_long
test_src := long_long.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.arithmetic/float
test_src := float.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.arithmetic/unsigned_long_long
test_src := unsigned_long_long.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.arithmetic/double
test_src := double.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.arithmetic/long
test_src := long.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))