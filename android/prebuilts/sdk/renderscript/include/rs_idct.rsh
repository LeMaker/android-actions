#ifndef __RS_IDCT_RSH__
#define __RS_IDCT_RSH__

#if RS_VERSION > 19

extern void rsIdct4x4(const rs_allocation input, rs_allocation dest, int eob, int xoff, int yoff);
extern void rsIdct8x8(const rs_allocation input, rs_allocation dest, int eob, int xoff, int yoff);
extern void rsIdct16x16(const rs_allocation input, rs_allocation dest, int eob, int xoff, int yoff);
extern void rsIdct32x32(const rs_allocation input, rs_allocation dest, int eob, int xoff, int yoff);

#endif

#endif
