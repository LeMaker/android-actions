/*
 * Copyright © 2013 The Android Open Source Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/*
 * Copyright © 2000 SuSE, Inc.
 * Copyright © 2007 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */
/*
 * Copyright © 2009 ARM Ltd, Movial Creative Technologies Oy
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of ARM Ltd not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  ARM Ltd makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author:  Ian Rickards (ian.rickards@arm.com)
 * Author:  Jonathan Morton (jonathan.morton@movial.com)
 * Author:  Markku Vire (markku.vire@movial.com)
 *
 */

#include "config.h"
#include "pixman-android.h"
#include "pixman-private.h"
#include <cpu-features.h>

static force_inline void scaled_nearest_scanline_8888_8888_none_SRC(
        uint32_t *dst, const uint32_t *src, int32_t w, pixman_fixed_t vx,
        pixman_fixed_t unit_x, pixman_fixed_t src_width_fixed) {
    uint32_t d;
    uint32_t s1, s2;
    uint8_t a1, a2;
    int x1, x2;

    while ((w -= 2) >= 0) {
        x1 = pixman_fixed_to_int(vx);
        vx += unit_x;
        s1 = *(src + x1);
        x2 = pixman_fixed_to_int(vx);
        vx += unit_x;
        s2 = *(src + x2);
        *dst++ = s1;
        *dst++ = s2;
    }
    if (w & 1) {
        x1 = pixman_fixed_to_int(vx);
        s1 = *(src + x1);
        *dst++ = s1;
    }
}

static force_inline int pixman_fixed_to_bilinear_weight(pixman_fixed_t x) {
    return (x >> (16 - BILINEAR_INTERPOLATION_BITS))
            & ((1 << BILINEAR_INTERPOLATION_BITS) - 1);
}

/*
 * For each scanline fetched from source image with PAD repeat:
 * - calculate how many pixels need to be padded on the left side
 * - calculate how many pixels need to be padded on the right side
 * - update width to only count pixels which are fetched from the image
 * All this information is returned via 'width', 'left_pad', 'right_pad'
 * arguments. The code is assuming that 'unit_x' is positive.
 *
 * Note: 64-bit math is used in order to avoid potential overflows, which
 *       is probably excessive in many cases. This particular function
 *       may need its own correctness test and performance tuning.
 */
static force_inline void pad_repeat_get_scanline_bounds(
        int32_t source_image_width, pixman_fixed_t vx, pixman_fixed_t unit_x,
        int32_t * width, int32_t * left_pad, int32_t * right_pad) {
    int64_t max_vx = (int64_t) source_image_width << 16;
    int64_t tmp;
    if (vx < 0) {
        tmp = ((int64_t) unit_x - 1 - vx) / unit_x;
        if (tmp > *width) {
            *left_pad = *width;
            *width = 0;
        } else {
            *left_pad = (int32_t) tmp;
            *width -= (int32_t) tmp;
        }
    } else {
        *left_pad = 0;
    }
    tmp = ((int64_t) unit_x - 1 - vx + max_vx) / unit_x - *left_pad;
    if (tmp < 0) {
        *right_pad = *width;
        *width = 0;
    } else if (tmp >= *width) {
        *right_pad = 0;
    } else {
        *right_pad = *width - (int32_t) tmp;
        *width = (int32_t) tmp;
    }
}

static force_inline void bilinear_pad_repeat_get_scanline_bounds(
        int32_t source_image_width, pixman_fixed_t vx, pixman_fixed_t unit_x,
        int32_t * left_pad, int32_t * left_tz, int32_t * width,
        int32_t * right_tz, int32_t * right_pad) {
    int width1 = *width, left_pad1, right_pad1;
    int width2 = *width, left_pad2, right_pad2;

    pad_repeat_get_scanline_bounds(source_image_width, vx, unit_x, &width1,
            &left_pad1, &right_pad1);
    pad_repeat_get_scanline_bounds(source_image_width, vx + pixman_fixed_1,
            unit_x, &width2, &left_pad2, &right_pad2);

    *left_pad = left_pad2;
    *left_tz = left_pad1 - left_pad2;
    *right_tz = right_pad2 - right_pad1;
    *right_pad = right_pad1;
    *width -= *left_pad + *left_tz + *right_tz + *right_pad;
}

#ifdef USE_ARM_NEON
void pixman_scaled_bilinear_scanline_8888_8888_SRC_asm_neon(uint32_t *dst,
        const uint32_t *top, const uint32_t *bottom, int wt, int wb,
        pixman_fixed_t x, pixman_fixed_t ux, int width);

static void android_bilinear_filter_neon(android_simple_image* src_image,
        android_simple_image* dst_image, float scale, int src_x, int src_y) {
    int32_t src_width = src_image->width;
    int32_t src_height = src_image->height;
    pixman_vector_t v;
    int32_t left_pad, left_tz, right_tz, right_pad;
    pixman_fixed_t unit_x, unit_y;
    int32_t width = dst_image->width;
    int32_t height = dst_image->height;
    uint32_t dst_line = 0;
    uint32_t* dst;
    int y1, y2;
    pixman_fixed_t vx, vy;
    /* reference point is the center of the pixel */
    v.vector[0] = pixman_double_to_fixed((src_x + 0.5f) * scale);
    v.vector[1] = pixman_double_to_fixed((src_y + 0.5f) * scale);
    v.vector[2] = pixman_fixed_1;
    unit_x = unit_y = pixman_double_to_fixed(scale);
    vy = v.vector[1];
    bilinear_pad_repeat_get_scanline_bounds(src_width, v.vector[0], unit_x,
            &left_pad, &left_tz, &width, &right_tz, &right_pad);
    v.vector[0] += left_pad * unit_x;
    while (--height >= 0) {
        int weight1, weight2;
        dst_image->get_scanline(dst_image, (void**)(&dst), dst_line);
        dst_line++;
        vx = v.vector[0];
        y1 = pixman_fixed_to_int(vy);
        weight2 = pixman_fixed_to_bilinear_weight(vy);
        if (weight2) {
            /* both weight1 and weight2 are smaller than BILINEAR_INTERPOLATION_RANGE */
            y2 = y1 + 1;
            weight1 = BILINEAR_INTERPOLATION_RANGE - weight2;
        } else {
            /* set both top and bottom row to the same scanline and tweak weights */
            y2 = y1;
            weight1 = weight2 = BILINEAR_INTERPOLATION_RANGE / 2;
        }
        vy += unit_y;
        uint32_t buf1[2];
        uint32_t buf2[2];
        uint32_t* src1;
        uint32_t* src2;
        /* handle top/bottom zero padding by just setting weights to 0 if needed */
        if (y1 < 0) {
            weight1 = 0;
            y1 = 0;
        }
        if (y1 >= src_height) {
            weight1 = 0;
            y1 = src_height - 1;
        }
        if (y2 < 0) {
            weight2 = 0;
            y2 = 0;
        }
        if (y2 >= src_height) {
            weight2 = 0;
            y2 = src_height - 1;
        }
        src_image->get_scanline(src_image, (void**)(&src1), y1);
        src_image->get_scanline(src_image, (void**)(&src2), y2);
        if (left_pad > 0) {
            buf1[0] = buf1[1] = 0;
            buf2[0] = buf2[1] = 0;
            pixman_scaled_bilinear_scanline_8888_8888_SRC_asm_neon(
                    dst, buf1, buf2, weight1, weight2, 0, 0, left_pad);
            dst += left_pad;
        }
        if (left_tz > 0) {
            buf1[0] = 0;
            buf1[1] = src1[0];
            buf2[0] = 0;
            buf2[1] = src2[0];
            pixman_scaled_bilinear_scanline_8888_8888_SRC_asm_neon(
                    dst, buf1, buf2, weight1, weight2,
                    pixman_fixed_frac(vx), unit_x, left_tz);
            dst += left_tz;
            vx += left_tz * unit_x;
        }
        if (width > 0) {
            pixman_scaled_bilinear_scanline_8888_8888_SRC_asm_neon(
                    dst, src1, src2, weight1, weight2, vx, unit_x, width);
            dst += width;
            vx += width * unit_x;
        }
        if (right_tz > 0) {
            buf1[0] = src1[src_width - 1];
            buf1[1] = 0;
            buf2[0] = src2[src_width - 1];
            buf2[1] = 0;
            pixman_scaled_bilinear_scanline_8888_8888_SRC_asm_neon(
                    dst, buf1, buf2, weight1, weight2,
                    pixman_fixed_frac(vx), unit_x, right_tz);
            dst += right_tz;
        }
        if (right_pad > 0) {
            buf1[0] = buf1[1] = 0;
            buf2[0] = buf2[1] = 0;
            pixman_scaled_bilinear_scanline_8888_8888_SRC_asm_neon(
                    dst, buf1, buf2, weight1, weight2, 0, 0, right_pad);
        }
    }
}
#endif // ARM_USE_NEON

static void android_nearest_filter(android_simple_image* src_image,
        android_simple_image* dst_image, float scale, int src_x, int src_y) {
    int32_t src_width = src_image->width;
    int32_t src_height = src_image->height;
    int32_t width = dst_image->width;
    int32_t height = dst_image->height;
    uint32_t dst_line = 0;
    int y;
    pixman_fixed_t src_width_fixed = pixman_int_to_fixed(src_width);
    pixman_fixed_t max_vy;
    pixman_vector_t v;
    pixman_fixed_t vx, vy;
    pixman_fixed_t unit_x, unit_y;
    int32_t left_pad, right_pad;
    uint32_t *src;
    uint32_t *dst;
    /* reference point is the center of the pixel */
    v.vector[0] = pixman_double_to_fixed((src_x + 0.5f) * scale);
    v.vector[1] = pixman_double_to_fixed((src_y + 0.5f) * scale);
    v.vector[2] = pixman_fixed_1;
    unit_x = unit_y = pixman_double_to_fixed(scale);
    vx = v.vector[0];
    vy = v.vector[1];
    pad_repeat_get_scanline_bounds(src_width, vx, unit_x,
            &width, &left_pad, &right_pad);
    vx += left_pad * unit_x;
    while (--height >= 0) {
        dst_image->get_scanline(dst_image, (void**)(&dst), dst_line);
        dst_line++;
        y = ((int) ((vy) >> 16));
        vy += unit_y;
        static const uint32_t zero[1] = { 0 };
        if (y < 0 || y >= src_height) {
            scaled_nearest_scanline_8888_8888_none_SRC(
                    dst, zero + 1, left_pad + width + right_pad,
                    -((pixman_fixed_t) 1), 0, src_width_fixed);
            continue;
        }
        src_image->get_scanline(src_image, (void**)(&src), y);
        if (left_pad > 0) {
            scaled_nearest_scanline_8888_8888_none_SRC(
                    dst, zero + 1, left_pad, -((pixman_fixed_t) 1), 0,
                    src_width_fixed);
        }
        if (width > 0) {
            scaled_nearest_scanline_8888_8888_none_SRC(
                    dst + left_pad, src + src_width, width,
                    vx - src_width_fixed, unit_x, src_width_fixed);
        }
        if (right_pad > 0) {
            scaled_nearest_scanline_8888_8888_none_SRC(
                    dst + left_pad + width, zero + 1, right_pad,
                    -((pixman_fixed_t) 1), 0, src_width_fixed);
        }
    }
}

PIXMAN_EXPORT void android_simple_scale(android_simple_image* src_image,
        android_simple_image* dst_image, float scale, int src_x, int src_y) {
#ifdef USE_ARM_NEON
    if (android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM
            && (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON)) {
        android_bilinear_filter_neon(src_image, dst_image, scale, src_x, src_y);
        return;
    }
#endif
    android_nearest_filter(src_image, dst_image, scale, src_x, src_y);
}
