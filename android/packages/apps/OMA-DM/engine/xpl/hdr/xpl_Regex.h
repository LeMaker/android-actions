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

#ifndef XPL_REGEX_H
#define XPL_REGEX_H
 
#include "xpl_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===================================================================================
                                     FUNCTION PROTOTYPES
=====================================================================================*/

// DM: pseudo pattern for P2K implementation
#define XPL_RG_PATTERN_IS_DATE      "<date>"
#define XPL_RG_PATTERN_IS_TIME      "<time>"
#define XPL_RG_PATTERN_IS_FLOAT     "<float>"


/* Function shall compile the regular expression contained in the string pointed to
 * by the pattern argument and compare with string pointed by str. */
BOOLEAN XPL_RG_Comp(CPCHAR pattern, CPCHAR str);


#ifdef __cplusplus
}
#endif


#endif /* XPL_REGEX_H */
