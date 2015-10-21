#ifndef __RS_IADST_RSH__
#define __RS_IADST_RSH__

#if RS_VERSION > 19

extern void rsIadst4(const rs_allocation input, rs_allocation output, int32_t xoff);

extern void rsIadst8(const rs_allocation input, rs_allocation output, int32_t xoff);

extern void rsIadst16(const rs_allocation input, rs_allocation output, int32_t xoff);

#endif

#endif