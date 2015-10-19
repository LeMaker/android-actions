#ifndef __VENC_COMMON_H__
#define __VENC_COMMON_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "./common/al_libc.h"
#include "./common/stream_input.h"

typedef enum{
  DISK_WRITE = 0x0,
  DISK_READ,
  DISK_RW,
}disk_flag_t;

stream_input_t* filestream_init(int cache_size);
int filestream_dispose(stream_input_t *stream_manage); 

int stream_open_file(stream_input_t *stream_manage,FILE *file_handle,char *file_name,int use_size,int rw_flag);
int stream_close_file(stream_input_t *stream_manage); 




#ifdef __cplusplus
}
#endif
#endif // __VENC_COMMON_H__