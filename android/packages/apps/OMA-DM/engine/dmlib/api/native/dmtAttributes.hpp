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

#ifndef __DMTATTRIBUTES_H__
#define __DMTATTRIBUTES_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/**  
 \file dmtAttributes.hpp
 \brief  The dmtAttributes.hpp header file contains DmtAttributes class definition. \n
           This class encapsulates all standard DMT attributes.
*/

#include "jem_defs.hpp"
#include "dmtAcl.hpp"

/**
* This class encapsulates all standard DMT attributes.<P>
* 
* <TABLE>
* <TR><TD>ACL</TD><TD>Access Control List</TD></TR>
* <TR><TD>Format</TD><TD>Specifies how object values should be interpreted (b64, int, boolean, string бн)</TD></TR>
* <TR><TD>Name</TD><TD>The name of the object in the tree</TD></TR>
* <TR><TD>Size</TD><TD>Size of the object value in bytes</TD></TR>
* <TR><TD>Title</TD><TD>Human readable name</TD></TR>
* <TR><TD>TStamp</TD><TD>Time stamp, date and time of last change</TD></TR>
* <TR><TD>Type</TD><TD>The MIME type of the object</TD></TR>
* <TR><TD>VerNo</TD><TD>Version number, automatically incremented at each modification</TD></TR>      
* </TABLE>  
* \par Category: General  
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
class DmtAttributes 
{ 
private:
  DMString  m_strName;
  DMString  m_strFormat;
  DMString  m_strTitle;
  DMString  m_strType;
  INT32     m_nVersion;
  INT32     m_nSize;
  JemDate    m_nTimestamp;
  DmtAcl     m_oAcl;

public:
 /**
  * Default constructor - no memory allocation performed.
  */
  DmtAttributes();
 
  /**
   * Constructs DmtAttributes object with provided values . The memory required for DmtAttributes object will be allocated.
   * \param name [in] - Name of attribute
   * \param format [in] - Attribute format
   * \param title [in] - Attribute titles
   * \param type [in] - Attribute type
   * \param version [in] - Version number
   * \param size [in] - Attribute size
   * \param timestamp [in] - Timestamp is number of milliseconds since the standard base time known as "the epoch", 
   * namely January 1, 1970, 00:00:00 GMT.
   * which is locale-independent.
   * \param acl [in] - access control list (DmtAcl type)
   */
  DmtAttributes( CPCHAR name,
                 CPCHAR format,
                 CPCHAR title,
                 CPCHAR type,
                 INT32 version,
                 INT32 size,
                 const JemDate& timestamp,
                 const DmtAcl& acl);

  /**
  * Sets all required parameters. 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param name [in] - Name of attribute
  * \param format [in] - Attribute format
  * \param title [in] - Attribute titles
  * \param type [in] - Attribute type
  * \param version [in] - Version number
  * \param size [in] - Attribute size
  * \param timestamp [in] - Timestamp is number of milliseconds since the standard base time known as "the epoch", 
  * namely January 1, 1970, 00:00:00 GMT.
  * which is locale-independent.
  * \param acl [in] - access control list (DmtAcl type)
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_DEVICE_FULL - indicate that operation cannot be performed. \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   SYNCML_DM_RET_STATUS_T Set( CPCHAR name,
                              CPCHAR format,
                              CPCHAR title,
                              CPCHAR type,
                              INT32 version,
                              INT32 size,
                              const JemDate& timestamp,
                              const DmtAcl& acl);



  /**
  * Sets all required parameters. 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param oAttr [in] - reference to DmtAttributes object with all attributes
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_DEVICE_FULL - indicate that operation cannot be performed. \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   SYNCML_DM_RET_STATUS_T Set( const DmtAttributes & oAttr );

 
  /**
  * Sets name of the Node 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param name [in] - name of the node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_DEVICE_FULL - indicate that operation cannot be performed. \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   SYNCML_DM_RET_STATUS_T SetName( CPCHAR name);

  /**
  * Sets title of the Node  
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param title [in] - title of the node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_DEVICE_FULL - indicate that operation cannot be performed. \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   SYNCML_DM_RET_STATUS_T SetTitle( CPCHAR title);

  /**
  * Sets format of the Node
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param format [in] - format of the node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_DEVICE_FULL - indicate that operation cannot be performed. \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   SYNCML_DM_RET_STATUS_T SetFormat( CPCHAR format);

  /**
  * Sets size of the Node value in bytes
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param size [in] - size of the node value in bytes
  * \return None
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   void SetSize( INT32 size);
   
  /**
  * Retrieves format of the Node.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Format (String, Binary, Integer...) of the Node 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   inline const DMString& GetFormat() const
   {
      return m_strFormat;
   }


  /**
  * Retrieves name of the node
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Node name.
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   inline const DMString& GetName() const
   {
      return m_strName;
   }

  /**
  * Retrieves access control list for the node
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return DmtAcl (DMT access control list ) for node
  * \warning DMT access control could be null!
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   inline const DmtAcl& GetAcl() const
   {
      return m_oAcl;
   }

  /**
  * Retrieves node size
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return node value's size (32-bit).
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   inline INT32 GetSize() const
   {
      return m_nSize;
   }

  /**
  * Retrieves node titles
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return title (optional), could be null.
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   inline const DMString& GetTitle() const
   {
      return m_strTitle;
   }

  /**
  * Retrieves node type
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Mime-type of the Node, Could be null.
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   inline const DMString& GetType() const
   {
      return m_strType;
   }

  /**
  * Retrieves  timestamp
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return number of milliseconds since the standard base time known as "the epoch", namely January 1, 1970, 00:00:00 GMT.
  * which is locale-independent
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   inline const JemDate& GetTimestamp() const
   {
      return m_nTimestamp;
   }

  /**
  * Retrieves version
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Version ( a 16 bit unsigned integer ). Each time an object with this property changes value,\n
  * through a management operation or other event, this value is incremented. If the property\n
  * value has reached FFFF, and then is incremented, it returns to 0000.\n
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   inline INT32 GetVersion() const 
   {
      return m_nVersion;
   }

};

//Changed Version from String to int to be compatible with Java API

#endif
