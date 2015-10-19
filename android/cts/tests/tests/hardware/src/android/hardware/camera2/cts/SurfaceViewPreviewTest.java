/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.camera2.cts;

import static android.hardware.camera2.cts.CameraTestUtils.*;

import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCaptureSession.CaptureCallback;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CaptureFailure;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.TotalCaptureResult;
import android.util.Size;
import android.hardware.camera2.cts.CameraTestUtils.SimpleCaptureCallback;
import android.hardware.camera2.cts.testcases.Camera2SurfaceViewTestCase;
import android.util.Log;
import android.util.Range;

import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatcher;

import static org.mockito.Mockito.*;

import java.util.Arrays;
import java.util.List;

/**
 * CameraDevice preview test by using SurfaceView.
 */
public class SurfaceViewPreviewTest extends Camera2SurfaceViewTestCase {
    private static final String TAG = "SurfaceViewPreviewTest";
    private static final boolean VERBOSE = Log.isLoggable(TAG, Log.VERBOSE);
    private static final int FRAME_TIMEOUT_MS = 1000;
    private static final int NUM_FRAMES_VERIFIED = 30;
    private static final int NUM_TEST_PATTERN_FRAMES_VERIFIED = 60;
    private static final float FRAME_DURATION_ERROR_MARGIN = 0.005f; // 0.5 percent error margin.

    @Override
    protected void setUp() throws Exception {
        super.setUp();
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
    }

    /**
     * Test all supported preview sizes for each camera device.
     * <p>
     * For the first  {@link #NUM_FRAMES_VERIFIED}  of capture results,
     * the {@link CaptureCallback} callback availability and the capture timestamp
     * (monotonically increasing) ordering are verified.
     * </p>
     */
    public void testCameraPreview() throws Exception {
        for (int i = 0; i < mCameraIds.length; i++) {
            try {
                Log.i(TAG, "Testing preview for Camera " + mCameraIds[i]);
                openDevice(mCameraIds[i]);

                previewTestByCamera();
            } finally {
                closeDevice();
            }
        }
    }

    /**
     * Basic test pattern mode preview.
     * <p>
     * Only test the test pattern preview and capture result, the image buffer
     * is not validated.
     * </p>
     */
    public void testBasicTestPatternPreview() throws Exception{
        for (int i = 0; i < mCameraIds.length; i++) {
            try {
                Log.i(TAG, "Testing preview for Camera " + mCameraIds[i]);
                openDevice(mCameraIds[i]);

                previewTestPatternTestByCamera();
            } finally {
                closeDevice();
            }
        }
    }

    /**
     * Test {@link CaptureRequest#CONTROL_AE_TARGET_FPS_RANGE} for preview, validate the preview
     * frame duration and exposure time.
     */
    public void testPreviewFpsRange() throws Exception {
        for (String id : mCameraIds) {
            try {
                openDevice(id);

                previewFpsRangeTestByCamera();
            } finally {
                closeDevice();
            }
        }
    }

    /**
     * Test preview fps range for all supported ranges. The exposure time are frame duration are
     * validated.
     */
    private void previewFpsRangeTestByCamera() throws Exception {
        final int FPS_RANGE_SIZE = 2;
        Size maxPreviewSz = mOrderedPreviewSizes.get(0);
        Range<Integer>[] fpsRanges = mStaticInfo.getAeAvailableTargetFpsRangesChecked();
        boolean antiBandingOffIsSupported = mStaticInfo.isAntiBandingOffModeSupported();
        Range<Integer> fpsRange;
        CaptureRequest.Builder requestBuilder =
                mCamera.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
        SimpleCaptureCallback resultListener = new SimpleCaptureCallback();
        startPreview(requestBuilder, maxPreviewSz, resultListener);

        for (int i = 0; i < fpsRanges.length; i += 1) {
            fpsRange = fpsRanges[i];

            requestBuilder.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE, fpsRange);
            // Turn off auto antibanding to avoid exposure time and frame duration interference
            // from antibanding algorithm.
            if (antiBandingOffIsSupported) {
                requestBuilder.set(CaptureRequest.CONTROL_AE_ANTIBANDING_MODE,
                        CaptureRequest.CONTROL_AE_ANTIBANDING_MODE_OFF);
            } else {
                // The device doesn't implement the OFF mode, test continues. It need make sure
                // that the antibanding algorithm doesn't interfere with the fps range control.
                Log.i(TAG, "OFF antibanding mode is not supported, the camera device output must" +
                        " satisfy the specified fps range regardless of its current antibanding" +
                        " mode");
            }

            resultListener = new SimpleCaptureCallback();
            mSession.setRepeatingRequest(requestBuilder.build(), resultListener, mHandler);

            verifyPreviewTargetFpsRange(resultListener, NUM_FRAMES_VERIFIED, fpsRange,
                    maxPreviewSz);
        }

        stopPreview();
    }

    private void verifyPreviewTargetFpsRange(SimpleCaptureCallback resultListener,
            int numFramesVerified, Range<Integer> fpsRange, Size previewSz) {
        CaptureResult result = resultListener.getCaptureResult(WAIT_FOR_RESULT_TIMEOUT_MS);
        List<Integer> capabilities = mStaticInfo.getAvailableCapabilitiesChecked();

        if (capabilities.contains(CaptureRequest.REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR)) {
            long frameDuration = getValueNotNull(result, CaptureResult.SENSOR_FRAME_DURATION);
            long[] frameDurationRange =
                    new long[]{(long) (1e9 / fpsRange.getUpper()), (long) (1e9 / fpsRange.getLower())};
            mCollector.expectInRange(
                    "Frame duration must be in the range of " + Arrays.toString(frameDurationRange),
                    frameDuration, (long) (frameDurationRange[0] * (1 - FRAME_DURATION_ERROR_MARGIN)),
                    (long) (frameDurationRange[1] * (1 + FRAME_DURATION_ERROR_MARGIN)));
            long expTime = getValueNotNull(result, CaptureResult.SENSOR_EXPOSURE_TIME);
            mCollector.expectTrue(String.format("Exposure time %d must be no larger than frame"
                    + "duration %d", expTime, frameDuration), expTime <= frameDuration);

            Long minFrameDuration = mMinPreviewFrameDurationMap.get(previewSz);
            boolean findDuration = mCollector.expectTrue("Unable to find minFrameDuration for size "
                    + previewSz.toString(), minFrameDuration != null);
            if (findDuration) {
                mCollector.expectTrue("Frame duration " + frameDuration + " must be no smaller than"
                        + " minFrameDuration " + minFrameDuration, frameDuration >= minFrameDuration);
            }
        } else {
            Log.i(TAG, "verifyPreviewTargetFpsRange - MANUAL_SENSOR control is not supported," +
                    " skipping duration and exposure time check.");
        }
    }

    /**
     * Test all supported preview sizes for a camera device
     *
     * @throws Exception
     */
    private void previewTestByCamera() throws Exception {
        List<Size> previewSizes = getSupportedPreviewSizes(
                mCamera.getId(), mCameraManager, PREVIEW_SIZE_BOUND);

        for (final Size sz : previewSizes) {
            if (VERBOSE) {
                Log.v(TAG, "Testing camera preview size: " + sz.toString());
            }

            // TODO: vary the different settings like crop region to cover more cases.
            CaptureRequest.Builder requestBuilder =
                    mCamera.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            CaptureCallback mockCaptureCallback =
                    mock(CameraCaptureSession.CaptureCallback.class);

            startPreview(requestBuilder, sz, mockCaptureCallback);
            verifyCaptureResults(mSession, mockCaptureCallback, NUM_FRAMES_VERIFIED,
                    NUM_FRAMES_VERIFIED * FRAME_TIMEOUT_MS);
            stopPreview();
        }
    }

    private void previewTestPatternTestByCamera() throws Exception {
        Size maxPreviewSize = mOrderedPreviewSizes.get(0);
        int[] testPatternModes = mStaticInfo.getAvailableTestPatternModesChecked();
        CaptureRequest.Builder requestBuilder =
                mCamera.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
        CaptureCallback mockCaptureCallback;

        final int[] TEST_PATTERN_DATA = {0, 0xFFFFFFFF, 0xFFFFFFFF, 0}; // G:100%, RB:0.
        for (int mode : testPatternModes) {
            if (VERBOSE) {
                Log.v(TAG, "Test pattern mode: " + mode);
            }
            requestBuilder.set(CaptureRequest.SENSOR_TEST_PATTERN_MODE, mode);
            if (mode == CaptureRequest.SENSOR_TEST_PATTERN_MODE_SOLID_COLOR) {
                // Assign color pattern to SENSOR_TEST_PATTERN_MODE_DATA
                requestBuilder.set(CaptureRequest.SENSOR_TEST_PATTERN_DATA, TEST_PATTERN_DATA);
            }
            mockCaptureCallback = mock(CaptureCallback.class);
            startPreview(requestBuilder, maxPreviewSize, mockCaptureCallback);
            verifyCaptureResults(mSession, mockCaptureCallback, NUM_TEST_PATTERN_FRAMES_VERIFIED,
                    NUM_TEST_PATTERN_FRAMES_VERIFIED * FRAME_TIMEOUT_MS);
        }

        stopPreview();
    }

    private class IsCaptureResultValid extends ArgumentMatcher<TotalCaptureResult> {
        @Override
        public boolean matches(Object obj) {
            TotalCaptureResult result = (TotalCaptureResult)obj;
            Long timeStamp = result.get(CaptureResult.SENSOR_TIMESTAMP);
            if (timeStamp != null && timeStamp.longValue() > 0L) {
                return true;
            }
            return false;
        }
    }

    private void verifyCaptureResults(
            CameraCaptureSession session,
            CaptureCallback mockListener,
            int expectResultCount,
            int timeOutMs) {
        // Should receive expected number of onCaptureStarted callbacks.
        ArgumentCaptor<Long> timestamps = ArgumentCaptor.forClass(Long.class);
        ArgumentCaptor<Long> frameNumbers = ArgumentCaptor.forClass(Long.class);
        verify(mockListener,
                timeout(timeOutMs).atLeast(expectResultCount))
                        .onCaptureStarted(
                                eq(session),
                                isA(CaptureRequest.class),
                                timestamps.capture(),
                                frameNumbers.capture());

        // Validate timestamps: all timestamps should be larger than 0 and monotonically increase.
        long timestamp = 0;
        for (Long nextTimestamp : timestamps.getAllValues()) {
            assertNotNull("Next timestamp is null!", nextTimestamp);
            assertTrue("Captures are out of order", timestamp < nextTimestamp);
            timestamp = nextTimestamp;
        }

        // Validate framenumbers: all framenumbers should be consecutive and positive
        long frameNumber = -1;
        for (Long nextFrameNumber : frameNumbers.getAllValues()) {
            assertNotNull("Next frame number is null!", nextFrameNumber);
            assertTrue("Captures are out of order",
                    (frameNumber == -1) || (frameNumber + 1 == nextFrameNumber));
            frameNumber = nextFrameNumber;
        }

        // Should receive expected number of capture results.
        verify(mockListener,
                timeout(timeOutMs).atLeast(expectResultCount))
                        .onCaptureCompleted(
                                eq(session),
                                isA(CaptureRequest.class),
                                argThat(new IsCaptureResultValid()));

        // Should not receive any capture failed callbacks.
        verify(mockListener, never())
                        .onCaptureFailed(
                                eq(session),
                                isA(CaptureRequest.class),
                                isA(CaptureFailure.class));
    }

}
