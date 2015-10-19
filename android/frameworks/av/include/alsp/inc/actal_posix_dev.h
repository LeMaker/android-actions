/*******************************************************************************
 *                              5003
 *                            Module: common
 *                 Copyright(c) 2003-2008 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>    <time>           <version >             <desc>
 *       kkli     2009-02-01 15:00     1.0             build this file
*******************************************************************************/
/*!
 * \file     actal_posix_dev.h
 * \brief    Ëã·¨posix thread½Ó¿Ú
 * \author   kkli
 * \version 1.0
 * \date  2009/2/1
*******************************************************************************/
#ifndef __ACTAL_POSIX_DEV_H__
#define __ACTAL_POSIX_DEV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>         /* for write */
#include <errno.h>
#include <pthread.h>        /* pthread_cond_*, pthread_mutex_* */
#include <semaphore.h>      /* sem_wait, sem_post */
#include <signal.h>

#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdarg.h>

#include <sys/types.h>
#include <linux/ion-owl.h>
#include "./common/al_libc.h"


#define TRUE            1
#define FALSE           0

#ifdef __cplusplus
}
#endif
#endif // __ACTAL_POSIX_DEV_H__
