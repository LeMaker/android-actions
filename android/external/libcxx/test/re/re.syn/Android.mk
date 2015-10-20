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
test_makefile := external/libcxx/test/re/re.syn/Android.mk

test_name := re/re.syn/cregex_iterator
test_src := cregex_iterator.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/wcmatch
test_src := wcmatch.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/wcregex_token_iterator
test_src := wcregex_token_iterator.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/ssub_match
test_src := ssub_match.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/csub_match
test_src := csub_match.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/wsmatch
test_src := wsmatch.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/wcregex_iterator
test_src := wcregex_iterator.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/wregex
test_src := wregex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/wcsub_match
test_src := wcsub_match.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/sregex_token_iterator
test_src := sregex_token_iterator.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/wsregex_iterator
test_src := wsregex_iterator.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/smatch
test_src := smatch.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/wsregex_token_iterator
test_src := wsregex_token_iterator.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/cmatch
test_src := cmatch.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/wssub_match
test_src := wssub_match.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/regex
test_src := regex.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/sregex_iterator
test_src := sregex_iterator.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.syn/cregex_token_iterator
test_src := cregex_token_iterator.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))