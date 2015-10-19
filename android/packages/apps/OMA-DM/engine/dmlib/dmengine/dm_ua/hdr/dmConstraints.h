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

#ifndef DMCONSTRAINTS_H
#define DMCONSTRAINTS_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

Header Name: dmConstrians.h

General Description: This file contains declaration of the DMConstraints class, which specifies MDF constraints   

==================================================================================================*/

#include "dmMetaDataBuffer.h"

/* These enum values are used to indicate the node access type for SYNCML_DM_ACCESS_TYPE_T */
enum
{
    SYNCML_DM_ADD_ACCESS_TYPE       = SYNCML_DM_ACL_ADD,
    SYNCML_DM_DELETE_ACCESS_TYPE   = SYNCML_DM_ACL_DELETE,
    SYNCML_DM_GET_ACCESS_TYPE        = SYNCML_DM_ACL_GET,
    SYNCML_DM_REPLACE_ACCESS_TYPE = SYNCML_DM_ACL_REPLACE,
    SYNCML_DM_EXEC_ACCESS_TYPE      = SYNCML_DM_ACL_EXEC,
    SYNCML_DM_LOCAL_ACCESS_TYPE    = 0x020,
};
typedef UINT8 SYNCML_DM_ACCESS_TYPE_T;


enum
{
    SYNCML_DM_DDF_MIN = 1,
    SYNCML_DM_DDF_MAX,
    SYNCML_DM_DDF_VALUES,
    SYNCML_DM_DDF_DEFAULTVALUE,
    SYNCML_DM_DDF_MINLEN,
    SYNCML_DM_DDF_MAXLEN,
    SYNCML_DM_DDF_REGEXP,
    SYNCML_DM_DDF_NMAXLEN,
    SYNCML_DM_DDF_NVALUES,
    SYNCML_DM_DDF_NREGEXP,
    SYNCML_DM_DDF_AUTONODE,
    SYNCML_DM_DDF_RECUR_AFTER_SEGMENT,
    SYNCML_DM_DDF_MAX_RECURRANCE,
    SYNCML_DM_DDF_FK,
    SYNCML_DM_DDF_CHILD,
    SYNCML_DM_DDF_DEPEND,
    SYNCML_DM_DDF_MAX_MULTINODES
    
};
typedef UINT8 SYNCML_DM_DDF_CONSTRAINT_TYPE_T;


/* These enum values are used to indicate the format of the data for SYNCML_DM_FORMAT_T */
enum
{
    SYNCML_DM_DDF_MIME_TYPE_TEXTPLAIN = 0
};
typedef UINT8 SYNCML_DM_DDF_MIME_TYPE_T;



/**
* DMConstraints represent MDF constraints 
*/
class DMConstraints
{

  friend class DMMetaDataManager;
  friend class DMMetaDataNode;
  
public:
   /**
 * Default constructor
 */
  DMConstraints();

   /**
 * Destructor
 */
  ~DMConstraints();

   /**
  * Operator to allocate memory
  * \param sz [in] - number of bytes to be allocated
  */
  inline void* operator new(size_t sz)
  {
    return (DmAllocMem(sz));
  }

  /**
  * Operator to free memory
  * \param buf [in] - pointer on buffer to be freed
  */
  inline void operator delete(void* buf)
  {
    DmFreeMem(buf);
  }

  /**
  * Reads constraints from MDF file
  * \param pBuffer [in] - pointer on Meta Data buffer
  * \param count [in] - number constraints to read
  * \param nNodeType [in] - node type
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T Read(DMMetaDataBuffer * pBuffer, 
                                              UINT8 count,
                                              INT32 nNodeType);

   /**
  * Checks if default value may be set 
  * \return TRUE if set 
  */
   inline BOOLEAN IsDefaultSet() const 
   {
       return m_nNodeType != SYNCML_DM_FORMAT_NULL;
   }

   /**
  * Retrieves default value as a string
  * \param strValue [out] - default value as a string
  */
   void GetDefaultString( DMString& strValue )const;
   
private:  

  /* Minimum value of int node*/
  INT32 m_nMin;
  /* Maximum value of int node*/
  INT32 m_nMax;
  /* Comma separated values */
  CPCHAR m_psValues;  
  /* Node type */
  INT32 m_nNodeType;
  union  {
    /* Default value as a string */
    CPCHAR m_psDef_Value;
    /* Default int value */
    INT32   m_nDef_Value;
    /* Default float value */
    FLOAT   m_fDef_Value;
  };

  /* Maximum length of value */ 
  UINT16 m_nMaxLen;
  /* Minimum length of value */ 
  UINT16 m_nMinLen;
  /* Regular expression for value */
  CPCHAR m_psRegexp; 
  /* Maximum length of node name */ 
  UINT8 m_nnMaxLen;
  /* Comma separated node names */
  CPCHAR m_psnValues;
  /* Regular expression for node name */
  CPCHAR m_psnRegexp;
  /* Auto nodes */
  CPCHAR m_psAutoNodes; 
  /* Reccurrance pattern */
  CPCHAR m_psRecurAfterSegment; 
  /* Maximum times the recurrance pattern can occur */
  UINT16 m_nMaxRecurrance;
  /* Foreign key */
  CPCHAR m_psForeignKey; 
  /* Hard child dependency */
  CPCHAR m_psChild;      
  /*  Soft child dependency */
  CPCHAR m_psDepend;     
  /* Max number of multinodes to be created */
  UINT16 m_nMaxMultiNodes;


  /**
  * Initializes object
  */
  void Init();

  /**
  * Reads default value from MDF file
  * \param pBuffer [in] - pointer on Meta Data buffer
  * \param nNodeType [in] - node type
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T ReadDefaultValue(DMMetaDataBuffer * pBuffer, 
                                          INT32 nNodeType);
  
};

#endif
