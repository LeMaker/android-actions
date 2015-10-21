#ifndef __RS_DCT_RSH__
#define __RS_DCT_RSH__

#if RS_VERSION > 19

extern void rsDct4x4(rs_allocation input, rs_allocation output, int xoff, int yoff);
extern void rsDct8x8(rs_allocation input, rs_allocation output, int xoff, int yoff);
extern void rsDct16x16(rs_allocation input, rs_allocation output, int xoff, int yoff);
extern void rsDct32x32(rs_allocation input, rs_allocation out, int xoff, int yoff);
extern void rsDct32x32_rd(rs_allocation input, rs_allocation out,  int xoff, int yoff);

#endif

#endif
