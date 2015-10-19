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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _DM_TPT_UTILS_H
#define _DM_TPT_UTILS_H

//-----------------------------------------------------------------------
//                                                                               
//   Header Name: dm_tpt_utils.h
//
//   General Description: This file contains Transport utility function 
//	 prototypes. 
//-----------------------------------------------------------------------

#include "syncml_dm_data_types.h"

//-----------------------------------------------------------------------
//                            MACROS
//-----------------------------------------------------------------------

#define SKIP_WHITESPACE(s) \
    while (((s)[0] == ' ')||((s)[0] == ',')||((s)[0] == '\t')||((s)[0] == ';')) (s) ++

//-----------------------------------------------------------------------
//                        FUNCTION PROTOTYPES
//-----------------------------------------------------------------------


UINT8* DMTPTTrimString(UINT8 *pbOrigHmacStr);

UINT8* DM_TPT_splitParamValue (UINT8* pbLine, // i: line
                               UINT8** ppbParm,  // o: ptr to extracted parameter
                               UINT8** ppbValue);  // o: ptr to extracted parameter value

#endif //_DM_TPT_UTILS_H

#ifdef __cplusplus
}
#endif
