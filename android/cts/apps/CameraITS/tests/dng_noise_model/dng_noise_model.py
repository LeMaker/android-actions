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

import its.device
import its.objects
import its.image
import pprint
import pylab
import os.path
import matplotlib
import matplotlib.pyplot
import numpy
import math

def main():
    """Compute the DNG noise model from a color checker chart.

    TODO: Make this more robust; some manual futzing may be needed.
    """
    NAME = os.path.basename(__file__).split(".")[0]

    with its.device.ItsSession() as cam:

        props = cam.get_camera_properties()

        white_level = float(props['android.sensor.info.whiteLevel'])
        black_levels = props['android.sensor.blackLevelPattern']
        idxs = its.image.get_canonical_cfa_order(props)
        black_levels = [black_levels[i] for i in idxs]

        # Expose for the scene with min sensitivity
        sens_min, sens_max = props['android.sensor.info.sensitivityRange']
        s_ae,e_ae,awb_gains,awb_ccm,_  = cam.do_3a(get_results=True)
        s_e_prod = s_ae * e_ae

        # Make the image brighter since the script looks at linear Bayer
        # raw patches rather than gamma-encoded YUV patches (and the AE
        # probably under-exposes a little for this use-case).
        s_e_prod *= 2

        # Capture raw frames across the full sensitivity range.
        NUM_SENS_STEPS = 9
        sens_step = int((sens_max - sens_min - 1) / float(NUM_SENS_STEPS))
        reqs = []
        sens = []
        for s in range(sens_min, sens_max, sens_step):
            e = int(s_e_prod / float(s))
            req = its.objects.manual_capture_request(s, e)
            req["android.colorCorrection.transform"] = \
                    its.objects.float_to_rational(awb_ccm)
            req["android.colorCorrection.gains"] = awb_gains
            reqs.append(req)
            sens.append(s)

        caps = cam.do_capture(reqs, cam.CAP_RAW)

        # A list of the (x,y) coords of the center pixel of a collection of
        # patches of a color checker chart. Each patch should be uniform,
        # however the actual color doesn't matter. Note that the coords are
        # relative to the *converted* RGB image, which is 1/2 x 1/2 of the
        # full size; convert back to full.
        img = its.image.convert_capture_to_rgb_image(caps[0], props=props)
        patches = its.image.get_color_checker_chart_patches(img, NAME+"_debug")
        patches = [(2*x,2*y) for (x,y) in sum(patches,[])]

        lines = []
        for iouter, (s,cap) in enumerate(zip(sens,caps)):
            # For each capture, compute the mean value in each patch, for each
            # Bayer plane; discard patches where pixels are close to clamped.
            # Also compute the variance.
            CLAMP_THRESH = 0.2
            planes = its.image.convert_capture_to_planes(cap, props)
            points = []
            for i,plane in enumerate(planes):
                plane = (plane * white_level - black_levels[i]) / (
                        white_level - black_levels[i])
                for j,(x,y) in enumerate(patches):
                    tile = plane[y/2-16:y/2+16:,x/2-16:x/2+16:,::]
                    mean = its.image.compute_image_means(tile)[0]
                    var = its.image.compute_image_variances(tile)[0]
                    if (mean > CLAMP_THRESH and mean < 1.0-CLAMP_THRESH):
                        # Each point is a (mean,variance) tuple for a patch;
                        # for a given ISO, there should be a linear
                        # relationship between these values.
                        points.append((mean,var))

            # Fit a line to the points, with a line equation: y = mx + b.
            # This line is the relationship between mean and variance (i.e.)
            # between signal level and noise, for this particular sensor.
            # In the DNG noise model, the gradient (m) is "S", and the offset
            # (b) is "O".
            points.sort()
            xs = [x for (x,y) in points]
            ys = [y for (x,y) in points]
            m,b = numpy.polyfit(xs, ys, 1)
            lines.append((s,m,b))
            print s, "->", m, b

            # TODO: Clean up these checks (which currently fail in some cases).
            # Some sanity checks:
            # * Noise levels should increase with brightness.
            # * Extrapolating to a black image, the noise should be positive.
            # Basically, the "b" value should correspond to the read noise,
            # which is the noise level if the sensor was operating in zero
            # light.
            #assert(m > 0)
            #assert(b >= 0)

            if iouter == 0:
                pylab.plot(xs, ys, 'r', label="Measured")
                pylab.plot([0,xs[-1]],[b,m*xs[-1]+b],'b', label="Fit")
            else:
                pylab.plot(xs, ys, 'r')
                pylab.plot([0,xs[-1]],[b,m*xs[-1]+b],'b')

        pylab.xlabel("Mean")
        pylab.ylabel("Variance")
        pylab.legend()
        matplotlib.pyplot.savefig("%s_plot_mean_vs_variance.png" % (NAME))

        # Now fit a line across the (m,b) line parameters for each sensitivity.
        # The gradient (m) params are fit to the "S" line, and the offset (b)
        # params are fit to the "O" line, both as a function of sensitivity.
        gains = [d[0] for d in lines]
        Ss = [d[1] for d in lines]
        Os = [d[2] for d in lines]
        mS,bS = numpy.polyfit(gains, Ss, 1)
        mO,bO = numpy.polyfit(gains, Os, 1)

        # Plot curve "O" as 10x, so it fits in the same scale as curve "S".
        fig = matplotlib.pyplot.figure()
        pylab.plot(gains, [10*o for o in Os], 'r', label="Measured")
        pylab.plot([gains[0],gains[-1]],
                [10*mO*gains[0]+10*bO, 10*mO*gains[-1]+10*bO],'r--',label="Fit")
        pylab.plot(gains, Ss, 'b', label="Measured")
        pylab.plot([gains[0],gains[-1]], [mS*gains[0]+bS,mS*gains[-1]+bS],'b--',
                label="Fit")
        pylab.xlabel("Sensitivity")
        pylab.ylabel("Model parameter: S (blue), O x10 (red)")
        pylab.legend()
        matplotlib.pyplot.savefig("%s_plot_S_O.png" % (NAME))

        print """
        /* Generated test code to dump a table of data for external validation
         * of the noise model parameters.
         */
        #include <stdio.h>
        #include <assert.h>
        double compute_noise_model_entry_S(int sens);
        double compute_noise_model_entry_O(int sens);
        int main(void) {
            int sens;
            for (sens = %d; sens <= %d; sens += 100) {
                double o = compute_noise_model_entry_O(sens);
                double s = compute_noise_model_entry_S(sens);
                printf("%%d,%%lf,%%lf\\n", sens, o, s);
            }
            return 0;
        }

        /* Generated functions to map a given sensitivity to the O and S noise
         * model parameters in the DNG noise model.
         */
        double compute_noise_model_entry_S(int sens) {
            double s = %e * sens + %e;
            return s < 0.0 ? 0.0 : s;
        }
        double compute_noise_model_entry_O(int sens) {
            double o = %e * sens + %e;
            return o < 0.0 ? 0.0 : o;
        }
        """%(sens_min,sens_max,mS,bS,mO,bO)

if __name__ == '__main__':
    main()

