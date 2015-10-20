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
test_makefile := external/libcxx/test/utilities/utility/forward/Android.mk

test_name := utilities/utility/forward/forward
test_src := forward.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/utility/forward/move_if_noexcept
test_src := move_if_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/utility/forward/move_only
test_src := move_only.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := utilities/utility/forward/move_copy
test_src := move_copy.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))