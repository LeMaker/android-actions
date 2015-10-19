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

#ifndef __DMTOKEN_H__
#define __DMTOKEN_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

Header Name: dmToken.h

General Description: This file contains declaration of utility classes DMToken, DMURI, DMParser

==================================================================================================*/

#include "xpl_Types.h"
#include "dmStringUtil.h"
#include "dm_uri_utils.h"

/**
 * DMToken represents a parser of a string with segments separated by specified delimeter.
 */
class DMToken 
{
 public:
  /**
  * Default constructor
  */
  DMToken();

  /**
  * Constructor that sets value of delimeter
  * \param delimeter [in] - delimeter 
  */
  DMToken(char delimeter);

  /**
  * Constructor that sets parameters of an object
  * \param bIsAlloc [in] - specifies if internal buffer should be allocated and string copied into it 
  * \param szStr [in] - string to parse 
  * \param delimeter [in] - delimeter 
  */
  DMToken(BOOLEAN bIsAlloc, CPCHAR szStr, char delimeter);

  /**
  * Destructor
  */
  ~DMToken();  

   /**
  * Assigns string to parse
  * \param szStr [in] - string to parse 
  * \return Return Type (CPCHAR) 
  * - Pointer on internal buffer if operation is completed successfully. 
  * - NULL otherwise. 
  */
  CPCHAR assign(CPCHAR szStr);

   /**
  * Retrieves next segment of a string
  * \return Return Type (CPCHAR) 
  * - Pointer on next segment if operation is completed successfully. 
  * - NULL otherwise. 
  */
  CPCHAR nextSegment();

  /**
  * Retrieves pointer on string
  */
  char * getBuffer() const { return m_pStr; } 

   /**
  * Retrieves length of a string
  */
  INT32 length() { return m_pStr ? DmStrlen(m_pStr):0; }

  /**
  * Retrieves count of segments in a string
  */
  UINT32 getSegmentsCount(); 

  /**
  * Resets object
  */
  void reset();
 
  
 protected:
  /* Pointer on current segment position */
  char * m_pTokenPos;
  /* Pointer on current delimeter position */
  char * m_pDelimPos;
  /* Pointer on parsed string */
  char * m_pStr; 
  /* Delimeter */
  char m_cDelim;
  /* Specifies if internal buffer should eb allocated */
  BOOLEAN m_bIsAlloc;

  
};


/**
* DMToken represents a URI parser
*/
class DMURI : public DMToken
{

public:  
  /**
  * Default constructor
  */
  DMURI();
  
   /**
  * Constructor that sets parameters of an object
  * \param bIsAlloc [in] - specifies if internal buffer should be allocated and string copied into it 
  */
  DMURI(BOOLEAN bIsAlloc);

   /**
  * Constructor that sets parameters of an object
  * \param bIsAlloc [in] - specifies if internal buffer should be allocated and string copied into it 
  * \param szURI [in] - string to parse 
  */ 
  DMURI(BOOLEAN bIsAlloc, CPCHAR szURI);

   /**
  * Retrieves last segment of URI
   * \return Return Type (CPCHAR) 
  * - Pointer on last segment if operation is completed successfully. 
  * - NULL otherwise. 
  */
  CPCHAR getLastSegment();

  /**
  * Retrieves tail segments of URI
   * \return Return Type (CPCHAR) 
  * - Pointer on last segment if operation is completed successfully. 
  * - NULL otherwise. 
  */
  CPCHAR getTailSegments() const;

   /**
  * Retrieves parent URI (without last segment) 
   * \return Return Type (CPCHAR) 
  * - Pointer on last segment if operation is completed successfully. 
  * - NULL otherwise. 
  */
  CPCHAR getParentURI(); 

};


/**
* Represents segments of URI
*/
struct DM_URI_SEGMENT_T
{

public:
   /**
  * Default constructor
  */
  DM_URI_SEGMENT_T()
  {
    m_pStr = 0;
    m_nLen = 0;
  }  

  /* Pointer on segment */ 
  char * m_pStr;
  /* Segment length */
  INT32  m_nLen;
};


/**
* DMToken represents a URI parser
*/
class DMParser
{

public:  
  /**
  * Constructor that sets parameters of an object
  * \param delimeter [in] - delimeter 
  */
  DMParser(char delimeter = SYNCML_DM_FORWARD_SLASH);
  
  /**
  * Constructor that sets parameters of an object
  * \param szURI [in] - URI to parse 
  * \param delimeter [in] - delimeter 
  */
  DMParser(CPCHAR szURI, char delimeter = SYNCML_DM_FORWARD_SLASH);

  /**
  * Destructor
  */
  ~DMParser();

   /**
  * Retrieves count of segments in a string
  */
  inline UINT32 getSegmentsCount() const { return m_nSegmentsCount; } 

   /**
  * Assigns string to parse
  * \param szStr [in] - string to parse 
  * \return Return Type (CPCHAR) 
  * - Pointer on internal buffer if operation is completed successfully. 
  * - NULL otherwise. 
  */ 
  CPCHAR assign(CPCHAR szStr);

   /**
  * Retrieves next segment of a string
  * \return Return Type (CPCHAR) 
  * - Pointer on next segment if operation is completed successfully. 
  * - NULL otherwise. 
  */ 
  CPCHAR nextSegment();

  /**
  * Searches segment in the URI
  * \return TRUE if segment is found 
  */ 
  BOOLEAN findSegment(CPCHAR szSegment);

  /**
  * Resets object
  */ 
  void reset();

protected:
  /* Parsed segments */
  DM_URI_SEGMENT_T * m_aSegments;
  /* Current segment index */
  INT32 m_nCurrentSegment;
  /* Segments count */
  INT32 m_nSegmentsCount;
    /* Pointer on current segment position */
  char * m_pTokenPos;
  /* Pointer on current delimeter position */
  char * m_pDelimPos;
  /* Pointer on parsed string */
  char * m_pStr; 
  /* Delimeter */
  char m_cDelim;
 
};

#endif   
