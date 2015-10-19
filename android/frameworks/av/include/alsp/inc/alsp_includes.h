#ifndef __ALSP_INCLUDES_H__
#define __ALSP_INCLUDES_H__

// 系统相关的外部头文件, 主要用于中间件等系统相关模块




#if defined(_OS_UC_)
#include <libc/stdio.h>
#include <libc/stdlib.h>
#include <libc/stdarg.h>
#include <libc/string.h>
#include <libc/pthread.h>
#include <libc/fcntl.h>
#include <libc/time.h>
#include <libc/sys/ioctl.h>
#include <libc/sys/types.h>
#include <libc/sys/mman.h>
#include <libc/dlfcn.h>
#include <libc/unistd.h>
#include <libc/semaphore.h>
#include "asm-mips/mach-atj228x/actions_reg_gl5005.h"
#include "ucos/page.h"
#include "act_mem.h"
#include "fb.h"
#endif



#ifdef _OS_UX_
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <unistd.h>
#include <semaphore.h>
#include "actions_reg_gl5005.h"
#include "dma_mem.h"
#include "fb_drv.h"                /////////atv600x framebuffer的头文件
#include "asoc_ioctl.h"            /////////该方案的ioctl命令字定义

#endif

extern int actal_error(const char *format, ...);
extern int actal_printf(const char *format, ...);

#endif //#ifndef __ALSP_INCLUDES_H__
