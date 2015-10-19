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

    Source Name: dmConfigManager.cc

    General Description: Implementation of the DMConfigManager class

==================================================================================================*/

#include "dm_tree_class.H"
#include "dmConfigManager.h"
#include "dmLockingHelper.h"
#include "dm_uri_utils.h"
#include "xpl_Logger.h"

DMConfigManager::DMConfigManager()
  : m_pEnv( NULL ),
    m_pTree( NULL )
{
  m_bChanged = FALSE;
  m_bLoaded = FALSE;
  m_nLastLoadTimeStamp = 0;
}

DMConfigManager::~DMConfigManager()
{
}




SYNCML_DM_RET_STATUS_T DMConfigManager::Init( CEnv* env, 
                                                                          DMTree* tree,
                                                                          SYNCML_DM_FILE_TYPE_T fileType)
{
  if( !env || !tree ) return SYNCML_DM_FAIL;
  
  m_pEnv = env;
  m_pTree = tree;
  m_efileType = fileType;

  GetFileName(m_strFileName);
  if ( !XPL_FS_Exist(m_strFileName) )
  {   
        DMFileHandler tf(m_strFileName);    
        tf.open(XPL_FS_FILE_WRITE);
        tf.close();
  }
  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DMConfigManager::DeInit()
{
  m_bChanged = FALSE;
  m_bLoaded = FALSE;
  m_nLastLoadTimeStamp = 0;
  m_strFileName = NULL;
  m_pEnv = NULL;
  m_pTree = NULL;
  ClearMemory(); 
  return SYNCML_DM_SUCCESS;
}



SYNCML_DM_RET_STATUS_T 
DMConfigManager::DeserializeDictionary(DMFileHandler& dmf,
                                                     DMMap<INT32, DMString>& aDict)
{

    DMString strLineBuffer;
    char *line = strLineBuffer.AllocateBuffer(DM_MAX_CONFIG_LINE);
  
    if ( !line ) 
    {
        XPL_LOG_DM_TMN_Error(("Device memory is full"));
        return SYNCML_DM_DEVICE_FULL;
    }

    while ( GetNextLine(dmf, line) && line[0] ) 
    {
        DMString strID, strSrv = line;
  
        DmStringParserGetItem( strID, strSrv, ':' );
        aDict.put( DmAtoi(strID), strSrv );
    }

    return SYNCML_DM_SUCCESS;

}


SYNCML_DM_RET_STATUS_T DMConfigManager::Deserialize()
{
    // check if we have latest copy in memory already
    XPL_CLK_CLOCK_T lastLoadTimeStamp = XPL_FS_GetModTime(m_strFileName.c_str());
    if ( m_bLoaded && lastLoadTimeStamp == m_nLastLoadTimeStamp )
        return SYNCML_DM_SUCCESS;    

    DMGlobalLockHelper oGlobalLock;  // protect serialize/recovery from multi process access

    CheckRecovery();
  
    ClearMemory();

    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    DMFileHandler evtFile(m_strFileName);

    if ( evtFile.open(XPL_FS_FILE_READ) != SYNCML_DM_SUCCESS ) 
    {
        XPL_LOG_DM_TMN_Error(("Failed to open config file, %s\n", m_strFileName.c_str()));
        return SYNCML_DM_IO_FAILURE;
    }

    if ( !evtFile.size() )
        return SYNCML_DM_SUCCESS;

    DMMap<INT32, DMString> aDict;
    dm_stat = DeserializeDictionary(evtFile, aDict);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
 
    DMString strLineBuffer;
    char *line = strLineBuffer.AllocateBuffer(DM_MAX_CONFIG_LINE);
    UINT16   lineNo = 0;

    if ( !line ) 
    {
        XPL_LOG_DM_TMN_Error(("Device memory is full"));
        return SYNCML_DM_DEVICE_FULL;
    }
  
    while ( GetNextLine(evtFile, line ) ) 
    {
        lineNo++;
    
        DMString strPath;
        if ( !GetPath( line, strPath ) )
            continue;

        // get the value
        if (!GetNextLine(evtFile, line )) 
        {
            XPL_LOG_DM_TMN_Error(("config file format error at line %d - unexpected end of file\n \n", lineNo ));
            dm_stat = SYNCML_DM_IO_FAILURE;
            break; // end of file
        }

        dm_stat = Add(strPath, line,aDict);
        if ( dm_stat != SYNCML_DM_SUCCESS )
            break;
    }

    evtFile.close();
    m_bChanged = FALSE;
    m_nLastLoadTimeStamp = lastLoadTimeStamp;
    m_bLoaded = TRUE;
    return SYNCML_DM_SUCCESS;  
}


BOOLEAN
DMConfigManager::GetPath( char* line, 
                                        DMString& strPath )
{
  char* startP = DmStrchr( line, '[' );
  if (!startP) 
    return FALSE;

  char* endP = DmStrchr(line, ']' );
  
  if ( !endP || endP < startP ) {
    XPL_LOG_DM_TMN_Error(("config file format error \n %s\n", line));
    return FALSE;
  }

  // remove [], add '.' if necessary to create a full uri
  *endP = 0;
  startP++;
  
  if ( *startP != '.' )
    strPath = ".";

  strPath += startP;
  return TRUE;
}


void DMConfigManager::CheckRecovery() const
{
    DMString strTempFile = m_strFileName;  
    DMString strBakFile = m_strFileName;  
    UINT8 state = 0;

    strTempFile += DMFileHandler::TEMP_FILE_EXTENSION;
    strBakFile += DMFileHandler::BAK_FILE_EXTENSION;

#define TEMP_EXISTS    1    // .temp file exists state bit
#define BAK_EXISTS     2    // .bak file exists state bit
    
    if (XPL_FS_Exist(strTempFile.c_str()))
        state |= TEMP_EXISTS;
    
    if (XPL_FS_Exist(strBakFile.c_str()))
        state |= BAK_EXISTS;
        
    switch(state) {
        case (TEMP_EXISTS | BAK_EXISTS): // case C
            if (XPL_FS_Rename(strBakFile.c_str(), m_strFileName.c_str()) == XPL_FS_RET_FAIL)
                break;
            //continue to delete .temp files
        
        case TEMP_EXISTS: // case B
            XPL_FS_Remove(strTempFile.c_str());
            break;
        
        case BAK_EXISTS: // case D
            XPL_FS_Remove(strBakFile.c_str());
            break;
        
        default: // no action needed
            break;
    }
}

BOOLEAN 
DMConfigManager::GetNextLine(DMFileHandler& dmf,
                                             char *line) 
                                 
{
 
  line[0]= 0;
  
  while (!dmf.iseof()) {
    dmf.fgets(line, DM_MAX_CONFIG_LINE);
    
    if(*line != '#') 
      return TRUE;
  }

  return FALSE;
}


SYNCML_DM_RET_STATUS_T DMConfigManager::Serialize()
{
    if ( !m_bChanged )
        return SYNCML_DM_SUCCESS;

    DMGlobalLockHelper oGlobalLock;  // protect serialize/recovery from multi process access

    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS; 
  
    DMString strTmpName = m_strFileName; 
    DMString strBakName = m_strFileName; 

    strTmpName += DMFileHandler::TEMP_FILE_EXTENSION;
    strBakName += DMFileHandler::BAK_FILE_EXTENSION;
    
    DMFileHandler tf(strTmpName.c_str());    

    if (tf.open(XPL_FS_FILE_WRITE) != SYNCML_DM_SUCCESS) {
        XPL_LOG_DM_TMN_Error(("Error opening %s\n", strTmpName.c_str()));
        return SYNCML_DM_IO_FAILURE;
    }

    if ( !m_aConfig.size() )
    { 
         if (tf.close() != SYNCML_DM_SUCCESS) 
            return SYNCML_DM_IO_FAILURE;
         XPL_FS_Rename(m_strFileName.c_str(), strBakName.c_str()); 
         XPL_FS_Rename(strTmpName.c_str(), m_strFileName.c_str());
         XPL_FS_Remove(strBakName.c_str());
         m_nLastLoadTimeStamp = XPL_FS_GetModTime(m_strFileName.c_str());
         m_bChanged = FALSE;
         m_bLoaded = TRUE;
         return  SYNCML_DM_SUCCESS;
    }

    DMMap<DMString, INT32> aDict;
    UpdateDictionary(aDict);

    dm_stat = SerializeDictionary(tf,aDict);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
  
    // empty line between dictionary and data
    if ( tf.write( "\n", 1 ) != SYNCML_DM_SUCCESS )
        return SYNCML_DM_IO_FAILURE;

    for ( INT32 index=0; index<m_aConfig.size(); index++)
    {

#ifdef TEST_DM_RECOVERY
         if ((power_fail_point != NULL) && (DmStrcmp(power_fail_point, "PF1") == 0))
          {
                 printf("Type Ctrl-C to simulate Power Fail ...\n");
                 sleep(30);
          }
#endif
         PDMConfigItem & oConfigItem = m_aConfig[index];
         if ( oConfigItem->Serialize( tf, aDict )  != SYNCML_DM_SUCCESS )
             return SYNCML_DM_IO_FAILURE;
    }
    
    if (tf.close() != SYNCML_DM_SUCCESS) 
        return SYNCML_DM_IO_FAILURE;

#ifdef TEST_DM_RECOVERY
    if ((power_fail_point != NULL) && (DmStrcmp(power_fail_point, "PF2") == 0)) 
    {
        printf("Type Ctrl-C to simulate Power Fail ...\n");
        sleep(30);
    }
#endif

    // rename to .bak file
    XPL_FS_Rename(m_strFileName.c_str(), strBakName.c_str()); 
#ifdef TEST_DM_RECOVERY
    if ((power_fail_point != NULL) && (DmStrcmp(power_fail_point, "PF3") == 0)) 
    {
         printf("Type Ctrl-C to simulate Power Fail ...\n");
         sleep(30);
    }
#endif

    // rename .temp file to original name
    XPL_FS_Rename(strTmpName.c_str(), m_strFileName.c_str());
#ifdef TEST_DM_RECOVERY
    if ((power_fail_point != NULL) && (DmStrcmp(power_fail_point, "PF4") == 0)) {
         printf("Type Ctrl-C to simulate Power Fail ...\n");
         sleep(30);
    }
#endif

    // delete .bak file
    XPL_FS_Remove(strBakName.c_str());
  
    m_nLastLoadTimeStamp = XPL_FS_GetModTime(m_strFileName.c_str());
    m_bChanged = FALSE;
    m_bLoaded = TRUE;

    return SYNCML_DM_SUCCESS ;

}


SYNCML_DM_RET_STATUS_T 
DMConfigManager::SerializeDictionary( DMFileHandler& dmf,
                                                        const DMMap<DMString, INT32>& aDict)
{

     char szNumBuffer[20] = ""; // big enough to hold a number
      for ( DMMap<DMString, INT32>::POS pos = aDict.begin(); pos < aDict.end(); pos++ )
      {
            INT32 nLen = sprintf( szNumBuffer, "%d:", aDict.get_value(pos) );
            if ( dmf.write( szNumBuffer, nLen ) != SYNCML_DM_SUCCESS ||
                  dmf.write( aDict.get_key(pos).c_str(), aDict.get_key(pos).length())  != SYNCML_DM_SUCCESS ||
                  dmf.write( "\n", 1 )  != SYNCML_DM_SUCCESS )
             return SYNCML_DM_IO_FAILURE;
      }
  
      return SYNCML_DM_SUCCESS;

}


SYNCML_DM_RET_STATUS_T DMConfigManager::Revert()
{
  if ( !m_bLoaded || !m_bChanged )
    return SYNCML_DM_SUCCESS;

  m_bChanged = false;
  m_bLoaded = false;
  return Deserialize();
}


void DMConfigManager::CheckLocking()
{
    if ( !m_bChanged ) 
    {
        m_pTree->GetLockContextManager().InsureFileLocked(m_efileType);
        Deserialize();
    }
}


INT32 DMConfigManager::Find(CPCHAR szPath) const 
{
    for (INT32 index=0; index<m_aConfig.size(); index ++)
    {
        const PDMConfigItem & pItem = m_aConfig[index]; 
        if ( pItem->GetPath() == szPath )
        {
            return index;
        }
    }
    return -1;

}    


INT32 DMConfigManager::Find(CPCHAR szPath, PDMConfigItem & oConfigItem) const
{
    INT32 index = Find(szPath);
    if ( index != -1 )
        oConfigItem = m_aConfig[index];
    return index;

}    


void DMConfigManager::ClearMemory()
{
     m_aConfig.clear();
}     


SYNCML_DM_RET_STATUS_T 
DMConfigManager::Add(const DMString & szPath,
                               CPCHAR szConfig,
                               const DMMap<INT32, DMString>& aDict)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

     if ( Find(szPath) != -1 )
        return SYNCML_DM_FAIL;

    DMConfigItem * pConfigItem = AllocateConfigItem();

    if ( !pConfigItem ) 
        return SYNCML_DM_DEVICE_FULL;

    dm_stat = pConfigItem->Set( szPath, szConfig, aDict );
    if ( dm_stat != SYNCML_DM_SUCCESS )
    {
        delete pConfigItem;
        return dm_stat;
    }    
    
    m_aConfig.push_back(PDMConfigItem(pConfigItem));
    return SYNCML_DM_SUCCESS;

}


void DMConfigManager::UpdateDictionary(DMMap<DMString, INT32>& aDict)
{

    for (INT32 index=0; index<m_aConfig.size(); index++)
    {
        PDMConfigItem & pConfigItem = m_aConfig[index];
        pConfigItem->UpdateDictionary( aDict );
    }
}


SYNCML_DM_RET_STATUS_T
DMConfigManager::Delete(CPCHAR szPath)
{
    INT32 index = Find(szPath);
    if ( index == -1 )
        return SYNCML_DM_NOT_FOUND;

    CheckLocking();
    m_aConfig.remove(index);
    m_bChanged = true;

    return SYNCML_DM_SUCCESS;

}


SYNCML_DM_RET_STATUS_T 
DMConfigManager::Get(CPCHAR szPath, 
                                  PDMConfigItem & pItem) const
{

    if ( Find(szPath, pItem) == -1 ) 
    {
        DMString szMDF;
        SYNCML_DM_RET_STATUS_T dm_stat;
        DMMetaDataManager & m_oMDFObj =  m_pTree->GetMetaDataManager();
        dm_stat =  m_oMDFObj.GetPath(szPath, szMDF);

        if ( dm_stat != SYNCML_DM_SUCCESS )
            return dm_stat;

        if ( szMDF != szPath )
        {
            if ( Find(szMDF, pItem) == -1 ) 
                return SYNCML_DM_NOT_FOUND;
        }    
        else
              return SYNCML_DM_NOT_FOUND;
  }  
 
  return SYNCML_DM_SUCCESS;
}
