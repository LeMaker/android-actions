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

/*=======================================================================

    Module Name: dm_tree_util.c
    General Description: Implementation of UTILITY functions provided by
                         DMTNM module
=======================================================================*/

#include "dm_tree_util.h"   
#include "dm_uri_utils.h"
#include "xpl_Logger.h"

SYNCML_DM_RET_STATUS_T dmGetNodeValue(CPCHAR uriPath, DmtData& oDmtData)
{
   SYNCML_DM_RET_STATUS_T status = SYNCML_DM_SUCCESS;
  
   DMGetData getData;
   status = dmTreeObj.Get(uriPath, getData, SYNCML_DM_REQUEST_TYPE_INTERNAL);
   if (status == SYNCML_DM_SUCCESS) {
      dmBuildData(getData.m_nFormat, getData.m_oData, oDmtData);
   }
   return status;
}

SYNCML_DM_RET_STATUS_T dmBuildData(SYNCML_DM_FORMAT_T format, const DMBuffer & oData, DmtData & oDmtData)
{

   SYNCML_DM_RET_STATUS_T res = SYNCML_DM_SUCCESS;
   switch (format) 
   {
      case SYNCML_DM_FORMAT_CHR:
      case SYNCML_DM_FORMAT_INT:
      case SYNCML_DM_FORMAT_BOOL:
      case SYNCML_DM_FORMAT_FLOAT:
      case SYNCML_DM_FORMAT_DATE:
      case SYNCML_DM_FORMAT_TIME:
         res = oDmtData.SetString((CPCHAR)oData.getBuffer(), format);
         break;
          
      case SYNCML_DM_FORMAT_BIN:
         res = oDmtData.SetBinary(oData.getBuffer(), oData.getSize());
         break;
          
      case SYNCML_DM_FORMAT_NULL:
        oDmtData = DmtData( (CPCHAR) NULL );
        break;
        
      case SYNCML_DM_FORMAT_NODE: 
      case SYNCML_DM_FORMAT_INVALID:  
         oDmtData = DmtData();
         break;
            
      default:
         oDmtData = DmtData();
         return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;
  }
  return res; 
}     


SYNCML_DM_RET_STATUS_T DMGetData::set(SYNCML_DM_FORMAT_T wFormat,
                                        CPCHAR pData, 
                                        UINT32 dataLength,
                                        CPCHAR pMimeType)
{

    clear();
    m_nFormat = wFormat;

    if( dataLength != 0 && pData != NULL )
    {
      m_oData.assign(pData, dataLength);
      if ( m_oData.getBuffer() == NULL )
        return SYNCML_DM_DEVICE_FULL;
    }
  
    if( pMimeType )
    {
       m_oMimeType.assign(pMimeType);
       if ( m_oMimeType.getBuffer() == NULL && pMimeType[0] ) 
       return SYNCML_DM_DEVICE_FULL;
    }

    return (SYNCML_DM_SUCCESS);
    
}

#ifdef LOB_SUPPORT  
 SYNCML_DM_RET_STATUS_T DMGetData::set(DmtDataChunk  *dmtChunk, UINT32 dmtChunkoffset)
{
   m_chunkOffset = dmtChunkoffset; 
   chunkData = dmtChunk;
    return (SYNCML_DM_SUCCESS);
}
void DMGetData::clearESNData() 
{	m_chunkOffset = 0L; 
	chunkData = NULL;
	m_bESN = FALSE;
}
#endif

SYNCML_DM_RET_STATUS_T DMGetData::set(const DmtData & data, CPCHAR pMimeType)
{
   SYNCML_DM_RET_STATUS_T res = SYNCML_DM_SUCCESS;
 
   clear();
#ifdef LOB_SUPPORT 
  clearESNData();
#endif
   switch ( data.GetType() ) 
   {
     default:
       {
           DMString strValue;
           data.GetString(strValue);
           if ( strValue.length() )
           {
             m_oData.assign(strValue); 
             if ( m_oData.getBuffer() == NULL )
                 return SYNCML_DM_DEVICE_FULL;    
           }
       }    
       break;

     case SYNCML_DM_DATAFORMAT_STRING:
       {
           const DMString & strValue = data.GetStringValue();
           if ( strValue.length() )
           {
             m_oData.assign(strValue); 
             if ( m_oData.getBuffer() == NULL )
                 return SYNCML_DM_DEVICE_FULL;    
           }
       }    
       break;
    
     case SYNCML_DM_DATAFORMAT_UNDEFINED:
     case SYNCML_DM_DATAFORMAT_NULL: 
       break;

     case SYNCML_DM_DATAFORMAT_BIN:
       {
           DMVector<UINT8> & buffer = (DMVector<UINT8> &)data.GetBinaryValue();
           if ( buffer.size() )
           {
              m_oData.assign(buffer.get_data(), buffer.size()); 
              if ( m_oData.getBuffer() == NULL )
                 return SYNCML_DM_DEVICE_FULL; 
           }   
       }    
       break;
   }

    
   switch ( data.GetType() ) 
   {
    case SYNCML_DM_DATAFORMAT_UNDEFINED:
      m_nFormat=SYNCML_DM_FORMAT_INVALID;
      break;

    case SYNCML_DM_DATAFORMAT_NULL:
    case SYNCML_DM_DATAFORMAT_BIN:
    case SYNCML_DM_DATAFORMAT_STRING:
    case SYNCML_DM_DATAFORMAT_INT:
    case SYNCML_DM_DATAFORMAT_FLOAT:
    case SYNCML_DM_DATAFORMAT_DATE:
    case SYNCML_DM_DATAFORMAT_TIME:
    case SYNCML_DM_DATAFORMAT_BOOL:
      m_nFormat=data.GetType();
      break;
   }

   if( pMimeType )
   {
      m_oMimeType.assign(pMimeType);
      if ( m_oMimeType.getBuffer() == NULL && pMimeType[0] ) 
         return SYNCML_DM_DEVICE_FULL;
   }

   return (SYNCML_DM_SUCCESS);
}

SYNCML_DM_RET_STATUS_T DMAddData::set(SYNCML_DM_FORMAT_T wFormat,
						  CPCHAR pMimeType)
{
    return  DMGetData::set(wFormat, NULL, 0, pMimeType);

}

SYNCML_DM_RET_STATUS_T DMAddData::set(CPCHAR pURI,
                            SYNCML_DM_FORMAT_T wFormat,
                            CPCHAR pData, 
                            UINT32 dataLength,
                            CPCHAR pMimeType)
{
    SYNCML_DM_RET_STATUS_T res = SYNCML_DM_SUCCESS;

    clear();
#ifdef LOB_SUPPORT 
  clearESNData();
#endif

    res = DMGetData::set(wFormat,pData,dataLength, pMimeType);
    if ( res != SYNCML_DM_SUCCESS )
        return res;

    if ( pURI )
    {
        m_oURI.assign(pURI);
        if ( m_oURI.getBuffer() == NULL ) 
             return SYNCML_DM_DEVICE_FULL;
    }    
    
    return (SYNCML_DM_SUCCESS);
}

#ifdef LOB_SUPPORT
 SYNCML_DM_RET_STATUS_T DMAddData::set(CPCHAR pURI,
  							DmtDataChunk  *dmtChunk, 
  							UINT32 dmtChunkoffset,
  							BOOLEAN isLastChunk)
{
   SYNCML_DM_RET_STATUS_T res = SYNCML_DM_SUCCESS;
   m_oURI.clear();

   if ( pURI )
   {
       m_oURI.assign(pURI);
       if ( pURI[0] && m_oURI.getBuffer() == NULL ) 
           return SYNCML_DM_DEVICE_FULL;
   }     

   res = DMGetData::set(dmtChunk, dmtChunkoffset);
   if ( res != SYNCML_DM_SUCCESS )
      return res;

   m_bLastChunk = isLastChunk;
   return (SYNCML_DM_SUCCESS);
}
#endif
SYNCML_DM_RET_STATUS_T DMAddData::set(CPCHAR pURI, const DmtData & data, CPCHAR pMimeType)
{

   SYNCML_DM_RET_STATUS_T res = SYNCML_DM_SUCCESS;

   m_oURI.clear();

   res = DMGetData::set(data, pMimeType);
   if ( res != SYNCML_DM_SUCCESS )
      return res;

   if ( pURI )
   {
       m_oURI.assign(pURI);
       if ( pURI[0] && m_oURI.getBuffer() == NULL ) 
           return SYNCML_DM_DEVICE_FULL;
   }     
    
   return (SYNCML_DM_SUCCESS);
}

#ifdef LOB_SUPPORT  
SYNCML_DM_RET_STATUS_T DMAddNodeProp::setESNFileName(CPCHAR pFileName)
{
    if ( pFileName )
    {
        m_oESNFileName.assign(pFileName);
        if ( pFileName[0] && m_oESNFileName.getBuffer() == NULL ) 
           return SYNCML_DM_DEVICE_FULL;
    }
   return (SYNCML_DM_SUCCESS);
}
#endif


SYNCML_DM_RET_STATUS_T DMAddNodeProp::set(CPCHAR pName,
                                           CPCHAR pTitle,
                                           SYNCML_DM_FORMAT_T wFormat,
                                           CPCHAR pData, 
                                           UINT32 dataLength,
                                           CPCHAR pMimeType,
                                           UINT16 flag)
{

    SYNCML_DM_RET_STATUS_T res = SYNCML_DM_SUCCESS;
    
    m_oName.clear();
    m_oTitle.clear();
    
    res = DMAddData::set(NULL, wFormat, pData, dataLength, pMimeType);
    if ( res != SYNCML_DM_SUCCESS )
       return res;

    if ( pName )
    {
        m_oName.assign(pName);
        if ( pName[0] && m_oName.getBuffer() == NULL ) 
           return SYNCML_DM_DEVICE_FULL;
    }

    if ( pTitle )
    {
        m_oTitle.assign(pName);
        if ( pTitle[0] && m_oTitle.getBuffer() == NULL ) 
           return SYNCML_DM_DEVICE_FULL;
    }    
        
    m_nFlags = flag;

    return SYNCML_DM_SUCCESS;



}



 /**
 * Free the memory of the GET data structure in the Map
 * 
 * @author Andy
 * @param getDataMap the GET data structure Map to be free
 * @return the status of the operation
 */
void dmFreeGetMap( DMMap<DMString, UINT32>& getDataMap) 
{
   DMGetData* tmpData = NULL;   
   for ( DMMap<DMString, UINT32>::POS it = getDataMap.begin(); it < getDataMap.end(); it++ ) 
   {
       tmpData = (DMGetData*)getDataMap.get_value(it);   

       if (tmpData != NULL) 
         delete tmpData;
   }
   
   getDataMap.clear();
}

/**
 * Free the memory of the ADD data structure in the Map
 * 
 * @author Andy
 * @param addDataMap the ADD data structure Map to be free
 * @return the status of the operation
 */
void dmFreeAddMap( DMMap<DMString, UINT32>& addDataMap) 
{
   DMAddData * tmpData = NULL;   
   for ( DMMap<DMString, UINT32>::POS it = addDataMap.begin(); it < addDataMap.end(); it++ ) 
   {
       tmpData = (DMAddData *)addDataMap.get_value(it);   
       delete tmpData;      
   }
   addDataMap.clear();
}


/**
 * Convert the GET data map to ADD data map
 * 
 * @author Andy
 * @param path the path to the parent of the nodes in the map
 * @param mapNodes, the GET data map
 * @param newChildrenMap the ADD data map
 * @return the status of the operation
 */
SYNCML_DM_RET_STATUS_T dmConvertDataMap(CPCHAR path, const DMMap<DMString, DmtData>& mapNodes, DMMap<DMString, UINT32>& newChildrenMap) 
{
  SYNCML_DM_RET_STATUS_T res;
  DMAddData * pData = NULL;
  
  for ( DMMap<DMString, DmtData>::POS it = mapNodes.begin(); it != mapNodes.end(); it++ )
  {
    DMString strChildPath = path; 
    strChildPath += "/"; 
    strChildPath += mapNodes.get_key( it );
    pData = new DMAddData();
    if ( pData == NULL )
    {
        XPL_LOG_DM_TMN_Error(("dmConvertDataMap : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL;
    }    
      
    res = pData->set(strChildPath,mapNodes.get_value( it ),"text/plain");
    if ( res != SYNCML_DM_SUCCESS ) 
    {
      delete pData;
      return res;
    }
    newChildrenMap.put(mapNodes.get_key( it ), (UINT32)pData);
  }
  
  return SYNCML_DM_SUCCESS;
}
