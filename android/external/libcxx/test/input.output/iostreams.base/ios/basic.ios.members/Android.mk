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
test_makefile := external/libcxx/test/input.output/iostreams.base/ios/basic.ios.members/Android.mk

test_name := input.output/iostreams.base/ios/basic.ios.members/narow
test_src := narow.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/basic.ios.members/tie
test_src := tie.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/basic.ios.members/swap
test_src := swap.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/basic.ios.members/rdbuf_streambuf
test_src := rdbuf_streambuf.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/basic.ios.members/copyfmt
test_src := copyfmt.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/basic.ios.members/rdbuf
test_src := rdbuf.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/basic.ios.members/move
test_src := move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/basic.ios.members/tie_ostream
test_src := tie_ostream.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/basic.ios.members/widen
test_src := widen.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/basic.ios.members/imbue
test_src := imbue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/basic.ios.members/fill_char_type
test_src := fill_char_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/basic.ios.members/set_rdbuf
test_src := set_rdbuf.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios/basic.ios.members/fill
test_src := fill.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))