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
test_makefile := external/libcxx/test/input.output/iostreams.base/std.ios.manip/fmtflags.manip/Android.mk

test_name := input.output/iostreams.base/std.ios.manip/fmtflags.manip/unitbuf
test_src := unitbuf.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/std.ios.manip/fmtflags.manip/noskipws
test_src := noskipws.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/std.ios.manip/fmtflags.manip/nounitbuf
test_src := nounitbuf.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/std.ios.manip/fmtflags.manip/noshowpoint
test_src := noshowpoint.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/std.ios.manip/fmtflags.manip/showbase
test_src := showbase.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/std.ios.manip/fmtflags.manip/skipws
test_src := skipws.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/std.ios.manip/fmtflags.manip/uppercase
test_src := uppercase.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/std.ios.manip/fmtflags.manip/showpoint
test_src := showpoint.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/std.ios.manip/fmtflags.manip/noshowbase
test_src := noshowbase.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/std.ios.manip/fmtflags.manip/showpos
test_src := showpos.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/std.ios.manip/fmtflags.manip/noboolalpha
test_src := noboolalpha.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/std.ios.manip/fmtflags.manip/noshowpos
test_src := noshowpos.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/std.ios.manip/fmtflags.manip/nouppercase
test_src := nouppercase.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/std.ios.manip/fmtflags.manip/boolalpha
test_src := boolalpha.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))