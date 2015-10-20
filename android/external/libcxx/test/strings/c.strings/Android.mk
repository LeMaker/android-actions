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
test_makefile := external/libcxx/test/strings/c.strings/Android.mk

test_name := strings/c.strings/cctype
test_src := cctype.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/c.strings/version_cwctype
test_src := version_cwctype.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/c.strings/cwctype
test_src := cwctype.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/c.strings/cwchar
test_src := cwchar.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/c.strings/cstring
test_src := cstring.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/c.strings/version_cwchar
test_src := version_cwchar.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/c.strings/version_cctype
test_src := version_cctype.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/c.strings/version_cstring
test_src := version_cstring.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))
