/*************************************************************************/
/* module:          SyncML Communication Protocol include file (Windows) */
/* file:            src/xpt/win/xptdef.h                                 */
/* target system:   win                                                  */
/* target OS:       win                                                  */
/* Purpose:         Define platform-specific values for public header    */
/*                  files.                                               */
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

#ifndef XPTDEF_H
#define XPTDEF_H

#include <stdlib.h>     /* For NULL and size_t */

#define XPT_SECTION
#define XPT_DATA_SECTION



/* TK: to improve interoperability and handling we 
 * switched to using .def files instead of compiler
 * specific per function definitions. As long as we only
 * use C this is the easiest and cleanes way
 */
#define XPT_API 
#define XPT_API_DEF
#define XPTAPI
#define XPTEXP1 //__declspec(dllexport)
#define XPTEXP2
#define XPTDECLEXP1 //__declspec(dllexport)

/* TK: Old, now obsolete code follows here */
#ifdef FOOBAZZBUMMBAGGEL

#if defined(__IBMC__) || defined(__IBMCPP__)
 #define XPTAPI __stdcall
 #define XPTEXP1
 #define XPTEXP2 _Export
 #define XPTDECLEXP1
#else
 #define XPTAPI __stdcall
 #define XPTEXP1 __declspec(dllexport)
 #define XPTEXP2

// #ifdef BUILDING_XPT
 #ifdef BUILDING_DLL
  #define XPTDECLEXP1 XPTEXP1
 #else
  #define XPTDECLEXP1 __declspec(dllimport)
 #endif
#endif
#endif // FOBAZZBUMMBAGGEL

#define stricmp _stricmp
#define memicmp _memicmp

#endif
