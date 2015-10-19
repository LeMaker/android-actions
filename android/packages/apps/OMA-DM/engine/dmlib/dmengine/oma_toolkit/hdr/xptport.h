/*************************************************************************/
/* module:          SyncML xpt portability header file                   */
/* file:            src/xpt/inc/xptport.h                                */
/* target system:   all                                                  */
/* target OS:       all                                                  */
/*************************************************************************/


/*
 * Copyright Notice
 * Copyright (c) Ericsson, IBM, Lotus, Matsushita Communication 
 * Industrial Co., Ltd., Motorola, Nokia, Openwave Systems, Inc., 
 * Palm, Inc., Psion, Starfish Software, Symbian, Ltd. (2001).
 * All Rights Reserved.
 * Implementation of all or part of any Specification may require 
 * licenses under third party intellectual property rights, 
 * including without limitation, patent rights (such a third party 
 * may or may not be a Supporter). The Sponsors of the Specification 
 * are not responsible and shall not be held responsible in any 
 * manner for identifying or failing to identify any or all such 
 * third party intellectual property rights.
 * 
 * THIS DOCUMENT AND THE INFORMATION CONTAINED HEREIN ARE PROVIDED 
 * ON AN "AS IS" BASIS WITHOUT WARRANTY OF ANY KIND AND ERICSSON, IBM, 
 * LOTUS, MATSUSHITA COMMUNICATION INDUSTRIAL CO. LTD, MOTOROLA, 
 * NOKIA, PALM INC., PSION, STARFISH SOFTWARE AND ALL OTHER SYNCML 
 * SPONSORS DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING 
 * BUT NOT LIMITED TO ANY WARRANTY THAT THE USE OF THE INFORMATION 
 * HEREIN WILL NOT INFRINGE ANY RIGHTS OR ANY IMPLIED WARRANTIES OF 
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
 * SHALL ERICSSON, IBM, LOTUS, MATSUSHITA COMMUNICATION INDUSTRIAL CO., 
 * LTD, MOTOROLA, NOKIA, PALM INC., PSION, STARFISH SOFTWARE OR ANY 
 * OTHER SYNCML SPONSOR BE LIABLE TO ANY PARTY FOR ANY LOSS OF 
 * PROFITS, LOSS OF BUSINESS, LOSS OF USE OF DATA, INTERRUPTION OF 
 * BUSINESS, OR FOR DIRECT, INDIRECT, SPECIAL OR EXEMPLARY, INCIDENTAL, 
 * PUNITIVE OR CONSEQUENTIAL DAMAGES OF ANY KIND IN CONNECTION WITH 
 * THIS DOCUMENT OR THE INFORMATION CONTAINED HEREIN, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH LOSS OR DAMAGE.
 * 
 * The above notice and this paragraph must be included on all copies 
 * of this document that are made.
 * 
 */


/**
 * Isolate here the differences between ANSI string and memory functions and
 * Palm string and memory functions.  Note the sml tree contains similar
 * mappings, but we avoid those for two reasons:
 *
 *  1. They are all implemented as functions, which makes calling them less
 *     efficient, especially on the Palm.  Also, if they are implemented as
 *     functions, then the xpt DLL and the DLLs of all transport
 *     implementations must be linked to the sml DLL, which seems unnecessary.
 *
 *  2. Even on the Palm, the libstr.h and libmem.h header files include ANSI C
 *     header files like string.h and stdlib.h.  These shouldn't be included
 *     on the Palm, because with some compilers, the introduced typedefs
 *     conflict with typedefs from the Palm SDK, causing compilation errors.
 *
 * We start the names of these functions with a different prefix so they are
 * not confused with functions of the official xpt interface.  We use "xpp"
 * here instead of "xpt".
 */

#ifndef XPTPORT_H
#define XPTPORT_H

#include <xptdef.h>

/*
 * If we neglect some of the return values, most of the functions have simple
 * mappings.
 */
#ifdef __PALM_OS__

 /* Palm mappings */

 #include <StringMgr.h>
 #include <MemoryMgr.h>

 #define xppStrcpy(target, source)        StrCopy(target, source)
 #define xppStrncpy(target, source, cnt)  StrNCopy(target, source, cnt)
 #define xppStrcat(target, source)        StrCat(target, source)
 #define xppStrncat(target, source, cnt)  StrNCat(target, source, cnt)
 #define xppStrcmp(first, second)         StrCompare(first, second)
 #define xppStrncmp(first, second, cnt)   StrNCompare(first, second, cnt)
 #define xppStrchr(string, c)             StrChr(string, c)
 #define xppStrstr(string, substr)        StrStr(string, substr)
 #define xppStrlen(string)                StrLen(string)
 #define xppAtoi(string)                  StrAToI(string)

 #define xppMemset(s, c, n)               MemSet(s, n, c)
 #define xppMemcpy(target, source, cnt)   MemMove(target, source, cnt)
 #define xppMemmove(target, source, cnt)  MemMove(target, source, cnt)
 #define xppMemcmp(target, source, cnt)   MemCmp(target, source, cnt)
 #define xppMalloc(size)                  MemPtrNew(size)
 XPTDECLEXP1 void * XPTAPI xppRealloc(void *ptr, size_t size) XPT_SECTION;
 #define xppFree(ptr)                     MemPtrFree(ptr)

 #define xppStricmp(first, second)        StrCaselessCompare(first, second)
 #define xppMemicmp(first, second, cnt)   StrNCaselessCompare(first, second, cnt)

#endif
#if !defined(__EPOC_OS__) && !defined(__PALM_OS__)

 /* ANSI C mappings */

 #include <string.h>
 #include <stdlib.h>

 #define xppStrcpy(target, source)        strcpy(target, source)
 #define xppStrncpy(target, source, cnt)  strncpy(target, source, cnt)
 #define xppStrcat(target, source)        strcat(target, source)
 #define xppStrncat(target, source, cnt)  strncat(target, source, cnt)
 #define xppStrcmp(first, second)         strcmp(first, second)
 #define xppStrncmp(first, second, cnt)   strncmp(first, second, cnt)
 #define xppStrchr(string, c)             strchr(string, c)
 #define xppStrstr(string, substr)        strstr(string, substr)
 #define xppStrlen(string)                strlen(string)
 #define xppAtoi(string)                  atoi(string)

 #define xppMemset(s, c, n)               memset(s, c, n)
 #define xppMemcpy(target, source, cnt)   memcpy(target, source, cnt)
 #define xppMemmove(target, source, cnt)  memmove(target, source, cnt)
 #define xppMemcmp(target, source, cnt)   memcmp(target, source, cnt)
 #define xppMalloc(size)                  malloc(size)
 #define xppRealloc(ptr, size)            realloc(ptr, size)
 #define xppFree(ptr)                     free(ptr)

 /* These aren't ANSI C functions, but they're pretty common */

 #ifdef _WIN32
  #define xppStricmp(first, second)       stricmp(first, second)
 #endif

 /* Most other systems call it strcasecmp */
 #ifndef xppStricmp
  #define xppStricmp(first, second)       strcasecmp(first, second)
 #endif

 #ifdef _WIN32
  #define xppMemicmp(first, second, cnt)  memicmp(first, second, cnt)
 #endif

 #ifndef xppMemicmp
  #define xppMemicmp(first, second, cnt)  strncasecmp(first, second, cnt)
 #endif

#endif

#if defined(__EPOC_OS__)

/* EPOC mappings */

#include <string.h>
#include <stdlib.h>

#define xppStrcpy(target, source)			strcpy(target, source)
#define xppStrncpy(target, source, cnt)		strncpy(target, source, cnt)
#define xppStrcat(target, source)			strcat(target, source)
#define xppStrncat(target, source, cnt)		strncat(target, source, cnt)
#define xppStrcmp(first, second)			strcmp(first, second)
#define xppStrncmp(first, second, cnt)		strncmp(first, second, cnt)
#define xppStrchr(string, c)				strchr(string, c)
#define xppStrstr(string, substr)			strstr(string, substr)
#define xppStrlen(string)					strlen(string)
#define xppAtoi(string)						atoi(string)

#define xppMemset(s, c, n)					memset(s, c, n)
#define xppMemcpy(target, source, cnt)		memcpy(target, source, cnt)
#define xppMemmove(target, source, cnt)		memmove(target, source, cnt)
#define xppMemcmp(target, source, cnt)		memcmp(target, source, cnt)
#define xppMalloc(size)						malloc(size)
#define xppRealloc(ptr, size)				realloc(ptr, size)
#define xppFree(ptr)						free(ptr)

#define xppStricmp(first, second)			strcasecmp(first, second)
#define xppMemicmp(first, second, cnt)		strncasecmp(first, second, cnt)

#endif

#endif
