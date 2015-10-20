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
 * Copyright © 2009 Nokia Corporation
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
 *
 * Author:  Siarhei Siamashka (siarhei.siamashka@nokia.com)
 */

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif

    .text
    .fpu neon
    .arch armv7a
    .object_arch armv4
    .eabi_attribute 10, 0 /* suppress Tag_FP_arch */
    .eabi_attribute 12, 0 /* suppress Tag_Advanced_SIMD_arch */
    .arm
    .altmacro
    .p2align 2

#include "pixman-private.h"
#include "pixman-arm-neon-asm.h"

.set RESPECT_STRICT_ALIGNMENT, 1
.set PREFETCH_TYPE_DEFAULT, PREFETCH_TYPE_ADVANCED
.set PREFETCH_DISTANCE_SIMPLE, 64

.set BILINEAR_FLAG_UNROLL_4,          0
.set BILINEAR_FLAG_UNROLL_8,          1
.set BILINEAR_FLAG_USE_ALL_NEON_REGS, 2

/* Supplementary macro for setting function attributes */
.macro pixman_asm_function fname
    .func fname
    .global fname
#ifdef __ELF__
    .hidden fname
    .type fname, %function
#endif
fname:
.endm

.macro bilinear_load_8888 reg1, reg2, tmp
    mov       TMP1, X, asr #16
    add       X, X, UX
    add       TMP1, TOP, TMP1, asl #2
    vld1.32   {reg1}, [TMP1], STRIDE
    vld1.32   {reg2}, [TMP1]
.endm

.macro bilinear_load_and_vertical_interpolate_two_8888 \
                    acc1, acc2, reg1, reg2, reg3, reg4, tmp1, tmp2

    bilinear_load_8888 reg1, reg2, tmp1
    vmull.u8  acc1, reg1, d28
    vmlal.u8  acc1, reg2, d29
    bilinear_load_8888 reg3, reg4, tmp2
    vmull.u8  acc2, reg3, d28
    vmlal.u8  acc2, reg4, d29
.endm

.macro bilinear_store_8888 numpix, tmp1, tmp2
.if numpix == 4
    vst1.32   {d0, d1}, [OUT, :128]!
.elseif numpix == 2
    vst1.32   {d0}, [OUT, :64]!
.elseif numpix == 1
    vst1.32   {d0[0]}, [OUT, :32]!
.else
    .error bilinear_store_8888 numpix is unsupported
.endif
.endm

.macro bilinear_interpolate_last_pixel src_fmt, dst_fmt
    bilinear_load_&src_fmt d0, d1, d2
    vmull.u8  q1, d0, d28
    vmlal.u8  q1, d1, d29
    /* 5 cycles bubble */
    vshll.u16 q0, d2, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q0, d2, d30
    vmlal.u16 q0, d3, d30
    /* 5 cycles bubble */
    vshrn.u32 d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
    /* 3 cycles bubble */
    vmovn.u16 d0, q0
    /* 1 cycle bubble */
    bilinear_store_&dst_fmt 1, q2, q3
.endm

.macro bilinear_interpolate_two_pixels src_fmt, dst_fmt
    bilinear_load_and_vertical_interpolate_two_&src_fmt \
                q1, q11, d0, d1, d20, d21, d22, d23
    vshll.u16 q0, d2, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q0, d2, d30
    vmlal.u16 q0, d3, d30
    vshll.u16 q10, d22, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q10, d22, d31
    vmlal.u16 q10, d23, d31
    vshrn.u32 d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d1, q10, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vadd.u16  q12, q12, q13
    vmovn.u16 d0, q0
    bilinear_store_&dst_fmt 2, q2, q3
.endm

.macro bilinear_interpolate_four_pixels src_fmt, dst_fmt
    bilinear_load_and_vertical_interpolate_four_&src_fmt \
                q1, q11, d0, d1, d20, d21, d22, d23 \
                q3, q9,  d4, d5, d16, d17, d18, d19
    pld       [TMP1, PF_OFFS]
    sub       TMP1, TMP1, STRIDE
    vshll.u16 q0, d2, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q0, d2, d30
    vmlal.u16 q0, d3, d30
    vshll.u16 q10, d22, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q10, d22, d31
    vmlal.u16 q10, d23, d31
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vshll.u16 q2, d6, #BILINEAR_INTERPOLATION_BITS
    vmlsl.u16 q2, d6, d30
    vmlal.u16 q2, d7, d30
    vshll.u16 q8, d18, #BILINEAR_INTERPOLATION_BITS
    pld       [TMP2, PF_OFFS]
    vmlsl.u16 q8, d18, d31
    vmlal.u16 q8, d19, d31
    vadd.u16  q12, q12, q13
    vshrn.u32 d0, q0, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d1, q10, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d4, q2, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshrn.u32 d5, q8, #(2 * BILINEAR_INTERPOLATION_BITS)
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vmovn.u16 d0, q0
    vmovn.u16 d1, q2
    vadd.u16  q12, q12, q13
    bilinear_store_&dst_fmt 4, q2, q3
.endm

.macro bilinear_interpolate_four_pixels_head src_fmt, dst_fmt
.ifdef have_bilinear_interpolate_four_pixels_&src_fmt&_&dst_fmt
    bilinear_interpolate_four_pixels_&src_fmt&_&dst_fmt&_head
.else
    bilinear_interpolate_four_pixels src_fmt, dst_fmt
.endif
.endm

.macro bilinear_interpolate_four_pixels_tail src_fmt, dst_fmt
.ifdef have_bilinear_interpolate_four_pixels_&src_fmt&_&dst_fmt
    bilinear_interpolate_four_pixels_&src_fmt&_&dst_fmt&_tail
.endif
.endm

.macro bilinear_interpolate_four_pixels_tail_head src_fmt, dst_fmt
.ifdef have_bilinear_interpolate_four_pixels_&src_fmt&_&dst_fmt
    bilinear_interpolate_four_pixels_&src_fmt&_&dst_fmt&_tail_head
.else
    bilinear_interpolate_four_pixels src_fmt, dst_fmt
.endif
.endm

.macro bilinear_load_and_vertical_interpolate_four_8888 \
                xacc1, xacc2, xreg1, xreg2, xreg3, xreg4, xacc2lo, xacc2hi \
                yacc1, yacc2, yreg1, yreg2, yreg3, yreg4, yacc2lo, yacc2hi

    bilinear_load_and_vertical_interpolate_two_8888 \
                xacc1, xacc2, xreg1, xreg2, xreg3, xreg4, xacc2lo, xacc2hi
    bilinear_load_and_vertical_interpolate_two_8888 \
                yacc1, yacc2, yreg1, yreg2, yreg3, yreg4, yacc2lo, yacc2hi
.endm

.macro generate_bilinear_scanline_func fname, src_fmt, dst_fmt, \
                                       src_bpp_shift, dst_bpp_shift, \
                                       prefetch_distance, flags

pixman_asm_function fname
    OUT       .req      r0
    TOP       .req      r1
    BOTTOM    .req      r2
    WT        .req      r3
    WB        .req      r4
    X         .req      r5
    UX        .req      r6
    WIDTH     .req      ip
    TMP1      .req      r3
    TMP2      .req      r4
    PF_OFFS   .req      r7
    TMP3      .req      r8
    TMP4      .req      r9
    STRIDE    .req      r2

    mov       ip, sp
    push      {r4, r5, r6, r7, r8, r9}
    mov       PF_OFFS, #prefetch_distance
    ldmia     ip, {WB, X, UX, WIDTH}
    mul       PF_OFFS, PF_OFFS, UX

.if ((flags) & BILINEAR_FLAG_USE_ALL_NEON_REGS) != 0
    vpush     {d8-d15}
.endif

    sub       STRIDE, BOTTOM, TOP
    .unreq    BOTTOM

    cmp       WIDTH, #0
    ble       3f

    vdup.u16  q12, X
    vdup.u16  q13, UX
    vdup.u8   d28, WT
    vdup.u8   d29, WB
    vadd.u16  d25, d25, d26

    /* ensure good destination alignment  */
    cmp       WIDTH, #1
    blt       0f
    tst       OUT, #(1 << dst_bpp_shift)
    beq       0f
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vadd.u16  q12, q12, q13
    bilinear_interpolate_last_pixel src_fmt, dst_fmt
    sub       WIDTH, WIDTH, #1
0:
    vadd.u16  q13, q13, q13
    vshr.u16  q15, q12, #(16 - BILINEAR_INTERPOLATION_BITS)
    vadd.u16  q12, q12, q13

    cmp       WIDTH, #2
    blt       0f
    tst       OUT, #(1 << (dst_bpp_shift + 1))
    beq       0f
    bilinear_interpolate_two_pixels src_fmt, dst_fmt
    sub       WIDTH, WIDTH, #2
0:
.if ((flags) & BILINEAR_FLAG_UNROLL_8) != 0
/*********** 8 pixels per iteration *****************/
    cmp       WIDTH, #4
    blt       0f
    tst       OUT, #(1 << (dst_bpp_shift + 2))
    beq       0f
    bilinear_interpolate_four_pixels src_fmt, dst_fmt
    sub       WIDTH, WIDTH, #4
0:
    subs      WIDTH, WIDTH, #8
    blt       1f
    mov       PF_OFFS, PF_OFFS, asr #(16 - src_bpp_shift)
    bilinear_interpolate_eight_pixels_head src_fmt, dst_fmt
    subs      WIDTH, WIDTH, #8
    blt       5f
0:
    bilinear_interpolate_eight_pixels_tail_head src_fmt, dst_fmt
    subs      WIDTH, WIDTH, #8
    bge       0b
5:
    bilinear_interpolate_eight_pixels_tail src_fmt, dst_fmt
1:
    tst       WIDTH, #4
    beq       2f
    bilinear_interpolate_four_pixels src_fmt, dst_fmt
2:
.else
/*********** 4 pixels per iteration *****************/
    subs      WIDTH, WIDTH, #4
    blt       1f
    mov       PF_OFFS, PF_OFFS, asr #(16 - src_bpp_shift)
    bilinear_interpolate_four_pixels_head src_fmt, dst_fmt
    subs      WIDTH, WIDTH, #4
    blt       5f
0:
    bilinear_interpolate_four_pixels_tail_head src_fmt, dst_fmt
    subs      WIDTH, WIDTH, #4
    bge       0b
5:
    bilinear_interpolate_four_pixels_tail src_fmt, dst_fmt
1:
/****************************************************/
.endif
    /* handle the remaining trailing pixels */
    tst       WIDTH, #2
    beq       2f
    bilinear_interpolate_two_pixels src_fmt, dst_fmt
2:
    tst       WIDTH, #1
    beq       3f
    bilinear_interpolate_last_pixel src_fmt, dst_fmt
3:
.if ((flags) & BILINEAR_FLAG_USE_ALL_NEON_REGS) != 0
    vpop      {d8-d15}
.endif
    pop       {r4, r5, r6, r7, r8, r9}
    bx        lr

    .unreq    OUT
    .unreq    TOP
    .unreq    WT
    .unreq    WB
    .unreq    X
    .unreq    UX
    .unreq    WIDTH
    .unreq    TMP1
    .unreq    TMP2
    .unreq    PF_OFFS
    .unreq    TMP3
    .unreq    TMP4
    .unreq    STRIDE
.endfunc

.endm

generate_bilinear_scanline_func \
    pixman_scaled_bilinear_scanline_8888_8888_SRC_asm_neon, 8888, 8888, \
    2, 2, 28, BILINEAR_FLAG_UNROLL_4

