#ifndef __FORMAT_CHECK_H__
#define __FORMAT_CHECK_H__

#include "actal_posix_dev.h"
#include "format_dev.h"
#define LOG_TAG "format_check"
#include <utils/Log.h>
#include<dlfcn.h>
#define actal_printf ALOGD
#define actal_error ALOGE

#ifdef __cplusplus
extern "C" {
#endif

int mpccheck(storage_io_t  *input);
int mp3check(storage_io_t *input);
int aacflag(storage_io_t *input);
int wmaflag(storage_io_t *input);
int aacplusflag(storage_io_t *input);
int adts_aac_check(storage_io_t *input);
int dts_check(storage_io_t  *input);
int rm_check(storage_io_t *input);
int ts_check(storage_io_t *input);
#ifdef __cplusplus
}
#endif

#endif // __FORMAT_CHECK_H__
