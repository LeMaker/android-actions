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
test_makefile := external/libcxx/test/re/re.regex/re.regex.construct/Android.mk

test_name := re/re.regex/re.regex.construct/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.construct/string
test_src := string.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.construct/awk_oct
test_src := awk_oct.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.construct/iter_iter_flg
test_src := iter_iter_flg.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.construct/iter_iter
test_src := iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.construct/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.construct/ptr_size_flg
test_src := ptr_size_flg.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.construct/bad_escape
test_src := bad_escape.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.construct/ptr
test_src := ptr.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.construct/string_flg
test_src := string_flg.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.construct/ptr_flg
test_src := ptr_flg.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.regex/re.regex.construct/il_flg
test_src := il_flg.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))