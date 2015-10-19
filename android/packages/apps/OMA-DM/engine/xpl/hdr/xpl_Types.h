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

#ifndef XPL_TYPES_H
#define XPL_TYPES_H

/**
    \file xpl_Types.h
    \brief The xpl_Types.h header file contains constants and basic types definition
*/

#include <stdbool.h>

/************** CONSTANTS ****************************************************/

#ifndef TRUE
/** Define TRUE */
#define TRUE  1
#endif

#ifndef FALSE
/** Define FALSE */
#define FALSE 0
#endif

#ifndef XPL_NULL
/** Define XPL_NULL */
  #define XPL_NULL (0)
#endif

#ifndef NULL
/** Define NULL */
  #define NULL ((void*) 0)
#endif


/************** STRUCTURES, ENUMS, AND TYPEDEFS ******************************/

#ifndef INT8
/** Definition INT8 as signed char */
typedef signed char INT8;
#endif

#ifndef UINT8
/** Definition UINT8 as unsigned char */
typedef unsigned char UINT8;
#endif

#ifndef INT16
/** Definition INT16 as short integer */
typedef short int INT16;
#endif 

#ifndef UINT16
/**  Definition UINT16 as unsigned short integer*/
typedef unsigned short int UINT16;
#endif

#ifndef INT32
/** Definition INT32 as integer*/
typedef int INT32;
#endif

#ifndef UINT32
/** Definition UINT32 as unsigned integer */
typedef unsigned int UINT32;
#endif

#ifndef INT64
/** Definition INT64 as long integer */
typedef long long int INT64;
#endif

#ifndef UINT64
/** Definition UINT64 as unsigned long integer */
typedef unsigned long long int UINT64;
#endif


#ifndef BOOLEAN
/** Definition BOOLEAN as unsigned char*/
typedef unsigned char BOOLEAN;
#endif

#ifndef BOOLTYPE
/**
*  BOOLTYPE introduced for EZX backward compatibility . 
*  It should be used in DmtData and DmtNode constructor and access methods only. 
*/
typedef bool BOOLTYPE;
#endif

/** Definition CPCHAR as c onstant character pointer */
typedef const char* CPCHAR;

#ifndef FLOAT
/** Definition FLOAT as float*/
typedef float FLOAT;
#endif

#endif /* XPL_TYPES_H */
