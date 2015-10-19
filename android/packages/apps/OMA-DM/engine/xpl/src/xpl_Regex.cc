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

#include <string.h>
#include "xpl_Regex.h"
#include "dmStringUtil.h"

#include "dmNewDataTypesValidation.h"
#include "dmStringUtil.h"

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// FUNCTION: XPL_RG_Comp 
//
// DESCRIPTION: Compare the Regular Expression
//
// ARGUMENTS PASSED:
//          INPUT : CPCHAR pattern   Regular Expression Pattern
//                  CPCHAR str       Parameter's data 
//
// RETURN VALUE:    True or False
//
// IMPORTANT NOTES: 
//==============================================================================

BOOLEAN XPL_RG_Comp(CPCHAR pattern, CPCHAR str)
{
    int i;
    BOOLEAN   result = FALSE;
    DMString  value( str );

  

    if(strcmp(pattern, "[1-65535]") == 0)
   {
     i = atoi(str);
    if( i < 1 || i > 65535)
     return FALSE;
    else
     return TRUE;
   }

  else if( 0 == DmStrcmp( pattern, XPL_RG_PATTERN_IS_DATE ) )

  {

    result = ::is_date( value );

  }

  else if( 0 == DmStrcmp( pattern, XPL_RG_PATTERN_IS_TIME ) )

  {

    result = ::is_time( value );

  }

  else if( 0 == DmStrcmp( pattern, XPL_RG_PATTERN_IS_FLOAT ) )

  {

    result = ::is_float( value );

  }



  return result;

}

#ifdef __cplusplus
}
#endif
