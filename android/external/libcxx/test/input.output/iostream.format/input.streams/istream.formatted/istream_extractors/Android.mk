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
test_makefile := external/libcxx/test/input.output/iostream.format/input.streams/istream.formatted/istream_extractors/Android.mk

test_name := input.output/iostream.format/input.streams/istream.formatted/istream_extractors/signed_char
test_src := signed_char.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.formatted/istream_extractors/unsigned_char_pointer
test_src := unsigned_char_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.formatted/istream_extractors/unsigned_char
test_src := unsigned_char.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.formatted/istream_extractors/basic_ios
test_src := basic_ios.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.formatted/istream_extractors/streambuf
test_src := streambuf.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.formatted/istream_extractors/ios_base
test_src := ios_base.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.formatted/istream_extractors/signed_char_pointer
test_src := signed_char_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.formatted/istream_extractors/chart
test_src := chart.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.formatted/istream_extractors/wchar_t_pointer
test_src := wchar_t_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.formatted/istream_extractors/istream
test_src := istream.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))