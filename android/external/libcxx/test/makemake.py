# Copyright (C) 2014 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#            http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import os


def main():
    build_mk_path = 'external/libcxx/test/Android.build.mk'
    for root, _dirs, files in os.walk('.'):
        tests = []
        for test in [x for x in files if x.endswith('.pass.cpp')]:
            path = os.path.join(root, test)
            out_name = os.path.splitext(path)[0]    # trim .cpp
            out_name = os.path.splitext(out_name)[0]    # trim .pass
            out_name = os.path.normpath(out_name)
            tests.append((test, out_name))
        with open(os.path.join(root, 'Android.mk'), 'w') as makefile:
            makefile_path = os.path.normpath(os.path.join(
                    'external/libcxx/test/', root, 'Android.mk'))
            makefile.write('''#
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
''')
            makefile.write('LOCAL_PATH := $(call my-dir)\n')
            makefile.write('test_makefile := {}\n'.format(makefile_path))
            if len(tests) > 0:
                for test, out_name in tests:
                    makefile.write('''
test_name := {}
test_src := {}
include {}
'''.format(out_name, test, build_mk_path))
            makefile.write(
                    '\ninclude $(call all-makefiles-under,$(LOCAL_PATH))')


if __name__ == '__main__':
    main()
