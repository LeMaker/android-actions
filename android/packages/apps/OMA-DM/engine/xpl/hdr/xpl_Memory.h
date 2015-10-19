/*
 * Copyright (C) 2014 The Android Open Source Project
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

/*
 *  DESCRIPTION:
 *      The XPL_Memory.h header file contains constants and function prototypes
 *      for memory management     
 */

#ifndef XPL_MEMORY_H
#define XPL_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

/************** HEADER FILE INCLUDES *****************************************/

#include <stddef.h>
#include <string.h>
#include "xpl_Types.h"

/************** CONSTANTS ****************************************************/

#define xplAllocMem(bufsize) xplAllocMemEx(bufsize,__FILE__,__LINE__)
#define xplFreeMem(buf) { xplFreeMemEx(buf,__FILE__,__LINE__); (buf) = XPL_NULL; }


#define XPL_FreeAndSetNull(_x) \
	if ((_x)!=NULL) { xplFreeMem(_x); (_x)=XPL_NULL; }


/*==============================================================================
                                     FUNCTION PROTOTYPES
================================================================================*/

/* Allocates memory buffer */
void * xplAllocMemEx(UINT32 bufsize, CPCHAR szFile, int nLine );

/* Deallocate memory buffer */
void xplFreeMemEx(void *ptr, CPCHAR szFile, int nLine);


#ifdef __cplusplus
}
#endif

#endif /* XPL_MEMORY_H */
