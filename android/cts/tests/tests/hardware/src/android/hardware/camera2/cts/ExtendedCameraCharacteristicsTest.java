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

import android.content.Context;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraCharacteristics.Key;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.cts.helpers.CameraErrorCollector;
import android.hardware.camera2.params.BlackLevelPattern;
import android.hardware.camera2.params.ColorSpaceTransform;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.ImageReader;
import android.test.AndroidTestCase;
import android.util.Log;
import android.util.Rational;
import android.util.Range;
import android.util.Size;
import android.view.Surface;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;

import static android.hardware.camera2.cts.helpers.AssertHelpers.*;

/**
 * Extended tests for static camera characteristics.
 */
public class ExtendedCameraCharacteristicsTest extends AndroidTestCase {
    private static final String TAG = "ExChrsTest"; // must be short so next line doesn't throw
    private static final boolean VERBOSE = Log.isLoggable(TAG, Log.VERBOSE);

    private static final String PREFIX_ANDROID = "android";
    private static final String PREFIX_VENDOR = "com";

    /*
     * Constants for static RAW metadata.
     */
    private static final int MIN_ALLOWABLE_WHITELEVEL = 32; // must have sensor bit depth > 5

    private CameraManager mCameraManager;
    private List<CameraCharacteristics> mCharacteristics;
    private String[] mIds;
    private CameraErrorCollector mCollector;

    private static final Size VGA = new Size(640, 480);

    /*
     * HW Levels short hand
     */
    private static final int LEGACY = CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY;
    private static final int LIMITED = CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    private static final int FULL = CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    private static final int OPT = Integer.MAX_VALUE;  // For keys that are optional on all hardware levels.

    /*
     * Capabilities short hand
     */
    private static final int NONE = -1;
    private static final int BC =
            CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE;
    private static final int MANUAL_SENSOR =
            CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR;
    private static final int MANUAL_POSTPROC =
            CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_MANUAL_POST_PROCESSING;
    private static final int RAW =
            CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_RAW;

    @Override
    public void setContext(Context context) {
        super.setContext(context);
        mCameraManager = (CameraManager)context.getSystemService(Context.CAMERA_SERVICE);
        assertNotNull("Can't connect to camera manager", mCameraManager);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mIds = mCameraManager.getCameraIdList();
        mCharacteristics = new ArrayList<>();
        mCollector = new CameraErrorCollector();
        for (int i = 0; i < mIds.length; i++) {
            CameraCharacteristics props = mCameraManager.getCameraCharacteristics(mIds[i]);
            assertNotNull(String.format("Can't get camera characteristics from: ID %s", mIds[i]),
                    props);
            mCharacteristics.add(props);
        }
    }

    @Override
    protected void tearDown() throws Exception {
        mCharacteristics = null;

        try {
            mCollector.verify();
        } catch (Throwable e) {
            // When new Exception(e) is used, exception info will be printed twice.
            throw new Exception(e.getMessage());
        } finally {
            super.tearDown();
        }
    }

    /**
     * Test that the available stream configurations contain a few required formats and sizes.
     */
    public void testAvailableStreamConfigs() {
        int counter = 0;
        for (CameraCharacteristics c : mCharacteristics) {
            StreamConfigurationMap config =
                    c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            assertNotNull(String.format("No stream configuration map found for: ID %s",
                    mIds[counter]), config);
            int[] outputFormats = config.getOutputFormats();

            // Check required formats exist (JPEG, and YUV_420_888).
            assertArrayContains(
                    String.format("No valid YUV_420_888 preview formats found for: ID %s",
                            mIds[counter]), outputFormats, ImageFormat.YUV_420_888);
            assertArrayContains(String.format("No JPEG image format for: ID %s",
                    mIds[counter]), outputFormats, ImageFormat.JPEG);

            Size[] sizes = config.getOutputSizes(ImageFormat.YUV_420_888);
            CameraTestUtils.assertArrayNotEmpty(sizes,
                    String.format("No sizes for preview format %x for: ID %s",
                            ImageFormat.YUV_420_888, mIds[counter]));

            assertArrayContains(String.format(
                            "Required VGA size not found for format %x for: ID %s",
                            ImageFormat.YUV_420_888, mIds[counter]), sizes, VGA);

            counter++;
        }
    }

    /**
     * Test {@link CameraCharacteristics#getKeys}
     */
    public void testKeys() {
        int counter = 0;
        for (CameraCharacteristics c : mCharacteristics) {
            mCollector.setCameraId(mIds[counter]);

            if (VERBOSE) {
                Log.v(TAG, "testKeys - testing characteristics for camera " + mIds[counter]);
            }

            List<CameraCharacteristics.Key<?>> allKeys = c.getKeys();
            assertNotNull("Camera characteristics keys must not be null", allKeys);
            assertFalse("Camera characteristics keys must have at least 1 key",
                    allKeys.isEmpty());

            for (CameraCharacteristics.Key<?> key : allKeys) {
                assertKeyPrefixValid(key.getName());

                // All characteristics keys listed must never be null
                mCollector.expectKeyValueNotNull(c, key);

                // TODO: add a check that key must not be @hide
            }

            /*
             * List of keys that must be present in camera characteristics (not null).
             *
             * Keys for LIMITED, FULL devices might be available despite lacking either
             * the hardware level or the capability. This is *OK*. This only lists the
             * *minimal* requirements for a key to be listed.
             *
             * LEGACY devices are a bit special since they map to api1 devices, so we know
             * for a fact most keys are going to be illegal there so they should never be
             * available.
             *
             * (TODO: Codegen this)
             */
            {
                //                                           (Key Name)                                     (HW Level)  (Capabilities <Var-Arg>)
                expectKeyAvailable(c, CameraCharacteristics.COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES     , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.CONTROL_AE_AVAILABLE_ANTIBANDING_MODES          , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.CONTROL_AE_AVAILABLE_MODES                      , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES          , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.CONTROL_AE_COMPENSATION_RANGE                   , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.CONTROL_AE_COMPENSATION_STEP                    , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES                      , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.CONTROL_AVAILABLE_EFFECTS                       , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.CONTROL_AVAILABLE_SCENE_MODES                   , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES     , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.CONTROL_AWB_AVAILABLE_MODES                     , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.CONTROL_MAX_REGIONS_AE                          , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.CONTROL_MAX_REGIONS_AF                          , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.CONTROL_MAX_REGIONS_AWB                         , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.EDGE_AVAILABLE_EDGE_MODES                       , FULL     ,   NONE                 );
                expectKeyAvailable(c, CameraCharacteristics.FLASH_INFO_AVAILABLE                            , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES             , OPT      ,   RAW                  );
                expectKeyAvailable(c, CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL                   , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.JPEG_AVAILABLE_THUMBNAIL_SIZES                  , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.LENS_FACING                                     , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.LENS_INFO_AVAILABLE_APERTURES                   , FULL     ,   MANUAL_SENSOR        );
                expectKeyAvailable(c, CameraCharacteristics.LENS_INFO_AVAILABLE_FILTER_DENSITIES            , FULL     ,   MANUAL_SENSOR        );
                expectKeyAvailable(c, CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS               , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION       , LIMITED  ,   MANUAL_SENSOR        );
                expectKeyAvailable(c, CameraCharacteristics.LENS_INFO_FOCUS_DISTANCE_CALIBRATION            , LIMITED  ,   MANUAL_SENSOR        );
                expectKeyAvailable(c, CameraCharacteristics.LENS_INFO_HYPERFOCAL_DISTANCE                   , LIMITED  ,   MANUAL_SENSOR        );
                expectKeyAvailable(c, CameraCharacteristics.LENS_INFO_MINIMUM_FOCUS_DISTANCE                , LIMITED  ,   NONE                 );
                expectKeyAvailable(c, CameraCharacteristics.NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES                  , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.REQUEST_MAX_NUM_OUTPUT_PROC                     , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.REQUEST_MAX_NUM_OUTPUT_PROC_STALLING            , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.REQUEST_MAX_NUM_OUTPUT_RAW                      , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.REQUEST_PARTIAL_RESULT_COUNT                    , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.REQUEST_PIPELINE_MAX_DEPTH                      , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.SCALER_AVAILABLE_MAX_DIGITAL_ZOOM               , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP                 , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.SCALER_CROPPING_TYPE                            , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_AVAILABLE_TEST_PATTERN_MODES             , LEGACY   ,   NONE                 );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_BLACK_LEVEL_PATTERN                      , FULL     ,   MANUAL_SENSOR, RAW   );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_CALIBRATION_TRANSFORM1                   , OPT      ,   RAW                  );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_CALIBRATION_TRANSFORM2                   , OPT      ,   RAW                  );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_COLOR_TRANSFORM1                         , OPT      ,   RAW                  );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_COLOR_TRANSFORM2                         , OPT      ,   RAW                  );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_FORWARD_MATRIX1                          , OPT      ,   RAW                  );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_FORWARD_MATRIX2                          , OPT      ,   RAW                  );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE                   , LEGACY   ,   BC, RAW              );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT            , FULL     ,   RAW                  );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_INFO_EXPOSURE_TIME_RANGE                 , FULL     ,   MANUAL_SENSOR        );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_INFO_MAX_FRAME_DURATION                  , FULL     ,   MANUAL_SENSOR        );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_INFO_PHYSICAL_SIZE                       , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE                    , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_INFO_SENSITIVITY_RANGE                   , FULL     ,   MANUAL_SENSOR        );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_INFO_WHITE_LEVEL                         , OPT      ,   RAW                  );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_INFO_TIMESTAMP_SOURCE                    , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_MAX_ANALOG_SENSITIVITY                   , FULL     ,   MANUAL_SENSOR        );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_ORIENTATION                              , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT1                    , OPT      ,   RAW                  );
                expectKeyAvailable(c, CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT2                    , OPT      ,   RAW                  );
                expectKeyAvailable(c, CameraCharacteristics.STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES     , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES   , OPT      ,   RAW                  );
                expectKeyAvailable(c, CameraCharacteristics.STATISTICS_INFO_MAX_FACE_COUNT                  , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.SYNC_MAX_LATENCY                                , LEGACY   ,   BC                   );
                expectKeyAvailable(c, CameraCharacteristics.TONEMAP_AVAILABLE_TONE_MAP_MODES                , FULL     ,   MANUAL_POSTPROC      );
                expectKeyAvailable(c, CameraCharacteristics.TONEMAP_MAX_CURVE_POINTS                        , FULL     ,   MANUAL_POSTPROC      );

                // Future: Use column editors for modifying above, ignore line length to keep 1 key per line

                // TODO: check that no other 'android' keys are listed in #getKeys if they aren't in the above list
            }

            counter++;
        }
    }

    /**
     * Test values for static metadata used by the RAW capability.
     */
    public void testStaticRawCharacteristics() {
        int counter = 0;
        for (CameraCharacteristics c : mCharacteristics) {
            int[] actualCapabilities = c.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES);
            assertNotNull("android.request.availableCapabilities must never be null",
                    actualCapabilities);
            if (!arrayContains(actualCapabilities,
                    CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_RAW)) {
                Log.i(TAG, "RAW capability is not supported in camera " + counter++ +
                        ". Skip the test.");
                continue;
            }

            Integer actualHwLevel = c.get(CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL);
            if (actualHwLevel != null && actualHwLevel == FULL) {
                mCollector.expectKeyValueContains(c,
                        CameraCharacteristics.HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES,
                        CameraCharacteristics.HOT_PIXEL_MODE_FAST);
            }
            mCollector.expectKeyValueContains(c,
                    CameraCharacteristics.STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES, false);
            mCollector.expectKeyValueGreaterThan(c, CameraCharacteristics.SENSOR_INFO_WHITE_LEVEL,
                    MIN_ALLOWABLE_WHITELEVEL);

            mCollector.expectKeyValueIsIn(c,
                    CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT,
                    CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB,
                    CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG,
                    CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG,
                    CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR);
            // TODO: SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGB isn't supported yet.

            mCollector.expectKeyValueInRange(c, CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT1,
                    CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT,
                    CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT1_ISO_STUDIO_TUNGSTEN);
            mCollector.expectKeyValueInRange(c, CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT2,
                    (byte) CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT,
                    (byte) CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT1_ISO_STUDIO_TUNGSTEN);

            Rational[] zeroes = new Rational[9];
            Arrays.fill(zeroes, Rational.ZERO);

            ColorSpaceTransform zeroed = new ColorSpaceTransform(zeroes);
            mCollector.expectNotEquals("Forward Matrix1 should not contain all zeroes.", zeroed,
                    c.get(CameraCharacteristics.SENSOR_FORWARD_MATRIX1));
            mCollector.expectNotEquals("Forward Matrix2 should not contain all zeroes.", zeroed,
                    c.get(CameraCharacteristics.SENSOR_FORWARD_MATRIX2));
            mCollector.expectNotEquals("Calibration Transform1 should not contain all zeroes.",
                    zeroed, c.get(CameraCharacteristics.SENSOR_CALIBRATION_TRANSFORM1));
            mCollector.expectNotEquals("Calibration Transform2 should not contain all zeroes.",
                    zeroed, c.get(CameraCharacteristics.SENSOR_CALIBRATION_TRANSFORM2));
            mCollector.expectNotEquals("Color Transform1 should not contain all zeroes.",
                    zeroed, c.get(CameraCharacteristics.SENSOR_COLOR_TRANSFORM1));
            mCollector.expectNotEquals("Color Transform2 should not contain all zeroes.",
                    zeroed, c.get(CameraCharacteristics.SENSOR_COLOR_TRANSFORM2));

            BlackLevelPattern blackLevel = mCollector.expectKeyValueNotNull(c,
                    CameraCharacteristics.SENSOR_BLACK_LEVEL_PATTERN);
            if (blackLevel != null) {
                int[] blackLevelPattern = new int[BlackLevelPattern.COUNT];
                blackLevel.copyTo(blackLevelPattern, /*offset*/0);
                Integer whitelevel = c.get(CameraCharacteristics.SENSOR_INFO_WHITE_LEVEL);
                if (whitelevel != null) {
                    mCollector.expectValuesInRange("BlackLevelPattern", blackLevelPattern, 0,
                            whitelevel);
                } else {
                    mCollector.addMessage(
                            "No WhiteLevel available, cannot check BlackLevelPattern range.");
                }
            }

            // TODO: profileHueSatMap, and profileToneCurve aren't supported yet.
            counter++;
        }
    }

    /**
     * Test values for static metadata used by the BURST capability.
     */
    public void testStaticBurstCharacteristics() {
        int counter = 0;
        final float SIZE_ERROR_MARGIN = 0.03f;
        for (CameraCharacteristics c : mCharacteristics) {
            int[] actualCapabilities = c.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES);
            assertNotNull("android.request.availableCapabilities must never be null",
                    actualCapabilities);

            // Check if the burst capability is defined
            boolean haveBurstCapability = arrayContains(actualCapabilities,
                    CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_BURST_CAPTURE);

            StreamConfigurationMap config =
                    c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            assertNotNull(String.format("No stream configuration map found for: ID %s",
                    mIds[counter]), config);
            Rect activeRect = c.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE);
            Size sensorSize = new Size(activeRect.width(), activeRect.height());

            // Ensure that max YUV size matches max JPEG size
            Size maxYuvSize = CameraTestUtils.getMaxSize(
                    config.getOutputSizes(ImageFormat.YUV_420_888));
            Size maxJpegSize = CameraTestUtils.getMaxSize(config.getOutputSizes(ImageFormat.JPEG));

            boolean haveMaxYuv = maxYuvSize != null ?
                (maxJpegSize.getWidth() <= maxYuvSize.getWidth() &&
                        maxJpegSize.getHeight() <= maxYuvSize.getHeight()) : false;

            boolean maxYuvMatchSensor =
                    (maxYuvSize.getWidth() <= sensorSize.getWidth() * (1.0 + SIZE_ERROR_MARGIN) &&
                     maxYuvSize.getWidth() >= sensorSize.getWidth() * (1.0 - SIZE_ERROR_MARGIN) &&
                     maxYuvSize.getHeight() <= sensorSize.getHeight() * (1.0 + SIZE_ERROR_MARGIN) &&
                     maxYuvSize.getHeight() >= sensorSize.getHeight() * (1.0 - SIZE_ERROR_MARGIN));

            // Ensure that YUV output is fast enough - needs to be at least 20 fps

            long maxYuvRate =
                config.getOutputMinFrameDuration(ImageFormat.YUV_420_888, maxYuvSize);
            final long MIN_DURATION_BOUND_NS = 50000000; // 50 ms, 20 fps

            boolean haveMaxYuvRate = maxYuvRate <= MIN_DURATION_BOUND_NS;

            // Ensure that there's an FPS range that's fast enough to capture at above
            // minFrameDuration, for full-auto bursts
            Range[] fpsRanges = c.get(CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES);
            float minYuvFps = 1.f / maxYuvRate;

            boolean haveFastAeTargetFps = false;
            for (Range<Integer> r : fpsRanges) {
                if (r.getLower() >= minYuvFps) {
                    haveFastAeTargetFps = true;
                    break;
                }
            }

            // Ensure that maximum sync latency is small enough for fast setting changes, even if
            // it's not quite per-frame

            Integer maxSyncLatencyValue = c.get(CameraCharacteristics.SYNC_MAX_LATENCY);
            assertNotNull(String.format("No sync latency declared for ID %s", mIds[counter]),
                    maxSyncLatencyValue);

            int maxSyncLatency = maxSyncLatencyValue;
            final long MAX_LATENCY_BOUND = 4;
            boolean haveFastSyncLatency =
                (maxSyncLatency <= MAX_LATENCY_BOUND) && (maxSyncLatency >= 0);

            if (haveBurstCapability) {
                assertTrue(
                        String.format("BURST-capable camera device %s does not have maximum YUV " +
                                "size that is at least max JPEG size",
                                mIds[counter]),
                        haveMaxYuv);
                assertTrue(
                        String.format("BURST-capable camera device %s YUV frame rate is too slow" +
                                "(%d ns min frame duration reported, less than %d ns expected)",
                                mIds[counter], maxYuvRate, MIN_DURATION_BOUND_NS),
                        haveMaxYuvRate);
                assertTrue(
                        String.format("BURST-capable camera device %s does not list an AE target " +
                                " FPS range with min FPS >= %f, for full-AUTO bursts",
                                mIds[counter], minYuvFps),
                        haveFastAeTargetFps);
                assertTrue(
                        String.format("BURST-capable camera device %s YUV sync latency is too long" +
                                "(%d frames reported, [0, %d] frames expected)",
                                mIds[counter], maxSyncLatency, MAX_LATENCY_BOUND),
                        haveFastSyncLatency);
                assertTrue(
                        "Active array size and max YUV size should be similar",
                        maxYuvMatchSensor);
            } else {
                assertTrue(
                        String.format("Camera device %s has all the requirements for BURST" +
                                " capability but does not report it!", mIds[counter]),
                        !(haveMaxYuv && haveMaxYuvRate && haveFastAeTargetFps &&
                                haveFastSyncLatency && maxYuvMatchSensor));
            }

            counter++;
        }
    }

    /**
     * Cross-check StreamConfigurationMap output
     */
    public void testStreamConfigurationMap() {
        int counter = 0;
        for (CameraCharacteristics c : mCharacteristics) {
            Log.i(TAG, "testStreamConfigurationMap: Testing camera ID " + mIds[counter]);
            StreamConfigurationMap config =
                    c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            assertNotNull(String.format("No stream configuration map found for: ID %s",
                            mIds[counter]), config);

            assertTrue("ImageReader must be supported",
                    config.isOutputSupportedFor(android.media.ImageReader.class));
            assertTrue("MediaRecorder must be supported",
                    config.isOutputSupportedFor(android.media.MediaRecorder.class));
            assertTrue("MediaCodec must be supported",
                    config.isOutputSupportedFor(android.media.MediaCodec.class));
            assertTrue("Allocation must be supported",
                    config.isOutputSupportedFor(android.renderscript.Allocation.class));
            assertTrue("SurfaceHolder must be supported",
                    config.isOutputSupportedFor(android.view.SurfaceHolder.class));
            assertTrue("SurfaceTexture must be supported",
                    config.isOutputSupportedFor(android.graphics.SurfaceTexture.class));

            assertTrue("YUV_420_888 must be supported",
                    config.isOutputSupportedFor(ImageFormat.YUV_420_888));
            assertTrue("JPEG must be supported",
                    config.isOutputSupportedFor(ImageFormat.JPEG));

            // Legacy YUV formats should not be listed
            assertTrue("NV21 must not be supported",
                    !config.isOutputSupportedFor(ImageFormat.NV21));

            int[] actualCapabilities = c.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES);
            assertNotNull("android.request.availableCapabilities must never be null",
                    actualCapabilities);
            if (arrayContains(actualCapabilities,
                    CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_RAW)) {
                assertTrue("RAW_SENSOR must be supported if RAW capability is advertised",
                    config.isOutputSupportedFor(ImageFormat.RAW_SENSOR));
            }

            // Cross check public formats and sizes

            int[] supportedFormats = config.getOutputFormats();
            for (int format : supportedFormats) {
                assertTrue("Format " + format + " fails cross check",
                        config.isOutputSupportedFor(format));
                Size[] supportedSizes = config.getOutputSizes(format);
                assertTrue("Supported format " + format + " has no sizes listed",
                        supportedSizes.length > 0);
                for (Size size : supportedSizes) {
                    if (VERBOSE) {
                        Log.v(TAG,
                                String.format("Testing camera %s, format %d, size %s",
                                        mIds[counter], format, size.toString()));
                    }

                    long stallDuration = config.getOutputStallDuration(format, size);
                    switch(format) {
                        case ImageFormat.YUV_420_888:
                            assertTrue("YUV_420_888 may not have a non-zero stall duration",
                                    stallDuration == 0);
                            break;
                        default:
                            assertTrue("Negative stall duration for format " + format,
                                    stallDuration >= 0);
                            break;
                    }
                    long minDuration = config.getOutputMinFrameDuration(format, size);
                    if (arrayContains(actualCapabilities,
                            CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR)) {
                        assertTrue("MANUAL_SENSOR capability, need positive min frame duration for"
                                + "format " + format + " for size " + size + " minDuration " +
                                minDuration,
                                minDuration > 0);
                    } else {
                        assertTrue("Need non-negative min frame duration for format " + format,
                                minDuration >= 0);
                    }

                    ImageReader testReader = ImageReader.newInstance(
                        size.getWidth(),
                        size.getHeight(),
                        format,
                        1);
                    Surface testSurface = testReader.getSurface();

                    assertTrue(
                        String.format("isOutputSupportedFor fails for config %s, format %d",
                                size.toString(), format),
                        config.isOutputSupportedFor(testSurface));

                    testReader.close();

                } // sizes

                // Try an invalid size in this format, should round
                Size invalidSize = findInvalidSize(supportedSizes);
                // WAR: the intended threshold is 1920, but to counter the bug
                // in Lollipop framework, we need to set it to 1080 here.
                // The threshold will be changed back to 1920 in next Android release.
                int MAX_ROUNDING_WIDTH = 1080;
                if (invalidSize.getWidth() <= MAX_ROUNDING_WIDTH) {
                    ImageReader testReader = ImageReader.newInstance(
                                                                     invalidSize.getWidth(),
                                                                     invalidSize.getHeight(),
                                                                     format,
                                                                     1);
                    Surface testSurface = testReader.getSurface();

                    assertTrue(
                               String.format("isOutputSupportedFor fails for config %s, %d",
                                       invalidSize.toString(), format),
                               config.isOutputSupportedFor(testSurface));

                    testReader.close();
                }
            } // formats

            // Cross-check opaque format and sizes

            SurfaceTexture st = new SurfaceTexture(1);
            Surface surf = new Surface(st);

            Size[] opaqueSizes = config.getOutputSizes(SurfaceTexture.class);
            assertTrue("Opaque format has no sizes listed",
                    opaqueSizes.length > 0);
            for (Size size : opaqueSizes) {
                long stallDuration = config.getOutputStallDuration(SurfaceTexture.class, size);
                assertTrue("Opaque output may not have a non-zero stall duration",
                        stallDuration == 0);

                long minDuration = config.getOutputMinFrameDuration(SurfaceTexture.class, size);
                if (arrayContains(actualCapabilities,
                                CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR)) {
                    assertTrue("MANUAL_SENSOR capability, need positive min frame duration for"
                            + "opaque format",
                            minDuration > 0);
                } else {
                    assertTrue("Need non-negative min frame duration for opaque format ",
                            minDuration >= 0);
                }
                st.setDefaultBufferSize(size.getWidth(), size.getHeight());

                assertTrue(
                    String.format("isOutputSupportedFor fails for SurfaceTexture config %s",
                            size.toString()),
                    config.isOutputSupportedFor(surf));

            } // opaque sizes

            // Try invalid opaque size, should get rounded
            Size invalidSize = findInvalidSize(opaqueSizes);
            st.setDefaultBufferSize(invalidSize.getWidth(), invalidSize.getHeight());
            assertTrue(
                String.format("isOutputSupportedFor fails for SurfaceTexture config %s",
                        invalidSize.toString()),
                config.isOutputSupportedFor(surf));

            counter++;
        } // mCharacteristics

    }

    /**
     * Create an invalid size that's close to one of the good sizes in the list, but not one of them
     */
    private Size findInvalidSize(Size[] goodSizes) {
        Size invalidSize = new Size(goodSizes[0].getWidth() + 1, goodSizes[0].getHeight());
        while(arrayContains(goodSizes, invalidSize)) {
            invalidSize = new Size(invalidSize.getWidth() + 1, invalidSize.getHeight());
        }
        return invalidSize;
    }

    /**
     * Check key is present in characteristics if the hardware level is at least {@code hwLevel};
     * check that the key is present if the actual capabilities are one of {@code capabilities}.
     *
     * @return value of the {@code key} from {@code c}
     */
    private <T> T expectKeyAvailable(CameraCharacteristics c, CameraCharacteristics.Key<T> key,
            int hwLevel, int... capabilities) {

        Integer actualHwLevel = c.get(CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL);
        assertNotNull("android.info.supportedHardwareLevel must never be null", actualHwLevel);

        int[] actualCapabilities = c.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES);
        assertNotNull("android.request.availableCapabilities must never be null",
                actualCapabilities);

        List<Key<?>> allKeys = c.getKeys();

        T value = c.get(key);

        if (compareHardwareLevel(actualHwLevel, hwLevel) >= 0) {
            mCollector.expectTrue(
                    String.format("Key (%s) must be in characteristics for this hardware level " +
                            "(required minimal HW level %s, actual HW level %s)",
                            key.getName(), toStringHardwareLevel(hwLevel),
                            toStringHardwareLevel(actualHwLevel)),
                    value != null);
            mCollector.expectTrue(
                    String.format("Key (%s) must be in characteristics list of keys for this " +
                            "hardware level (required minimal HW level %s, actual HW level %s)",
                            key.getName(), toStringHardwareLevel(hwLevel),
                            toStringHardwareLevel(actualHwLevel)),
                    allKeys.contains(key));
        } else if (arrayContainsAnyOf(actualCapabilities, capabilities)) {
            mCollector.expectTrue(
                    String.format("Key (%s) must be in characteristics for these capabilities " +
                            "(required capabilities %s, actual capabilities %s)",
                            key.getName(), Arrays.toString(capabilities),
                            Arrays.toString(actualCapabilities)),
                    value != null);
            mCollector.expectTrue(
                    String.format("Key (%s) must be in characteristics list of keys for " +
                            "these capabilities (required capabilities %s, actual capabilities %s)",
                            key.getName(), Arrays.toString(capabilities),
                            Arrays.toString(actualCapabilities)),
                    allKeys.contains(key));
        } else {
            if (actualHwLevel == LEGACY && hwLevel != OPT) {
                if (value != null || allKeys.contains(key)) {
                    Log.w(TAG, String.format(
                            "Key (%s) is not required for LEGACY devices but still appears",
                            key.getName()));
                }
            }
            // OK: Key may or may not be present.
        }
        return value;
    }

    private static boolean arrayContains(int[] arr, int needle) {
        if (arr == null) {
            return false;
        }

        for (int elem : arr) {
            if (elem == needle) {
                return true;
            }
        }

        return false;
    }

    private static <T> boolean arrayContains(T[] arr, T needle) {
        if (arr == null) {
            return false;
        }

        for (T elem : arr) {
            if (elem.equals(needle)) {
                return true;
            }
        }

        return false;
    }

    private static boolean arrayContainsAnyOf(int[] arr, int[] needles) {
        for (int needle : needles) {
            if (arrayContains(arr, needle)) {
                return true;
            }
        }
        return false;
    }

    /**
     * The key name has a prefix of either "android." or "com."; other prefixes are not valid.
     */
    private static void assertKeyPrefixValid(String keyName) {
        assertStartsWithAnyOf(
                "All metadata keys must start with 'android.' (built-in keys) " +
                "or 'com.' (vendor-extended keys)", new String[] {
                        PREFIX_ANDROID + ".",
                        PREFIX_VENDOR + ".",
                }, keyName);
    }

    private static void assertTrueForKey(String msg, CameraCharacteristics.Key<?> key,
            boolean actual) {
        assertTrue(msg + " (key = '" + key.getName() + "')", actual);
    }

    private static <T> void assertOneOf(String msg, T[] expected, T actual) {
        for (int i = 0; i < expected.length; ++i) {
            if (Objects.equals(expected[i], actual)) {
                return;
            }
        }

        fail(String.format("%s: (expected one of %s, actual %s)",
                msg, Arrays.toString(expected), actual));
    }

    private static <T> void assertStartsWithAnyOf(String msg, String[] expected, String actual) {
        for (int i = 0; i < expected.length; ++i) {
            if (actual.startsWith(expected[i])) {
                return;
            }
        }

        fail(String.format("%s: (expected to start with any of %s, but value was %s)",
                msg, Arrays.toString(expected), actual));
    }

    /** Return a positive int if left > right, 0 if left==right, negative int if left < right */
    private static int compareHardwareLevel(int left, int right) {
        return remapHardwareLevel(left) - remapHardwareLevel(right);
    }

    /** Remap HW levels worst<->best, 0 = worst, 2 = best */
    private static int remapHardwareLevel(int level) {
        switch (level) {
            case OPT:
                return Integer.MAX_VALUE;
            case LEGACY:
                return 0; // lowest
            case LIMITED:
                return 1; // second lowest
            case FULL:
                return 2; // best
        }

        fail("Unknown HW level: " + level);
        return -1;
    }

    private static String toStringHardwareLevel(int level) {
        switch (level) {
            case LEGACY:
                return "LEGACY";
            case LIMITED:
                return "LIMITED";
            case FULL:
                return "FULL";
        }

        // unknown
        Log.w(TAG, "Unknown hardware level " + level);
        return Integer.toString(level);
    }
}
