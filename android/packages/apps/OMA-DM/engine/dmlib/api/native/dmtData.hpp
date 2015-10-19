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

#ifndef __DMTDATA_H__
#define __DMTDATA_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/** 
 \file dmtData.hpp
 \brief  The dmtData.hpp header file contains DmtData class definition. \n
           This class encapsulates various Device Management  Tree leaf node \n
           data formats. 
*/

#include "jem_defs.hpp"
#include "dmtDefs.h"

/**
* NULLVALUE MUST BE USED TO get a NULLTYPE DATA\n
* Or (const char *)0, Do NOT use NULL as some SYSTEM header file\n
* Will define NULL as integer 0.\n
*/
#define NULLVALUE ((const char *)0)



/**
* DmtData encapsulates various DMT leaf node data formats.\n
* Note: Functions "Get" returns error code; possible errors are: \n
* <i>enumDmtResult_Success </i> - means success, \n
* <i>enumDmtResult_TypeMismatch </i> - means datatype of stored \n
* value and asked type are mismatched and automatic conversion is not possible\n
* <pre>
* <b>Data formats includes:</b>
*    String
*    Integer
*    Boolean
*    Binary
* </pre>
* \par Category: General  
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
class DmtData 
{
   /**
    * Data Type as defined in SyncML DM Spec.<P>
    * Format (b64 | bool | chr | int | node | float | date| time |
    * null | xml)<P>
    * Currently only support bool, chr, int, bin, node, float, date
    * and time.
    */
    
public:
  /**Deprecated: For legacy API calls support only. The api/common/dmtDefs.h 
   * will be the global place to define and expose DM data type 
   * enumerations that consist with MDF definition.
   */
  enum 
  {
    UNDEFINED = SYNCML_DM_DATAFORMAT_UNDEFINED,   
    NULLTYPE = SYNCML_DM_DATAFORMAT_NULL,
    STRING  = SYNCML_DM_DATAFORMAT_STRING,
    INT     = SYNCML_DM_DATAFORMAT_INT,
    BOOL    = SYNCML_DM_DATAFORMAT_BOOL,
    BIN     = SYNCML_DM_DATAFORMAT_BIN,
    NODE    = SYNCML_DM_DATAFORMAT_NODE,
    DATE    = SYNCML_DM_DATAFORMAT_DATE,
    TIME    = SYNCML_DM_DATAFORMAT_TIME,
    FLOATTYPE    = SYNCML_DM_DATAFORMAT_FLOAT
  };
  /**Deprecated: For legacy API calls support only. */
  typedef SYNCML_DM_DATAFORMAT_T EnumDataType;

   //instance variables
private:
  EnumDataType metaFormat;        // same as Format in SYNCML_DM_FORMAT
  
  DMString     m_strValue;
  INT32        m_nValue;
  BOOLEAN      m_bValue;
  DMVector<UINT8> m_pBinValue;
  DMStringVector  m_astrNodeValue;  //Child Node Names
   
public:
   /**
    * Default Constructor - no memory allocation performed.
    */
   DmtData();

   /**
    * Constructor for all data types with string based serialization/
    * de-serialization with 1 param, i.e., String/int/Bool/Float/Date/Time data type. 
    * Default type is STRING for backward compatibility. The memory for the size of parameter "szStr" will be allocated.
    * \param szStr [in] - data as nul terminated string
    */
   DmtData(CPCHAR szStr);

   /**
    * Constructor for all data types with string based serialization/
    * de-serialization with 2 params, i.e., String/int/Bool/Float/Date/Time data type. 
    * Default type is STRING for backward compatibility. 
    * \par Important Notes:
    * - Note: The memory for the size of parameter "szStr" will be allocated if type is bin or string.
    * \param szStr [in] - data as nul terminated string
    * \param type [in] - DM data type 
    */

   DmtData(CPCHAR szStr , SYNCML_DM_DATAFORMAT_T type);
   
   /**
    * Data represents an integer type. No memory allocation performed.
    * \param integer [in] - DM integer data  
    */
   DmtData(INT32 integer);
   
   /**
    * Data represents a boolean type (explicit for backward compatibility). No memory allocation performed.
    * \param b [in] - DM boolean data  
    */
   explicit DmtData(BOOLEAN b);
   
   /**
    * Data represents a boolean type (for backward compatibility). No memory allocation performed.
    * \param b [in] - DM boolean data  
    */
   DmtData(BOOLTYPE b);


   /**
    * Data represents a binary type. The memory for the size of parameter "szStr" (parameter "len") will be allocated. 
    * \param bin [in] - DM binary data  
    * \param len [in] - length of the data 
    */
   DmtData(const UINT8 * bin, INT32 len);

   /**
    *  Supports node type as defined in SyncML DM Spec
    *  if the data type is node, it means it contains child node names. 
    * The memory required for the parameter "aChildren" will be allocated. 
    * \param aChildren [in] - vector with child node names  
    **/
   DmtData(const DMStringVector& aChildren);

   /** Destructor - freeing all dynamic resources */
   ~DmtData();

  /**
   * Retrieves data type.
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \return Return type (SYNCML_DM_DATAFORMAT_T) - data type. 
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
  SYNCML_DM_DATAFORMAT_T GetType() const { return  metaFormat; /* Type is format in this version.*/ }

  /** 
   * Retrieves data string value.
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param str [out] - string representation of the value
   * \return  Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_INVALID_PARAMETER - indicating the operation cannot be performed. \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
  SYNCML_DM_RET_STATUS_T GetString( DMString& str ) const;

   /**
   *Retrieves boolean value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param bValue [out] -boolean value
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_INVALID_PARAMETER - indicating the operation cannot be performed. \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T GetBoolean( BOOLEAN& bValue ) const;

   /**
   *Retrieves boolean value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param bValue [out] -boolean type value
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T GetBoolean( BOOLTYPE& bValue ) const;

   /**
    * Retrieves integer value 
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param nValue [out] -integer value
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_INVALID_PARAMETER - indicating the operation cannot be performed. \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T  GetInt( INT32& nValue ) const;

   /**
   * Retrieves date value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param sDate [out] -date string value
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_INVALID_PARAMETER - indicating the operation cannot be performed. \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T  GetDate( DMString& sDate ) const;

   /**
   * Retrieves time value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param sTime [out] -time string value
   * \return status code
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T  GetTime( DMString& sTime ) const;

   /**
    * Retrieves float value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param fValue [out] -float value
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_INVALID_PARAMETER - indicating the operation cannot be performed. \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T  GetFloat( DMString& fValue ) const;

   /**
   * Retrieves binary value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param buffer [out] -binary value
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_INVALID_PARAMETER - indicating the operation cannot be performed. \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T GetBinary(DMVector<UINT8>& buffer ) const;

   /**
   * As mentioned before the complete structure of all management objects and the root (the
   * device itself) forms a tree. Management servers can explore the structure of the tree by
   * using the GET command. If the accessed object has child objects linked to it the name of
   * these child objects are returned as a result of the GET. 
   * \par Important Notes:
   * - Note: If there are no children the object MUST have a value, which could be null, and this value is returned.
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param aChildren [out] - child node names
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_INVALID_PARAMETER - indicating the operation cannot be performed. \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T GetNodeValue( DMStringVector& aChildren ) const;

   /**
   * Sets  string value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param szStr [in] -string value
   * \param type [in] -SYNCML_DM_DATAFORMAT_T value
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_DEVICE_FULL - indicating the operation cannot be performed. \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T SetString(CPCHAR szStr, SYNCML_DM_DATAFORMAT_T type =
                                                  SYNCML_DM_DATAFORMAT_STRING);
   /**
   * Sets  boolean value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param bValue [in] -boolean value
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T SetBoolean(BOOLEAN bValue);

   /**
   * Sets  integer value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param nValue [in] -integer value
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T SetInt(INT32 nValue);

   /**
    * Sets  date value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param sDate [in] -date string value
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_DEVICE_FULL - indicating the operation cannot be performed. \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T SetDate(CPCHAR sDate);
   
   /**
    * Sets  time value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param sTime [in] -time string value
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_DEVICE_FULL - indicating the operation cannot be performed. \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T SetTime(CPCHAR sTime);

   /**
    * Sets  float value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param fValue [in] -float value
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_DEVICE_FULL - indicating the operation cannot be performed. \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T SetFloat(CPCHAR fValue);

   /**
    * Sets  binary value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param buf [in] -binary value
   * \param len [in] -size
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_DEVICE_FULL - indicating the operation cannot be performed. \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T SetBinary(const UINT8 * buf, INT32 len);

   
   /**
    * Sets  node value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param aChildren [in] -child node names
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_DEVICE_FULL - indicating the operation cannot be performed. \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T SetNodeValue( const DMStringVector& aChildren );


   /**
    * Sets data
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param oData [in] -dmt data
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_DEVICE_FULL - indicating the operation cannot be performed. \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T Set( const DmtData & oData );

   /**
    * Adds node value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \param sChild [in] -child node name
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_DEVICE_FULL - indicating the operation cannot be performed. \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   SYNCML_DM_RET_STATUS_T AddNodeValue( const DMString & sChild );


   /**
    * Retrieves string value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \return reference on string value
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   const DMString & GetStringValue() const;

   /**
    * Retrieves binary value
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \return reference on binary value
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   const DMVector<UINT8> & GetBinaryValue() const;

   /**
    * Retrieves node value
   * \return node value
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */
   const DMStringVector & GetNodeValue() const;

   /**
   * Retrieves value(data) size
   * \par Sync (or) Async:
   * This is a Synchronous function.
   * \par Secure (or) Non-Secure (or) N/A:
   * This is a Non-Secure function.
   * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
   * - SYNCML_DM_INVALID_PARAMETER - indicating the operation cannot be performed. \n
   * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
   * \par Prospective Clients:
   * All potential applications that require configuration settings.
   */


  SYNCML_DM_RET_STATUS_T  GetSize(INT32 &dataSize) const;

};

#endif
