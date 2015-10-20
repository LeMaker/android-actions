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
test_makefile := external/libcxx/test/depr/depr.str.strstreams/depr.strstreambuf/depr.strstreambuf.cons/Android.mk

test_name := depr/depr.str.strstreams/depr.strstreambuf/depr.strstreambuf.cons/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.str.strstreams/depr.strstreambuf/depr.strstreambuf.cons/ccp_size
test_src := ccp_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.str.strstreams/depr.strstreambuf/depr.strstreambuf.cons/cp_size_cp
test_src := cp_size_cp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.str.strstreams/depr.strstreambuf/depr.strstreambuf.cons/cucp_size
test_src := cucp_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.str.strstreams/depr.strstreambuf/depr.strstreambuf.cons/custom_alloc
test_src := custom_alloc.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.str.strstreams/depr.strstreambuf/depr.strstreambuf.cons/ucp_size_ucp
test_src := ucp_size_ucp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.str.strstreams/depr.strstreambuf/depr.strstreambuf.cons/scp_size_scp
test_src := scp_size_scp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.str.strstreams/depr.strstreambuf/depr.strstreambuf.cons/cscp_size
test_src := cscp_size.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))