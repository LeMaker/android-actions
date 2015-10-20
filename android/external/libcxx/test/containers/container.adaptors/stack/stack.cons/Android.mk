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
test_makefile := external/libcxx/test/containers/container.adaptors/stack/stack.cons/Android.mk

test_name := containers/container.adaptors/stack/stack.cons/move_noexcept
test_src := move_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.cons/ctor_copy
test_src := ctor_copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.cons/ctor_container
test_src := ctor_container.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.cons/default_noexcept
test_src := default_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.cons/move_assign_noexcept
test_src := move_assign_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.cons/ctor_rcontainer
test_src := ctor_rcontainer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.cons/ctor_move
test_src := ctor_move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.cons/dtor_noexcept
test_src := dtor_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/stack/stack.cons/ctor_default
test_src := ctor_default.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))