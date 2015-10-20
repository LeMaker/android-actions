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
test_makefile := external/libcxx/test/input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.character/Android.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.character/CharT
test_src := CharT.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.character/signed_char
test_src := signed_char.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.character/char_to_wide_pointer
test_src := char_to_wide_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.character/unsigned_char_pointer
test_src := unsigned_char_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.character/char
test_src := char.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.character/unsigned_char
test_src := unsigned_char.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.character/char_to_wide
test_src := char_to_wide.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.character/char_pointer
test_src := char_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.character/signed_char_pointer
test_src := signed_char_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.character/CharT_pointer
test_src := CharT_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))