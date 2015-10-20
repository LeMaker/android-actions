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
import getopt
import multiprocessing
import os
import re
import subprocess
import sys


class ProgressBarWrapper(object):
    def __init__(self, maxval):
        try:
            import progressbar
            self.pb = progressbar.ProgressBar(maxval=maxval)
        except ImportError:
            self.pb = None

    def start(self):
        if self.pb:
            self.pb.start()

    def update(self, value):
        if self.pb:
            self.pb.update(value)

    def finish(self):
        if self.pb:
            self.pb.finish()


class HostTest(object):
    def __init__(self, path):
        self.src_path = re.sub(r'\.pass\.cpp', '', path)
        self.name = '{0}'.format(self.src_path)
        self.path = '{0}/bin/libc++tests/{1}'.format(
            os.getenv('ANDROID_HOST_OUT'), self.name)

    def run(self):
        return subprocess.call(['timeout', '30', self.path],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)


class DeviceTest(object):
    def __init__(self, path):
        self.src_path = re.sub(r'\.pass\.cpp', '', path)
        self.name = '{0}'.format(self.src_path)
        self.path = '/system/bin/libc++tests/{0}'.format(self.name)

    def run(self):
        return adb_shell(self.path)


def adb_shell(command):
    proc = subprocess.Popen(['timeout', '30',
        'adb', 'shell', '{0}; echo $? 2>&1'.format(command)],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = proc.communicate()
    proc.wait()
    if proc.returncode:
        return proc.returncode
    out = [x for x in out.split('\r\n') if x]
    return int(out[-1])


def get_all_tests(subdir):
    tests = {'host': [], 'device': []}
    for path, _dirs, files in os.walk(subdir):
        path = os.path.normpath(path)
        if path == '.':
            path = ''
        for test in [t for t in files if t.endswith('.pass.cpp')]:
            tests['host'].append(HostTest(os.path.join(path, test)))
            tests['device'].append(DeviceTest(os.path.join(path, test)))
    return tests


def get_tests_in_subdirs(subdirs):
    tests = {'host': [], 'device': []}
    for subdir in subdirs:
        subdir_tests = get_all_tests(subdir=subdir)
        tests['host'].extend(subdir_tests['host'])
        tests['device'].extend(subdir_tests['device'])
    return tests


def run_tests(tests, num_threads):
    pb = ProgressBarWrapper(maxval=len(tests))
    pool = multiprocessing.Pool(num_threads)

    pb.start()
    results = pool.imap(pool_task, tests)
    num_run = {'host': 0, 'device': 0}
    failures = {'host': [], 'device': []}
    for name, status, target in results:
        num_run[target] += 1
        if status:
            failures[target].append(name)
        pb.update(sum(num_run.values()))
    pb.finish()
    return {'num_run': num_run, 'failures': failures}


def report_results(results):
    num_run = results['num_run']
    failures = results['failures']
    failed_both = sorted(filter(
        lambda x: x in failures['host'],
        failures['device']))

    for target, failed in failures.iteritems():
        failed = [x for x in failed if x not in failed_both]
        print '{0} tests run: {1}'.format(target, num_run[target])
        print '{0} tests failed: {1}'.format(target, len(failed))
        for failure in sorted(failed):
            print '\t{0}'.format(failure)
        print

    if len(failed_both):
        print '{0} tests failed in both environments'.format(len(failed_both))
        for failure in failed_both:
            print '\t{0}'.format(failure)


def pool_task(test):
    target = 'host' if isinstance(test, HostTest) else 'device'
    #print '{0} run {1}'.format(target, test.name)
    return (test.name, test.run(), target)


def main():
    try:
        opts, args = getopt.getopt(
                sys.argv[1:], 'n:t:', ['threads=', 'target='])
    except getopt.GetoptError as err:
        sys.exit(str(err))

    subdirs = ['.']
    target = 'both'
    num_threads = multiprocessing.cpu_count() * 2
    for opt, arg in opts:
        if opt in ('-n', '--threads'):
            num_threads = int(arg)
        elif opt in ('-t', '--target'):
            target = arg
        else:
            sys.exit('Unknown option {0}'.format(opt))

    if len(args):
        subdirs = args

    tests = get_tests_in_subdirs(subdirs)
    if target == 'both':
        tests = tests['host'] + tests['device']
    else:
        tests = tests[target]

    results = run_tests(tests, num_threads)
    report_results(results)


if __name__ == '__main__':
    main()
