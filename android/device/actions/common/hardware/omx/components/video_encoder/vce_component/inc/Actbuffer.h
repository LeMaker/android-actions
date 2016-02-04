#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "omx_malloc.h"
#include "buffer_mng.h"

#define  DEFAULT_RingBuffers_NO  90
#define  MIN_RingBuffers_NO    60
#define  MAX_RingBuffers_NO  MAX_VCE_MNG_Buffers/*120*/

int pool_open(unsigned int size, void **pool);
void pool_dispose(void *pool);
void *get_wbuf(void *pool, int size);
void *get_wbuf_phy(void *pool);
int move_wptr(void *pool, int size);
void free_wbuf(void *pool, void *ptr, int allocLen);
int clearBufferPool(void *pool);
int get_poolsize(void *pool);
unsigned char *get_poolbase(void *pool);

#endif
