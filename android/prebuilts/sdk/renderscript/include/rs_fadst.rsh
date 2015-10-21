#ifndef __RS_FADST_RSH__
#define __RS_FADST_RSH__

#if RS_VERSION > 19

extern void rsFadst4(const rs_allocation input, rs_allocation output, int32_t xoff);

extern void rsFadst8(const rs_allocation input, rs_allocation output, int32_t xoff);

extern void rsFadst16(const rs_allocation input, rs_allocation output, int32_t xoff);

#endif

#endif