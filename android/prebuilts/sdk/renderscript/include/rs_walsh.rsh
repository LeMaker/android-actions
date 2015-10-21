#ifndef __RS_WALSH_RSH__
#define __RS_WALSH_RSH__

#if RS_VERSION > 19

extern void rsWalsh4x4(rs_allocation input, rs_allocation dest, int xoff, int yoff);
extern void rsWalsh4x4_1(rs_allocation input, rs_allocation dest, int xoff, int yoff);

#endif

#endif
