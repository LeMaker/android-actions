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
test_makefile := external/libcxx/test/containers/container.adaptors/priority.queue/priqueue.cons/Android.mk

test_name := containers/container.adaptors/priority.queue/priqueue.cons/move_noexcept
test_src := move_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/priority.queue/priqueue.cons/ctor_copy
test_src := ctor_copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/priority.queue/priqueue.cons/default_noexcept
test_src := default_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/priority.queue/priqueue.cons/ctor_iter_iter_comp_cont
test_src := ctor_iter_iter_comp_cont.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/priority.queue/priqueue.cons/assign_copy
test_src := assign_copy.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/priority.queue/priqueue.cons/move_assign_noexcept
test_src := move_assign_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/priority.queue/priqueue.cons/ctor_comp
test_src := ctor_comp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/priority.queue/priqueue.cons/ctor_iter_iter
test_src := ctor_iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/priority.queue/priqueue.cons/ctor_iter_iter_comp
test_src := ctor_iter_iter_comp.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/priority.queue/priqueue.cons/ctor_comp_container
test_src := ctor_comp_container.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/priority.queue/priqueue.cons/ctor_move
test_src := ctor_move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/priority.queue/priqueue.cons/dtor_noexcept
test_src := dtor_noexcept.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/priority.queue/priqueue.cons/ctor_comp_rcontainer
test_src := ctor_comp_rcontainer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/priority.queue/priqueue.cons/assign_move
test_src := assign_move.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/priority.queue/priqueue.cons/ctor_default
test_src := ctor_default.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := containers/container.adaptors/priority.queue/priqueue.cons/ctor_iter_iter_comp_rcont
test_src := ctor_iter_iter_comp_rcont.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))