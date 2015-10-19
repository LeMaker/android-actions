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

#ifndef __DMTNODE_H__
#define __DMTNODE_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/** 
 \file dmtNode.hpp
 \brief  The dmtNode.hpp header file contains DmtNode class definition. \n
                DmtNode is the object representing tree nodes as they are\n 
                created and added to the tree. The class is NOT designed\n 
                to serve navigation of the DM Tree. 
*/

#include "jem_defs.hpp"

#include "dmt.hpp"

class DmtTree;

/**
DmtNode is the object representing tree nodes as they are
created and added to the tree. The class is NOT designed
for its objects to serve for navigation of the tree.
Almost all methods return a smart pointer to the 
error description object if failed and NULL if succeeded

<P>

Sample usage:<P>

<PRE>
   DmtPrincipal principal("localhost");
   PDmtTree ptrTree;
   SYNCML_DM_RET_STATUS_T ret_status ;

   if ( (ret_status = DmtFactory::GetTree(principal, ptrTree ) ) != SYNCML_DM_SUCCESS )
   {
     ... error handling
     return;
   }

   PDmtNode ptrNode;
   
   if ( (ret_status = ptrTree->GetNode( "./SyncML/DMAcc/GUID/AddrType", ptrNode ) ) != SYNCML_DM_SUCCESS )
   {
      ... error handling
     return;
   }
   
   std::string str;
   if ( (ptrError = ptrNode->GetStringValue( str )) != NULL )
   {
      ... error handling
     return;
   }
   
   printf( "String value is %s\n", str.c_str() );
</PRE>
* \par Category: General  
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
class DmtNode : public JemBaseObject
{ 
protected:
  /** Destructor - freeing all dynamic resources */
  virtual ~DmtNode(){}

public:
  /** 
  * Retrieves device management tree
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param ptrTree [out] - reference to the DM tree
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  virtual SYNCML_DM_RET_STATUS_T GetTree( PDmtTree& ptrTree ) const = 0;

  /** 
  * Retrieves node path
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [out] - reference to path in the tree
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  virtual SYNCML_DM_RET_STATUS_T GetPath( DMString & path ) const = 0;

  /**
  * Gets a copy of DmtAttributes, user can modify the DmtAttributes individually.
  * \warning Any change made to DmtAttributes will not propagated to the Node until a setAttributes is called.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param  oAttr [out] - reference to DM tree attributes
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  virtual SYNCML_DM_RET_STATUS_T GetAttributes( DmtAttributes& oAttr ) const = 0;

  /**
  * Updates title information for the node.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szTitle [in] - node titles string
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  virtual SYNCML_DM_RET_STATUS_T SetTitle( CPCHAR szTitle ) = 0;
  
  /**
   * Updates ACL for the node.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param oAcl [in] - reference to DMT ACL object
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  virtual SYNCML_DM_RET_STATUS_T SetAcl( const DmtAcl& oAcl ) = 0;
  
  /** 
  * The DmtValue is a copy of current data of the node. Could set empty DmtData object if there is no value associated with it.
  * \warning The node value will not be changed until a setValue is called.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param oData [out] - reference to DmtData object
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  virtual SYNCML_DM_RET_STATUS_T GetValue( DmtData& oData ) const = 0;

  /**
  * Changes the value of a node. If not successful, return an error code.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param value [in] - new node value 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  virtual SYNCML_DM_RET_STATUS_T SetValue( const DmtData& value ) = 0;

  /**
  * Fills in the vector oChildren list of child nodes.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param oChildren [out] - vector for child nodes
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  virtual SYNCML_DM_RET_STATUS_T GetChildNodes( DMVector<PDmtNode>& oChildren ) const = 0;


  /**
  * Checks if a node is a leaf
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return 'true' if the node is a leaf
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  virtual BOOLEAN IsLeaf() const = 0;

  /**
  * Function sets child node object by name 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szPath [in] - name of node
  * \param ptrNode [out] - reference to DmtNode
  * \return status code
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  virtual SYNCML_DM_RET_STATUS_T GetChildNode( CPCHAR szPath, PDmtNode& ptrNode ) = 0;

  /**
  * Executes a node according to the specified path, passing a String
  * parameter to the executable code  
  * \warning This functions is  for internal usage only!!!
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param strData [in] - data to be executed as a string
  * \param result [out] - the result of executing will be set to this string
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  virtual SYNCML_DM_RET_STATUS_T Execute( CPCHAR strData, DMString& result ) = 0;


  /** 
  * This is helper function: gets value of the node with type "string".
  * It is a shortcut to the GetValue()->GetString().
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param str [out] - result string
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  SYNCML_DM_RET_STATUS_T GetStringValue( DMString& str ) const;
  
  /**  
  * This is a helper function: gets value of the node with type "integer".
  * It is a shortcut to the GetValue()->GetInt()
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param nValue [out] - result integer
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  SYNCML_DM_RET_STATUS_T GetIntValue( INT32& nValue ) const;
   
  /** 
  * This is a helper function: gets value of the node with type "boolean".
  * It is a shortcut to the GetValue()->GetBoolean()
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param bValue [out] - result boolean
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  SYNCML_DM_RET_STATUS_T GetBooleanValue( BOOLEAN& bValue ) const;

  /** 
  * This is a helper function:  gets value of the node with type "boolean"  (use for backward compatibility BOOLTYPE == bool). 
  * It is a  shortcut to the GetValue()->GetBoolean().
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param bValue [out] - result boolean
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  SYNCML_DM_RET_STATUS_T GetBooleanValue( BOOLTYPE& bValue ) const;

  /** 
  * This is a helper function: gets value of the node with type "float".
  * It is a  shortcut to the GetValue()->GetFloat()
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param sFloat [out] - result float
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  SYNCML_DM_RET_STATUS_T GetFloatValue(DMString& sFloat ) const;
   
  /** 
   * This is a helper function: gets value of the node with type ""date".
   * It is a shortcut to GetValue()->GetDate().
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param sDate [out] - result date string
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  SYNCML_DM_RET_STATUS_T GetDateValue( DMString& sDate ) const;
   
 /** 
  * This is a helper function: gets value of the node with type "string".
  * It is a shortcut to GetValue()->GetTime().
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param sTime [out] - result time string
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  SYNCML_DM_RET_STATUS_T GetTimeValue( DMString& sTime ) const;
   
  /** 
  * This is a helper function: gets value of the node with type "binary".
  * It is a shortcut to GetValue()->GetBinary(). 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param binValue [out] - result binary blob
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  SYNCML_DM_RET_STATUS_T GetBinaryValue( DMVector<UINT8>& binValue ) const;
  
  /** 
  * This is a helper function: sets value of the node with type "string".
  * It is a  shortcut to the SetValue(DmtData()).
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param str [in] -  the value that should be set
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  SYNCML_DM_RET_STATUS_T SetStringValue( CPCHAR str );
  
  /** 
  * This is a helper function: sets value of the node with type "integer".
  * It is a  shortcut to the SetValue(DmtData()).
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param nValue  [in]-  the value that should be set
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  SYNCML_DM_RET_STATUS_T SetIntValue( INT32 nValue );
   
  /** 
  * This is a helper function: sets value of the node with type "boolean".
  * It is a  shortcut to the SetValue(DmtData())
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param bValue [in] -  the value that should be set
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  SYNCML_DM_RET_STATUS_T SetBooleanValue( BOOLEAN bValue );
   
  /** 
  * This is a helper function: sets value of the node with type "float".
  * It is a shortcut to the SetValue(DmtData())
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param fValue [in] -  the value that should be set
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  SYNCML_DM_RET_STATUS_T SetFloatValue( CPCHAR fValue );
   
  /** 
  * This is a helper function: sets value of the node with type "date".
  * It is a shortcut to the SetValue(DmtData()).
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param sDate [in] - the value that should be set
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  SYNCML_DM_RET_STATUS_T SetDateValue( CPCHAR sDate );
   
  /** 
  * This is a helper function: sets value of the node with type "time".
  * It is a shortcut to the SetValue(DmtData()).
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param sTime [in] -  the value that should be set
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  SYNCML_DM_RET_STATUS_T SetTimeValue( CPCHAR sTime );
   
  /** 
  * This is a helper function: sets value of the node with type "binary".
  * It is a shortcut to the SetValue(DmtData()).
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param bin [in] -  binary blob that should be set
  * \param len  [in] - length of the parameter "bin"
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  SYNCML_DM_RET_STATUS_T SetBinaryValue( const UINT8 * bin, INT32 len );
   
   
  /**
  * Gets the name of the node.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param name [out] - name of the node 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  virtual SYNCML_DM_RET_STATUS_T GetNodeName(DMString & name) const = 0;

  /**
  * Gets the name of the node (wrapper for backward compatibility).
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return name of the node  \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  virtual DMString GetNodeName() const = 0;
  
  /**
  * Gets the first chunk of  an ESN (External Storage Node) data . The chunk buffer is allocated internally.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param dmtChunkData [out] - result as a reference to the  DMT Data Chunk object 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that the ESN data reading successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
virtual SYNCML_DM_RET_STATUS_T GetFirstChunk(DmtDataChunk&  dmtChunkData) = 0; 

  /**
  * Gets the next chunk of  an ESN (External Storage Node) data.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param dmtChunkData [out] - result as a reference to the  DMT Data Chunk object 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that the next chunk of ESN data reading successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
virtual SYNCML_DM_RET_STATUS_T GetNextChunk(DmtDataChunk& dmtChunkData) = 0; 
  
  /**
  * Sets the first chunk of  an ESN (External Storage Node) data . The chunk buffer is allocated internally.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param dmtChunkData [in] - reference to the  DMT Data Chunk object that should be set. 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that the ESN data reading successfully and first chunk of ESN data has been written. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
virtual SYNCML_DM_RET_STATUS_T SetFirstChunk(DmtDataChunk& dmtChunkData) = 0;  

  /**
  * Sets the next chunk of  an ESN (External Storage Node) data .
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param dmtChunkData [in] - reference to the  DMT Data Chunk object that should be set. 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that the next  chunk of ESN data has been written successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
virtual SYNCML_DM_RET_STATUS_T SetNextChunk(DmtDataChunk& dmtChunkData) = 0;  
  
  /**
  * Sets the last chunk of  an ESN (External Storage Node) data .
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param dmtChunkData [in] - reference to the  DMT Data Chunk object that should be set. 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that the last  chunk of ESN data has been written successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
virtual SYNCML_DM_RET_STATUS_T SetLastChunk(DmtDataChunk& dmtChunkData) = 0;  

  /**
  * This function verifies if a node is an  External Storage Node
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return 'true' if the node is an External Storage Node
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
virtual boolean IsExternalStorageNode(void) const= 0;
};

  /** 
  * Gets  value of the node with type "string".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param str [out] - value of the node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
inline SYNCML_DM_RET_STATUS_T DmtNode::GetStringValue( DMString& str ) const
{
  DmtData data;
  SYNCML_DM_RET_STATUS_T ptrError = GetValue( data );
  if ( ptrError != SYNCML_DM_SUCCESS )
    return ptrError;
  return data.GetString( str );
}

  /** 
  * Gets  value of the node with type "integer".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param nValue [out] - result integer
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
inline SYNCML_DM_RET_STATUS_T DmtNode::GetIntValue( INT32& nValue ) const
{
  DmtData data;
  SYNCML_DM_RET_STATUS_T ptrError = GetValue( data );
  if ( ptrError != SYNCML_DM_SUCCESS )
    return ptrError;
  return data.GetInt( nValue );
}

  /** 
  * Gets value of the node with type "boolean".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param bValue [out] - result boolean
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
inline SYNCML_DM_RET_STATUS_T DmtNode::GetBooleanValue( BOOLEAN& bValue ) const
{
  DmtData data;
  SYNCML_DM_RET_STATUS_T ptrError = GetValue( data );
  if ( ptrError != SYNCML_DM_SUCCESS )
    return ptrError;
  return data.GetBoolean( bValue );
}

  /** 
  * This is a helper function: gets boolean value of the node (use for backward compatibility BOOLTYPE == bool).
  * It is a shortcut to the GetValue()->GetBoolean().
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param bValue [out] - result boolean 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
inline SYNCML_DM_RET_STATUS_T DmtNode::GetBooleanValue( BOOLTYPE& bValue ) const
{
  DmtData data;
  SYNCML_DM_RET_STATUS_T ptrError = GetValue( data );
  if ( ptrError != SYNCML_DM_SUCCESS )
    return ptrError;
  return data.GetBoolean( bValue );
}

  /** 
  * Gets value of the node with type "float".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param  sFloat [out] - result float as a string
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
inline SYNCML_DM_RET_STATUS_T DmtNode::GetFloatValue( DMString& sFloat ) const
{
  DmtData data;
  SYNCML_DM_RET_STATUS_T ptrError = GetValue( data );
  if ( ptrError != SYNCML_DM_SUCCESS )
    return ptrError;
  return data.GetFloat( sFloat );
}

  /** 
  * Gets value of the node with type "date".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param sDate [out] - result date as a string
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
inline SYNCML_DM_RET_STATUS_T DmtNode::GetDateValue( DMString& sDate ) const
{
  DmtData data;
  SYNCML_DM_RET_STATUS_T ptrError = GetValue( data );
  if ( ptrError != SYNCML_DM_SUCCESS )
    return ptrError;
  return data.GetDate( sDate );
}

 /** 
  * Gets value of the node with type "time".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param sTime [out] - result time as a string
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
inline SYNCML_DM_RET_STATUS_T DmtNode::GetTimeValue( DMString& sTime ) const
{
  DmtData data;
  SYNCML_DM_RET_STATUS_T ptrError = GetValue( data );
  if ( ptrError != SYNCML_DM_SUCCESS )
    return ptrError;
  return data.GetTime( sTime );
}

/** 
  * Gets value of the node with type "binary";
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param binValue [out] - result binary blob
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
inline SYNCML_DM_RET_STATUS_T DmtNode::GetBinaryValue( DMVector<UINT8>& binValue ) const
{
  DmtData data;
  SYNCML_DM_RET_STATUS_T ptrError = GetValue( data );
  if ( ptrError != SYNCML_DM_SUCCESS )
    return ptrError;
  return data.GetBinary( binValue );
}

  /** 
   *  Sets value of the node with type "string".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param str [in] -  the value that should be set
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
inline SYNCML_DM_RET_STATUS_T DmtNode::SetStringValue( CPCHAR str )
{
  return SetValue( DmtData( str ) );
}

  /** 
  *  Sets value of the node with type "integer".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.   
  * \param nValue [in] -  the value that should be set
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
inline SYNCML_DM_RET_STATUS_T DmtNode::SetIntValue( INT32 nValue )
{
  return SetValue( DmtData( nValue ) );
}

   /** 
  *  Sets value of the node with type "boolean".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.   
  * \param bValue [in] -  the value that should be set
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
inline SYNCML_DM_RET_STATUS_T DmtNode::SetBooleanValue( BOOLEAN bValue )
{
  return SetValue( DmtData( bValue ) );
}


  /** 
  *  Sets value of the node with type "float".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.   
  * \param sFloat [in] - the value that should be set
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
inline SYNCML_DM_RET_STATUS_T DmtNode::SetFloatValue( CPCHAR sFloat )
{
  return SetValue( DmtData( sFloat, SYNCML_DM_DATAFORMAT_FLOAT ) );
}

 /** 
  *  Sets value of the node with type "date".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.   
  * \param sDate [in] -  the value that should be set
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h   
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
inline SYNCML_DM_RET_STATUS_T DmtNode::SetDateValue( CPCHAR sDate )
{
  return SetValue( DmtData( sDate, SYNCML_DM_DATAFORMAT_DATE ) );
}

  /** 
  *  Sets value of the node with type "time".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.   
  * \param sTime [in] - the value that should be set
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */

inline SYNCML_DM_RET_STATUS_T DmtNode::SetTimeValue( CPCHAR sTime )
{
  return SetValue( DmtData( sTime, SYNCML_DM_DATAFORMAT_TIME ) );
}

  /** 
  *  Sets value of the node with type "binary".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.   
  * \param bin [in] - binary blob  that should be set
  * \param len [in] - length of the parameter "bin"
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
inline SYNCML_DM_RET_STATUS_T DmtNode::SetBinaryValue( const UINT8 * bin, INT32 len )
{
  return SetValue( DmtData( bin, len ) );
}

#endif
