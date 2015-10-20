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
test_makefile := external/libcxx/test/depr/depr.c.headers/Android.mk

test_name := depr/depr.c.headers/stdarg_h
test_src := stdarg_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/string_h
test_src := string_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/ciso646
test_src := ciso646.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/tgmath_h
test_src := tgmath_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/time_h
test_src := time_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/stdlib_h
test_src := stdlib_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/wchar_h
test_src := wchar_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/ctype_h
test_src := ctype_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/iso646_h
test_src := iso646_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/errno_h
test_src := errno_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/float_h
test_src := float_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/locale_h
test_src := locale_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/inttypes_h
test_src := inttypes_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/wctype_h
test_src := wctype_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/complex.h
test_src := complex.h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/limits_h
test_src := limits_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/signal_h
test_src := signal_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/stdio_h
test_src := stdio_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/stdint_h
test_src := stdint_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/stdbool_h
test_src := stdbool_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/assert_h
test_src := assert_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/setjmp_h
test_src := setjmp_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/stddef_h
test_src := stddef_h.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.c.headers/fenv_h
test_src := fenv_h.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))
