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
test_makefile := external/libcxx/test/input.output/iostream.format/input.streams/istream.unformatted/Android.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/peek
test_src := peek.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/sync
test_src := sync.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/unget
test_src := unget.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/get_pointer_size_chart
test_src := get_pointer_size_chart.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/ignore
test_src := ignore.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/putback
test_src := putback.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/get_streambuf
test_src := get_streambuf.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/getline_pointer_size
test_src := getline_pointer_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/read
test_src := read.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/get_chart
test_src := get_chart.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/seekg
test_src := seekg.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/get
test_src := get.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/ignore_0xff
test_src := ignore_0xff.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/get_streambuf_chart
test_src := get_streambuf_chart.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/seekg_off
test_src := seekg_off.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/readsome
test_src := readsome.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/tellg
test_src := tellg.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/getline_pointer_size_chart
test_src := getline_pointer_size_chart.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostream.format/input.streams/istream.unformatted/get_pointer_size
test_src := get_pointer_size.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))