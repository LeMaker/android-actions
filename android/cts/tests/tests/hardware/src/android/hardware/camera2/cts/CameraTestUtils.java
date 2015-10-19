/*
 * Copyright 2013 The Android Open Source Project
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

import static com.android.ex.camera2.blocking.BlockingStateCallback.*;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.PointF;
import android.graphics.Rect;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CaptureFailure;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.TotalCaptureResult;
import android.hardware.cts.helpers.CameraUtils;
import android.util.Size;
import android.hardware.camera2.params.MeteringRectangle;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.media.Image.Plane;
import android.os.Handler;
import android.util.Log;
import android.view.Surface;

import com.android.ex.camera2.blocking.BlockingCameraManager;
import com.android.ex.camera2.blocking.BlockingCameraManager.BlockingOpenException;
import com.android.ex.camera2.blocking.BlockingSessionCallback;
import com.android.ex.camera2.blocking.BlockingStateCallback;
import com.android.ex.camera2.exceptions.TimeoutRuntimeException;

import junit.framework.Assert;

import org.mockito.Mockito;

import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.reflect.Array;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;

/**
 * A package private utility class for wrapping up the camera2 cts test common utility functions
 */
public class CameraTestUtils extends Assert {
    private static final String TAG = "CameraTestUtils";
    private static final boolean VERBOSE = Log.isLoggable(TAG, Log.VERBOSE);
    private static final boolean DEBUG = Log.isLoggable(TAG, Log.DEBUG);
    public static final Size SIZE_BOUND_1080P = new Size(1920, 1088);
    public static final Size SIZE_BOUND_2160P = new Size(3840, 2160);
    // Only test the preview size that is no larger than 1080p.
    public static final Size PREVIEW_SIZE_BOUND = SIZE_BOUND_1080P;
    // Default timeouts for reaching various states
    public static final int CAMERA_OPEN_TIMEOUT_MS = 3000;
    public static final int CAMERA_CLOSE_TIMEOUT_MS = 3000;
    public static final int CAMERA_IDLE_TIMEOUT_MS = 3000;
    public static final int CAMERA_ACTIVE_TIMEOUT_MS = 1000;
    public static final int CAMERA_BUSY_TIMEOUT_MS = 1000;
    public static final int CAMERA_UNCONFIGURED_TIMEOUT_MS = 1000;
    public static final int CAMERA_CONFIGURE_TIMEOUT_MS = 3000;
    public static final int CAPTURE_RESULT_TIMEOUT_MS = 3000;
    public static final int CAPTURE_IMAGE_TIMEOUT_MS = 3000;

    public static final int SESSION_CONFIGURE_TIMEOUT_MS = 3000;
    public static final int SESSION_CLOSE_TIMEOUT_MS = 3000;
    public static final int SESSION_READY_TIMEOUT_MS = 3000;
    public static final int SESSION_ACTIVE_TIMEOUT_MS = 1000;

    public static final int MAX_READER_IMAGES = 5;

    /**
     * Create an {@link android.media.ImageReader} object and get the surface.
     *
     * @param size The size of this ImageReader to be created.
     * @param format The format of this ImageReader to be created
     * @param maxNumImages The max number of images that can be acquired simultaneously.
     * @param listener The listener used by this ImageReader to notify callbacks.
     * @param handler The handler to use for any listener callbacks.
     */
    public static ImageReader makeImageReader(Size size, int format, int maxNumImages,
            ImageReader.OnImageAvailableListener listener, Handler handler) {
        ImageReader reader =  ImageReader.newInstance(size.getWidth(), size.getHeight(), format,
                maxNumImages);
        reader.setOnImageAvailableListener(listener, handler);
        if (VERBOSE) Log.v(TAG, "Created ImageReader size " + size);
        return reader;
    }

    /**
     * Close pending images and clean up an {@link android.media.ImageReader} object.
     * @param reader an {@link android.media.ImageReader} to close.
     */
    public static void closeImageReader(ImageReader reader) {
        if (reader != null) {
            reader.close();
        }
    }

    /**
     * Dummy listener that release the image immediately once it is available.
     *
     * <p>
     * It can be used for the case where we don't care the image data at all.
     * </p>
     */
    public static class ImageDropperListener implements ImageReader.OnImageAvailableListener {
        @Override
        public void onImageAvailable(ImageReader reader) {
            Image image = null;
            try {
                image = reader.acquireNextImage();
            } finally {
                if (image != null) {
                    image.close();
                }
            }
        }
    }

    /**
     * Image listener that release the image immediately after validating the image
     */
    public static class ImageVerifierListener implements ImageReader.OnImageAvailableListener {
        private Size mSize;
        private int mFormat;

        public ImageVerifierListener(Size sz, int format) {
            mSize = sz;
            mFormat = format;
        }

        @Override
        public void onImageAvailable(ImageReader reader) {
            Image image = null;
            try {
                image = reader.acquireNextImage();
            } finally {
                if (image != null) {
                    validateImage(image, mSize.getWidth(), mSize.getHeight(), mFormat, null);
                    image.close();
                }
            }
        }
    }

    public static class SimpleImageReaderListener
            implements ImageReader.OnImageAvailableListener {
        private final LinkedBlockingQueue<Image> mQueue =
                new LinkedBlockingQueue<Image>();

        @Override
        public void onImageAvailable(ImageReader reader) {
            try {
                mQueue.put(reader.acquireNextImage());
            } catch (InterruptedException e) {
                throw new UnsupportedOperationException(
                        "Can't handle InterruptedException in onImageAvailable");
            }
        }

        /**
         * Get an image from the image reader.
         *
         * @param timeout Timeout value for the wait.
         * @return The image from the image reader.
         */
        public Image getImage(long timeout) throws InterruptedException {
            Image image = mQueue.poll(timeout, TimeUnit.MILLISECONDS);
            assertNotNull("Wait for an image timed out in " + timeout + "ms", image);
            return image;
        }
    }

    public static class SimpleCaptureCallback extends CameraCaptureSession.CaptureCallback {
        private final LinkedBlockingQueue<CaptureResult> mQueue =
                new LinkedBlockingQueue<CaptureResult>();
        private AtomicLong mNumFramesArrived = new AtomicLong(0);

        @Override
        public void onCaptureStarted(CameraCaptureSession session, CaptureRequest request,
                long timestamp, long frameNumber)
        {
        }

        @Override
        public void onCaptureCompleted(CameraCaptureSession session, CaptureRequest request,
                TotalCaptureResult result) {
            try {
                mNumFramesArrived.incrementAndGet();
                mQueue.put(result);
            } catch (InterruptedException e) {
                throw new UnsupportedOperationException(
                        "Can't handle InterruptedException in onCaptureCompleted");
            }
        }

        @Override
        public void onCaptureFailed(CameraCaptureSession session, CaptureRequest request,
                CaptureFailure failure) {
        }

        @Override
        public void onCaptureSequenceCompleted(CameraCaptureSession session, int sequenceId,
                long frameNumber) {
        }

        public long getTotalNumFrames() {
            return mNumFramesArrived.get();
        }

        public CaptureResult getCaptureResult(long timeout) {
            try {
                CaptureResult result = mQueue.poll(timeout, TimeUnit.MILLISECONDS);
                assertNotNull("Wait for a capture result timed out in " + timeout + "ms", result);
                return result;
            } catch (InterruptedException e) {
                throw new UnsupportedOperationException("Unhandled interrupted exception", e);
            }
        }

        /**
         * Get the {@link #CaptureResult capture result} for a given
         * {@link #CaptureRequest capture request}.
         *
         * @param myRequest The {@link #CaptureRequest capture request} whose
         *            corresponding {@link #CaptureResult capture result} was
         *            being waited for
         * @param numResultsWait Number of frames to wait for the capture result
         *            before timeout.
         * @throws TimeoutRuntimeException If more than numResultsWait results are
         *            seen before the result matching myRequest arrives, or each
         *            individual wait for result times out after
         *            {@value #CAPTURE_RESULT_TIMEOUT_MS}ms.
         */
        public CaptureResult getCaptureResultForRequest(CaptureRequest myRequest,
                int numResultsWait) {
            if (numResultsWait < 0) {
                throw new IllegalArgumentException("numResultsWait must be no less than 0");
            }

            CaptureResult result;
            int i = 0;
            do {
                result = getCaptureResult(CAPTURE_RESULT_TIMEOUT_MS);
                if (result.getRequest().equals(myRequest)) {
                    return result;
                }
            } while (i++ < numResultsWait);

            throw new TimeoutRuntimeException("Unable to get the expected capture result after "
                    + "waiting for " + numResultsWait + " results");
        }

        public boolean hasMoreResults()
        {
            return mQueue.isEmpty();
        }
    }

    /**
     * Block until the camera is opened.
     *
     * <p>Don't use this to test #onDisconnected/#onError since this will throw
     * an AssertionError if it fails to open the camera device.</p>
     *
     * @return CameraDevice opened camera device
     *
     * @throws IllegalArgumentException
     *            If the handler is null, or if the handler's looper is current.
     * @throws CameraAccessException
     *            If open fails immediately.
     * @throws BlockingOpenException
     *            If open fails after blocking for some amount of time.
     * @throws TimeoutRuntimeException
     *            If opening times out. Typically unrecoverable.
     */
    public static CameraDevice openCamera(CameraManager manager, String cameraId,
            CameraDevice.StateCallback listener, Handler handler) throws CameraAccessException,
            BlockingOpenException {

        /**
         * Although camera2 API allows 'null' Handler (it will just use the current
         * thread's Looper), this is not what we want for CTS.
         *
         * In CTS the default looper is used only to process events in between test runs,
         * so anything sent there would not be executed inside a test and the test would fail.
         *
         * In this case, BlockingCameraManager#openCamera performs the check for us.
         */
        return (new BlockingCameraManager(manager)).openCamera(cameraId, listener, handler);
    }


    /**
     * Block until the camera is opened.
     *
     * <p>Don't use this to test #onDisconnected/#onError since this will throw
     * an AssertionError if it fails to open the camera device.</p>
     *
     * @throws IllegalArgumentException
     *            If the handler is null, or if the handler's looper is current.
     * @throws CameraAccessException
     *            If open fails immediately.
     * @throws BlockingOpenException
     *            If open fails after blocking for some amount of time.
     * @throws TimeoutRuntimeException
     *            If opening times out. Typically unrecoverable.
     */
    public static CameraDevice openCamera(CameraManager manager, String cameraId, Handler handler)
            throws CameraAccessException,
            BlockingOpenException {
        return openCamera(manager, cameraId, /*listener*/null, handler);
    }

    /**
     * Configure a new camera session with output surfaces.
     *
     * @param camera The CameraDevice to be configured.
     * @param outputSurfaces The surface list that used for camera output.
     * @param listener The callback CameraDevice will notify when capture results are available.
     */
    public static CameraCaptureSession configureCameraSession(CameraDevice camera,
            List<Surface> outputSurfaces,
            CameraCaptureSession.StateCallback listener, Handler handler)
            throws CameraAccessException {
        BlockingSessionCallback sessionListener = new BlockingSessionCallback(listener);
        camera.createCaptureSession(outputSurfaces, sessionListener, handler);

        return sessionListener.waitAndGetSession(SESSION_CONFIGURE_TIMEOUT_MS);
    }

    public static <T> void assertArrayNotEmpty(T arr, String message) {
        assertTrue(message, arr != null && Array.getLength(arr) > 0);
    }

    /**
     * Check if the format is a legal YUV format camera supported.
     */
    public static void checkYuvFormat(int format) {
        if ((format != ImageFormat.YUV_420_888) &&
                (format != ImageFormat.NV21) &&
                (format != ImageFormat.YV12)) {
            fail("Wrong formats: " + format);
        }
    }

    /**
     * Check if image size and format match given size and format.
     */
    public static void checkImage(Image image, int width, int height, int format) {
        assertNotNull("Input image is invalid", image);
        assertEquals("Format doesn't match", format, image.getFormat());
        assertEquals("Width doesn't match", width, image.getWidth());
        assertEquals("Height doesn't match", height, image.getHeight());
    }

    /**
     * <p>Read data from all planes of an Image into a contiguous unpadded, unpacked
     * 1-D linear byte array, such that it can be write into disk, or accessed by
     * software conveniently. It supports YUV_420_888/NV21/YV12 and JPEG input
     * Image format.</p>
     *
     * <p>For YUV_420_888/NV21/YV12/Y8/Y16, it returns a byte array that contains
     * the Y plane data first, followed by U(Cb), V(Cr) planes if there is any
     * (xstride = width, ystride = height for chroma and luma components).</p>
     *
     * <p>For JPEG, it returns a 1-D byte array contains a complete JPEG image.</p>
     */
    public static byte[] getDataFromImage(Image image) {
        assertNotNull("Invalid image:", image);
        int format = image.getFormat();
        int width = image.getWidth();
        int height = image.getHeight();
        int rowStride, pixelStride;
        byte[] data = null;

        // Read image data
        Plane[] planes = image.getPlanes();
        assertTrue("Fail to get image planes", planes != null && planes.length > 0);

        // Check image validity
        checkAndroidImageFormat(image);

        ByteBuffer buffer = null;
        // JPEG doesn't have pixelstride and rowstride, treat it as 1D buffer.
        if (format == ImageFormat.JPEG) {
            buffer = planes[0].getBuffer();
            assertNotNull("Fail to get jpeg ByteBuffer", buffer);
            data = new byte[buffer.remaining()];
            buffer.get(data);
            buffer.rewind();
            return data;
        }

        int offset = 0;
        data = new byte[width * height * ImageFormat.getBitsPerPixel(format) / 8];
        int maxRowSize = planes[0].getRowStride();
        for (int i = 0; i < planes.length; i++) {
            if (maxRowSize < planes[i].getRowStride()) {
                maxRowSize = planes[i].getRowStride();
            }
        }
        byte[] rowData = new byte[maxRowSize];
        if(VERBOSE) Log.v(TAG, "get data from " + planes.length + " planes");
        for (int i = 0; i < planes.length; i++) {
            buffer = planes[i].getBuffer();
            assertNotNull("Fail to get bytebuffer from plane", buffer);
            rowStride = planes[i].getRowStride();
            pixelStride = planes[i].getPixelStride();
            assertTrue("pixel stride " + pixelStride + " is invalid", pixelStride > 0);
            if (VERBOSE) {
                Log.v(TAG, "pixelStride " + pixelStride);
                Log.v(TAG, "rowStride " + rowStride);
                Log.v(TAG, "width " + width);
                Log.v(TAG, "height " + height);
            }
            // For multi-planar yuv images, assuming yuv420 with 2x2 chroma subsampling.
            int w = (i == 0) ? width : width / 2;
            int h = (i == 0) ? height : height / 2;
            assertTrue("rowStride " + rowStride + " should be >= width " + w , rowStride >= w);
            for (int row = 0; row < h; row++) {
                int bytesPerPixel = ImageFormat.getBitsPerPixel(format) / 8;
                int length;
                if (pixelStride == bytesPerPixel) {
                    // Special case: optimized read of the entire row
                    length = w * bytesPerPixel;
                    buffer.get(data, offset, length);
                    offset += length;
                } else {
                    // Generic case: should work for any pixelStride but slower.
                    // Use intermediate buffer to avoid read byte-by-byte from
                    // DirectByteBuffer, which is very bad for performance
                    length = (w - 1) * pixelStride + bytesPerPixel;
                    buffer.get(rowData, 0, length);
                    for (int col = 0; col < w; col++) {
                        data[offset++] = rowData[col * pixelStride];
                    }
                }
                // Advance buffer the remainder of the row stride
                if (row < h - 1) {
                    buffer.position(buffer.position() + rowStride - length);
                }
            }
            if (VERBOSE) Log.v(TAG, "Finished reading data from plane " + i);
            buffer.rewind();
        }
        return data;
    }

    /**
     * <p>Check android image format validity for an image, only support below formats:</p>
     *
     * <p>YUV_420_888/NV21/YV12, can add more for future</p>
     */
    public static void checkAndroidImageFormat(Image image) {
        int format = image.getFormat();
        Plane[] planes = image.getPlanes();
        switch (format) {
            case ImageFormat.YUV_420_888:
            case ImageFormat.NV21:
            case ImageFormat.YV12:
                assertEquals("YUV420 format Images should have 3 planes", 3, planes.length);
                break;
            case ImageFormat.JPEG:
            case ImageFormat.RAW_SENSOR:
                assertEquals("Jpeg Image should have one plane", 1, planes.length);
                break;
            default:
                fail("Unsupported Image Format: " + format);
        }
    }

    public static void dumpFile(String fileName, Bitmap data) {
        FileOutputStream outStream;
        try {
            Log.v(TAG, "output will be saved as " + fileName);
            outStream = new FileOutputStream(fileName);
        } catch (IOException ioe) {
            throw new RuntimeException("Unable to create debug output file " + fileName, ioe);
        }

        try {
            data.compress(Bitmap.CompressFormat.JPEG, /*quality*/90, outStream);
            outStream.close();
        } catch (IOException ioe) {
            throw new RuntimeException("failed writing data to file " + fileName, ioe);
        }
    }

    public static void dumpFile(String fileName, byte[] data) {
        FileOutputStream outStream;
        try {
            Log.v(TAG, "output will be saved as " + fileName);
            outStream = new FileOutputStream(fileName);
        } catch (IOException ioe) {
            throw new RuntimeException("Unable to create debug output file " + fileName, ioe);
        }

        try {
            outStream.write(data);
            outStream.close();
        } catch (IOException ioe) {
            throw new RuntimeException("failed writing data to file " + fileName, ioe);
        }
    }

    /**
     * Get the available output sizes for the user-defined {@code format}.
     *
     * <p>Note that implementation-defined/hidden formats are not supported.</p>
     */
    public static Size[] getSupportedSizeForFormat(int format, String cameraId,
            CameraManager cameraManager) throws CameraAccessException {
        CameraCharacteristics properties = cameraManager.getCameraCharacteristics(cameraId);
        assertNotNull("Can't get camera characteristics!", properties);
        if (VERBOSE) {
            Log.v(TAG, "get camera characteristics for camera: " + cameraId);
        }
        StreamConfigurationMap configMap =
                properties.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
        Size[] availableSizes = configMap.getOutputSizes(format);
        assertArrayNotEmpty(availableSizes, "availableSizes should not be empty");
        if (VERBOSE) Log.v(TAG, "Supported sizes are: " + Arrays.deepToString(availableSizes));
        return availableSizes;
    }

    /**
     * Size comparator that compares the number of pixels it covers.
     *
     * <p>If two the areas of two sizes are same, compare the widths.</p>
     */
    public static class SizeComparator implements Comparator<Size> {
        @Override
        public int compare(Size lhs, Size rhs) {
            return CameraUtils
                    .compareSizes(lhs.getWidth(), lhs.getHeight(), rhs.getWidth(), rhs.getHeight());
        }
    }

    /**
     * Get sorted size list in descending order. Remove the sizes larger than
     * the bound. If the bound is null, don't do the size bound filtering.
     */
    static public List<Size> getSupportedPreviewSizes(String cameraId,
            CameraManager cameraManager, Size bound) throws CameraAccessException {
        return getSortedSizesForFormat(cameraId, cameraManager, ImageFormat.YUV_420_888, bound);
    }

    /**
     * Get a sorted list of sizes from a given size list.
     *
     * <p>
     * The size is compare by area it covers, if the areas are same, then
     * compare the widths.
     * </p>
     *
     * @param sizeList The input size list to be sorted
     * @param ascending True if the order is ascending, otherwise descending order
     * @return The ordered list of sizes
     */
    static public List<Size> getAscendingOrderSizes(final List<Size> sizeList, boolean ascending) {
        if (sizeList == null) {
            throw new IllegalArgumentException("sizeList shouldn't be null");
        }

        Comparator<Size> comparator = new SizeComparator();
        List<Size> sortedSizes = new ArrayList<Size>();
        sortedSizes.addAll(sizeList);
        Collections.sort(sortedSizes, comparator);
        if (!ascending) {
            Collections.reverse(sortedSizes);
        }

        return sortedSizes;
    }

    /**
     * Get sorted (descending order) size list for given format. Remove the sizes larger than
     * the bound. If the bound is null, don't do the size bound filtering.
     */
    static private List<Size> getSortedSizesForFormat(String cameraId,
            CameraManager cameraManager, int format, Size bound) throws CameraAccessException {
        Comparator<Size> comparator = new SizeComparator();
        Size[] sizes = getSupportedSizeForFormat(format, cameraId, cameraManager);
        List<Size> sortedSizes = null;
        if (bound != null) {
            sortedSizes = new ArrayList<Size>(/*capacity*/1);
            for (Size sz : sizes) {
                if (comparator.compare(sz, bound) <= 0) {
                    sortedSizes.add(sz);
                }
            }
        } else {
            sortedSizes = Arrays.asList(sizes);
        }
        assertTrue("Supported size list should have at least one element",
                sortedSizes.size() > 0);

        Collections.sort(sortedSizes, comparator);
        // Make it in descending order.
        Collections.reverse(sortedSizes);
        return sortedSizes;
    }

    /**
     * Get supported video size list for a given camera device.
     *
     * <p>
     * Filter out the sizes that are larger than the bound. If the bound is
     * null, don't do the size bound filtering.
     * </p>
     */
    static public List<Size> getSupportedVideoSizes(String cameraId,
            CameraManager cameraManager, Size bound) throws CameraAccessException {
        return getSortedSizesForFormat(cameraId, cameraManager, ImageFormat.YUV_420_888, bound);
    }

    /**
     * Get supported video size list (descending order) for a given camera device.
     *
     * <p>
     * Filter out the sizes that are larger than the bound. If the bound is
     * null, don't do the size bound filtering.
     * </p>
     */
    static public List<Size> getSupportedStillSizes(String cameraId,
            CameraManager cameraManager, Size bound) throws CameraAccessException {
        return getSortedSizesForFormat(cameraId, cameraManager, ImageFormat.JPEG, bound);
    }

    static public Size getMinPreviewSize(String cameraId, CameraManager cameraManager)
            throws CameraAccessException {
        List<Size> sizes = getSupportedPreviewSizes(cameraId, cameraManager, null);
        return sizes.get(sizes.size() - 1);
    }

    /**
     * Get max supported preview size for a camera device.
     */
    static public Size getMaxPreviewSize(String cameraId, CameraManager cameraManager)
            throws CameraAccessException {
        return getMaxPreviewSize(cameraId, cameraManager, /*bound*/null);
    }

    /**
     * Get max preview size for a camera device in the supported sizes that are no larger
     * than the bound.
     */
    static public Size getMaxPreviewSize(String cameraId, CameraManager cameraManager, Size bound)
            throws CameraAccessException {
        List<Size> sizes = getSupportedPreviewSizes(cameraId, cameraManager, bound);
        return sizes.get(0);
    }

    /**
     * Get the largest size by area.
     *
     * @param sizes an array of sizes, must have at least 1 element
     *
     * @return Largest Size
     *
     * @throws IllegalArgumentException if sizes was null or had 0 elements
     */
    public static Size getMaxSize(Size[] sizes) {
        if (sizes == null || sizes.length == 0) {
            throw new IllegalArgumentException("sizes was empty");
        }

        Size sz = sizes[0];
        for (Size size : sizes) {
            if (size.getWidth() * size.getHeight() > sz.getWidth() * sz.getHeight()) {
                sz = size;
            }
        }

        return sz;
    }

    /**
     * Returns true if the given {@code array} contains the given element.
     *
     * @param array {@code array} to check for {@code elem}
     * @param elem {@code elem} to test for
     * @return {@code true} if the given element is contained
     */
    public static boolean contains(int[] array, int elem) {
        if (array == null) return false;
        for (int i = 0; i < array.length; i++) {
            if (elem == array[i]) return true;
        }
        return false;
    }

    /**
     * Get object array from byte array.
     *
     * @param array Input byte array to be converted
     * @return Byte object array converted from input byte array
     */
    public static Byte[] toObject(byte[] array) {
        return convertPrimitiveArrayToObjectArray(array, Byte.class);
    }

    /**
     * Get object array from int array.
     *
     * @param array Input int array to be converted
     * @return Integer object array converted from input int array
     */
    public static Integer[] toObject(int[] array) {
        return convertPrimitiveArrayToObjectArray(array, Integer.class);
    }

    /**
     * Get object array from float array.
     *
     * @param array Input float array to be converted
     * @return Float object array converted from input float array
     */
    public static Float[] toObject(float[] array) {
        return convertPrimitiveArrayToObjectArray(array, Float.class);
    }

    /**
     * Get object array from double array.
     *
     * @param array Input double array to be converted
     * @return Double object array converted from input double array
     */
    public static Double[] toObject(double[] array) {
        return convertPrimitiveArrayToObjectArray(array, Double.class);
    }

    /**
     * Convert a primitive input array into its object array version (e.g. from int[] to Integer[]).
     *
     * @param array Input array object
     * @param wrapperClass The boxed class it converts to
     * @return Boxed version of primitive array
     */
    private static <T> T[] convertPrimitiveArrayToObjectArray(final Object array,
            final Class<T> wrapperClass) {
        // getLength does the null check and isArray check already.
        int arrayLength = Array.getLength(array);
        if (arrayLength == 0) {
            throw new IllegalArgumentException("Input array shouldn't be empty");
        }

        @SuppressWarnings("unchecked")
        final T[] result = (T[]) Array.newInstance(wrapperClass, arrayLength);
        for (int i = 0; i < arrayLength; i++) {
            Array.set(result, i, Array.get(array, i));
        }
        return result;
    }

    /**
     * Validate image based on format and size.
     * <p>
     * Only RAW_SENSOR, YUV420_888 and JPEG formats are supported. Calling this
     * method with other formats will cause a UnsupportedOperationException.
     * </p>
     *
     * @param image The image to be validated.
     * @param width The image width.
     * @param height The image height.
     * @param format The image format.
     * @param filePath The debug dump file path, null if don't want to dump to
     *            file.
     * @throws UnsupportedOperationException if calling with format other than
     *             RAW_SENSOR, YUV420_888 or JPEG.
     */
    public static void validateImage(Image image, int width, int height, int format,
            String filePath) {
        checkImage(image, width, height, format);

        /**
         * TODO: validate timestamp:
         * 1. capture result timestamp against the image timestamp (need
         * consider frame drops)
         * 2. timestamps should be monotonically increasing for different requests
         */
        if(VERBOSE) Log.v(TAG, "validating Image");
        byte[] data = getDataFromImage(image);
        assertTrue("Invalid image data", data != null && data.length > 0);

        switch (format) {
            case ImageFormat.JPEG:
                validateJpegData(data, width, height, filePath);
                break;
            case ImageFormat.YUV_420_888:
            case ImageFormat.YV12:
                validateYuvData(data, width, height, format, image.getTimestamp(), filePath);
                break;
            case ImageFormat.RAW_SENSOR:
                validateRaw16Data(data, width, height, format, image.getTimestamp(), filePath);
                break;
            default:
                throw new UnsupportedOperationException("Unsupported format for validation: "
                        + format);
        }
    }

    /**
     * Provide a mock for {@link CameraDevice.StateCallback}.
     *
     * <p>Only useful because mockito can't mock {@link CameraDevice.StateCallback} which is an
     * abstract class.</p>
     *
     * <p>
     * Use this instead of other classes when needing to verify interactions, since
     * trying to spy on {@link BlockingStateCallback} (or others) will cause unnecessary extra
     * interactions which will cause false test failures.
     * </p>
     *
     */
    public static class MockStateCallback extends CameraDevice.StateCallback {

        @Override
        public void onOpened(CameraDevice camera) {
        }

        @Override
        public void onDisconnected(CameraDevice camera) {
        }

        @Override
        public void onError(CameraDevice camera, int error) {
        }

        private MockStateCallback() {}

        /**
         * Create a Mockito-ready mocked StateCallback.
         */
        public static MockStateCallback mock() {
            return Mockito.spy(new MockStateCallback());
        }
    }

    private static void validateJpegData(byte[] jpegData, int width, int height, String filePath) {
        BitmapFactory.Options bmpOptions = new BitmapFactory.Options();
        // DecodeBound mode: only parse the frame header to get width/height.
        // it doesn't decode the pixel.
        bmpOptions.inJustDecodeBounds = true;
        BitmapFactory.decodeByteArray(jpegData, 0, jpegData.length, bmpOptions);
        assertEquals(width, bmpOptions.outWidth);
        assertEquals(height, bmpOptions.outHeight);

        // Pixel decoding mode: decode whole image. check if the image data
        // is decodable here.
        assertNotNull("Decoding jpeg failed",
                BitmapFactory.decodeByteArray(jpegData, 0, jpegData.length));
        if (DEBUG && filePath != null) {
            String fileName =
                    filePath + "/" + width + "x" + height + ".jpeg";
            dumpFile(fileName, jpegData);
        }
    }

    private static void validateYuvData(byte[] yuvData, int width, int height, int format,
            long ts, String filePath) {
        checkYuvFormat(format);
        if (VERBOSE) Log.v(TAG, "Validating YUV data");
        int expectedSize = width * height * ImageFormat.getBitsPerPixel(format) / 8;
        assertEquals("Yuv data doesn't match", expectedSize, yuvData.length);

        // TODO: Can add data validation for test pattern.

        if (DEBUG && filePath != null) {
            String fileName =
                    filePath + "/" + width + "x" + height + "_" + ts / 1e6 + ".yuv";
            dumpFile(fileName, yuvData);
        }
    }

    private static void validateRaw16Data(byte[] rawData, int width, int height, int format,
            long ts, String filePath) {
        if (VERBOSE) Log.v(TAG, "Validating raw data");
        int expectedSize = width * height * ImageFormat.getBitsPerPixel(format) / 8;
        assertEquals("Yuv data doesn't match", expectedSize, rawData.length);

        // TODO: Can add data validation for test pattern.

        if (DEBUG && filePath != null) {
            String fileName =
                    filePath + "/" + width + "x" + height + "_" + ts / 1e6 + ".raw16";
            dumpFile(fileName, rawData);
        }

        return;
    }

    public static <T> T getValueNotNull(CaptureResult result, CaptureResult.Key<T> key) {
        if (result == null) {
            throw new IllegalArgumentException("Result must not be null");
        }

        T value = result.get(key);
        assertNotNull("Value of Key " + key.getName() + "shouldn't be null", value);
        return value;
    }

    /**
     * Get a crop region for a given zoom factor and center position.
     * <p>
     * The center position is normalized position in range of [0, 1.0], where
     * (0, 0) represents top left corner, (1.0. 1.0) represents bottom right
     * corner. The center position could limit the effective minimal zoom
     * factor, for example, if the center position is (0.75, 0.75), the
     * effective minimal zoom position becomes 2.0. If the requested zoom factor
     * is smaller than 2.0, a crop region with 2.0 zoom factor will be returned.
     * </p>
     * <p>
     * The aspect ratio of the crop region is maintained the same as the aspect
     * ratio of active array.
     * </p>
     *
     * @param zoomFactor The zoom factor to generate the crop region, it must be
     *            >= 1.0
     * @param center The normalized zoom center point that is in the range of [0, 1].
     * @param maxZoom The max zoom factor supported by this device.
     * @param activeArray The active array size of this device.
     * @return crop region for the given normalized center and zoom factor.
     */
    public static Rect getCropRegionForZoom(float zoomFactor, final PointF center,
            final float maxZoom, final Rect activeArray) {
        if (zoomFactor < 1.0) {
            throw new IllegalArgumentException("zoom factor " + zoomFactor + " should be >= 1.0");
        }
        if (center.x > 1.0 || center.x < 0) {
            throw new IllegalArgumentException("center.x " + center.x
                    + " should be in range of [0, 1.0]");
        }
        if (center.y > 1.0 || center.y < 0) {
            throw new IllegalArgumentException("center.y " + center.y
                    + " should be in range of [0, 1.0]");
        }
        if (maxZoom < 1.0) {
            throw new IllegalArgumentException("max zoom factor " + maxZoom + " should be >= 1.0");
        }
        if (activeArray == null) {
            throw new IllegalArgumentException("activeArray must not be null");
        }

        float minCenterLength = Math.min(Math.min(center.x, 1.0f - center.x),
                Math.min(center.y, 1.0f - center.y));
        float minEffectiveZoom =  0.5f / minCenterLength;
        if (minEffectiveZoom > maxZoom) {
            throw new IllegalArgumentException("Requested center " + center.toString() +
                    " has minimal zoomable factor " + minEffectiveZoom + ", which exceeds max"
                            + " zoom factor " + maxZoom);
        }

        if (zoomFactor < minEffectiveZoom) {
            Log.w(TAG, "Requested zoomFactor " + zoomFactor + " > minimal zoomable factor "
                    + minEffectiveZoom + ". It will be overwritten by " + minEffectiveZoom);
            zoomFactor = minEffectiveZoom;
        }

        int cropCenterX = (int)(activeArray.width() * center.x);
        int cropCenterY = (int)(activeArray.height() * center.y);
        int cropWidth = (int) (activeArray.width() / zoomFactor);
        int cropHeight = (int) (activeArray.height() / zoomFactor);

        return new Rect(
                /*left*/cropCenterX - cropWidth / 2,
                /*top*/cropCenterY - cropHeight / 2,
                /*right*/ cropCenterX + cropWidth / 2 - 1,
                /*bottom*/cropCenterY + cropHeight / 2 - 1);
    }

    /**
     * Calculate output 3A region from the intersection of input 3A region and cropped region.
     *
     * @param requestRegions The input 3A regions
     * @param cropRect The cropped region
     * @return expected 3A regions output in capture result
     */
    public static MeteringRectangle[] getExpectedOutputRegion(
            MeteringRectangle[] requestRegions, Rect cropRect){
        MeteringRectangle[] resultRegions = new MeteringRectangle[requestRegions.length];
        for (int i = 0; i < requestRegions.length; i++) {
            Rect requestRect = requestRegions[i].getRect();
            Rect resultRect = new Rect();
            assertTrue("Input 3A region must intersect cropped region",
                        resultRect.setIntersect(requestRect, cropRect));
            resultRegions[i] = new MeteringRectangle(
                    resultRect,
                    requestRegions[i].getMeteringWeight());
        }
        return resultRegions;
    }
}
