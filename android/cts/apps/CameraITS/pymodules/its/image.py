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

import matplotlib
matplotlib.use('Agg')

import its.error
import pylab
import sys
import Image
import numpy
import math
import unittest
import cStringIO
import scipy.stats
import copy

DEFAULT_YUV_TO_RGB_CCM = numpy.matrix([
                                [1.000,  0.000,  1.402],
                                [1.000, -0.344, -0.714],
                                [1.000,  1.772,  0.000]])

DEFAULT_YUV_OFFSETS = numpy.array([0, 128, 128])

DEFAULT_GAMMA_LUT = numpy.array(
        [math.floor(65535 * math.pow(i/65535.0, 1/2.2) + 0.5)
         for i in xrange(65536)])

DEFAULT_INVGAMMA_LUT = numpy.array(
        [math.floor(65535 * math.pow(i/65535.0, 2.2) + 0.5)
         for i in xrange(65536)])

MAX_LUT_SIZE = 65536

def convert_capture_to_rgb_image(cap,
                                 ccm_yuv_to_rgb=DEFAULT_YUV_TO_RGB_CCM,
                                 yuv_off=DEFAULT_YUV_OFFSETS,
                                 props=None):
    """Convert a captured image object to a RGB image.

    Args:
        cap: A capture object as returned by its.device.do_capture.
        ccm_yuv_to_rgb: (Optional) the 3x3 CCM to convert from YUV to RGB.
        yuv_off: (Optional) offsets to subtract from each of Y,U,V values.
        props: (Optional) camera properties object (of static values);
            required for processing raw images.

    Returns:
        RGB float-3 image array, with pixel values in [0.0, 1.0].
    """
    w = cap["width"]
    h = cap["height"]
    if cap["format"] == "raw10":
        assert(props is not None)
        cap = unpack_raw10_capture(cap, props)
    if cap["format"] == "yuv":
        y = cap["data"][0:w*h]
        u = cap["data"][w*h:w*h*5/4]
        v = cap["data"][w*h*5/4:w*h*6/4]
        return convert_yuv420_to_rgb_image(y, u, v, w, h)
    elif cap["format"] == "jpeg":
        return decompress_jpeg_to_rgb_image(cap["data"])
    elif cap["format"] == "raw":
        assert(props is not None)
        r,gr,gb,b = convert_capture_to_planes(cap, props)
        return convert_raw_to_rgb_image(r,gr,gb,b, props, cap["metadata"])
    else:
        raise its.error.Error('Invalid format %s' % (cap["format"]))

def unpack_raw10_capture(cap, props):
    """Unpack a raw-10 capture to a raw-16 capture.

    Args:
        cap: A raw-10 capture object.
        props: Camera properties object.

    Returns:
        New capture object with raw-16 data.
    """
    # Data is packed as 4x10b pixels in 5 bytes, with the first 4 bytes holding
    # the MSPs of the pixels, and the 5th byte holding 4x2b LSBs.
    w,h = cap["width"], cap["height"]
    if w % 4 != 0:
        raise its.error.Error('Invalid raw-10 buffer width')
    cap = copy.deepcopy(cap)
    cap["data"] = unpack_raw10_image(cap["data"].reshape(h,w*5/4))
    cap["format"] = "raw"
    return cap

def unpack_raw10_image(img):
    """Unpack a raw-10 image to a raw-16 image.

    Output image will have the 10 LSBs filled in each 16b word, and the 6 MSBs
    will be set to zero.

    Args:
        img: A raw-10 image, as a uint8 numpy array.

    Returns:
        Image as a uint16 numpy array, with all row padding stripped.
    """
    if img.shape[1] % 5 != 0:
        raise its.error.Error('Invalid raw-10 buffer width')
    w = img.shape[1]*4/5
    h = img.shape[0]
    # Cut out the 4x8b MSBs and shift to bits [10:2] in 16b words.
    msbs = numpy.delete(img, numpy.s_[4::5], 1)
    msbs = msbs.astype(numpy.uint16)
    msbs = numpy.left_shift(msbs, 2)
    msbs = msbs.reshape(h,w)
    # Cut out the 4x2b LSBs and put each in bits [2:0] of their own 8b words.
    lsbs = img[::, 4::5].reshape(h,w/4)
    lsbs = numpy.right_shift(
            numpy.packbits(numpy.unpackbits(lsbs).reshape(h,w/4,4,2),3), 6)
    lsbs = lsbs.reshape(h,w)
    # Fuse the MSBs and LSBs back together
    img16 = numpy.bitwise_or(msbs, lsbs).reshape(h,w)
    return img16

def convert_capture_to_planes(cap, props=None):
    """Convert a captured image object to separate image planes.

    Decompose an image into multiple images, corresponding to different planes.

    For YUV420 captures ("yuv"):
        Returns Y,U,V planes, where the Y plane is full-res and the U,V planes
        are each 1/2 x 1/2 of the full res.

    For Bayer captures ("raw" or "raw10"):
        Returns planes in the order R,Gr,Gb,B, regardless of the Bayer pattern
        layout. Each plane is 1/2 x 1/2 of the full res.

    For JPEG captures ("jpeg"):
        Returns R,G,B full-res planes.

    Args:
        cap: A capture object as returned by its.device.do_capture.
        props: (Optional) camera properties object (of static values);
            required for processing raw images.

    Returns:
        A tuple of float numpy arrays (one per plane), consisting of pixel
            values in the range [0.0, 1.0].
    """
    w = cap["width"]
    h = cap["height"]
    if cap["format"] == "raw10":
        assert(props is not None)
        cap = unpack_raw10_capture(cap, props)
    if cap["format"] == "yuv":
        y = cap["data"][0:w*h]
        u = cap["data"][w*h:w*h*5/4]
        v = cap["data"][w*h*5/4:w*h*6/4]
        return ((y.astype(numpy.float32) / 255.0).reshape(h, w, 1),
                (u.astype(numpy.float32) / 255.0).reshape(h/2, w/2, 1),
                (v.astype(numpy.float32) / 255.0).reshape(h/2, w/2, 1))
    elif cap["format"] == "jpeg":
        rgb = decompress_jpeg_to_rgb_image(cap["data"]).reshape(w*h*3)
        return (rgb[::3].reshape(h,w,1),
                rgb[1::3].reshape(h,w,1),
                rgb[2::3].reshape(h,w,1))
    elif cap["format"] == "raw":
        assert(props is not None)
        white_level = float(props['android.sensor.info.whiteLevel'])
        img = numpy.ndarray(shape=(h*w,), dtype='<u2',
                            buffer=cap["data"][0:w*h*2])
        img = img.astype(numpy.float32).reshape(h,w) / white_level
        imgs = [img[::2].reshape(w*h/2)[::2].reshape(h/2,w/2,1),
                img[::2].reshape(w*h/2)[1::2].reshape(h/2,w/2,1),
                img[1::2].reshape(w*h/2)[::2].reshape(h/2,w/2,1),
                img[1::2].reshape(w*h/2)[1::2].reshape(h/2,w/2,1)]
        idxs = get_canonical_cfa_order(props)
        return [imgs[i] for i in idxs]
    else:
        raise its.error.Error('Invalid format %s' % (cap["format"]))

def get_canonical_cfa_order(props):
    """Returns a mapping from the Bayer 2x2 top-left grid in the CFA to
    the standard order R,Gr,Gb,B.

    Args:
        props: Camera properties object.

    Returns:
        List of 4 integers, corresponding to the positions in the 2x2 top-
            left Bayer grid of R,Gr,Gb,B, where the 2x2 grid is labeled as
            0,1,2,3 in row major order.
    """
    # Note that raw streams aren't croppable, so the cropRegion doesn't need
    # to be considered when determining the top-left pixel color.
    cfa_pat = props['android.sensor.info.colorFilterArrangement']
    if cfa_pat == 0:
        # RGGB
        return [0,1,2,3]
    elif cfa_pat == 1:
        # GRBG
        return [1,0,3,2]
    elif cfa_pat == 2:
        # GBRG
        return [2,3,0,1]
    elif cfa_pat == 3:
        # BGGR
        return [3,2,1,0]
    else:
        raise its.error.Error("Not supported")

def get_gains_in_canonical_order(props, gains):
    """Reorders the gains tuple to the canonical R,Gr,Gb,B order.

    Args:
        props: Camera properties object.
        gains: List of 4 values, in R,G_even,G_odd,B order.

    Returns:
        List of gains values, in R,Gr,Gb,B order.
    """
    cfa_pat = props['android.sensor.info.colorFilterArrangement']
    if cfa_pat in [0,1]:
        # RGGB or GRBG, so G_even is Gr
        return gains
    elif cfa_pat in [2,3]:
        # GBRG or BGGR, so G_even is Gb
        return [gains[0], gains[2], gains[1], gains[3]]
    else:
        raise its.error.Error("Not supported")

def convert_raw_to_rgb_image(r_plane, gr_plane, gb_plane, b_plane,
                             props, cap_res):
    """Convert a Bayer raw-16 image to an RGB image.

    Includes some extremely rudimentary demosaicking and color processing
    operations; the output of this function shouldn't be used for any image
    quality analysis.

    Args:
        r_plane,gr_plane,gb_plane,b_plane: Numpy arrays for each color plane
            in the Bayer image, with pixels in the [0.0, 1.0] range.
        props: Camera properties object.
        cap_res: Capture result (metadata) object.

    Returns:
        RGB float-3 image array, with pixel values in [0.0, 1.0]
    """
    # Values required for the RAW to RGB conversion.
    assert(props is not None)
    white_level = float(props['android.sensor.info.whiteLevel'])
    black_levels = props['android.sensor.blackLevelPattern']
    gains = cap_res['android.colorCorrection.gains']
    ccm = cap_res['android.colorCorrection.transform']

    # Reorder black levels and gains to R,Gr,Gb,B, to match the order
    # of the planes.
    idxs = get_canonical_cfa_order(props)
    black_levels = [black_levels[i] for i in idxs]
    gains = get_gains_in_canonical_order(props, gains)

    # Convert CCM from rational to float, as numpy arrays.
    ccm = numpy.array(its.objects.rational_to_float(ccm)).reshape(3,3)

    # Need to scale the image back to the full [0,1] range after subtracting
    # the black level from each pixel.
    scale = white_level / (white_level - max(black_levels))

    # Three-channel black levels, normalized to [0,1] by white_level.
    black_levels = numpy.array([b/white_level for b in [
            black_levels[i] for i in [0,1,3]]])

    # Three-channel gains.
    gains = numpy.array([gains[i] for i in [0,1,3]])

    h,w = r_plane.shape[:2]
    img = numpy.dstack([r_plane,(gr_plane+gb_plane)/2.0,b_plane])
    img = (((img.reshape(h,w,3) - black_levels) * scale) * gains).clip(0.0,1.0)
    img = numpy.dot(img.reshape(w*h,3), ccm.T).reshape(h,w,3).clip(0.0,1.0)
    return img

def convert_yuv420_to_rgb_image(y_plane, u_plane, v_plane,
                                w, h,
                                ccm_yuv_to_rgb=DEFAULT_YUV_TO_RGB_CCM,
                                yuv_off=DEFAULT_YUV_OFFSETS):
    """Convert a YUV420 8-bit planar image to an RGB image.

    Args:
        y_plane: The packed 8-bit Y plane.
        u_plane: The packed 8-bit U plane.
        v_plane: The packed 8-bit V plane.
        w: The width of the image.
        h: The height of the image.
        ccm_yuv_to_rgb: (Optional) the 3x3 CCM to convert from YUV to RGB.
        yuv_off: (Optional) offsets to subtract from each of Y,U,V values.

    Returns:
        RGB float-3 image array, with pixel values in [0.0, 1.0].
    """
    y = numpy.subtract(y_plane, yuv_off[0])
    u = numpy.subtract(u_plane, yuv_off[1]).view(numpy.int8)
    v = numpy.subtract(v_plane, yuv_off[2]).view(numpy.int8)
    u = u.reshape(h/2, w/2).repeat(2, axis=1).repeat(2, axis=0)
    v = v.reshape(h/2, w/2).repeat(2, axis=1).repeat(2, axis=0)
    yuv = numpy.dstack([y, u.reshape(w*h), v.reshape(w*h)])
    flt = numpy.empty([h, w, 3], dtype=numpy.float32)
    flt.reshape(w*h*3)[:] = yuv.reshape(h*w*3)[:]
    flt = numpy.dot(flt.reshape(w*h,3), ccm_yuv_to_rgb.T).clip(0, 255)
    rgb = numpy.empty([h, w, 3], dtype=numpy.uint8)
    rgb.reshape(w*h*3)[:] = flt.reshape(w*h*3)[:]
    return rgb.astype(numpy.float32) / 255.0

def load_yuv420_to_rgb_image(yuv_fname,
                             w, h,
                             ccm_yuv_to_rgb=DEFAULT_YUV_TO_RGB_CCM,
                             yuv_off=DEFAULT_YUV_OFFSETS):
    """Load a YUV420 image file, and return as an RGB image.

    Args:
        yuv_fname: The path of the YUV420 file.
        w: The width of the image.
        h: The height of the image.
        ccm_yuv_to_rgb: (Optional) the 3x3 CCM to convert from YUV to RGB.
        yuv_off: (Optional) offsets to subtract from each of Y,U,V values.

    Returns:
        RGB float-3 image array, with pixel values in [0.0, 1.0].
    """
    with open(yuv_fname, "rb") as f:
        y = numpy.fromfile(f, numpy.uint8, w*h, "")
        v = numpy.fromfile(f, numpy.uint8, w*h/4, "")
        u = numpy.fromfile(f, numpy.uint8, w*h/4, "")
        return convert_yuv420_to_rgb_image(y,u,v,w,h,ccm_yuv_to_rgb,yuv_off)

def load_yuv420_to_yuv_planes(yuv_fname, w, h):
    """Load a YUV420 image file, and return separate Y, U, and V plane images.

    Args:
        yuv_fname: The path of the YUV420 file.
        w: The width of the image.
        h: The height of the image.

    Returns:
        Separate Y, U, and V images as float-1 Numpy arrays, pixels in [0,1].
        Note that pixel (0,0,0) is not black, since U,V pixels are centered at
        0.5, and also that the Y and U,V plane images returned are different
        sizes (due to chroma subsampling in the YUV420 format).
    """
    with open(yuv_fname, "rb") as f:
        y = numpy.fromfile(f, numpy.uint8, w*h, "")
        v = numpy.fromfile(f, numpy.uint8, w*h/4, "")
        u = numpy.fromfile(f, numpy.uint8, w*h/4, "")
        return ((y.astype(numpy.float32) / 255.0).reshape(h, w, 1),
                (u.astype(numpy.float32) / 255.0).reshape(h/2, w/2, 1),
                (v.astype(numpy.float32) / 255.0).reshape(h/2, w/2, 1))

def decompress_jpeg_to_rgb_image(jpeg_buffer):
    """Decompress a JPEG-compressed image, returning as an RGB image.

    Args:
        jpeg_buffer: The JPEG stream.

    Returns:
        A numpy array for the RGB image, with pixels in [0,1].
    """
    img = Image.open(cStringIO.StringIO(jpeg_buffer))
    w = img.size[0]
    h = img.size[1]
    return numpy.array(img).reshape(h,w,3) / 255.0

def apply_lut_to_image(img, lut):
    """Applies a LUT to every pixel in a float image array.

    Internally converts to a 16b integer image, since the LUT can work with up
    to 16b->16b mappings (i.e. values in the range [0,65535]). The lut can also
    have fewer than 65536 entries, however it must be sized as a power of 2
    (and for smaller luts, the scale must match the bitdepth).

    For a 16b lut of 65536 entries, the operation performed is:

        lut[r * 65535] / 65535 -> r'
        lut[g * 65535] / 65535 -> g'
        lut[b * 65535] / 65535 -> b'

    For a 10b lut of 1024 entries, the operation becomes:

        lut[r * 1023] / 1023 -> r'
        lut[g * 1023] / 1023 -> g'
        lut[b * 1023] / 1023 -> b'

    Args:
        img: Numpy float image array, with pixel values in [0,1].
        lut: Numpy table encoding a LUT, mapping 16b integer values.

    Returns:
        Float image array after applying LUT to each pixel.
    """
    n = len(lut)
    if n <= 0 or n > MAX_LUT_SIZE or (n & (n - 1)) != 0:
        raise its.error.Error('Invalid arg LUT size: %d' % (n))
    m = float(n-1)
    return (lut[(img * m).astype(numpy.uint16)] / m).astype(numpy.float32)

def apply_matrix_to_image(img, mat):
    """Multiplies a 3x3 matrix with each float-3 image pixel.

    Each pixel is considered a column vector, and is left-multiplied by
    the given matrix:

        [     ]   r    r'
        [ mat ] * g -> g'
        [     ]   b    b'

    Args:
        img: Numpy float image array, with pixel values in [0,1].
        mat: Numpy 3x3 matrix.

    Returns:
        The numpy float-3 image array resulting from the matrix mult.
    """
    h = img.shape[0]
    w = img.shape[1]
    img2 = numpy.empty([h, w, 3], dtype=numpy.float32)
    img2.reshape(w*h*3)[:] = (numpy.dot(img.reshape(h*w, 3), mat.T)
                             ).reshape(w*h*3)[:]
    return img2

def get_image_patch(img, xnorm, ynorm, wnorm, hnorm):
    """Get a patch (tile) of an image.

    Args:
        img: Numpy float image array, with pixel values in [0,1].
        xnorm,ynorm,wnorm,hnorm: Normalized (in [0,1]) coords for the tile.

    Returns:
        Float image array of the patch.
    """
    hfull = img.shape[0]
    wfull = img.shape[1]
    xtile = math.ceil(xnorm * wfull)
    ytile = math.ceil(ynorm * hfull)
    wtile = math.floor(wnorm * wfull)
    htile = math.floor(hnorm * hfull)
    return img[ytile:ytile+htile,xtile:xtile+wtile,:].copy()

def compute_image_means(img):
    """Calculate the mean of each color channel in the image.

    Args:
        img: Numpy float image array, with pixel values in [0,1].

    Returns:
        A list of mean values, one per color channel in the image.
    """
    means = []
    chans = img.shape[2]
    for i in xrange(chans):
        means.append(numpy.mean(img[:,:,i], dtype=numpy.float64))
    return means

def compute_image_variances(img):
    """Calculate the variance of each color channel in the image.

    Args:
        img: Numpy float image array, with pixel values in [0,1].

    Returns:
        A list of mean values, one per color channel in the image.
    """
    variances = []
    chans = img.shape[2]
    for i in xrange(chans):
        variances.append(numpy.var(img[:,:,i], dtype=numpy.float64))
    return variances

def write_image(img, fname, apply_gamma=False):
    """Save a float-3 numpy array image to a file.

    Supported formats: PNG, JPEG, and others; see PIL docs for more.

    Image can be 3-channel, which is interpreted as RGB, or can be 1-channel,
    which is greyscale.

    Can optionally specify that the image should be gamma-encoded prior to
    writing it out; this should be done if the image contains linear pixel
    values, to make the image look "normal".

    Args:
        img: Numpy image array data.
        fname: Path of file to save to; the extension specifies the format.
        apply_gamma: (Optional) apply gamma to the image prior to writing it.
    """
    if apply_gamma:
        img = apply_lut_to_image(img, DEFAULT_GAMMA_LUT)
    (h, w, chans) = img.shape
    if chans == 3:
        Image.fromarray((img * 255.0).astype(numpy.uint8), "RGB").save(fname)
    elif chans == 1:
        img3 = (img * 255.0).astype(numpy.uint8).repeat(3).reshape(h,w,3)
        Image.fromarray(img3, "RGB").save(fname)
    else:
        raise its.error.Error('Unsupported image type')

def downscale_image(img, f):
    """Shrink an image by a given integer factor.

    This function computes output pixel values by averaging over rectangular
    regions of the input image; it doesn't skip or sample pixels, and all input
    image pixels are evenly weighted.

    If the downscaling factor doesn't cleanly divide the width and/or height,
    then the remaining pixels on the right or bottom edge are discarded prior
    to the downscaling.

    Args:
        img: The input image as an ndarray.
        f: The downscaling factor, which should be an integer.

    Returns:
        The new (downscaled) image, as an ndarray.
    """
    h,w,chans = img.shape
    f = int(f)
    assert(f >= 1)
    h = (h/f)*f
    w = (w/f)*f
    img = img[0:h:,0:w:,::]
    chs = []
    for i in xrange(chans):
        ch = img.reshape(h*w*chans)[i::chans].reshape(h,w)
        ch = ch.reshape(h,w/f,f).mean(2).reshape(h,w/f)
        ch = ch.T.reshape(w/f,h/f,f).mean(2).T.reshape(h/f,w/f)
        chs.append(ch.reshape(h*w/(f*f)))
    img = numpy.vstack(chs).T.reshape(h/f,w/f,chans)
    return img

def __get_color_checker_patch(img, xc,yc, patch_size):
    r = patch_size/2
    tile = img[yc-r:yc+r:, xc-r:xc+r:, ::]
    return tile

def __measure_color_checker_patch(img, xc,yc, patch_size):
    tile = __get_color_checker_patch(img, xc,yc, patch_size)
    means = tile.mean(1).mean(0)
    return means

def get_color_checker_chart_patches(img, debug_fname_prefix=None):
    """Return the center coords of each patch in a color checker chart.

    Assumptions:
    * Chart is vertical or horizontal w.r.t. camera, but not diagonal.
    * Chart is (roughly) planar-parallel to the camera.
    * Chart is centered in frame (roughly).
    * Around/behind chart is white/grey background.
    * The only black pixels in the image are from the chart.
    * Chart is 100% visible and contained within image.
    * No other objects within image.
    * Image is well-exposed.
    * Standard color checker chart with standard-sized black borders.

    The values returned are in the coordinate system of the chart; that is,
    patch (0,0) is the brown patch that is in the chart's top-left corner when
    it is in the normal upright/horizontal orientation. (The chart may be any
    of the four main orientations in the image.)

    Args:
        img: Input image, as a numpy array with pixels in [0,1].
        debug_fname_prefix: If not None, the (string) name of a file prefix to
            use to save a number of debug images for visualizing the output of
            this function; can be used to see if the patches are being found
            successfully.

    Returns:
        6x4 list of lists of integer (x,y) coords of the center of each patch,
        ordered in the "chart order" (6x4 row major).
    """

    # Shrink the original image.
    DOWNSCALE_FACTOR = 4
    img_small = downscale_image(img, DOWNSCALE_FACTOR)

    # Make a threshold image, which is 1.0 where the image is black,
    # and 0.0 elsewhere.
    BLACK_PIXEL_THRESH = 0.2
    mask_img = scipy.stats.threshold(
                img_small.max(2), BLACK_PIXEL_THRESH, 1.1, 0.0)
    mask_img = 1.0 - scipy.stats.threshold(mask_img, -0.1, 0.1, 1.0)

    if debug_fname_prefix is not None:
        h,w = mask_img.shape
        write_image(img, debug_fname_prefix+"_0.jpg")
        write_image(mask_img.repeat(3).reshape(h,w,3),
                debug_fname_prefix+"_1.jpg")

    # Mask image flattened to a single row or column (by averaging).
    # Also apply a threshold to these arrays.
    FLAT_PIXEL_THRESH = 0.05
    flat_row = mask_img.mean(0)
    flat_col = mask_img.mean(1)
    flat_row = [0 if v < FLAT_PIXEL_THRESH else 1 for v in flat_row]
    flat_col = [0 if v < FLAT_PIXEL_THRESH else 1 for v in flat_col]

    # Start and end of the non-zero region of the flattened row/column.
    flat_row_nonzero = [i for i in range(len(flat_row)) if flat_row[i]>0]
    flat_col_nonzero = [i for i in range(len(flat_col)) if flat_col[i]>0]
    flat_row_min, flat_row_max = min(flat_row_nonzero), max(flat_row_nonzero)
    flat_col_min, flat_col_max = min(flat_col_nonzero), max(flat_col_nonzero)

    # Orientation of chart, and number of grid cells horz. and vertically.
    orient = "h" if flat_row_max-flat_row_min>flat_col_max-flat_col_min else "v"
    xgrids = 6 if orient=="h" else 4
    ygrids = 6 if orient=="v" else 4

    # Get better bounds on the patches region, lopping off some of the excess
    # black border.
    HRZ_BORDER_PAD_FRAC = 0.0138
    VERT_BORDER_PAD_FRAC = 0.0395
    xpad = HRZ_BORDER_PAD_FRAC if orient=="h" else VERT_BORDER_PAD_FRAC
    ypad = HRZ_BORDER_PAD_FRAC if orient=="v" else VERT_BORDER_PAD_FRAC
    xchart = flat_row_min + (flat_row_max - flat_row_min) * xpad
    ychart = flat_col_min + (flat_col_max - flat_col_min) * ypad
    wchart = (flat_row_max - flat_row_min) * (1 - 2*xpad)
    hchart = (flat_col_max - flat_col_min) * (1 - 2*ypad)

    # Get the colors of the 4 corner patches, in clockwise order, by measuring
    # the average value of a small patch at each of the 4 patch centers.
    colors = []
    centers = []
    for (x,y) in [(0,0), (xgrids-1,0), (xgrids-1,ygrids-1), (0,ygrids-1)]:
        xc = xchart + (x + 0.5)*wchart/xgrids
        yc = ychart + (y + 0.5)*hchart/ygrids
        xc = int(xc * DOWNSCALE_FACTOR + 0.5)
        yc = int(yc * DOWNSCALE_FACTOR + 0.5)
        centers.append((xc,yc))
        chan_means = __measure_color_checker_patch(img, xc,yc, 32)
        colors.append(sum(chan_means) / len(chan_means))

    # The brightest corner is the white patch, the darkest is the black patch.
    # The black patch should be counter-clockwise from the white patch.
    white_patch_index = None
    for i in range(4):
        if colors[i] == max(colors) and \
                colors[(i-1+4)%4] == min(colors):
            white_patch_index = i%4
    assert(white_patch_index is not None)

    # Return the coords of the origin (top-left when the chart is in the normal
    # upright orientation) patch's center, and the vector displacement to the
    # center of the second patch on the first row of the chart (when in the
    # normal upright orientation).
    origin_index = (white_patch_index+1)%4
    prev_index = (origin_index-1+4)%4
    next_index = (origin_index+1)%4
    origin_center = centers[origin_index]
    prev_center = centers[prev_index]
    next_center = centers[next_index]
    vec_across = tuple([(next_center[i]-origin_center[i])/5.0 for i in [0,1]])
    vec_down = tuple([(prev_center[i]-origin_center[i])/3.0 for i in [0,1]])

    # Compute the center of each patch.
    patches = [[],[],[],[]]
    for yi in range(4):
        for xi in range(6):
            x0,y0 = origin_center
            dxh,dyh = vec_across
            dxv,dyv = vec_down
            xc = int(x0 + dxh*xi + dxv*yi)
            yc = int(y0 + dyh*xi + dyv*yi)
            patches[yi].append((xc,yc))

    # Sanity check: test that the R,G,B,black,white patches are correct.
    sanity_failed = False
    patch_info = [(2,2,[0]), # Red
                  (2,1,[1]), # Green
                  (2,0,[2]), # Blue
                  (3,0,[0,1,2]), # White
                  (3,5,[])] # Black
    for i in range(len(patch_info)):
        yi,xi,high_chans = patch_info[i]
        low_chans = [i for i in [0,1,2] if i not in high_chans]
        xc,yc = patches[yi][xi]
        means = __measure_color_checker_patch(img, xc,yc, 64)
        if (min([means[i] for i in high_chans]+[1]) < \
                max([means[i] for i in low_chans]+[0])):
            sanity_failed = True

    if debug_fname_prefix is not None:
        gridimg = numpy.zeros([4*(32+2), 6*(32+2), 3])
        for yi in range(4):
            for xi in range(6):
                xc,yc = patches[yi][xi]
                tile = __get_color_checker_patch(img, xc,yc, 32)
                gridimg[yi*(32+2)+1:yi*(32+2)+1+32,
                        xi*(32+2)+1:xi*(32+2)+1+32, :] = tile
        write_image(gridimg, debug_fname_prefix+"_2.png")

    assert(not sanity_failed)

    return patches

class __UnitTest(unittest.TestCase):
    """Run a suite of unit tests on this module.
    """

    # TODO: Add more unit tests.

    def test_apply_matrix_to_image(self):
        """Unit test for apply_matrix_to_image.

        Test by using a canned set of values on a 1x1 pixel image.

            [ 1 2 3 ]   [ 0.1 ]   [ 1.4 ]
            [ 4 5 6 ] * [ 0.2 ] = [ 3.2 ]
            [ 7 8 9 ]   [ 0.3 ]   [ 5.0 ]
               mat         x         y
        """
        mat = numpy.array([[1,2,3],[4,5,6],[7,8,9]])
        x = numpy.array([0.1,0.2,0.3]).reshape(1,1,3)
        y = apply_matrix_to_image(x, mat).reshape(3).tolist()
        y_ref = [1.4,3.2,5.0]
        passed = all([math.fabs(y[i] - y_ref[i]) < 0.001 for i in xrange(3)])
        self.assertTrue(passed)

    def test_apply_lut_to_image(self):
        """ Unit test for apply_lut_to_image.

        Test by using a canned set of values on a 1x1 pixel image. The LUT will
        simply double the value of the index:

            lut[x] = 2*x
        """
        lut = numpy.array([2*i for i in xrange(65536)])
        x = numpy.array([0.1,0.2,0.3]).reshape(1,1,3)
        y = apply_lut_to_image(x, lut).reshape(3).tolist()
        y_ref = [0.2,0.4,0.6]
        passed = all([math.fabs(y[i] - y_ref[i]) < 0.001 for i in xrange(3)])
        self.assertTrue(passed)

if __name__ == '__main__':
    unittest.main()

