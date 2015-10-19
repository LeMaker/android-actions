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

//-----------------------------------------------------------------------
//
//   Module Name: dm_tpt_utils.c
//
//   General Description: This file contains the Implementation of
//                        utility functions: DMTPT_PostMessage,
//                                           DMTPT_Post_DataMessages &
//                                           DM_TPT_splitParamValue
//
//------------------------------------------------------------------------

/* This file contains Transport utility function prototypes. */
#include "dm_tpt_utils.h"

//==============================================================================
// FUNCTION:  DMTPTTrimString
//
// DESCRIPTION: Trim the Input string: Remove the blank spaces and tabs.
//
// ARGUMENTS PASSED:
//          INPUT : UINT8* pbOrigHmacStr   String to be trimmed
//
// RETURN VALUE: Pointer to the trimmed string
//
//
// IMPORTANT NOTES: This utility function is called to trim the input string
//==============================================================================
UINT8* DMTPTTrimString(UINT8 *pbOrigHmacStr)
{

  UINT8 bIndex1 =0;
  UINT8 bIndex2 =0;
  UINT8 bOrigHmacStrlen =0;
  UINT8 *pbHmacStr = NULL;

  bOrigHmacStrlen = DmStrlen((CPCHAR)pbOrigHmacStr);

  pbHmacStr = (UINT8*)DmAllocMem((sizeof(UINT8)*(bOrigHmacStrlen+1)));

  if (pbHmacStr == NULL)
  {
    return NULL;
  }

  memset(pbHmacStr,0,bOrigHmacStrlen+1);

  for (bIndex1 = 0; bIndex1 < bOrigHmacStrlen; bIndex1++)
  {

    if ((pbOrigHmacStr[bIndex1] != ' ') && (pbOrigHmacStr[bIndex1] != '\t' ))
    {
      pbHmacStr[bIndex2] = pbOrigHmacStr[bIndex1] ;
      bIndex2++;
    }

  }

  return pbHmacStr;
}

//==============================================================================
// FUNCTION: DM_TPT_splitParamValue
//
// DESCRIPTION: Parse one parameter from the specified line.
// it is assumed that the parameter has the following format:
//           parm={ value | "value" }[,] [;]
//          The function changes the contents of pbLine!!
//
// ARGUMENTS PASSED:
//          INPUT : UINT8* pbLine,   String to be split up
//          OUTPUT:
//                  UINT8** ppbParam   ptr to extracted parameter
//                  UINT8** ppbValue  ptr to extracted parameter value
//
//
//
// RETURN VALUE: Return the rest of the string, or NULL if here are no more
//
//
// IMPORTANT NOTES: This utility function is called to parse the string
//                  of the form parameter[;/,]value
//==============================================================================
UINT8* DM_TPT_splitParamValue (UINT8* pbLine,
                               UINT8** ppbParam,
                               UINT8** ppbValue)
{
    UINT8* pbToken = pbLine;
    UINT8* pbRest = NULL;

    if (pbToken == NULL)
    {
        return NULL;
    }

    /* skip leading blanks */
    SKIP_WHITESPACE (pbToken);
    pbRest = pbToken;
    *ppbParam = pbRest;

    if (pbToken [0] == '\0')
    {
        return NULL;
    }

    /* Find the delimiter */
    while ((*pbRest != '\0') && (*pbRest != '=') &&
    (*pbRest != ' ') && (*pbRest != ','))
        pbRest ++;

    switch (*pbRest)
    {
    case '\0': //
        *ppbValue = pbRest;
        break;

    case ',':
    case ' ': // whitespace: there is no value assigned to this parameter
        *pbRest = '\0';
        *ppbValue = pbRest;
        pbRest ++;
        break;

    case '=':
        /* The value part may or may not be enclosed in quotes */
        *pbRest = '\0';
        pbRest ++;
        SKIP_WHITESPACE (pbRest);
        if (pbRest[0] == '\"')
        {
            *ppbValue = ++pbRest;
            while ((*pbRest != '\0') && (*pbRest != '\"'))
                pbRest ++;
        }
        else
        {
            *ppbValue = pbRest;
            while ((*pbRest != '\0') && (*pbRest != ' ') && (*pbRest != ',') && (*pbRest != ';'))
                pbRest ++;
        }

        if (*pbRest)
        {
            *pbRest = '\0';
            pbRest ++;
        }

        break;
    }
    return pbRest;
}
