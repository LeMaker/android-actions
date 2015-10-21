#!/bin/bash

if [ $# -ne 2 ]; then
echo "Usage: $0 <path_to_sdk_platforms.zip> <path_to_sdk_support.zip>"
exit 1
fi

# Make sure we are in prebuilts/sdk/current
if [ $(realpath $(dirname $0)) != $(realpath $(pwd)) ]; then
echo "The script must be run from $(dirname $0)."
exit 1
fi

set -x -e

rm -f android.jar uiautomator.jar framework.aidl
unzip -j $1 */android.jar */uiautomator.jar */framework.aidl

rm -rf support/
unzip $2 >/dev/null

# Remove duplicates
rm -f support/v7/appcompat/libs/android-support-v4.jar
rm -f support/multidex/instrumentation/libs/android-support-multidex.jar

# Remove samples
rm -rf support/samples

# Remove source files
find support -name "*.java" \
  -o -name "*.aidl" \
  -o -name AndroidManifest.xml \
  | xargs rm

# Other misc files we don't need
find support -name "*.gradle" \
  -o -name ".classpath" \
  -o -name ".project" \
  -o -name "project.properties" \
  -o -name "source.properties" \
  -o -name ".readme" \
  -o -name "README.txt" \
  -o -name "package.html" \
  -o -name "NOTICE.txt" \
  | xargs rm

# Now we can remove empty dirs
find . -type d -empty -delete
