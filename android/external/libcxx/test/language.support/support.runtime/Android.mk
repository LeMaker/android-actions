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
test_makefile := external/libcxx/test/language.support/support.runtime/Android.mk

test_name := language.support/support.runtime/cstdbool
test_src := cstdbool.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.runtime/cstdarg
test_src := cstdarg.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.runtime/version_csetjmp
test_src := version_csetjmp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.runtime/version_ctime
test_src := version_ctime.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.runtime/version_cstdlib
test_src := version_cstdlib.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.runtime/ctime
test_src := ctime.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.runtime/version_csignal
test_src := version_csignal.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.runtime/version_cstdarg
test_src := version_cstdarg.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.runtime/csetjmp
test_src := csetjmp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.runtime/version_cstdbool
test_src := version_cstdbool.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.runtime/csignal
test_src := csignal.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.runtime/cstdlib
test_src := cstdlib.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))