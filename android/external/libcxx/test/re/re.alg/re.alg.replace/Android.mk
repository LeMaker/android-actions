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
test_makefile := external/libcxx/test/re/re.alg/re.alg.replace/Android.mk

test_name := re/re.alg/re.alg.replace/test2
test_src := test2.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.alg/re.alg.replace/test3
test_src := test3.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.alg/re.alg.replace/test5
test_src := test5.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.alg/re.alg.replace/test1
test_src := test1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.alg/re.alg.replace/test6
test_src := test6.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := re/re.alg/re.alg.replace/test4
test_src := test4.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))