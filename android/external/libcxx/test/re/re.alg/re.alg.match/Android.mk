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
test_makefile := external/libcxx/test/re/re.alg/re.alg.match/Android.mk

test_name := re/re.alg/re.alg.match/awk
test_src := awk.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.alg/re.alg.match/grep
test_src := grep.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.alg/re.alg.match/egrep
test_src := egrep.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.alg/re.alg.match/ecma
test_src := ecma.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.alg/re.alg.match/extended
test_src := extended.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.alg/re.alg.match/basic
test_src := basic.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.alg/re.alg.match/parse_curly_brackets
test_src := parse_curly_brackets.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.alg/re.alg.match/lookahead_capture
test_src := lookahead_capture.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))