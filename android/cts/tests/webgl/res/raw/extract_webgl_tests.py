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

import sys
import os

if len(sys.argv) != 3:
  raise Exception("Usage: extract_webgl_tests.py <webgl_sdk_tests_path> <version>")

top_list = sys.argv[1] + "/00_test_list.txt"
version = sys.argv[2]
tests = []
lists = []
lists.append(top_list)

def filter_by_version(lines):
  version_lines = [ line for line in lines if "--min-version" in line ]
  version_lines.extend([ line for line in lines if "--max-version" in line ])
  lines = [ line for line in lines if not line in version_lines ]
  for line in version_lines:
    assert len(line.split()) == 3
    min_version = line.split()[1] if line.split()[0] == "--min-version" else "0.0.0"
    max_version = line.split()[1] if line.split()[0] == "--max-version" else "9.9.9"
    test = line.split()[2]
    if (version >= min_version and version <= max_version):
      lines.append(test)
  return lines

while not len(lists) == 0:
  lists2 = lists
  lists = []
  for list in lists2:
    directory = os.path.dirname(os.path.realpath(list))
    with open(list) as file:
      # Filter out comments and --min-version
      lines = [ line.strip() for line in file.readlines()]
      lines = [ line for line in lines if not "//" in line ]
      lines = [ line for line in lines if not "#" in line ]
      lines = [ line.replace("--slow","") for line in lines ]
      lines = filter_by_version(lines)
      # Append lists and tests found in this list.
      lines = [ directory + "/" + line for line in lines ]
      lists.extend([ line for line in lines if "00_test_list.txt" in line ])
      tests.extend([ line for line in lines if ".html" in line ])

# Directories for formating test-names/relative-paths.
name_directory = os.path.dirname(os.path.realpath(top_list))
path_directory = os.path.realpath(os.path.join(name_directory, os.pardir))

tests = sorted(tests)
for test in tests:
  test_path = test.replace(path_directory + "/", "")
  test_name = test.replace(name_directory + "/", "")
  test_name = test_name.replace("/","_")
  test_name = test_name.replace(".","_")
  test_name = test_name.replace("-","_")
  print "    public void test_" + test_name + "() throws Exception { doTest(\"" + test_path + "\"); }"

