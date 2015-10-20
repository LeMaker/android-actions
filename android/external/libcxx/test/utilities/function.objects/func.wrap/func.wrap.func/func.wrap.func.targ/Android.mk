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
test_makefile := external/libcxx/test/utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.targ/Android.mk

test_name := utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.targ/target_type
test_src := target_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/function.objects/func.wrap/func.wrap.func/func.wrap.func.targ/target
test_src := target.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))