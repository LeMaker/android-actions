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
test_makefile := external/libcxx/test/numerics/rand/rand.eng/rand.eng.sub/Android.mk

test_name := numerics/rand/rand.eng/rand.eng.sub/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.eng/rand.eng.sub/discard
test_src := discard.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.eng/rand.eng.sub/io
test_src := io.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.eng/rand.eng.sub/result_type
test_src := result_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.eng/rand.eng.sub/ctor_result_type
test_src := ctor_result_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.eng/rand.eng.sub/eval
test_src := eval.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.eng/rand.eng.sub/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.eng/rand.eng.sub/assign
test_src := assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.eng/rand.eng.sub/values
test_src := values.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.eng/rand.eng.sub/ctor_sseq
test_src := ctor_sseq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.eng/rand.eng.sub/seed_result_type
test_src := seed_result_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.eng/rand.eng.sub/seed_sseq
test_src := seed_sseq.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))