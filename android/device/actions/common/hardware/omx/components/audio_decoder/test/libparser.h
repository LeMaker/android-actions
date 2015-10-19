#ifndef __LIBPARSR__
#define __LIBPARSR__

#ifdef __cplusplus
extern "C" {
#endif

#include "music_parser_lib.h"

int parser_open(char *filename, char *format, music_info_t* info);
int parser_chunk(char *buf, int *len);
int parser_dispose(void);
int parser_seek(int time_offset, int whence, int *chunk_start_time);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __LIBPARSR__