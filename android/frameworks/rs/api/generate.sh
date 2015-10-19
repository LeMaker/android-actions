#!/bin/bash
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

set -e
g++ gen_runtime.cpp -Wall -o gen_runtime
./gen_runtime -v 21 rs_core_math.spec
mv Test*.java ../../../cts/tests/tests/renderscript/src/android/renderscript/cts/
mv Test*.rs ../../../cts/tests/tests/renderscript/src/android/renderscript/cts/
mv rs_core_math.rsh ../scriptc/
rm ./gen_runtime
