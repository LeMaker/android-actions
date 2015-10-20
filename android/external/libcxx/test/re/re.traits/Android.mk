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
test_makefile := external/libcxx/test/re/re.traits/Android.mk

test_name := re/re.traits/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.traits/lookup_classname
test_src := lookup_classname.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.traits/length
test_src := length.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.traits/transform
test_src := transform.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.traits/translate_nocase
test_src := translate_nocase.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.traits/translate
test_src := translate.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.traits/imbue
test_src := imbue.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.traits/types
test_src := types.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.traits/isctype
test_src := isctype.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.traits/value
test_src := value.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.traits/lookup_collatename
test_src := lookup_collatename.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.traits/getloc
test_src := getloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.traits/transform_primary
test_src := transform_primary.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))