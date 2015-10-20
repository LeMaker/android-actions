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
test_makefile := external/libcxx/test/numerics/rand/rand.adapt/rand.adapt.shuf/Android.mk

test_name := numerics/rand/rand.adapt/rand.adapt.shuf/default
test_src := default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.adapt/rand.adapt.shuf/discard
test_src := discard.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.adapt/rand.adapt.shuf/io
test_src := io.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.adapt/rand.adapt.shuf/result_type
test_src := result_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.adapt/rand.adapt.shuf/ctor_result_type
test_src := ctor_result_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.adapt/rand.adapt.shuf/eval
test_src := eval.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.adapt/rand.adapt.shuf/ctor_engine_move
test_src := ctor_engine_move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.adapt/rand.adapt.shuf/copy
test_src := copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.adapt/rand.adapt.shuf/assign
test_src := assign.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.adapt/rand.adapt.shuf/values
test_src := values.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.adapt/rand.adapt.shuf/ctor_sseq
test_src := ctor_sseq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.adapt/rand.adapt.shuf/seed_result_type
test_src := seed_result_type.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.adapt/rand.adapt.shuf/seed_sseq
test_src := seed_sseq.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := numerics/rand/rand.adapt/rand.adapt.shuf/ctor_engine_copy
test_src := ctor_engine_copy.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))