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
test_makefile := external/libcxx/test/language.support/support.limits/limits/numeric.limits.members/Android.mk

test_name := language.support/support.limits/limits/numeric.limits.members/const_data_members
test_src := const_data_members.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/epsilon
test_src := epsilon.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/has_denorm_loss
test_src := has_denorm_loss.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/max_exponent10
test_src := max_exponent10.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/round_style
test_src := round_style.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/signaling_NaN
test_src := signaling_NaN.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/lowest
test_src := lowest.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/is_iec559
test_src := is_iec559.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/is_exact
test_src := is_exact.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/traps
test_src := traps.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/is_integer
test_src := is_integer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/is_signed
test_src := is_signed.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/min
test_src := min.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/tinyness_before
test_src := tinyness_before.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/has_signaling_NaN
test_src := has_signaling_NaN.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/has_infinity
test_src := has_infinity.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/digits
test_src := digits.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/radix
test_src := radix.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/max_digits10
test_src := max_digits10.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/round_error
test_src := round_error.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/digits10
test_src := digits10.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/is_modulo
test_src := is_modulo.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/max_exponent
test_src := max_exponent.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/has_denorm
test_src := has_denorm.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/is_bounded
test_src := is_bounded.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/min_exponent
test_src := min_exponent.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/max
test_src := max.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/quiet_NaN
test_src := quiet_NaN.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/min_exponent10
test_src := min_exponent10.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/infinity
test_src := infinity.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/denorm_min
test_src := denorm_min.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := language.support/support.limits/limits/numeric.limits.members/has_quiet_NaN
test_src := has_quiet_NaN.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))