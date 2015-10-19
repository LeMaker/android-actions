# Copyright 2014 The Android Open Source Project
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

import its.image
import its.device
import its.objects
import os.path

def main():
    """Test face detection.
    """
    NAME = os.path.basename(__file__).split(".")[0]

    with its.device.ItsSession() as cam:
        cam.do_3a()
        req = its.objects.auto_capture_request()
        req['android.statistics.faceDetectMode'] = 2
        caps = cam.do_capture([req]*5)
        for i,cap in enumerate(caps):
            md = cap['metadata']
            print "Frame %d face metadata:" % i
            print "  Ids:", md['android.statistics.faceIds']
            print "  Landmarks:", md['android.statistics.faceLandmarks']
            print "  Rectangles:", md['android.statistics.faceRectangles']
            print "  Scores:", md['android.statistics.faceScores']
            print ""

if __name__ == '__main__':
    main()

