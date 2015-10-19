/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _SIMG2IMG_H_
#define _SIMG2IMG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define _FILE_OFFSET_BITS 64

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <sys/types.h>
#include <unistd.h>

#include <sys/types.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#if defined(__APPLE__) && defined(__MACH__)
#define lseek64 lseek
#define ftruncate64 ftruncate
#define mmap64 mmap
#define off64_t off_t
#endif

#define __le64 u64
#define __le32 u32
#define __le16 u16

#define __be64 u64
#define __be32 u32
#define __be16 u16

#define __u64 u64
#define __u32 u32
#define __u16 u16
#define __u8 u8

typedef unsigned long long u64;
typedef signed long long s64;
typedef unsigned int u32;
typedef unsigned short int u16;
typedef unsigned char u8;

int simg2img(const char *sparseImageName, const char *rawImageName);

#ifdef __cplusplus
}
#endif

#endif
