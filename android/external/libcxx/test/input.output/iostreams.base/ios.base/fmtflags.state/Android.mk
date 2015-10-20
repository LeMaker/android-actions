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
test_makefile := external/libcxx/test/input.output/iostreams.base/ios.base/fmtflags.state/Android.mk

test_name := input.output/iostreams.base/ios.base/fmtflags.state/unsetf_mask
test_src := unsetf_mask.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios.base/fmtflags.state/width
test_src := width.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios.base/fmtflags.state/precision
test_src := precision.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios.base/fmtflags.state/setf_fmtflags
test_src := setf_fmtflags.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios.base/fmtflags.state/flags_fmtflags
test_src := flags_fmtflags.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios.base/fmtflags.state/flags
test_src := flags.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios.base/fmtflags.state/width_streamsize
test_src := width_streamsize.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios.base/fmtflags.state/setf_fmtflags_mask
test_src := setf_fmtflags_mask.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := input.output/iostreams.base/ios.base/fmtflags.state/precision_streamsize
test_src := precision_streamsize.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))