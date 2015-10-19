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

#ifndef _DM_TREE_TYPEDEF_H
#define _DM_TREE_TYPEDEF_H

/*========================================================================
        Header Name: dm_tree_typedef.h

    General Description: This file gives the data type definitions used
                       by the DMTNM module.
========================================================================*/

#include "syncml_dm_data_types.h"  /*data defns of UA global data types*/
#include "xpl_Time.h"
#include "dmbuffer.h"
#include "dmtData.hpp"
#include "dmt.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/*========================================================================
                            ENUMS
========================================================================*/
enum
{
  SYNCML_DM_GET_ON_LIST_STRUCT,    /* ?list=Struct found in URI */
  SYNCML_DM_GET_ON_LIST_STRUCTDATA /* ?list=StructData found in URI */

};
typedef UINT8 SYNCML_DM_GET_ON_LIST_T;

enum
{
  SYNCML_DM_SERIALIZATION_SUCCESS,
  SYNCML_DM_SERIALIZATION_FAIL,
  SYNCML_DM_TREE_TRAVERSING_OVER  
};
typedef UINT16 SYNCML_DM_SERIALIZATION_STATUS_T;

enum
{
  SYNCML_DM_REQUEST_TYPE_API,
  SYNCML_DM_REQUEST_TYPE_SERVER,
  SYNCML_DM_REQUEST_TYPE_INTERNAL
};
typedef UINT8 SYNCML_DM_REQUEST_TYPE_T;


#ifdef __cplusplus
}
#endif
/*========================================================================
                   STRUCTURES AND OTHER TYPEDEFS
========================================================================*/


/*class used for returning the data for GET operation */
class DMGetData
{ 
public:
  
  inline DMGetData() {
   m_nFormat = SYNCML_DM_FORMAT_INVALID; 
#ifdef LOB_SUPPORT
   m_chunkOffset = 0L; 		// offset
   chunkData = NULL;
#endif
  }

  inline ~DMGetData() {}
  
  inline void clear()
  { 
     m_nFormat = SYNCML_DM_FORMAT_INVALID; 
     m_TotalSize = -1;
     m_oMimeType.clear();
     m_oData.clear();
  }

  SYNCML_DM_RET_STATUS_T set(SYNCML_DM_FORMAT_T wFormat,
                             CPCHAR pData, 
                             UINT32 dataLength,
                             CPCHAR pMimeType);

  SYNCML_DM_RET_STATUS_T set(const DmtData & data, CPCHAR pMimeType);

  inline CPCHAR getCharData() const { return (CPCHAR)m_oData.getBuffer(); }

  inline CPCHAR getType() const { return (CPCHAR)m_oMimeType.getBuffer(); }
 
  inline void* operator new(size_t dwSize)   {return (DmAllocMem(dwSize)); }
  
  inline void operator delete(void *pvBuf)  { DmFreeMem(pvBuf); }

#ifdef LOB_SUPPORT
  SYNCML_DM_RET_STATUS_T set(DmtDataChunk  *dmtChunk, UINT32 dmtChunkoffset);
  void clearESNData();
  void  SetESN(BOOLEAN isESN) {m_bESN = isESN;}
  BOOLEAN  IsESN() const {return m_bESN;}
#else
  BOOLEAN  IsESN() const {return FALSE;}
#endif
  DMBuffer           m_oMimeType;
  SYNCML_DM_FORMAT_T m_nFormat;
  DMBuffer           m_oData;
  int m_TotalSize;

#ifdef LOB_SUPPORT
  UINT32 m_chunkOffset; 		// offset
  DmtDataChunk *chunkData;
   BOOLEAN  m_bESN;
#endif

};


/*class used for ADD on  a node*/
class DMAddData : public DMGetData
{
public:

   DMAddData ()
   {
	clear();
   }
  inline void clear()
  { 
     DMGetData::clear();
     m_oURI.clear();
  }

  inline CPCHAR getURI() const { return (CPCHAR)m_oURI.getBuffer(); }
  
  SYNCML_DM_RET_STATUS_T set(CPCHAR pURI,
                            SYNCML_DM_FORMAT_T wFormat,
                            CPCHAR pData, 
                            UINT32 dataLength,
                            CPCHAR pMimeType);

  SYNCML_DM_RET_STATUS_T set(SYNCML_DM_FORMAT_T wFormat, CPCHAR pMimeType);

  SYNCML_DM_RET_STATUS_T set(CPCHAR pURI, const DmtData & data, CPCHAR pMimeType);
#ifdef LOB_SUPPORT
  void clearESNData()
{   DMGetData::clearESNData();
     m_bLastChunk = FALSE;
}
  BOOLEAN  IsLastChunk() const {return m_bLastChunk;}
  SYNCML_DM_RET_STATUS_T set(CPCHAR pURI,
  							DmtDataChunk  *dmtChunk, 
  							UINT32 dmtChunkoffset,
  							BOOLEAN isLastChunk);
   BOOLEAN  m_bLastChunk;
#endif

  
  DMBuffer m_oURI;
};



struct SYNCML_DM_GET_ON_LIST_RET_DATA_T
{
  DMGetData *psRetData;
  DMString  _pbURI;
  DMString m_strStartURI;
  DMString m_strNextChild;

  inline SYNCML_DM_GET_ON_LIST_RET_DATA_T() {psRetData=NULL;}
};


/*structure for passing properties in ADD*/
class DMAddNodeProp : public DMAddData
{
public:

   DMAddNodeProp ()
   {
     m_nFlags = 0;
     m_nFormat = SYNCML_DM_FORMAT_NODE;
#ifndef DM_IGNORE_TSTAMP_AND_VERSION
     m_nTStamp = 0;
     m_nVerNo = 0;
#endif
 
     m_oURI.allocate(SYNCML_DM_URI_MAX_TOTAL_LENGTH);
     m_oName.allocate(SYNCML_DM_MAX_TITLE_LENGTH);
     m_oTitle.allocate(SYNCML_DM_MAX_TITLE_LENGTH);
     m_oMimeType.allocate(SYNCML_DM_MAX_TITLE_LENGTH);  
     m_oData.allocate(SYNCML_DM_MAX_TITLE_LENGTH);  
#ifdef LOB_SUPPORT  
     m_oESNFileName.allocate(SYNCML_DM_URI_MAX_TOTAL_LENGTH);
#endif
   }

   inline void clear()
   {
      DMAddData::clear();
      m_oName.clear(); 
      m_oTitle.clear();
#ifdef LOB_SUPPORT  
     m_oESNFileName.clear();
#endif
      m_nFlags = 0;
      m_nFormat = SYNCML_DM_FORMAT_NODE;
#ifndef DM_IGNORE_TSTAMP_AND_VERSION      
      m_nTStamp = 0;
      m_nVerNo = 0;
#endif     
      m_oOPiData.clear();
   }


   SYNCML_DM_RET_STATUS_T set(CPCHAR pName,
                              CPCHAR pTitle,
                              SYNCML_DM_FORMAT_T wFormat,
                              CPCHAR pData, 
                              UINT32 dataLength,
                              CPCHAR pMimeType,
                              UINT16 flag); 


   inline CPCHAR getName() const { return (CPCHAR)m_oName.getBuffer(); }

   inline CPCHAR getTitle() const { return (CPCHAR)m_oTitle.getBuffer(); }

#ifdef LOB_SUPPORT  
   inline CPCHAR getESNFileName() const { return (CPCHAR)m_oESNFileName.getBuffer(); }
   SYNCML_DM_RET_STATUS_T setESNFileName(CPCHAR pFileName); 
#endif
#ifndef DM_IGNORE_TSTAMP_AND_VERSION
   XPL_CLK_CLOCK_T m_nTStamp;
   UINT16 m_nVerNo;
#endif


   UINT16    m_nFlags;
#ifdef LOB_SUPPORT
  DMBuffer m_oESNFileName;
#endif
   DMBuffer m_oName;
   DMBuffer m_oTitle;
   DMBuffer m_oOPiData;
};

#endif  /*DM_TREE_TYPEDEF_H*/
