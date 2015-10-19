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

# The current version code scheme for the package apk is:
#      Mbbbba
# where
#    M - Chromium milestone number minus 36. (1 or more digits). This is to
#        ensure that our version codes are lower than the ones currently used by
#        Chrome for Android, so that we can adopt their scheme in future.
# bbbb - manually specified build number (exactly 4 digits). This defaults to
#        0000, or 9999 in local builds.
#    a - device architecture (exactly 1 digit). Current valid values are:
#           1 : armv7-a
#           2 : armv8-a (arm64)
#           4 : mips
#           7 : x86
#           8 : x86_64
#        64-bit architectures must be higher than their corresponding 32-bit
#        architectures to ensure that 64-bit devices receive the multiarch APK.
#        x86 must be higher than ARM to avoid x86 devices receiving ARM APKs
#        and running them in emulation.

# TODO(torne): get this from Chromium automatically.
version_milestone := 37
version_offset_milestone := $(shell echo $(version_milestone) \
                              | awk '{print $$1 - 36}')

ifneq "" "$(filter eng.%,$(BUILD_NUMBER))"
  version_build_number := 9999
  # BUILD_NUMBER has a timestamp in it, which means that
  # it will change every time. Pick a stable value.
  version_name_tag := eng.$(USER)
else
  ifeq "$(version_build_number)" ""
    version_build_number := 0000
  endif
  version_name_tag := $(BUILD_NUMBER)
endif

ifeq "$(TARGET_ARCH)" "x86_64"
  version_arch := 8
else ifeq "$(TARGET_ARCH)" "x86"
  version_arch := 7
else ifeq "$(TARGET_ARCH)" "mips"
  version_arch := 4
else ifeq "$(TARGET_ARCH)" "arm64"
  version_arch := 2
else ifeq "$(TARGET_ARCH)" "arm"
  version_arch := 1
else
  version_arch := 0
  $(warning Could not determine target architecture for versioning)
endif

version_code := $(version_offset_milestone)$(version_build_number)$(version_arch)

# Use the milestone, build number and architecture to construct a version
# name like "37 (1424323-arm64)".
# TODO(torne): get the full version number from Chromium.
version_name := $(version_milestone) ($(version_name_tag)-$(TARGET_ARCH))

# Clean up locals
version_milestone :=
version_build_number :=
version_name_tag :=
version_arch :=
