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
test_makefile := external/libcxx/test/algorithms/alg.modifying.operations/alg.replace/Android.mk

test_name := algorithms/alg.modifying.operations/alg.replace/replace_copy_if
test_src := replace_copy_if.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.modifying.operations/alg.replace/replace_copy
test_src := replace_copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.modifying.operations/alg.replace/replace_if
test_src := replace_if.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := algorithms/alg.modifying.operations/alg.replace/replace
test_src := replace.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))