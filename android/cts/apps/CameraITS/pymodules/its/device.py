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

import its.error
import os
import os.path
import sys
import re
import json
import time
import unittest
import socket
import subprocess
import hashlib
import numpy

class ItsSession(object):
    """Controls a device over adb to run ITS scripts.

    The script importing this module (on the host machine) prepares JSON
    objects encoding CaptureRequests, specifying sets of parameters to use
    when capturing an image using the Camera2 APIs. This class encapsulates
    sending the requests to the device, monitoring the device's progress, and
    copying the resultant captures back to the host machine when done. TCP
    forwarded over adb is the transport mechanism used.

    The device must have CtsVerifier.apk installed.

    Attributes:
        sock: The open socket.
    """

    # Open a connection to localhost:6000, forwarded to port 6000 on the device.
    # TODO: Support multiple devices running over different TCP ports.
    IPADDR = '127.0.0.1'
    PORT = 6000
    BUFFER_SIZE = 4096

    # Seconds timeout on each socket operation.
    SOCK_TIMEOUT = 10.0

    PACKAGE = 'com.android.cts.verifier.camera.its'
    INTENT_START = 'com.android.cts.verifier.camera.its.START'
    ACTION_ITS_RESULT = 'com.android.cts.verifier.camera.its.ACTION_ITS_RESULT'
    EXTRA_CAMERA_ID = 'camera.its.extra.CAMERA_ID'
    EXTRA_SUCCESS = 'camera.its.extra.SUCCESS'
    EXTRA_SUMMARY = 'camera.its.extra.SUMMARY'

    # TODO: Handle multiple connected devices.
    ADB = "adb -d"

    # Definitions for some of the common output format options for do_capture().
    # Each gets images of full resolution for each requested format.
    CAP_RAW = {"format":"raw"}
    CAP_DNG = {"format":"dng"}
    CAP_YUV = {"format":"yuv"}
    CAP_JPEG = {"format":"jpeg"}
    CAP_RAW_YUV = [{"format":"raw"}, {"format":"yuv"}]
    CAP_DNG_YUV = [{"format":"dng"}, {"format":"yuv"}]
    CAP_RAW_JPEG = [{"format":"raw"}, {"format":"jpeg"}]
    CAP_DNG_JPEG = [{"format":"dng"}, {"format":"jpeg"}]
    CAP_YUV_JPEG = [{"format":"yuv"}, {"format":"jpeg"}]
    CAP_RAW_YUV_JPEG = [{"format":"raw"}, {"format":"yuv"}, {"format":"jpeg"}]
    CAP_DNG_YUV_JPEG = [{"format":"dng"}, {"format":"yuv"}, {"format":"jpeg"}]

    # Method to handle the case where the service isn't already running.
    # This occurs when a test is invoked directly from the command line, rather
    # than as a part of a separate test harness which is setting up the device
    # and the TCP forwarding.
    def __pre_init(self):

        # This also includes the optional reboot handling: if the user
        # provides a "reboot" or "reboot=N" arg, then reboot the device,
        # waiting for N seconds (default 30) before returning.
        for s in sys.argv[1:]:
            if s[:6] == "reboot":
                duration = 30
                if len(s) > 7 and s[6] == "=":
                    duration = int(s[7:])
                print "Rebooting device"
                _run("%s reboot" % (ItsSession.ADB));
                _run("%s wait-for-device" % (ItsSession.ADB))
                time.sleep(duration)
                print "Reboot complete"

        # TODO: Figure out why "--user 0" is needed, and fix the problem.
        _run('%s shell am force-stop --user 0 %s' % (ItsSession.ADB, self.PACKAGE))
        _run(('%s shell am startservice --user 0 -t text/plain '
              '-a %s') % (ItsSession.ADB, self.INTENT_START))

        # Wait until the socket is ready to accept a connection.
        proc = subprocess.Popen(
                ItsSession.ADB.split() + ["logcat"],
                stdout=subprocess.PIPE)
        logcat = proc.stdout
        while True:
            line = logcat.readline().strip()
            if line.find('ItsService ready') >= 0:
                break
        proc.kill()

        # Setup the TCP-over-ADB forwarding.
        _run('%s forward tcp:%d tcp:%d' % (ItsSession.ADB,self.PORT,self.PORT))

    def __init__(self):
        if "noinit" not in sys.argv:
            self.__pre_init()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.IPADDR, self.PORT))
        self.sock.settimeout(self.SOCK_TIMEOUT)
        self.__close_camera()
        self.__open_camera()

    def __del__(self):
        if hasattr(self, 'sock') and self.sock:
            self.__close_camera()
            self.sock.close()

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        return False

    def __read_response_from_socket(self):
        # Read a line (newline-terminated) string serialization of JSON object.
        chars = []
        while len(chars) == 0 or chars[-1] != '\n':
            ch = self.sock.recv(1)
            if len(ch) == 0:
                # Socket was probably closed; otherwise don't get empty strings
                raise its.error.Error('Problem with socket on device side')
            chars.append(ch)
        line = ''.join(chars)
        jobj = json.loads(line)
        # Optionally read a binary buffer of a fixed size.
        buf = None
        if jobj.has_key("bufValueSize"):
            n = jobj["bufValueSize"]
            buf = bytearray(n)
            view = memoryview(buf)
            while n > 0:
                nbytes = self.sock.recv_into(view, n)
                view = view[nbytes:]
                n -= nbytes
            buf = numpy.frombuffer(buf, dtype=numpy.uint8)
        return jobj, buf

    def __open_camera(self):
        # Get the camera ID to open as an argument.
        camera_id = 0
        for s in sys.argv[1:]:
            if s[:7] == "camera=" and len(s) > 7:
                camera_id = int(s[7:])
        cmd = {"cmdName":"open", "cameraId":camera_id}
        self.sock.send(json.dumps(cmd) + "\n")
        data,_ = self.__read_response_from_socket()
        if data['tag'] != 'cameraOpened':
            raise its.error.Error('Invalid command response')

    def __close_camera(self):
        cmd = {"cmdName":"close"}
        self.sock.send(json.dumps(cmd) + "\n")
        data,_ = self.__read_response_from_socket()
        if data['tag'] != 'cameraClosed':
            raise its.error.Error('Invalid command response')

    def do_vibrate(self, pattern):
        """Cause the device to vibrate to a specific pattern.

        Args:
            pattern: Durations (ms) for which to turn on or off the vibrator.
                The first value indicates the number of milliseconds to wait
                before turning the vibrator on. The next value indicates the
                number of milliseconds for which to keep the vibrator on
                before turning it off. Subsequent values alternate between
                durations in milliseconds to turn the vibrator off or to turn
                the vibrator on.

        Returns:
            Nothing.
        """
        cmd = {}
        cmd["cmdName"] = "doVibrate"
        cmd["pattern"] = pattern
        self.sock.send(json.dumps(cmd) + "\n")
        data,_ = self.__read_response_from_socket()
        if data['tag'] != 'vibrationStarted':
            raise its.error.Error('Invalid command response')

    def start_sensor_events(self):
        """Start collecting sensor events on the device.

        See get_sensor_events for more info.

        Returns:
            Nothing.
        """
        cmd = {}
        cmd["cmdName"] = "startSensorEvents"
        self.sock.send(json.dumps(cmd) + "\n")
        data,_ = self.__read_response_from_socket()
        if data['tag'] != 'sensorEventsStarted':
            raise its.error.Error('Invalid command response')

    def get_sensor_events(self):
        """Get a trace of all sensor events on the device.

        The trace starts when the start_sensor_events function is called. If
        the test runs for a long time after this call, then the device's
        internal memory can fill up. Calling get_sensor_events gets all events
        from the device, and then stops the device from collecting events and
        clears the internal buffer; to start again, the start_sensor_events
        call must be used again.

        Events from the accelerometer, compass, and gyro are returned; each
        has a timestamp and x,y,z values.

        Note that sensor events are only produced if the device isn't in its
        standby mode (i.e.) if the screen is on.

        Returns:
            A Python dictionary with three keys ("accel", "mag", "gyro") each
            of which maps to a list of objects containing "time","x","y","z"
            keys.
        """
        cmd = {}
        cmd["cmdName"] = "getSensorEvents"
        self.sock.send(json.dumps(cmd) + "\n")
        data,_ = self.__read_response_from_socket()
        if data['tag'] != 'sensorEvents':
            raise its.error.Error('Invalid command response')
        return data['objValue']

    def get_camera_ids(self):
        """Get a list of camera device Ids that can be opened.

        Returns:
            a list of camera ID string
        """
        cmd = {}
        cmd["cmdName"] = "getCameraIds"
        self.sock.send(json.dumps(cmd) + "\n")
        data,_ = self.__read_response_from_socket()
        if data['tag'] != 'cameraIds':
            raise its.error.Error('Invalid command response')
        return data['objValue']['cameraIdArray']

    def get_camera_properties(self):
        """Get the camera properties object for the device.

        Returns:
            The Python dictionary object for the CameraProperties object.
        """
        cmd = {}
        cmd["cmdName"] = "getCameraProperties"
        self.sock.send(json.dumps(cmd) + "\n")
        data,_ = self.__read_response_from_socket()
        if data['tag'] != 'cameraProperties':
            raise its.error.Error('Invalid command response')
        return data['objValue']['cameraProperties']

    def do_3a(self, regions_ae=[[0,0,1,1,1]],
                    regions_awb=[[0,0,1,1,1]],
                    regions_af=[[0,0,1,1,1]],
                    do_ae=True, do_awb=True, do_af=True,
                    lock_ae=False, lock_awb=False,
                    get_results=False,
                    ev_comp=0):
        """Perform a 3A operation on the device.

        Triggers some or all of AE, AWB, and AF, and returns once they have
        converged. Uses the vendor 3A that is implemented inside the HAL.

        Throws an assertion if 3A fails to converge.

        Args:
            regions_ae: List of weighted AE regions.
            regions_awb: List of weighted AWB regions.
            regions_af: List of weighted AF regions.
            do_ae: Trigger AE and wait for it to converge.
            do_awb: Wait for AWB to converge.
            do_af: Trigger AF and wait for it to converge.
            lock_ae: Request AE lock after convergence, and wait for it.
            lock_awb: Request AWB lock after convergence, and wait for it.
            get_results: Return the 3A results from this function.
            ev_comp: An EV compensation value to use when running AE.

        Region format in args:
            Arguments are lists of weighted regions; each weighted region is a
            list of 5 values, [x,y,w,h, wgt], and each argument is a list of
            these 5-value lists. The coordinates are given as normalized
            rectangles (x,y,w,h) specifying the region. For example:
                [[0.0, 0.0, 1.0, 0.5, 5], [0.0, 0.5, 1.0, 0.5, 10]].
            Weights are non-negative integers.

        Returns:
            Five values are returned if get_results is true::
            * AE sensitivity; None if do_ae is False
            * AE exposure time; None if do_ae is False
            * AWB gains (list); None if do_awb is False
            * AWB transform (list); None if do_awb is false
            * AF focus position; None if do_af is false
            Otherwise, it returns five None values.
        """
        print "Running vendor 3A on device"
        cmd = {}
        cmd["cmdName"] = "do3A"
        cmd["regions"] = {"ae": sum(regions_ae, []),
                          "awb": sum(regions_awb, []),
                          "af": sum(regions_af, [])}
        cmd["triggers"] = {"ae": do_ae, "af": do_af}
        if lock_ae:
            cmd["aeLock"] = True
        if lock_awb:
            cmd["awbLock"] = True
        if ev_comp != 0:
            cmd["evComp"] = ev_comp
        self.sock.send(json.dumps(cmd) + "\n")

        # Wait for each specified 3A to converge.
        ae_sens = None
        ae_exp = None
        awb_gains = None
        awb_transform = None
        af_dist = None
        converged = False
        while True:
            data,_ = self.__read_response_from_socket()
            vals = data['strValue'].split()
            if data['tag'] == 'aeResult':
                ae_sens, ae_exp = [int(i) for i in vals]
            elif data['tag'] == 'afResult':
                af_dist = float(vals[0])
            elif data['tag'] == 'awbResult':
                awb_gains = [float(f) for f in vals[:4]]
                awb_transform = [float(f) for f in vals[4:]]
            elif data['tag'] == '3aConverged':
                converged = True
            elif data['tag'] == '3aDone':
                break
            else:
                raise its.error.Error('Invalid command response')
        if converged and not get_results:
            return None,None,None,None,None
        if (do_ae and ae_sens == None or do_awb and awb_gains == None
                or do_af and af_dist == None or not converged):
            raise its.error.Error('3A failed to converge')
        return ae_sens, ae_exp, awb_gains, awb_transform, af_dist

    def do_capture(self, cap_request, out_surfaces=None):
        """Issue capture request(s), and read back the image(s) and metadata.

        The main top-level function for capturing one or more images using the
        device. Captures a single image if cap_request is a single object, and
        captures a burst if it is a list of objects.

        The out_surfaces field can specify the width(s), height(s), and
        format(s) of the captured image. The formats may be "yuv", "jpeg",
        "dng", "raw", or "raw10". The default is a YUV420 frame ("yuv")
        corresponding to a full sensor frame.

        Note that one or more surfaces can be specified, allowing a capture to
        request images back in multiple formats (e.g.) raw+yuv, raw+jpeg,
        yuv+jpeg, raw+yuv+jpeg. If the size is omitted for a surface, the
        default is the largest resolution available for the format of that
        surface. At most one output surface can be specified for a given format,
        and raw+dng, raw10+dng, and raw+raw10 are not supported as combinations.

        Example of a single capture request:

            {
                "android.sensor.exposureTime": 100*1000*1000,
                "android.sensor.sensitivity": 100
            }

        Example of a list of capture requests:

            [
                {
                    "android.sensor.exposureTime": 100*1000*1000,
                    "android.sensor.sensitivity": 100
                },
                {
                    "android.sensor.exposureTime": 100*1000*1000,
                    "android.sensor.sensitivity": 200
                }
            ]

        Examples of output surface specifications:

            {
                "width": 640,
                "height": 480,
                "format": "yuv"
            }

            [
                {
                    "format": "jpeg"
                },
                {
                    "format": "raw"
                }
            ]

        The following variables defined in this class are shortcuts for
        specifying one or more formats where each output is the full size for
        that format; they can be used as values for the out_surfaces arguments:

            CAP_RAW
            CAP_DNG
            CAP_YUV
            CAP_JPEG
            CAP_RAW_YUV
            CAP_DNG_YUV
            CAP_RAW_JPEG
            CAP_DNG_JPEG
            CAP_YUV_JPEG
            CAP_RAW_YUV_JPEG
            CAP_DNG_YUV_JPEG

        If multiple formats are specified, then this function returns multiple
        capture objects, one for each requested format. If multiple formats and
        multiple captures (i.e. a burst) are specified, then this function
        returns multiple lists of capture objects. In both cases, the order of
        the returned objects matches the order of the requested formats in the
        out_surfaces parameter. For example:

            yuv_cap            = do_capture( req1                           )
            yuv_cap            = do_capture( req1,        yuv_fmt           )
            yuv_cap,  raw_cap  = do_capture( req1,        [yuv_fmt,raw_fmt] )
            yuv_caps           = do_capture( [req1,req2], yuv_fmt           )
            yuv_caps, raw_caps = do_capture( [req1,req2], [yuv_fmt,raw_fmt] )

        Args:
            cap_request: The Python dict/list specifying the capture(s), which
                will be converted to JSON and sent to the device.
            out_surfaces: (Optional) specifications of the output image formats
                and sizes to use for each capture.

        Returns:
            An object, list of objects, or list of lists of objects, where each
            object contains the following fields:
            * data: the image data as a numpy array of bytes.
            * width: the width of the captured image.
            * height: the height of the captured image.
            * format: image the format, in ["yuv","jpeg","raw","raw10","dng"].
            * metadata: the capture result object (Python dictionary).
        """
        cmd = {}
        cmd["cmdName"] = "doCapture"
        if not isinstance(cap_request, list):
            cmd["captureRequests"] = [cap_request]
        else:
            cmd["captureRequests"] = cap_request
        if out_surfaces is not None:
            if not isinstance(out_surfaces, list):
                cmd["outputSurfaces"] = [out_surfaces]
            else:
                cmd["outputSurfaces"] = out_surfaces
            formats = [c["format"] if c.has_key("format") else "yuv"
                       for c in cmd["outputSurfaces"]]
            formats = [s if s != "jpg" else "jpeg" for s in formats]
        else:
            formats = ['yuv']
        ncap = len(cmd["captureRequests"])
        nsurf = 1 if out_surfaces is None else len(cmd["outputSurfaces"])
        if len(formats) > len(set(formats)):
            raise its.error.Error('Duplicate format requested')
        if "dng" in formats and "raw" in formats or \
                "dng" in formats and "raw10" in formats or \
                "raw" in formats and "raw10" in formats:
            raise its.error.Error('Different raw formats not supported')
        print "Capturing %d frame%s with %d format%s [%s]" % (
                  ncap, "s" if ncap>1 else "", nsurf, "s" if nsurf>1 else "",
                  ",".join(formats))
        self.sock.send(json.dumps(cmd) + "\n")

        # Wait for ncap*nsurf images and ncap metadata responses.
        # Assume that captures come out in the same order as requested in
        # the burst, however individual images of different formats can come
        # out in any order for that capture.
        nbufs = 0
        bufs = {"yuv":[], "raw":[], "raw10":[], "dng":[], "jpeg":[]}
        mds = []
        widths = None
        heights = None
        while nbufs < ncap*nsurf or len(mds) < ncap:
            jsonObj,buf = self.__read_response_from_socket()
            if jsonObj['tag'] in ['jpegImage', 'yuvImage', 'rawImage', \
                    'raw10Image', 'dngImage'] and buf is not None:
                fmt = jsonObj['tag'][:-5]
                bufs[fmt].append(buf)
                nbufs += 1
            elif jsonObj['tag'] == 'captureResults':
                mds.append(jsonObj['objValue']['captureResult'])
                outputs = jsonObj['objValue']['outputs']
                widths = [out['width'] for out in outputs]
                heights = [out['height'] for out in outputs]
            else:
                # Just ignore other tags
                None
        rets = []
        for j,fmt in enumerate(formats):
            objs = []
            for i in range(ncap):
                obj = {}
                obj["data"] = bufs[fmt][i]
                obj["width"] = widths[j]
                obj["height"] = heights[j]
                obj["format"] = fmt
                obj["metadata"] = mds[i]
                objs.append(obj)
            rets.append(objs if ncap>1 else objs[0])
        return rets if len(rets)>1 else rets[0]

def report_result(camera_id, success, summary_path=None):
    """Send a pass/fail result to the device, via an intent.

    Args:
        camera_id: The ID string of the camera for which to report pass/fail.
        success: Boolean, indicating if the result was pass or fail.
        summary_path: (Optional) path to ITS summary file on host PC

    Returns:
        Nothing.
    """
    device_summary_path = "/sdcard/camera_" + camera_id + "_its_summary.txt"
    if summary_path is not None:
        _run("%s push %s %s" % (
                ItsSession.ADB, summary_path, device_summary_path))
        _run("%s shell am broadcast -a %s --es %s %s --es %s %s --es %s %s" % (
                ItsSession.ADB, ItsSession.ACTION_ITS_RESULT,
                ItsSession.EXTRA_CAMERA_ID, camera_id,
                ItsSession.EXTRA_SUCCESS, 'True' if success else 'False',
                ItsSession.EXTRA_SUMMARY, device_summary_path))
    else:
        _run("%s shell am broadcast -a %s --es %s %s --es %s %s --es %s %s" % (
                ItsSession.ADB, ItsSession.ACTION_ITS_RESULT,
                ItsSession.EXTRA_CAMERA_ID, camera_id,
                ItsSession.EXTRA_SUCCESS, 'True' if success else 'False',
                ItsSession.EXTRA_SUMMARY, "null"))

def _run(cmd):
    """Replacement for os.system, with hiding of stdout+stderr messages.
    """
    with open(os.devnull, 'wb') as devnull:
        subprocess.check_call(
                cmd.split(), stdout=devnull, stderr=subprocess.STDOUT)

class __UnitTest(unittest.TestCase):
    """Run a suite of unit tests on this module.
    """

    # TODO: Add some unit tests.
    None

if __name__ == '__main__':
    unittest.main()

