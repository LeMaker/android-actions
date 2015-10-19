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

/*==================================================================================================

    Source Name: DMMetaDataManager.cc

    General Description: Implementation of the DMMetaDataManager class

==================================================================================================*/

#include "dmStringUtil.h"
#include "dm_tree_util.h"
#include "dm_uri_utils.h"
#include "xpl_Logger.h"
#include "xpl_dm_Manager.h"
#include "dm_tree_plugin_util.H"
#include "dm_tree_plugin_root_node_class.H"
#include "dmMetaDataManager.h"
#include "xpl_Regex.h"

CPCHAR  DMMetaDataManager::m_pFloatPattern[] = 
{
   "^[+-]?[0-9]+[.]?[0-9]*[Ee]?[+-]?[0-9]*$", // ±dE±n or ±de±n
   "^[+-,0-9,Ee,.]*[Ee+-.]$", // ends in character 'E' 'e'  '+'  '-' 
   "^[^Ee]+[+-][0-9]$"  // '±' is not following 'E' or 'e'
};

CPCHAR  DMMetaDataManager::m_pDatePattern[] = 
{
   "^[0-9]{4}-[0-9]{2}-[0-9]{2}$", // YYYY-MM-DD 
   "^[0-9]{4}-[0-9]{2}$", // YYYY-MM 
   "^[0-9]{4}-[0-9]{3}$", //YYY-DDD
   "^[0-9]{4}-[W][0-9]{2}-[1-7]$",  //YYYY-Wxx-d
   "^[0-9]{4}-[W][0-9]{2}$",  //YYYY-Wxx
   "^[0-9]{8}$", //YYYYMMDD
   "^[0-9]{6}$",  //YYYYMM
   "^[0-9]{7}$", //YYYDDD
   "^[0-9]{4}$", // YYYY
   "^[0-9]{4}[W][0-9]{2}[1-7]$", //YYYYWxxd
   "^[0-9]{4}[W][0-9]{2}$" //YYYYWxx
};

CPCHAR DMMetaDataManager::m_pTimePattern[] = 
{
   "^[0-9]{2}:[0-9]{2}:[0-9]{2}$", //hh:mm:ss
   "^[0-9]{2}:[0-9]{2}$", //hh:mm
   "^[0-9]{6}$", //hhmmss
   "^[0-9]{4}$", //hhmm
   "^[0-9]{2}$",  //hh
   "^[0-9]{2}:[0-9]{2}:[0-9]{2}Z$", //hh:mm:ssZ
   "^[0-9]{2}:[0-9]{2}:[0-9]{2}[+-][0-9]{2}:[0-9]{2}$" //hh:mm:ss±hh:mm
};

DMMetaDataManager::DMMetaDataManager()
  : m_pEnv( NULL ),
    m_pTree( NULL ),
    m_bIsLoad( FALSE )
{
}

SYNCML_DM_RET_STATUS_T DMMetaDataManager::Init( CEnv* env, DMTree* tree )
{
  if( !env || !tree ) return SYNCML_DM_FAIL;
  m_pEnv  = env; 
  m_pTree = tree;
  return SYNCML_DM_SUCCESS;
}


void DMMetaDataManager::DeInit()
{
  UnLoad();
  m_pEnv = NULL; 
  m_pTree = NULL;
}

DMMetaDataManager::~DMMetaDataManager()
{
  DeInit();
}


void
DMMetaDataManager::UnLoad()
{
   if(m_bIsLoad == FALSE)
    return;

   XPL_LOG_DM_TMN_Debug(("UNLOAD\n"));
#ifndef DM_STATIC_FILES   
   UINT32 size = m_oDDFInfo.size();
   for (UINT32 i=0; i<size; i++)     
   {
        DMFileHandler * pFile = (DMFileHandler*)m_oDDFInfo[i];
        delete pFile;
   }    
#endif   
   m_oDDFInfo.clear();
   m_bIsLoad = FALSE;
   m_oLastNodeLocator.Init();
}

CPCHAR
DMMetaDataManager::GetNodeName(DMNode* pNode, CPCHAR szURI)
{
    if ( pNode->isPlugin() ) 
    {
        DMURI uri(FALSE,szURI);
        return uri.getLastSegment();
    } else 
        return pNode->abNodeName.GetBuffer();
}


BOOLEAN 
DMMetaDataManager::VerifyChildDependency(CPCHAR szNodeName,
                                          CPCHAR szURI,
                                          CPCHAR szOrigName,
                                          SYNCML_DM_MDF_CBACK callBackSoft,
                                          SYNCML_DM_MDF_CBACK callBackHard,
                                          SYNCML_DM_ACCESS_TYPE_T accessType)
{ 
    DMMetaDataNode oNodeMDF;
    DMConstraints *pConstraints = NULL;
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;
    
    ret_status = GetNode(szURI, FALSE, oNodeMDF);
    if ( ret_status != SYNCML_DM_SUCCESS )
    {
        return FALSE;
    }
    if ( oNodeMDF.m_nNodeFormat == SYNCML_DM_FORMAT_TEST )
        return TRUE;
 
    pConstraints = oNodeMDF.GetConstraints();
    if ( !pConstraints )   
        return TRUE;

    if( pConstraints->m_psChild != 0 )
    {
        if( !VerifyDependency(szNodeName,pConstraints->m_psChild,szOrigName,callBackHard) )
            return FALSE;
    }
    if( pConstraints->m_psDepend != 0 )
    {
        if( !VerifyDependency(szNodeName,pConstraints->m_psDepend,szOrigName,callBackSoft) )
            return FALSE;
    }
    return TRUE;
    
}



SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::VerifyDeleteParameters(DMNode* pNode, CPCHAR szURI)

{ 
    DMMetaDataNode oNodeMDF;
    CPCHAR strNodeName;
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;

    strNodeName = GetNodeName(pNode,szURI);
    if(strNodeName == NULL)
        return SYNCML_DM_COMMAND_FAILED;
    if (!VerifyChildDependency(strNodeName,szURI,NULL,ClearNodeValue,CheckFieldInUse,SYNCML_DM_DELETE_ACCESS_TYPE))
           return SYNCML_DM_COMMAND_FAILED;     
    return ret_status;
}



SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::VerifyData(DMAddData & oAddData)

{ 
    
    if(  !oAddData.m_oData.getSize() )
        return SYNCML_DM_SUCCESS;
    
    switch ( oAddData.m_nFormat )
    {
        case SYNCML_DM_FORMAT_INT:
            if ( !IsDigit(oAddData.m_oData) ) 
                 return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;
            break;

        case SYNCML_DM_FORMAT_FLOAT:
            if ( !IsFloat(oAddData.m_oData) ) 
                  return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;
            break;

         case SYNCML_DM_FORMAT_DATE:
            if ( !IsDate(oAddData.m_oData) ) 
                  return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;
            break;

         case SYNCML_DM_FORMAT_TIME:
             if ( !IsTime(oAddData.m_oData) ) 
                  return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;
             break;

          case SYNCML_DM_FORMAT_BOOL:
             if ( !IsBoolean(oAddData.m_oData) ) 
                 return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;
             break;
    }
    return SYNCML_DM_SUCCESS;
}   



SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::VerifyFormat(const DMMetaDataNode & oNodeMDF,
                                                DMAddData & oAddData)

{ 

    if ( m_pTree->IsVersion_12() == FALSE  )
    {
        if ( oAddData.m_nFormat ==  SYNCML_DM_FORMAT_FLOAT  ||
             oAddData.m_nFormat ==  SYNCML_DM_FORMAT_DATE  ||
             oAddData.m_nFormat ==  SYNCML_DM_FORMAT_TIME  )
        {
            XPL_LOG_DM_TMN_Debug(("DMMetaDataManager::VerifyFormat: format not supported: float,data, time\n"));
            return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;
        }
    }


    if ( oNodeMDF.m_nNodeFormat == SYNCML_DM_FORMAT_TEST )
        return SYNCML_DM_SUCCESS;
    
    if ( oAddData.m_nFormat != SYNCML_DM_FORMAT_INVALID && 
         oAddData.m_nFormat != SYNCML_DM_FORMAT_NULL )
    {
        if (oNodeMDF.m_nNodeFormat != oAddData.m_nFormat) 
        {
            XPL_LOG_DM_TMN_Debug(("DMMetaDataManager::VerifyFormat: unsupported mediatype: %d != %d\n", oNodeMDF.m_nNodeFormat, oAddData.m_nFormat));
            return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;
        } 
    }    
          
    if( oNodeMDF.VerifyMimeType(oAddData.getType()) != TRUE )
    { 
        XPL_LOG_DM_TMN_Debug(("DMMetaDataManager::VerifyFormat: verify mime type failed\n"));
        return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;
    }

    return SYNCML_DM_SUCCESS;
  
}   


SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::VerifyInteger(const DMBuffer & oData)
{

     if ( !oData.getSize() )
        return SYNCML_DM_SUCCESS;
     
     INT32 int_value = 0;
     char numbuf[MAX_INT_STRING_LENGTH];
   
     int_value = DmAtoi((CPCHAR)oData.getBuffer());
           
     DmSprintf( numbuf, "%d", int_value );
     if ( DmStrcmp(numbuf, (CPCHAR)oData.getBuffer()) != 0 ) 
        return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;

     return SYNCML_DM_SUCCESS;

}



SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::VerifyInteriorNodeConstraints(DMConstraints *pConstraints,
                                                                      CPCHAR szNodeName)

{ 
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
   
    if( pConstraints->m_nnMaxLen < DmStrlen(szNodeName) )
#ifdef DM_NO_REGEX
        return SYNCML_DM_REQUEST_ENTITY_TOO_LARGE;
#else        
        return SYNCML_DM_URI_TOO_LONG;
#endif
            
    if(VerifynValues(pConstraints, szNodeName) != TRUE)
        return SYNCML_DM_COMMAND_FAILED;

    if( pConstraints->m_psnRegexp != NULL)
        dm_stat = VerifyRegExp(pConstraints->m_psnRegexp,szNodeName);
   
    return dm_stat;
}


SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::SetDefaultValue(const DMMetaDataNode & oNodeMDF,
                                                     DMAddData & oAddData)
{
    DMConstraints *pConstraints = NULL;

    pConstraints = oNodeMDF.GetConstraints();

    if ( pConstraints == NULL )
        return SYNCML_DM_SUCCESS;

    if( !pConstraints->IsDefaultSet() )
        return(SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT);
        
    if(oNodeMDF.m_nNodeFormat == SYNCML_DM_FORMAT_INVALID)
        return(SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT);

    DMString strDefaultValue;
    pConstraints->GetDefaultString(strDefaultValue);
    if ( strDefaultValue.GetBuffer() )
    {
        oAddData.m_oData.assign(strDefaultValue);
        if ( oAddData.m_oData.getBuffer() == NULL )
             return SYNCML_DM_DEVICE_FULL;
    }   
    oAddData.m_nFormat = oNodeMDF.m_nNodeFormat;
    return SYNCML_DM_SUCCESS;

}


SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::VerifyLeafNodeConstraints(const DMMetaDataNode & oNodeMDF,
                                     DMAddData & oAddData)

{ 
    DMConstraints *pConstraints = NULL;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    pConstraints = oNodeMDF.GetConstraints();

    if ( pConstraints == NULL )
        return SYNCML_DM_SUCCESS;

#ifdef LOB_SUPPORT
    // ESN only support SYNCML_DM_FORMAT_BIN and  SYNCML_DM_FORMAT_STRING
    if(oNodeMDF.IsESN())
    {
         if ( oAddData.m_nFormat != SYNCML_DM_FORMAT_CHR  && 
              oAddData.m_nFormat != SYNCML_DM_FORMAT_BIN)
              return(SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT);
    }
#endif

    if(pConstraints->m_nMaxLen < oAddData.m_oData.getSize())
        return SYNCML_DM_REQUEST_ENTITY_TOO_LARGE;                               
                
    if (pConstraints->m_nMinLen > oAddData.m_oData.getSize())
        return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;                          

    if ( oAddData.m_nFormat == SYNCML_DM_FORMAT_INT )
    {
        INT32 int_value = 0;
        if ( oAddData.m_oData.getSize() )
            int_value = DmAtoi(oAddData.getCharData());
        
        if((pConstraints->m_nMin > int_value) || (pConstraints->m_nMax < int_value))
            return SYNCML_DM_COMMAND_FAILED;
     
    }

    if (pConstraints->m_psValues != NULL )
    {
        if ( oAddData.m_nFormat != SYNCML_DM_FORMAT_NULL )
        {
            if(!VerifyValues(pConstraints, oAddData.m_nFormat, oAddData.m_oData))
                return SYNCML_DM_COMMAND_FAILED;
        } 
        else
             return SYNCML_DM_COMMAND_FAILED;
    }    
  
    if ( pConstraints->m_psForeignKey != NULL )
    {
        if ( oAddData.m_nFormat != SYNCML_DM_FORMAT_NULL )
        {
            if(!VerifyForeignKey(pConstraints, oAddData.m_oData))
                return SYNCML_DM_COMMAND_FAILED;
        }
        else
             return SYNCML_DM_COMMAND_FAILED;
    }    

    if( pConstraints->m_psRegexp != NULL)
    {
        if ( oAddData.m_nFormat != SYNCML_DM_FORMAT_NULL )
        {
             dm_stat = VerifyRegExp(pConstraints->m_psRegexp,oAddData.getCharData());
             if (dm_stat != SYNCML_DM_SUCCESS) 
                return(SYNCML_DM_COMMAND_FAILED);      
         }
         else
               return SYNCML_DM_COMMAND_FAILED;
    }
            
    return dm_stat;
}


SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::VerifyConstraints(const DMMetaDataNode & oNodeMDF,
                                     DMNode* pNode,
                                     CPCHAR szNodeName,
                                     DMAddData & oAddData)

{ 
    DMConstraints *pConstraints = NULL;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    pConstraints = oNodeMDF.GetConstraints();

    if ( pConstraints == NULL )
        return SYNCML_DM_SUCCESS;

    dm_stat = VerifyInteriorNodeConstraints(pConstraints,szNodeName);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
    
 
    if ( oNodeMDF.m_nNodeFormat == SYNCML_DM_FORMAT_TEST )
        return SYNCML_DM_SUCCESS; 
        
    if ( oAddData.m_nFormat == SYNCML_DM_FORMAT_INVALID )
        dm_stat = SetDefaultValue(oNodeMDF,oAddData);
    else 
        if ( oAddData.m_nFormat != SYNCML_DM_FORMAT_NODE )
            dm_stat = VerifyLeafNodeConstraints(oNodeMDF, oAddData);
        else
            if ( oAddData.m_nFormat == SYNCML_DM_FORMAT_NULL && oAddData.m_oData.getSize() )
                return SYNCML_DM_COMMAND_FAILED;
            
    return dm_stat;
}



SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::VerifyParameters(const DMMetaDataNode & oNodeMDF,
                                     DMNode* pNode,
                                     CPCHAR szNodeName,
                                     DMAddData & oAddData)

{ 
    DMConstraints *pConstraints = NULL;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    dm_stat = VerifyFormat(oNodeMDF,oAddData);
    if ( dm_stat != SYNCML_DM_SUCCESS )
    {
        XPL_LOG_DM_TMN_Debug(("DMMetaDataManager::VerifyParameters: verify format failed \n")); 
        return dm_stat;
    } 

    dm_stat = VerifyData(oAddData);
    if ( dm_stat != SYNCML_DM_SUCCESS )
    {
        XPL_LOG_DM_TMN_Debug(("DMMetaDataManager::VerifyParameters: verify data failed \n"));
        return dm_stat;
    }
  
    pConstraints = oNodeMDF.GetConstraints();

    if ( !pConstraints )
    {
       if ( oAddData.m_nFormat == SYNCML_DM_FORMAT_INVALID || 
            oAddData.m_nFormat == SYNCML_DM_FORMAT_NULL )
            oAddData.m_nFormat = oNodeMDF.m_nNodeFormat;
    }   

    if ( oAddData.m_nFormat == SYNCML_DM_FORMAT_INT )
    {
        dm_stat = VerifyInteger(oAddData.m_oData);
        if ( dm_stat != SYNCML_DM_SUCCESS )
        {
             XPL_LOG_DM_TMN_Debug(("DMMetaDataManager::VerifyParameters: verify Integer failed \n"));
             return dm_stat;
        }
    }

    if ( pConstraints )
        dm_stat = VerifyConstraints(oNodeMDF,pNode,szNodeName,oAddData);    
    if ( dm_stat != SYNCML_DM_SUCCESS )
    {
         XPL_LOG_DM_TMN_Debug(("DMMetaDataManager::VerifyParameters: verify Constraits failed \n"));
    }

    return dm_stat;
}



SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::VerifyReplaceParameters(DMNode* pNode,
                                     CPCHAR szURI,
                                     DMAddData & oAddData,
                                     CPCHAR szOrigName)

{ 
    DMMetaDataNode oNodeMDF;
    CPCHAR strNodeName;
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;

    XPL_LOG_DM_TMN_Debug(("Enter to DMMetaDataManager::VerifyReplaceParameters %s\n",szURI));

    ret_status = GetNode(szURI, FALSE, oNodeMDF);
    XPL_LOG_DM_TMN_Error(("From VerifyReplaceParameters(): Call to  GetNode() returns %d \n", ret_status));
    
    if(ret_status == SYNCML_DM_NOT_FOUND)
        return SYNCML_DM_COMMAND_NOT_ALLOWED;
        
    else if(ret_status != SYNCML_DM_SUCCESS)
        return SYNCML_DM_COMMAND_FAILED;
    
#ifdef LOB_SUPPORT
    // ESN only only be created using empty data
    if(oNodeMDF.IsESN() && oAddData.m_oData.getSize() )
        return(SYNCML_DM_COMMAND_FAILED);
#endif

    strNodeName = GetNodeName(pNode,szURI);
    if(strNodeName == NULL){
        XPL_LOG_DM_TMN_Error(("From VerifyReplaceParameters(): call getNode()....  strNodeName == NULL"));
        return SYNCML_DM_COMMAND_FAILED;
    }
    ret_status = VerifyParameters(oNodeMDF,pNode,strNodeName,oAddData);
    XPL_LOG_DM_TMN_Error(("From VerifyReplaceParameters(): Call to  VerifyParameters() returns %d \n", ret_status));
    
    if(ret_status != SYNCML_DM_SUCCESS)
        return ret_status;
   
    if(szOrigName == NULL)
        return ret_status;
    
    if ( oNodeMDF.m_nNodeFormat == SYNCML_DM_FORMAT_TEST )
        return SYNCML_DM_SUCCESS;
    
    if (!VerifyChildDependency(strNodeName, szURI, szOrigName, ResetNodeValue, ResetNodeValue,
            SYNCML_DM_REPLACE_ACCESS_TYPE)) {
        XPL_LOG_DM_TMN_Error(("From VerifyReplaceParameters(): Call to VerifyChildDependency() returns FALSE"));
        return SYNCML_DM_COMMAND_FAILED; 
    }
    return ret_status;
}

SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::VerifyAddParameters(DMNode* pNode,
                                     DMAddData & oAddData,
                                     DMToken & oAutoNodes,
                                     BOOLEAN & bNodeGetAccess)
{ 
    DMMetaDataNode oNodeMDF;
    CPCHAR strNodeName;
    DMConstraints *pConstraints = NULL;
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;

    XPL_LOG_DM_TMN_Debug(("Enter to DMMetaDataManager::VerifyAddParameters %s\n",oAddData.getURI()));
        
    ret_status = GetNode(oAddData.getURI(), FALSE, oNodeMDF);
    if(ret_status != SYNCML_DM_SUCCESS)
    {
        XPL_LOG_DM_TMN_Debug(("DMMetaDataManager::VerifyAddParameters: failed to get node MDF.\n"));
        return SYNCML_DM_COMMAND_FAILED;
    }

#ifdef LOB_SUPPORT
    // ESN only only be created using empty data
    if(oNodeMDF.IsESN() && oAddData.m_oData.getSize() )
    {
        XPL_LOG_DM_TMN_Debug(("DMMetaDataManager::VerifyAddParameters: ESN, but not empty\n"));
        return(SYNCML_DM_COMMAND_FAILED);
    }
#endif
    strNodeName = GetNodeName(pNode,oAddData.getURI());
    if(strNodeName == NULL)
        return SYNCML_DM_COMMAND_FAILED;
    ret_status = VerifyParameters(oNodeMDF,pNode,strNodeName,oAddData);
    if(ret_status != SYNCML_DM_SUCCESS)
    { 
       XPL_LOG_DM_TMN_Debug(("DMMetaDataManager::VerifyAddParameters: failed to get verify parameters.\n"));
       return ret_status; 
    }

   bNodeGetAccess = oNodeMDF.VerifyAccessType(SYNCML_DM_GET_ACCESS_TYPE);
 
    if (oNodeMDF.m_nNodeFormat == SYNCML_DM_FORMAT_NODE )
    {
        pConstraints = oNodeMDF.GetConstraints();
        if ( pConstraints && pConstraints->m_psAutoNodes )
        {
            oAutoNodes.assign(pConstraints->m_psAutoNodes);
            if ( !oAutoNodes.getBuffer() )
                XPL_LOG_DM_TMN_Debug(("DMMetaDataManager::VerifyAddParameters: failed to get verify Access Type.\n"));
                return SYNCML_DM_COMMAND_FAILED;
        }    
    }
    return ret_status;
}



BOOLEAN 
DMMetaDataManager::IsDigit(const DMBuffer & oData)
{
   INT32 start_index = 0;
   UINT8 *pData = oData.getBuffer();
   
   if (pData[0] == '-' || pData[0] == '+')
     start_index++;

   for (UINT32 i=start_index; i<oData.getSize(); i++)
   {
      if (pData[i]<0x30 || pData[i]>0x39)
        return FALSE;
   }

   return TRUE;
}


BOOLEAN 
DMMetaDataManager::IsBoolean(const DMBuffer & oData)
{
    INT32 size = oData.getSize();
    UINT8 *pData = oData.getBuffer();

    if(size < 4 && size > 5)
         return FALSE;    

    char str[6];
    
    for (int i=0; i<size; i++)
       str[i] = DmTolower((char)pData[i]);
    str[size] = SYNCML_DM_NULL; 
   
    if ( DmStrcmp(str,"true") == 0 || DmStrcmp(str, "false") == 0 ) 
       return TRUE;

    return FALSE;
}


BOOLEAN 
DMMetaDataManager::IsFloat(const DMBuffer & oData)
{
#ifdef DM_NO_REGEX
    
    return ::XPL_RG_Comp( XPL_RG_PATTERN_IS_FLOAT, (CPCHAR)oData.getBuffer() )
              ? SYNCML_DM_SUCCESS
              : SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;
#else

   // Validate float string with XML 1.0 regular express
   if (VerifyRegExp((CPCHAR)m_pFloatPattern[0], (CPCHAR)oData.getBuffer())
          != SYNCML_DM_SUCCESS) 
   {
      return FALSE;
   }
   else 
   {
      // Float string ends with 'E', 'e', '+' '-' or '.' character.
      if (VerifyRegExp((CPCHAR)m_pFloatPattern[1], (CPCHAR)oData.getBuffer())
          == SYNCML_DM_SUCCESS) 
      {
         return FALSE;
      }
      else 
      {
          // '+' or '-' character is not next to 'E' or 'e'.
         if (VerifyRegExp((CPCHAR)m_pFloatPattern[2], (CPCHAR)oData.getBuffer())
                == SYNCML_DM_SUCCESS) 
         {
             return FALSE;
         }
         else 
         {
             return TRUE;
         }
      }
   }

   return TRUE;

#endif
}


BOOLEAN 
DMMetaDataManager::IsDate(const DMBuffer & oData)
{
#ifdef DM_NO_REGEX
    return ::XPL_RG_Comp( XPL_RG_PATTERN_IS_DATE, (CPCHAR)oData.getBuffer() )
              ? SYNCML_DM_SUCCESS
              : SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;
#else

   for (UINT8 i=0; i<sizeof(m_pDatePattern)/sizeof(int); i++) 
   {
      if (VerifyRegExp(m_pDatePattern[i],(CPCHAR)oData.getBuffer())==SYNCML_DM_SUCCESS) 
      {
         CPCHAR pData = (CPCHAR)oData.getBuffer();
         switch (i) 
         {
            case 0:  // YYYY-MM-DD
               if ((DmStrncmp(pData+5,"01",2)<0 || DmStrncmp(pData+5,"12",2)>0)||
                   (DmStrncmp(pData+8,"01",2)<0 || DmStrncmp(pData+8,"31",2)>0))
                  return FALSE;
               else 
                  return TRUE;
            case 1:  // YYYY-MM
               if ((DmStrncmp(pData+5,"01",2)<0 || DmStrncmp(pData+5,"12",2)>0))
                  return FALSE;
               else 
                  return TRUE;
            case 2:  // YYYY-DDD
               if ((DmStrncmp(pData+5,"001",3)<0 || DmStrncmp(pData+5,"366",3)>0))
                  return FALSE;
               else 
                  return TRUE;
            case 3:  // YYYY-Wxx-d
            case 4:  // YYYY-Wxx
               if ((DmStrncmp(pData+6,"01",2)<0 || DmStrncmp(pData+6,"52",2)>0))
                  return FALSE;
               else 
                  return TRUE;
            case 5:  // YYYYMMDD
               if ((DmStrncmp(pData+4,"01",2)<0 || DmStrncmp(pData+4,"12",2)>0)||
                   (DmStrncmp(pData+6,"01",2)<0 || DmStrncmp(pData+6,"31",2)>0))
                  return FALSE;
               else 
                  return TRUE;
            case 6:  // YYYYMM
               if ((DmStrncmp(pData+4,"01",2)<0 || DmStrncmp(pData+4,"12",2)>0))
                  return FALSE;
               else 
                  return TRUE;
            case 7:  // YYYYDDD
               if ((DmStrncmp(pData+4,"001",3)<0 || DmStrncmp(pData+4,"365",3)>0))
                  return FALSE;
               else 
                  return TRUE;
            case 8:  // YYYY
                  return TRUE;
            case 9:  // YYYYWxxd
            case 10:  // YYYYWxx
               if ((DmStrncmp(pData+5,"01",2)<0 || DmStrncmp(pData+5,"52",2)>0))
                  return FALSE;
               else 
                  return TRUE;
            default:
               return FALSE;
         }
      }
   }

   return FALSE;

#endif
}


BOOLEAN 
DMMetaDataManager::IsTime(const DMBuffer & oData)
{
#ifdef DM_NO_REGEX
    return ::XPL_RG_Comp( XPL_RG_PATTERN_IS_TIME, (CPCHAR)oData.getBuffer() )
              ? SYNCML_DM_SUCCESS
              : SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;
#else

   for (UINT8 i=0; i<sizeof(m_pTimePattern)/sizeof(int); i++) 
   {
      if (VerifyRegExp(m_pTimePattern[i], (CPCHAR)oData.getBuffer()) == SYNCML_DM_SUCCESS) 
      {
         CPCHAR pData = (CPCHAR)oData.getBuffer();
         // Validate hour
         if ((DmStrncmp(pData,"00",2)<0 || DmStrncmp(pData,"23",2)>0))
             return FALSE;

         switch (i) 
         {
            case 0:  // hh:mm:ss
            case 5:  // hh:mm:ssZ
               if ((DmStrncmp(pData+3,"00",2)<0 || DmStrncmp(pData+3,"59",2)>0) ||
                   (DmStrncmp(pData+6,"00",2)<0 || DmStrncmp(pData+6,"59",2)>0))
                  return FALSE;
               else 
                  return TRUE;
            case 1:  // hh:mm
               if ((DmStrncmp(pData+3,"00",2)<0 || DmStrncmp(pData+3,"59",2)>0))
                  return FALSE;
               else 
                  return TRUE;
            case 2:  // hhmmss
               if ((DmStrncmp(pData+2,"00",2)<0 || DmStrncmp(pData+2,"59",2)>0)||
                   (DmStrncmp(pData+4,"00",2)<0 || DmStrncmp(pData+4,"59",2)>0))
                  return FALSE;
               else 
                  return TRUE;
            case 3:  // hhmm
               if ((DmStrncmp(pData+2,"00",2)<0 || DmStrncmp(pData+2,"59",2)>0))
                  return FALSE;
               else 
                  return TRUE;
            case 4:  // hh
                  return TRUE;
            case 6:  // hh:mm:ss±hh:mm
               if ((DmStrncmp(pData,"00",2)<0 || DmStrncmp(pData,"23",2)>0)||
                   (DmStrncmp(pData+3,"00",2)<0 || DmStrncmp(pData+3,"59",2)>0)||
                   (DmStrncmp(pData+6,"00",2)<0 || DmStrncmp(pData+6,"59",2)>0)||
                   (DmStrncmp(pData+9,"00",2)<0 || DmStrncmp(pData+9,"23",2)>0)||
                   (DmStrncmp(pData+12,"00",2)<0 || DmStrncmp(pData+12,"59",2)>0))
                  return FALSE;
               else 
                  return TRUE;
            default:
               return FALSE;
         }
      }
   }

   return FALSE;

#endif
}

BOOLEAN 
DMMetaDataManager::VerifynValues(DMConstraints *pConstraints, const DMString & oNodeName)
{

    if ( pConstraints->m_psnValues == NULL )
        return TRUE;
    
    DMToken oValues(TRUE,pConstraints->m_psnValues,SYNCML_DM_COMMA);
    CPCHAR sName = NULL;
        
    while ( (sName = oValues.nextSegment()) != NULL )
        if( oNodeName == sName )
            return TRUE;
    return FALSE;
}


BOOLEAN 
DMMetaDataManager::VerifyValues(DMConstraints *pConstraints, SYNCML_DM_FORMAT_T format, const DMBuffer & oData)
{
    CPCHAR sValue = NULL;
  
    if (pConstraints->m_psValues == NULL)
       return TRUE;  

    DMToken oValues(TRUE,pConstraints->m_psValues,SYNCML_DM_COMMA);
    sValue = oValues.nextSegment();

    while ( sValue )
    {
        switch(format)
        {
            case SYNCML_DM_FORMAT_INT:
                if(DmAtoi((CPCHAR)oData.getBuffer()) == DmAtoi(sValue))
                    return TRUE;    
                break;
                        
            case SYNCML_DM_FORMAT_CHR:
            case SYNCML_DM_FORMAT_FLOAT:
            case SYNCML_DM_FORMAT_DATE:
            case SYNCML_DM_FORMAT_TIME:
                if( oData.compare(sValue) == TRUE )
                    return TRUE;
                break;

            default:
                return TRUE;
                break;
        }
        sValue = oValues.nextSegment();
    }    

    return FALSE;
    
}


BOOLEAN 
DMMetaDataManager::VerifyForeignKey(DMConstraints * pConstraints, const DMBuffer & oData)
{
    if (pConstraints->m_psForeignKey == NULL)
        return TRUE;
    
    DMGetData getData;   
    DMString strURI = pConstraints->m_psForeignKey;
    DMString strName( (const char*)oData.getBuffer(), oData.getSize() );

    strURI += "/";
    strURI += strName;
    
    /* Retrieve the child node names from the DM Tree.*/
    
    if( SYNCML_DM_SUCCESS != m_pTree->Get(strURI,getData,SYNCML_DM_REQUEST_TYPE_INTERNAL) )
        return FALSE;
    
  return TRUE;
}


SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::VerifyRegExp(CPCHAR sPattern, CPCHAR sData)
{
    BOOLEAN reg_status = FALSE;

#ifndef DM_NO_REGEX
    reg_status = XPL_RG_Comp(sPattern, sData);
#endif
    if ( reg_status ) 
        return SYNCML_DM_SUCCESS;
    else 
        return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;
}

#ifndef DM_STATIC_FILES 
SYNCML_DM_RET_STATUS_T
DMMetaDataManager::LoadBuffer(CPCHAR pPath)
{
    DMFileHandler *fp;

    fp = new DMFileHandler(pPath,FALSE);
    if ( fp == NULL )
        return SYNCML_DM_FAIL;
    
    if(fp->open(XPL_FS_FILE_READ) != SYNCML_DM_SUCCESS)
    {
       DMFileHandler::operator delete (fp);
       return SYNCML_DM_IO_FAILURE;
    }

    UINT8 * pBuffer = fp->mmap();
    if ( !pBuffer )
    {
      DMFileHandler::operator delete (fp);
      return SYNCML_DM_FAIL;
    }
    
    if ( pBuffer[0] == '[' && (pBuffer[1] == '/' || pBuffer[1] == '.') )
    {
       XPL_LOG_DM_TMN_Error(("BMDF: file %s not recognized as a bmdf file, ignored\n", pPath));
       DMFileHandler::operator delete (fp);
       return SYNCML_DM_IO_FAILURE;
    }    
#ifndef DM_IGNORE_BMDF_VERIFICATION
    {
      // verify file length
      DMMetaDataBuffer  buf( pBuffer );
      UINT32 nFileSize = buf.ReadUINT32();
      int nVersion = buf.ReadUINT16();

      if ( nFileSize!= fp->size() )
      {
        XPL_LOG_DM_TMN_Error(("BMDF: file %s size %d mismatch with buffer value %d, ignored\n",pPath,fp->size(),nFileSize));
        DMFileHandler::operator delete (fp);
        return SYNCML_DM_IO_FAILURE;
      }
      if ( nVersion != CurrentBMDFVersion )
      {
        XPL_LOG_DM_TMN_Error(("BMDF: file %s supported version %d mismatch with buffer value %d, ignored\n",
                              pPath, (int)CurrentBMDFVersion, nVersion));
        DMFileHandler::operator delete (fp);
        return SYNCML_DM_IO_FAILURE;
      }
    }
#endif  
    
    fp->close();
    XPL_LOG_DM_TMN_Debug(("LOAD BUFFER %s\n",pPath));
    m_oDDFInfo.push_back((UINT32)fp);
    return SYNCML_DM_SUCCESS;
}
#else
SYNCML_DM_RET_STATUS_T
DMMetaDataManager::LoadBuffer(UINT32 index)
{
    UINT8 * pBuffer; 
    UINT32 size;

    pBuffer = (UINT8*)XPL_DM_GetMDF(index,&size);
  
    if ( !pBuffer )
      return SYNCML_DM_FAIL;
    m_oDDFInfo.push_back((UINT32)pBuffer);
    return SYNCML_DM_SUCCESS;
}
#endif

UINT32 
DMMetaDataManager::FindNodeInChildrenList(DMMetaDataBuffer oBuffer, CPCHAR psName) const
{

    DMMetaDataNode oNode;
    DMMetaDataNode oChildrenNode;
    UINT32 offsetChildren = 0;
     
    oNode.Read(oBuffer,FALSE); 
    if ( oNode.IsHasMultiNodes() == FALSE )
    {
        for (int i=0; i<oNode.m_nNumChildren; i++)
        {
            oNode.SetChildrenOffset(&oBuffer,i);
            offsetChildren = oBuffer.GetOffset();
            oChildrenNode.ReadName(oBuffer);
            if ( !DmStrcmp(oChildrenNode.m_psName,psName) )
               return offsetChildren;
        }
    }    
    else
    {
        oNode.SetChildrenOffset(&oBuffer,0);
        return oBuffer.GetOffset();
    }    
    return 0;    
}


CPCHAR
DMMetaDataManager::BuildSearchURI(CPCHAR szURI,
                                                          DMMetaDataBuffer & oBuffer, 
                                                          SYNCML_DM_ACCESS_TYPE_T & accessType,
                                                          DMMetaDataNode & oNode )
{
 
    DMParser sURIParser(szURI);
    CPCHAR sSegment1 = NULL;
    CPCHAR sSegment2 = NULL;    

    UINT16 count1 = sURIParser.getSegmentsCount();
    UINT16 count2 = m_oLastNodeLocator.m_oLocator.size();
    UINT16 count = ( count1 > count2 ) ? count2 : count1; 
    UINT32 offset = 0;


    for (int i=0 ; i<count; i++)
    {
        sSegment1 = (m_oLastNodeLocator.m_oLocator[i]).m_szName;
        sSegment2 = sURIParser.nextSegment(); 

        if (sSegment2 != NULL && DmStrcmp(sSegment1,sSegment2) != 0 )
        {
            if ( i == 0 ) 
                return szURI;

            m_oLastNodeLocator.m_oLocator.set_size(i);
            oBuffer.SetBuffer(m_oLastNodeLocator.m_pBuffer);
            offset = (m_oLastNodeLocator.m_oLocator[i-1]).m_nOffset;
            accessType = (m_oLastNodeLocator.m_oLocator[i-1]).m_wAccessType;
            oNode.SetPath((m_oLastNodeLocator.m_oLocator[i-1]).m_strPath);
            oBuffer.SetOffset(offset);
            return sSegment2;
        }    
    }

    oBuffer.SetBuffer(m_oLastNodeLocator.m_pBuffer);
    if ( count2 >= count1 )
    {
        offset = (m_oLastNodeLocator.m_oLocator[count1-1]).m_nOffset;
        accessType = (m_oLastNodeLocator.m_oLocator[count1-1]).m_wAccessType;
        oNode.SetPath((m_oLastNodeLocator.m_oLocator[count1-1]).m_strPath);
        oBuffer.SetOffset(offset);
        return NULL;   
    }
    else
    {
        offset = (m_oLastNodeLocator.m_oLocator[count2-1]).m_nOffset;
        accessType = (m_oLastNodeLocator.m_oLocator[count2-1]).m_wAccessType;
        oNode.SetPath((m_oLastNodeLocator.m_oLocator[count2-1]).m_strPath);
           
        oBuffer.SetOffset(offset);
        return sURIParser.nextSegment(); 
    }

}


SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::SearchNode(CPCHAR szURI, 
                              DMMetaDataBuffer oBuffer, 
                              SYNCML_DM_ACCESS_TYPE_T parentAccessType,
                              BOOLEAN bCheckMultiNode,
                              DMMetaDataNode & oNode,
                              DMMetaPCharVector* pChildDependNodes ) 
{
   
     SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;
     CPCHAR sSegment = NULL;
     UINT32 nodeOffset = 0;
     DMURI oURI(FALSE,szURI);
     BOOLEAN bIsFound = FALSE;
     DMConstraints * pConstraints = NULL;

   
     sSegment = oURI.nextSegment();
     while( sSegment != NULL )
     {
        nodeOffset = FindNodeInChildrenList(oBuffer,sSegment);
        if ( nodeOffset == 0 )
        {
            oNode.Init();
            break;
        }    

        
        oBuffer.SetOffset(nodeOffset);
        oNode.Read(oBuffer,TRUE);   


        pConstraints = oNode.GetConstraints();
        if ( pConstraints && pConstraints->m_psRecurAfterSegment )
        {
            bIsFound = TRUE;        
            ret_status = RemoveRecursiveSegments((char*)oURI.getTailSegments(),oNode);
            if ( ret_status == SYNCML_DM_FAIL )
            {
                oNode.Init();
                return ret_status;
            }
        }  

        if ( pChildDependNodes && pConstraints )
        {
          if ( pConstraints->m_psChild )
            pChildDependNodes->push_back( pConstraints->m_psChild );

          if ( pConstraints->m_psDepend )
            pChildDependNodes->push_back( pConstraints->m_psDepend );
        }

        if ( oNode.m_wAccessType == 0 )
          oNode.SetAccessType(parentAccessType);
        
        parentAccessType = oNode.m_wAccessType;
        
        if ( !bIsFound )
        {
            oNode.AppendSegment();
            m_oLastNodeLocator.m_oLocator.push_back(DMMetaNodeLocator(oNode.m_psName,
                                                                                                     nodeOffset,
                                                                                                     oNode.m_wAccessType,
                                                                                                     oNode.GetPath()));
        }    

        sSegment = oURI.nextSegment();
        if( sSegment == NULL ) 
        {
            if (  bCheckMultiNode )
                oNode.GetMaxMultiNodeChildren(oBuffer);  
            return SYNCML_DM_SUCCESS;
        }
  
     }   

     return SYNCML_DM_NOT_FOUND; 

}


SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::GetRootNode(DMMetaDataNode & oNode)
{
        DMMetaDataBuffer oBuffer;

   #ifndef DM_STATIC_FILES 
        DMFileHandler * pFile = (DMFileHandler*)m_oDDFInfo[0];
        oBuffer.SetBuffer((MDF_BUFFER_T)pFile->mmap());
#else
        oBuffer.SetBuffer((MDF_BUFFER_T)m_oDDFInfo[0]);
#endif
        oBuffer.SetOffset(BMDFHeaderSize); // file size and version
        oNode.Read(oBuffer,TRUE);   

        return SYNCML_DM_SUCCESS;
}   


CPCHAR 
DMMetaDataManager::GetStartPos(CPCHAR szURI)
{
    switch ( szURI[0] )
     {
        case SYNCML_DM_DOT:
            if (DmStrlen(szURI) >= 3) 
                return &szURI[2];
            break;

        case SYNCML_DM_FORWARD_SLASH:
            return NULL;

        default:    
            return szURI;
    }
    return NULL;
}    


SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::FindCacheNode(CPCHAR szURI,
                                                    BOOLEAN bCheckMultiNode, 
                                                    DMMetaDataNode & oNode,
                                                    DMMetaDataBuffer & oBuffer,
                                                    DMMetaPCharVector* pChildDependNodes)
{
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;
    SYNCML_DM_ACCESS_TYPE_T parentAccessType;
    CPCHAR sStartPos = NULL;
    
    if ( m_oLastNodeLocator.m_oLocator.size() == 0)
        return SYNCML_DM_NOT_FOUND;
    
    sStartPos = BuildSearchURI((CPCHAR)szURI,oBuffer,parentAccessType, oNode);
    if ( oBuffer.GetBuffer() == NULL )
       return SYNCML_DM_NOT_FOUND;
    
    if ( sStartPos == NULL )
    {
        ret_status = oNode.Read(oBuffer,TRUE);   
        if ( ret_status == SYNCML_DM_SUCCESS && bCheckMultiNode )
            ret_status = oNode.GetMaxMultiNodeChildren(oBuffer);  
            
        if ( ret_status != SYNCML_DM_SUCCESS )
            oNode.Init();
        else
        {
            if ( oNode.m_wAccessType == 0 )
                oNode.m_wAccessType = parentAccessType;
        }    
        return ret_status;
    }
    ret_status = SearchNode(sStartPos,oBuffer,parentAccessType,bCheckMultiNode,oNode, pChildDependNodes);
    if ( ret_status == SYNCML_DM_SUCCESS )
        return ret_status;
    
    return SYNCML_DM_NOT_FOUND; 
    
}    


SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::GetNode(CPCHAR szURI, 
                                           BOOLEAN bCheckMultiNode, 
                                           DMMetaDataNode & oNode, 
                                           DMMetaPCharVector* pChildDependNodes /*=NULL*/) 
{
   
     SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;
     DMString sNodeURI;

     ret_status = oNode.AllocatePath(DmStrlen(szURI)+1);
     if ( ret_status != SYNCML_DM_SUCCESS )
           return ret_status;
     
     oNode.Init();

     if(m_bIsLoad == FALSE)
     {
        ret_status = Load();
        if ( ret_status != SYNCML_DM_SUCCESS )
             return ret_status;
     }

     if ( DmStrcmp(szURI,".") == 0 )
         return GetRootNode(oNode);

    sNodeURI = GetStartPos(szURI);
    if ( sNodeURI == NULL )
        return SYNCML_DM_FAIL;

    DMMetaDataBuffer oBuffer;
    ret_status = FindCacheNode((CPCHAR)sNodeURI,bCheckMultiNode,oNode,oBuffer,pChildDependNodes);
    if ( ret_status == SYNCML_DM_SUCCESS )
        return ret_status;
   
    SYNCML_DM_ACCESS_TYPE_T parentAccessType;
    MDF_BUFFER_T pSearchedBuffer = oBuffer.GetBuffer();
    UINT32 size = m_oDDFInfo.size();
    
    for (UINT32 i=0; i<size; i++) // loop through all mdf's files
    {
#ifndef DM_STATIC_FILES 
        DMFileHandler * pFile = (DMFileHandler*)m_oDDFInfo[i];
        oBuffer.SetBuffer((MDF_BUFFER_T)pFile->mmap());
#else
        oBuffer.SetBuffer((MDF_BUFFER_T)m_oDDFInfo[i]);
#endif
        oBuffer.SetOffset(BMDFHeaderSize); // file size and version
#ifndef DM_IGNORE_BMDF_VERIFICATION
        oBuffer.ResetCorrupted();
#endif
        if (pSearchedBuffer == oBuffer.GetBuffer())
            continue;
        
        oNode.Read(oBuffer,FALSE);   
        if (DmStrcmp(oNode.m_psName,".") != 0) 
        {
#ifndef DM_IGNORE_BMDF_VERIFICATION
            if ( oBuffer.IsCorrupted() ){ 
#ifndef DM_STATIC_FILES                
              XPL_LOG_DM_TMN_Error(("BMDF: File %s is corrupted!\n", pFile->getFullPath()));
              delete pFile; 
              pFile = NULL;
#endif
              m_oLastNodeLocator.Init();
              m_oDDFInfo.remove( i );
              i--; size--;
            }
#else
            continue;
#endif
        }
        parentAccessType = oNode.m_wAccessType;
        m_oLastNodeLocator.Init();
        m_oLastNodeLocator.m_pBuffer = oBuffer.GetBuffer();
        ret_status = SearchNode((CPCHAR)sNodeURI,oBuffer,parentAccessType,bCheckMultiNode,oNode, pChildDependNodes);

#ifndef DM_IGNORE_BMDF_VERIFICATION
        if ( oBuffer.IsCorrupted() )
        { 
#ifndef DM_STATIC_FILES                
           if (pFile != NULL) 
           {
              XPL_LOG_DM_TMN_Error(("BMDF: File %s is corrupted!\n", pFile->getFullPath()));
              delete pFile;
              pFile = NULL;
           }
           else
           {
              XPL_LOG_DM_TMN_Error(("BMDF: File is corrupted!\n"));
           }
#endif
          m_oLastNodeLocator.Init();
          m_oDDFInfo.remove( i );
          i--; size--;
        }
#endif  

        
        if ( ret_status != SYNCML_DM_NOT_FOUND )
             return ret_status;

    }    
  
    return SYNCML_DM_NOT_FOUND; 

}

BOOLEAN 
DMMetaDataManager::CheckFieldInUse(DMNode* pNode, 
                                                     PDmtNode pPluginNode, 
                                                     CPCHAR szNodeName, 
                                                     CPCHAR szOrigName)
{
    if (pPluginNode != NULL) 
    {
        SYNCML_DM_RET_STATUS_T e;
        DmtData data;

        e = pPluginNode->GetValue(data);
        if ( (e==SYNCML_DM_SUCCESS) && (data.GetType() == SYNCML_DM_DATAFORMAT_STRING) )
        {
            const DMString & strVal = data.GetStringValue();
            if (DmStrncmp((CPCHAR)strVal.c_str(), szNodeName, DmStrlen(szNodeName)) == 0 ) 
               return FALSE;
        }
        else 
            return FALSE;
    } 
    else 
        if ( pNode->getData() && pNode->getData()->getSize() )
        {
            if (DmStrncmp((CPCHAR)pNode->getData()->getBuffer(), szNodeName, pNode->getData()->getSize())==0)
                return FALSE;
        }
    return TRUE;
}


BOOLEAN 
DMMetaDataManager::ClearNodeValue(DMNode* pNode,
                                                    PDmtNode pPluginNode, 
                                                    CPCHAR szNodeName, 
                                                    CPCHAR szOrigName)
{
    if ( pPluginNode != NULL )
    {
        SYNCML_DM_RET_STATUS_T dm_stat;
        DmtData data;

        dm_stat = pPluginNode->GetValue(data);
        if ( dm_stat != SYNCML_DM_SUCCESS )
            return FALSE;
        if ( data.GetType() == SYNCML_DM_DATAFORMAT_STRING )
        {
            dm_stat = pPluginNode->SetValue(DmtData((CPCHAR)""));
            if (dm_stat != SYNCML_DM_SUCCESS )
                return FALSE;
        } 
        else 
            return FALSE;
    } 
    else
        if( pNode->getData() && pNode->getData()->getSize() )
        { 
            if (DmStrncmp((CPCHAR)pNode->getData()->getBuffer(), szNodeName, pNode->getData()->getSize())==0)
                pNode->getData()->clear();
        }
    return TRUE;
}


BOOLEAN 
DMMetaDataManager::ResetNodeValue(DMNode* pNode,
                                                     PDmtNode pPluginNode,
                                                     CPCHAR szNodeName,
                                                     CPCHAR szOrigName)
{
    if (pPluginNode != NULL)
    {
        SYNCML_DM_RET_STATUS_T dm_stat;
        DmtData data;
      
        dm_stat = pPluginNode->GetValue(data);
        if ( dm_stat != SYNCML_DM_SUCCESS )
            return FALSE;

        if ( data.GetType() == SYNCML_DM_DATAFORMAT_STRING )
        {
            // Get Data
            const DMString & strVal = data.GetStringValue();
            if (DmStrncmp( (CPCHAR)strVal.c_str(), szOrigName, DmStrlen(szOrigName)) == 0)
            {
                    dm_stat = pPluginNode->SetValue(DmtData(szNodeName));
                    if (dm_stat != SYNCML_DM_SUCCESS )
                        return FALSE;
            }
            
        } 
        else
            return FALSE;
    } 
    else 
    {
        if( !pNode->getData() || !pNode->getData()->getSize() )
            return TRUE;
        
        if (DmStrncmp((CPCHAR)pNode->getData()->getBuffer(),szOrigName,pNode->getData()->getSize()) ==0)
        {
            if ( pNode->getData()->assign(szNodeName) == NULL )
                return FALSE;
        }
    }
    return TRUE;
}


BOOLEAN 
DMMetaDataManager::VerifyOneURIDependency(DMNode* pNode, 
                                                                 char * sAbsoluteURI,
                                                                 CPCHAR szNodeName,
                                                                 CPCHAR szOneURIDep,
                                                                 CPCHAR szOrigName,
                                                                 SYNCML_DM_MDF_CBACK callBack,
                                                                 BOOLEAN bIsMultiNode)
{
    BOOLEAN ret_status = TRUE;
    DMNode *psTempNode = NULL;
    DMMetaDataNode oNodeMDF;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    psTempNode = pNode->pcFirstChild;
    if( !psTempNode )
        return TRUE;

    DMURI oOneURI(FALSE,szOneURIDep);
    CPCHAR szSegment = oOneURI.nextSegment();
 
    DmStrcat(sAbsoluteURI,"/");
    INT32 nLen = DmStrlen(sAbsoluteURI);
    while(psTempNode != NULL)
    {
        sAbsoluteURI[nLen] = 0;
        DmStrcat(sAbsoluteURI,psTempNode->abNodeName);
            
        dm_stat = GetNode(sAbsoluteURI, FALSE, oNodeMDF);
        if ( dm_stat == SYNCML_DM_SUCCESS )
        {
          if ( bIsMultiNode || psTempNode->abNodeName == szSegment )
          {
              if(oOneURI.getTailSegments() == NULL)
                   ret_status = callBack(psTempNode, NULL,szNodeName, szOrigName);
              else
                 if ( !psTempNode->isPlugin() ) 
                 {
                      ret_status = VerifyOneURIDependency(psTempNode, 
                                                          sAbsoluteURI,
                                                          szNodeName,
                                                          oOneURI.getTailSegments(),
                                                          szOrigName,
                                                          callBack,
                                                          oNodeMDF.IsHasMultiNodes());
                 }
                 else
                 {
                     PDmtAPIPluginTree tree;
                     dm_stat = ((DMPluginRootNode*)psTempNode)->GetTree(tree);
                     if ( dm_stat != SYNCML_DM_SUCCESS )
                         return FALSE;

                     CPCHAR pbPluginURI = ((DMPluginRootNode*)psTempNode)->GetPluginURI(sAbsoluteURI);
                     PDmtNode ptrNode;
                        
                      dm_stat = tree->GetNode(pbPluginURI, ptrNode);
                      if ( dm_stat != SYNCML_DM_SUCCESS )
                            return FALSE;   
                          
                      ret_status = VerifyPluginURIDependency(psTempNode,
                                                          ptrNode,
                                                          sAbsoluteURI,
                                                          szNodeName,
                                                          oOneURI.getTailSegments(),
                                                          szOrigName,
                                                          callBack,
                                                          oNodeMDF.IsHasMultiNodes());
                 }
                                                    
              if(ret_status == FALSE)
                  return FALSE;
          
              if(bIsMultiNode == FALSE)
                  return TRUE;
           }
        }
        
         psTempNode = psTempNode->pcNextSibling;
          
    }
  
   return ret_status;

}


BOOLEAN 
DMMetaDataManager::VerifyDependency(CPCHAR szNodeName,
                                     CPCHAR szDependecies,
                                     CPCHAR szOrigName,
                                     SYNCML_DM_MDF_CBACK callBack)
{
    BOOLEAN ret_status = TRUE;
  
    DMNode* pNode = NULL;
    CPCHAR sSegment = NULL;
    char * sAbsoluteURI = NULL;
    DMToken oDependensies(TRUE,szDependecies,SYNCML_DM_COMMA);


    if ( !oDependensies.getBuffer() )
        return FALSE;

    pNode = m_pTree->GetRootNode();
    
    if ( pNode == NULL )
        return FALSE;
    
    sAbsoluteURI = (char*)DmAllocMem( m_pTree->GetMaxTotalPathLength() + 1 );

    if ( sAbsoluteURI == NULL )
        return FALSE;

    memset(sAbsoluteURI, 0, m_pTree->GetMaxTotalPathLength() + 1 );

    sSegment = oDependensies.nextSegment();
    while ( sSegment )
    {
        if ( sSegment[0] == '.' && sSegment[1] == '/' )
            sSegment += 2; // skip "./" part
        
        DmStrcpy(sAbsoluteURI,".");
        ret_status = VerifyOneURIDependency(pNode, 
                                                  sAbsoluteURI,
                                                  szNodeName, 
                                                  sSegment,
                                                  szOrigName,
                                                  callBack,
                                                  FALSE);

            if( ret_status != TRUE) 
            {
                DmFreeMem(sAbsoluteURI);
                return FALSE;
            }
        sSegment = oDependensies.nextSegment();
    }

    DmFreeMem(sAbsoluteURI);
    return TRUE;
}



SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::SetAutoNodeProperty(CPCHAR szURI, DMAddData & pAdd)
{
    DMMetaDataNode oNode;
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;
    SYNCML_DM_FORMAT_T format;
  
    ret_status = GetNode(szURI, FALSE, oNode);

    if ( ret_status != SYNCML_DM_SUCCESS )
        return ret_status;

    DMConstraints *pConstraints = oNode.GetConstraints();   
    DMString mimeType;

    oNode.GetMimeType(mimeType);                    

    if ( !pConstraints || !pConstraints->IsDefaultSet() )
        format = ((oNode.m_nNodeFormat == SYNCML_DM_FORMAT_NODE) ? 
            SYNCML_DM_FORMAT_NODE : SYNCML_DM_FORMAT_NULL);
    else
        format = SYNCML_DM_FORMAT_INVALID;

    return pAdd.set(szURI,format,NULL,0,mimeType);
}


BOOLEAN DMMetaDataManager::IsLocal(CPCHAR szURI)
{

    DMMetaDataNode oNode;
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;

    ret_status = GetNode(szURI, FALSE, oNode, NULL);
    if ( ret_status == SYNCML_DM_SUCCESS )
    {
        return oNode.IsLocal();
    }    
    return FALSE; 
}


BOOLEAN DMMetaDataManager::IsLeaf(CPCHAR szURI)
{

    DMMetaDataNode oNode;
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;

    ret_status = GetNode(szURI, FALSE, oNode, NULL);
    if ( ret_status == SYNCML_DM_SUCCESS )
    {
        return oNode.IsLeaf();
    }    
    return FALSE; 
}


SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::GetPath(CPCHAR szURI, DMString & szMDF)
{

    DMMetaDataNode oNode;
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;

    ret_status = GetNode(szURI, FALSE, oNode, NULL);
    if ( ret_status == SYNCML_DM_SUCCESS )
    {
        const DMBuffer & oMDF = oNode.GetPath();
        oMDF.copyTo(szMDF);
    }    
    return ret_status; 
}

BOOLEAN
DMMetaDataManager::VerifyAccessType(CPCHAR szURI, 
                                                       SYNCML_DM_ACCESS_TYPE_T accessType, 
                                                       DMMetaPCharVector* pChildDependNodes /*=null*/) 
{   
    DMMetaDataNode oNode;
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;

    ret_status = GetNode(szURI, FALSE, oNode, pChildDependNodes);
    if( ret_status != SYNCML_DM_SUCCESS)
    {
        // If meta information not available, the default access is Get and Exec
        XPL_LOG_DM_TMN_Debug(("Meta Information isn't found %s\n",szURI)); 
        oNode.SetAccessType(SYNCML_DM_GET_ACCESS_TYPE | SYNCML_DM_EXEC_ACCESS_TYPE);
    }    

    XPL_LOG_DM_TMN_Debug(("DMMetaDataManager::VerifyAccessType: call oNode.VerifyAccessType() %s\n",szURI));
    BOOLEAN access = oNode.VerifyAccessType(accessType);
    return access;
}


BOOLEAN
DMMetaDataManager::VerifyChildrenMultiNodesCount(CPCHAR szURI,
                                                                       UINT16 count, 
                                                                       BOOLEAN& bOPiDataParent) 
{   
    DMMetaDataNode oNode;
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;

    ret_status = GetNode(szURI, TRUE, oNode);
    if( ret_status != SYNCML_DM_SUCCESS)
      return FALSE;

    return oNode.VerifyChildrenMultiNodesCount(count, bOPiDataParent);
}

BOOLEAN 
DMMetaDataManager::VerifyOPINode(CPCHAR szURI, 
                                                        CPCHAR& szID,
                                                        SYNCML_DM_ACCESS_TYPE_T&  wAccessType,
                                                        SYNCML_DM_FORMAT_T& nNodeFormat) 
{
    DMMetaDataNode oNode;
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;

    szID = NULL;
    ret_status = GetNode(szURI, FALSE, oNode);
    if( ret_status != SYNCML_DM_SUCCESS)
      return FALSE;

    return oNode.VerifyOPINode(szID, wAccessType, nNodeFormat);
}

BOOLEAN DMMetaDataManager::IsOPiDataParent( CPCHAR szURI )
{
    DMMetaDataNode oNode;
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;

    ret_status = GetNode(szURI, TRUE, oNode);
    if( ret_status != SYNCML_DM_SUCCESS)
      return FALSE;

    return oNode.IsOPiDataParent();
}

#ifdef LOB_SUPPORT
BOOLEAN DMMetaDataManager::IsESN( CPCHAR szURI )
{
    DMMetaDataNode oNode;
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;

    ret_status = GetNode(szURI, TRUE, oNode);
    if( ret_status != SYNCML_DM_SUCCESS)
        return FALSE;

    return oNode.IsESN();
}
#endif

UINT16 DMMetaDataManager::UpdateChildrenList( CPCHAR szURI, 
                                                  DMString& strChildrenList )
{
  UINT16 nChildrenCount = 0;
  DMMetaDataNode oNode, oChildrenNode;
  DMMetaDataBuffer oBuffer;
  
  if ( SYNCML_DM_SUCCESS != GetNode(szURI, TRUE, oNode) || 
    !m_oLastNodeLocator.m_pBuffer ||
    oNode.IsHasMultiNodes() )
    return 0;

  oBuffer.SetBuffer( m_oLastNodeLocator.m_pBuffer  );
  
  for (int i=0; i<oNode.m_nNumChildren; i++)
  {
      oNode.SetChildrenOffset(&oBuffer,i);
      oChildrenNode.Read(oBuffer, FALSE);
      
      if ( oChildrenNode.IsPluginNode() ) {
        if ( !strChildrenList.empty() )
          strChildrenList += "/";

        strChildrenList += oChildrenNode.m_psName;
      }
  }

  return nChildrenCount;
}


SYNCML_DM_RET_STATUS_T
DMMetaDataManager::RemoveRecursiveSegments(char* sTailSegment, 
                                            DMMetaDataNode & oNode)
{

    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_NOT_FOUND;
    DMConstraints * pConstraints = oNode.GetConstraints();
   

    if ( pConstraints == NULL ||
         sTailSegment == NULL )
        return ret_status;

    INT32 lenMatchSeg = DmStrlen(pConstraints->m_psRecurAfterSegment)+DmStrlen(oNode.m_psName)+1;
    INT32 lenTailSeg = DmStrlen(sTailSegment);
  
    if ( lenMatchSeg > lenTailSeg )
        return ret_status;

    DMString sMatchSegment;

    sMatchSegment = pConstraints->m_psRecurAfterSegment;
    sMatchSegment += "/";
    sMatchSegment += oNode.m_psName;
    
    DMParser oMatchSegmentParser(sMatchSegment);
 
    CPCHAR sSegment1 = NULL;
    CPCHAR sSegment2 = NULL; 
    CPCHAR sRestTail = NULL;
    for (int i=0; i<pConstraints->m_nMaxRecurrance; i++)
    {   
        DMURI oTailSegmentParser(TRUE,sTailSegment);
            
        oMatchSegmentParser.reset(); 
        for (UINT32 j=0; j<oMatchSegmentParser.getSegmentsCount()-1; j++)
        {
            sSegment1 = oMatchSegmentParser.nextSegment();
            sSegment2 = oTailSegmentParser.nextSegment();
            if ( sSegment2 == NULL )
            {
                return SYNCML_DM_SUCCESS;
            }
            if ( DmStrcmp(sSegment1,"*") != 0 &&
                 DmStrcmp(sSegment1,sSegment2) != 0 )
            {
                return SYNCML_DM_SUCCESS;
            } 
        }

        sSegment1 = oMatchSegmentParser.nextSegment();
        sSegment2 = oTailSegmentParser.nextSegment();
        if ( sSegment2 == NULL )
        {
            return SYNCML_DM_SUCCESS;
        }
        if ( !oNode.IsMultiNode() )
        {
            if ( DmStrcmp(sSegment1,"*") != 0 &&
                 DmStrcmp(sSegment1,sSegment2) != 0 )
            {
                 return SYNCML_DM_SUCCESS;
            } 
        }
            

        sRestTail = oTailSegmentParser.getTailSegments();
        if ( sRestTail != NULL )
            memmove(sTailSegment, sRestTail, DmStrlen(sRestTail)+1);
        else
        {
            sTailSegment[0] = SYNCML_DM_NULL;
            break;
        }
         
    }    

    return SYNCML_DM_SUCCESS;

    
}


SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::Load()
{
    SYNCML_DM_RET_STATUS_T ret_status;    

    if( !m_pEnv ) return SYNCML_DM_FAIL;

#ifndef DM_STATIC_FILES
    DMString sRootMDF;
    DMString sFullPath;

    m_pEnv->GetMainRFSFullPath(DM_ROOT_MDF_FILENAME,sRootMDF);
    
    ret_status = LoadBuffer(sRootMDF);

    if ( ret_status != SYNCML_DM_SUCCESS )  
        return ret_status;

    m_bIsLoad = TRUE;

    // load all extra mdf files...
    ret_status = LoadDir(m_pEnv->GetWFSFullPath(NULL,sFullPath), false);

    if ( ret_status != SYNCML_DM_SUCCESS )
        return SYNCML_DM_SUCCESS;

    for (INT32 nFS = 0; nFS < m_pEnv->GetRFSCount(); nFS++)
    {
        LoadDir( m_pEnv->GetRFSFullPath(nFS,NULL,sFullPath), nFS == 0); // ignore root.mdf only on first RO fs, since it's already loaded
    } 
#else
    UINT8 count = XPL_DM_GetMDFCount();
  
    for (UINT8 index=0; index<count; index++)
    {    
        ret_status = LoadBuffer(index);
        if ( ret_status != SYNCML_DM_SUCCESS )
            return ret_status;
    }    
    m_bIsLoad = TRUE;
#endif

    return SYNCML_DM_SUCCESS;
}


#ifndef DM_STATIC_FILES  
SYNCML_DM_RET_STATUS_T 
DMMetaDataManager::LoadDir(CPCHAR szDirectory, BOOLEAN bIgnoreRoot)
{
    
    XPL_FS_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;    
    XPL_FS_SHANDLE_T search_handle = XPL_FS_SHANDLE_INVALID;
    char file_name[XPL_FS_MAX_FILE_NAME_LENGTH];     

    search_handle = XPL_FS_StartSearch(szDirectory, "bmdf", TRUE, &ret_status);
    if ( ret_status != XPL_FS_RET_SUCCESS)    
      return SYNCML_DM_IO_FAILURE;
   
    // load all files with .mdf extension
    while ( XPL_FS_GetSearchResult(search_handle,file_name) != XPL_FS_RET_NOT_FOUND ) 
    {
        if (( bIgnoreRoot || DmStrcmp(file_name, DM_ROOT_MDF_FILENAME) != 0) )
        {
            ret_status = LoadBuffer(file_name);
            if ( ret_status != SYNCML_DM_SUCCESS)    
                break;
        }
    }    

    XPL_FS_EndSearch(search_handle);
    return ret_status;
}
#endif


BOOLEAN 
DMMetaDataManager::VerifyPluginURIDependency(DMNode* pNode, 
                                                                   PDmtNode pPluginNode,
                                                                   char * sAbsoluteURI,
                                                                   CPCHAR szNodeName,
                                                                   CPCHAR szOneURIDep,
                                                                   CPCHAR szOrigName,
                                                                   SYNCML_DM_MDF_CBACK callBack,
                                                                   BOOLEAN bIsMultiNode)
{
    BOOLEAN ret_status = TRUE;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS; 

    DmStrcat(sAbsoluteURI, "/");
    DMVector<PDmtNode> vecNodes;

    dm_stat = pPluginNode->GetChildNodes(vecNodes);
    if ( dm_stat == SYNCML_DM_SUCCESS )
    {
        DMMetaDataNode oNodeMDF;
        DMURI oOneURI(FALSE, szOneURIDep);
        CPCHAR szSegment = oOneURI.nextSegment();

        DMString nodeName;
        INT32 nLen = DmStrlen(sAbsoluteURI);
        
    
        for (INT32 i=0; i < vecNodes.size(); i++) 
        {
            sAbsoluteURI[nLen] = 0;
            dm_stat = vecNodes[i]->GetNodeName(nodeName);
            if ( dm_stat != SYNCML_DM_SUCCESS )
                return FALSE;
            
            DmStrcat(sAbsoluteURI, nodeName.c_str());

            dm_stat = GetNode(sAbsoluteURI, FALSE, oNodeMDF);
            if ( dm_stat == SYNCML_DM_SUCCESS)
            {
                if ( bIsMultiNode || nodeName == szSegment) 
                {
                    if (oOneURI.getTailSegments() == NULL) 
                    {
                        ret_status = callBack(pNode, vecNodes[i], szNodeName, szOrigName);
                    }
                    else 
                    {
                        ret_status = VerifyPluginURIDependency(pNode,
                                                                              vecNodes[i],
                                                                              sAbsoluteURI,
                                                                              szNodeName,
                                                                              oOneURI.getTailSegments(),
                                                                              szOrigName,
                                                                              callBack,
                                                                             oNodeMDF.IsHasMultiNodes());
                    }

                    if (ret_status == FALSE)
                        return FALSE;
                    if (bIsMultiNode == FALSE)
                        return TRUE;
                }
            }
        }
    }
    return ret_status;
}
