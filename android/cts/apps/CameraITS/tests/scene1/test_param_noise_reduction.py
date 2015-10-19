# Copyright 2013 The Android Open Source Project
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
import its.caps
import its.device
import its.objects
import its.target
import pylab
import os.path
import matplotlib
import matplotlib.pyplot

def main():
    """Test that the android.noiseReduction.mode param is applied when set.

    Capture images with the camera dimly lit. Uses a high analog gain to
    ensure the captured image is noisy.

    Captures three images, for NR off, "fast", and "high quality".
    Also captures an image with low gain and NR off, and uses the variance
    of this as the baseline.
    """
    NAME = os.path.basename(__file__).split(".")[0]

    # List of variances for Y,U,V.
    variances = [[],[],[]]

    # Reference (baseline) variance for each of Y,U,V.
    ref_variance = []

    nr_modes_reported = []

    with its.device.ItsSession() as cam:
        props = cam.get_camera_properties()
        its.caps.skip_unless(its.caps.compute_target_exposure(props) and
                             its.caps.per_frame_control(props))

        # NR mode 0 with low gain
        e, s = its.target.get_target_exposure_combos(cam)["minSensitivity"]
        req = its.objects.manual_capture_request(s, e)
        req["android.noiseReduction.mode"] = 0
        cap = cam.do_capture(req)
        its.image.write_image(
                its.image.convert_capture_to_rgb_image(cap),
                "%s_low_gain.jpg" % (NAME))
        planes = its.image.convert_capture_to_planes(cap)
        for j in range(3):
            img = planes[j]
            tile = its.image.get_image_patch(img, 0.45, 0.45, 0.1, 0.1)
            ref_variance.append(its.image.compute_image_variances(tile)[0])
        print "Ref variances:", ref_variance

        for i in range(3):
            # NR modes 0, 1, 2 with high gain
            e, s = its.target.get_target_exposure_combos(cam)["maxSensitivity"]
            req = its.objects.manual_capture_request(s, e)
            req["android.noiseReduction.mode"] = i
            cap = cam.do_capture(req)
            nr_modes_reported.append(
                    cap["metadata"]["android.noiseReduction.mode"])
            its.image.write_image(
                    its.image.convert_capture_to_rgb_image(cap),
                    "%s_high_gain_nr=%d.jpg" % (NAME, i))
            planes = its.image.convert_capture_to_planes(cap)
            for j in range(3):
                img = planes[j]
                tile = its.image.get_image_patch(img, 0.45, 0.45, 0.1, 0.1)
                variance = its.image.compute_image_variances(tile)[0]
                variances[j].append(variance / ref_variance[j])
        print "Variances with NR mode [0,1,2]:", variances

    # Draw a plot.
    for j in range(3):
        pylab.plot(range(3), variances[j], "rgb"[j])
    matplotlib.pyplot.savefig("%s_plot_variances.png" % (NAME))

    assert(nr_modes_reported == [0,1,2])

    # Check that the variance of the NR=0 image is higher than for the
    # NR=1 and NR=2 images.
    for j in range(3):
        for i in range(1,3):
            assert(variances[j][i] < variances[j][0])

if __name__ == '__main__':
    main()

