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
test_makefile := external/libcxx/test/numerics/rand/rand.predef/Android.mk

test_name := numerics/rand/rand.predef/mt19937
test_src := mt19937.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.predef/minstd_rand
test_src := minstd_rand.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.predef/ranlux24_base
test_src := ranlux24_base.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.predef/ranlux24
test_src := ranlux24.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.predef/ranlux48_base
test_src := ranlux48_base.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.predef/ranlux48
test_src := ranlux48.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.predef/mt19937_64
test_src := mt19937_64.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.predef/minstd_rand0
test_src := minstd_rand0.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.predef/knuth_b
test_src := knuth_b.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.predef/default_random_engine
test_src := default_random_engine.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))