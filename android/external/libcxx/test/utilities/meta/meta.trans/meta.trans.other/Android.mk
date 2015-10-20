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
test_makefile := external/libcxx/test/utilities/meta/meta.trans/meta.trans.other/Android.mk

test_name := utilities/meta/meta.trans/meta.trans.other/aligned_union
test_src := aligned_union.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.trans/meta.trans.other/decay
test_src := decay.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.trans/meta.trans.other/result_of
test_src := result_of.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.trans/meta.trans.other/underlying_type
test_src := underlying_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.trans/meta.trans.other/enable_if
test_src := enable_if.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.trans/meta.trans.other/conditional
test_src := conditional.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/meta/meta.trans/meta.trans.other/common_type
test_src := common_type.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))
