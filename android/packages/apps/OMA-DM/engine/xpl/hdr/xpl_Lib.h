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
 *      The xpl_Lib.h header file contains constants and function prototypes
 *      for loading dynamic libraries     
 */

#ifndef XPL_LIB_H
#define XPL_LIB_H

#include "xpl_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/************** CONSTANTS ****************************************************/

#define XPL_DL_HANDLE_INVALID (0)

/************** STRUCTURES, ENUMS, AND TYPEDEFS ******************************/

typedef void * XPL_DL_HANDLE_T;

/*=============================================================================
                                     FUNCTION PROTOTYPES
===============================================================================*/

/* Loads dynamic library */
XPL_DL_HANDLE_T XPL_DL_Load(CPCHAR dllib_name);

/* Gets function pointer */
void * XPL_DL_GetFunction (XPL_DL_HANDLE_T lib_handle, CPCHAR name);

/* Unloads shared objects */
void XPL_DL_Unload(XPL_DL_HANDLE_T lib_handle);

#ifdef __cplusplus
}
#endif

#endif /* XPL_LIB_H */
