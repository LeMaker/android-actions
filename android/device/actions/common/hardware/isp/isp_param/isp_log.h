#ifndef __ISP_LOG_H__
#define __ISP_LOG_H__

#ifdef ANDROID
#include <utils/Log.h>
#define printf ALOGD
#define printf_err ALOGE
#undef LOG_TAG
#define LOG_TAG "ISP_PARM"
#else
#include <stdio.h>
#define printf_err printf
#endif

#endif /* __ISP_LOG_H__ */
