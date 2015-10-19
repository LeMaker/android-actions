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

    Source Name: dmPluginManager.cc

    General Description: Implementation of the DMPluginManager class

==================================================================================================*/

#include "dmt.hpp"
#include "xpl_Logger.h"
#include "xpl_dm_Manager.h"
#include "dmBufferReader.h"
#include "dm_uri_utils.h"
#include "dm_tree_class.H" 
#include "SyncML_DM_FileHandle.H"
#include "dm_tree_plugin_root_node_class.H"
#include "dmPluginManager.h"
#include "dm_tree_default_interior_node_class.H"

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

const SYNCML_DM_PLUGIN_TYPE_T DMPluginManager::m_nIndex[] =
                                                 { SYNCML_DM_DATA_PLUGIN,
                                                    SYNCML_DM_EXECUTE_PLUGIN,
                                                    SYNCML_DM_CONSTRAINT_PLUGIN,
                                                    SYNCML_DM_COMMIT_PLUGIN
                                                 };  

DMPluginManager::DMPluginManager() 
  : m_pTree( NULL )
{
}

DMPluginManager::~DMPluginManager()
{
    for( DMMap<DMString,XPL_DL_HANDLE_T>::POS pos = m_pluginLibs.begin(); 
          pos != m_pluginLibs.end(); pos++ )
    {
        XPL_DL_Unload(m_pluginLibs.get_value(pos));
    }  
}


XPL_DL_HANDLE_T 
DMPluginManager::LoadLib(CPCHAR libName)
{
    XPL_DL_HANDLE_T handle;
    if ( m_pluginLibs.lookup(libName, handle) == TRUE )
        return handle;

    handle = XPL_DL_Load(libName);
        m_pluginLibs.put(DMString(libName), handle);

   return handle;
}
  

#ifndef DM_STATIC_PLUGINS 
SYNCML_DM_RET_STATUS_T 
DMPluginManager::LoadDirectory(CPCHAR szPath)
{
   SYNCML_DM_RET_STATUS_T status=SYNCML_DM_SUCCESS;

   XPL_FS_SHANDLE_T search_handle;
   XPL_FS_RET_STATUS_T fs_result;
   char file_name[XPL_FS_MAX_FILE_NAME_LENGTH]; 

   XPL_LOG_DM_PLG_Debug(("Loading %s\n", szPath));
   search_handle = XPL_FS_StartSearch(szPath, "ini", TRUE, &fs_result);
   if ( search_handle == XPL_FS_SHANDLE_INVALID)
      return SYNCML_DM_IO_FAILURE;
  
   //Loop through all .ini files in the directory
   while ( XPL_FS_GetSearchResult(search_handle,file_name) != XPL_FS_RET_NOT_FOUND ) 
   {
      status=LoadPluginFile(file_name);
   }
   XPL_FS_EndSearch(search_handle);
   return status;
}
#endif


SYNCML_DM_PLUGIN_TYPE_T 
DMPluginManager::CheckPlugin(DMStringMap & aMap, 
                                            CPCHAR filePath, 
                                            CPCHAR pluginName, 
                                            SYNCML_DM_PLUGIN_TYPE_T type)
{
   SYNCML_DM_PLUGIN_TYPE_T mask = 0;

   DMString strName;
   
   if (aMap.lookup(pluginName, strName))
   {
#ifndef DM_STATIC_PLUGINS   
    /*  CPCHAR szName=strName.c_str();
      DMString newPath;
      if ( *szName != '/')   //only relative path
      {
         newPath=filePath;
         newPath +="/";
         newPath +=strName;
         aMap.put(pluginName, newPath);
         szName=newPath.c_str();
      } */
      mask = mask | type; 
#else 
      mask = mask | type;
#endif 
   }
   return mask;

}


SYNCML_DM_RET_STATUS_T 
DMPluginManager::UpdatePluginNodes(CPCHAR szRootPath)
{
   SYNCML_DM_RET_STATUS_T res=SYNCML_DM_SUCCESS;
   XPL_LOG_DM_PLG_Debug(("UpdatePluginNodes subTreeRootPath=%s\n", szRootPath));

   INT32 size = m_dataPlugins.size();

   DMAddNodeProp props;
   for ( int i=0; i<size; i++ )
   {
        CPCHAR pluginPath = m_dataPlugins[i]->GetPath();
      
        SyncML_DM_Archive* archive =m_pTree->GetArchiver().getArchiveByURI(pluginPath);
        if ( DmStrcmp(szRootPath,archive->getURI()) == 0 )
        {
            res=MountNode(m_dataPlugins[i],props);
            if ( res != SYNCML_DM_SUCCESS )
                return res;
        }    
   }   
   return res;
}


BOOLEAN 
DMPluginManager::IsMountPointEnabled(CPCHAR szPath) const
{
      INT32 size =  m_oBlockedPlugins.size();
      for (INT32 index = 0; index < size; index++)
      {
            const DMString & str = m_oBlockedPlugins[index];
            if ( DmIsParentURI( str.c_str(), szPath ) )
               return FALSE;
      }  
      return TRUE;
}


SYNCML_DM_RET_STATUS_T 
DMPluginManager::AddPlugin(CPCHAR filename, 
                                            DMString &  strPath, 
                                            DMStringMap & aMap)
{
   SYNCML_DM_RET_STATUS_T status=SYNCML_DM_SUCCESS;
   PDMPlugin pPlugin;

   BOOLEAN bFeatureEnabled = TRUE;
   bFeatureEnabled = this->IsMountPointEnabled(strPath);
   if ( bFeatureEnabled == TRUE )
        bFeatureEnabled = DmIsEnabledURI(strPath);

   if ( bFeatureEnabled == FALSE )
   {
      m_oBlockedPlugins.push_back(strPath);
      return SYNCML_DM_SUCCESS;
   }

#ifndef DM_STATIC_PLUGINS  
   CPCHAR ptr = NULL;
   INT32 len=0;

   if( !m_pTree ) return SYNCML_DM_FAIL;

   ptr=DmStrrchr(filename, '/');
   if (ptr != NULL)
      len=(INT32)(ptr-filename);
   DMString strFilePath(filename, len);
#else
   CPCHAR strFilePath = NULL;
#endif
   

   SYNCML_DM_PLUGIN_TYPE_T type=0;
   BOOLEAN bOverlayPlugin = FALSE;
   
   type = CheckPlugin(aMap, strFilePath, PLUGIN_DATA_NAME, SYNCML_DM_DATA_PLUGIN);

   if ( CheckPlugin(aMap, strFilePath, PLUGIN_DATA_OVERLAY_NAME, SYNCML_DM_DATA_PLUGIN) ){
     // check if it's already a data
     if ( (type & SYNCML_DM_DATA_PLUGIN) != 0 ){
       XPL_LOG_DM_PLG_Error(("Plugin can be either data or overlay but not both"));
       return SYNCML_DM_INVALID_PARAMETER;
     }
     
     type = type | SYNCML_DM_DATA_PLUGIN;
     bOverlayPlugin = TRUE;
   }
   
   type = type | CheckPlugin(aMap, strFilePath, PLUGIN_EXECUTE_NAME, SYNCML_DM_EXECUTE_PLUGIN);
   type = type | CheckPlugin(aMap, strFilePath, PLUGIN_CONSTRAINT_NAME, SYNCML_DM_CONSTRAINT_PLUGIN);
   type = type | CheckPlugin(aMap, strFilePath, PLUGIN_COMMIT_NAME, SYNCML_DM_COMMIT_PLUGIN);

   if (type == 0)
      return status;
      
   /*
   DMString strKeyTmp = "_exec";
   DMString strValueTmp;
   if (aMap.lookup(strKeyTmp, strValueTmp))
   {
       XPL_LOG_DM_PLG_Debug(("new plugin %s=%s\n", strKeyTmp.c_str(), strValueTmp.c_str()));
   }
   strKeyTmp = "_data";
   if (aMap.lookup(strKeyTmp, strValueTmp))
   {
       XPL_LOG_DM_PLG_Debug(("new plugin %s=%s\n", strKeyTmp.c_str(), strValueTmp.c_str()));
   }
   */
   pPlugin=new DMPlugin(type, bOverlayPlugin, strPath, aMap);

   if ( pPlugin == NULL )
    return SYNCML_DM_DEVICE_FULL;

#ifndef DM_STATIC_PLUGINS
   //Add plugin into file collections
   DMString strFilename(filename);
   DMPluginVector vector;
   if ( m_mapFilePluginVectors.lookup(strFilename, vector))
   {
      vector.push_back(pPlugin);      
   } else
   {
      vector.push_back(pPlugin);
      m_mapFilePluginVectors.put(strFilename, vector);
   }
#endif   


   XPL_LOG_DM_PLG_Debug(("Add plug-in %s\n",strPath.c_str())); 
   //Add plugin into all typed collections
   for (int i=0; i<MAX_PLUGINTYPES;i++)
   {
      if ((type & (1<<i)) != 0 )
      {
         DMPluginVector * pPlugins = GetPlugins(m_nIndex[i]);
         pPlugins->push_back(pPlugin);
      }   
   }

#ifndef DM_STATIC_PLUGINS
//   if ((type & SYNCML_DM_DATA_PLUGIN) != 0)
//   {
//       if (m_pTree->GetRootNode() )
//       {
//          DMAddNodeProp props;
//         MountNode(pPlugin,props);
//       }   
//   }
   
   if ((type & SYNCML_DM_COMMIT_PLUGIN) != 0)
   {
       DMString strEvent;
       if (!aMap.lookup("_event", strEvent))
           strEvent = "Add&Replace&Delete&Indirect&Detail";
       pPlugin->Set(strEvent); 
   }
#endif   
   return status;
}


/*
#ifndef DM_STATIC_PLUGINS  
SYNCML_DM_RET_STATUS_T 
DMPluginManager::removePlugin(PDMPlugin & plugin)
{
   SYNCML_DM_RET_STATUS_T status=SYNCML_DM_SUCCESS;

   XPL_LOG_DM_PLG_Debug(("DMPluginManager::removePlugin %s\n", (CPCHAR)plugin->GetPath()));

   for (int i=0; i<MAX_PLUGINTYPES;i++) 
   {
       DMPluginVector  * pPlugins = GetPlugins(m_nIndex[i]);
       INT32 index = pPlugins->find(plugin);
       if ( index != -1 )
          pPlugins->remove(index);
    }    

   SYNCML_DM_PLUGIN_TYPE_T type = plugin->GetPluginType();
   
   if ((type & SYNCML_DM_DATA_PLUGIN) != 0)
      UnmountNode(plugin);
 
   return status;
}
#endif
*/

SYNCML_DM_RET_STATUS_T 
DMPluginManager::LoadPluginFile(CPCHAR filename)
{   
   SYNCML_DM_RET_STATUS_T ret_status=SYNCML_DM_SUCCESS;
   
   XPL_LOG_DM_PLG_Debug(("DMPluginManager::LoadPluginFile %s\n", filename));

   DMFileHandler fileHandler(filename);
   ret_status = fileHandler.open(XPL_FS_FILE_READ);
   if ( ret_status != SYNCML_DM_SUCCESS)
      return ret_status;

   DMString strLineBuffer;
   char *line = strLineBuffer.AllocateBuffer(DM_MAX_CONFIG_LINE);
   DMString strSectionName;
   DMStringMap nvMap;

   if ( !line )
    return SYNCML_DM_DEVICE_FULL;
   
   while(!fileHandler.iseof())
   {
      fileHandler.fgets((char *)line, DM_MAX_CONFIG_LINE);

      //Skip '#' and empty string
      if ( line[0]=='#' || line[0] == '\0' )
        continue;

      ret_status = ParseLine(filename, line, strSectionName, nvMap);
      if ( ret_status != SYNCML_DM_SUCCESS )
        return ret_status;
      
   }

   if ( strSectionName.length() >0 )
   {
      AddPlugin( filename, strSectionName, nvMap );
      nvMap.clear();
   }
   fileHandler.close();

   return ret_status;
}


#ifdef DM_STATIC_FILES
SYNCML_DM_RET_STATUS_T 
DMPluginManager::LoadPluginFile(UINT8 * pBuffer, UINT32 size)
{   
   SYNCML_DM_RET_STATUS_T ret_status=SYNCML_DM_SUCCESS;

   XPL_LOG_DM_PLG_Debug(("DMPluginManager::LoadPluginFile\n"));

   DMBufferReader bufferHandler(pBuffer,size);

   char * line;
   DMString strSectionName;
   DMStringMap nvMap;

   while(!bufferHandler.iseof())
   {
      line = bufferHandler.fgets();
      if ( line == NULL )
        return SYNCML_DM_FAIL;
  
      ret_status = ParseLine(NULL, line, strSectionName, nvMap);
      if ( ret_status != SYNCML_DM_SUCCESS )
        return ret_status;
   }

   if ( strSectionName.length() >0 )
   {
      DMAddNodeProp  props;
      AddPlugin(NULL, strSectionName, nvMap );
      nvMap.clear();
   }
   return ret_status;
}
#endif

SYNCML_DM_RET_STATUS_T 
DMPluginManager::ParseLine(CPCHAR szFileName,
                                          CPCHAR szLine,
                                          DMString &strSectionName,
                                          DMStringMap &aMap)
{
    SYNCML_DM_RET_STATUS_T ret_status;
    INT32 len = 0;

    if ( szLine[0] == '[' )
    {
       // Finish previous section
       if ( strSectionName.length() >0 )
       {
          ret_status = AddPlugin(szFileName, strSectionName, aMap );
          aMap.clear();
          strSectionName="";
          if ( ret_status != SYNCML_DM_SUCCESS )
            return ret_status;
       }
    
       len = DmStrlen(szLine);
       if ( len == 0 || szLine[len-1] !=  ']' || len == 2 )
         return SYNCML_DM_FAIL;
         
       strSectionName.assign(&szLine[1],len-2);
   }
   else  //read a line not starting with [
   {      
      if ( strSectionName.length() == 0 )
        return SYNCML_DM_SUCCESS;
         
      DMString name;
      DMString value;
      
      char * ptr = (char*)DmStrchr(szLine,'=');
   
      if ( ptr != NULL )
      {
        if ( ptr == szLine )
           return SYNCML_DM_FAIL;

        len = ptr-szLine;
        while ( len > 0 && szLine[len-1] == ' ' )
            len--;

        name.assign(szLine,len); 
            
        ptr++;
        while (*ptr == ' ' && *ptr != '\0' )
           ptr++;

        if ( *ptr == '\0' )
           return SYNCML_DM_FAIL;

        value = ptr; 
      }
      else
       name = szLine; 
      

      aMap.put(name, value);
   }   

   return SYNCML_DM_SUCCESS;
 }


void 
DMPluginManager::DeInit()
{
   XPL_LOG_DM_PLG_Debug(("DMPluginManager::DeInit\n"));

   m_dataPlugins.clear();
   m_constPlugins.clear();
   m_execPlugins.clear();
   m_commPlugins.clear();

#ifndef DM_STATIC_PLUGINS
   m_mapFilePluginVectors.clear();
#endif
  m_pTree = NULL;
}


DMPluginVector * 
DMPluginManager::GetPlugins(SYNCML_DM_PLUGIN_TYPE_T type)
{
    switch ( type )
    {
        case SYNCML_DM_DATA_PLUGIN:
            return &m_dataPlugins;
            
        case SYNCML_DM_EXECUTE_PLUGIN:
            return &m_execPlugins;
            
        case SYNCML_DM_CONSTRAINT_PLUGIN:
            return &m_constPlugins;
            
        case SYNCML_DM_COMMIT_PLUGIN:
            return &m_commPlugins;

        default:
            return NULL;
    }         
}


BOOLEAN 
DMPluginManager::PathMatch(CPCHAR szNodePath, CPCHAR szPluginPath) 
{
   /* szNodePath must NOT contain "*"
    * szPluginPath may contain "*" */
   BOOLEAN matched=FALSE;

   CPCHAR ptr = szNodePath;
   CPCHAR ptr1 = szPluginPath;

   while ( TRUE  )
   {
      if ( *ptr1 == '\0' )
      {
         matched = TRUE;
         return matched;
      }
   
      if ( *ptr1 == '*')
         break;
               
      if (* ptr == '\0' || *ptr != * ptr1)
      {
         matched = FALSE;
         return matched;
      }
      ptr++;
      ptr1++;
   } 

   //Now there is * in ptr1;
   ptr1++;
   if ( DmStrstr(ptr, ptr1) != NULL)
   {
      matched = TRUE;
      return matched;
   }
      
   return matched;
}
  
PDMPlugin 
DMPluginManager::FindPlugin(SYNCML_DM_PLUGIN_TYPE_T type,  CPCHAR szPath)
{
   XPL_LOG_DM_PLG_Debug(("DMPluginManager::FindPlugin, type=%d path=%s\n", type, szPath));
   DMPluginVector * pPlugins = GetPlugins(type);
   INT32 size = pPlugins->size();
   for (int i = 0; i<size; i++)
   {
      CPCHAR szPluginPath = (*pPlugins)[i]->GetPath();
      if ( PathMatch(szPath, szPluginPath )  )
         return (*pPlugins)[i];
   }
   return NULL;
}


PDMPlugin  
DMPluginManager::FindCommitPlugin(CPCHAR szPath)
{
   for (int i = 0; i<m_commPlugins.size(); i++)
   {
      if ( (m_commPlugins[i])->GetPath() == szPath )
        return m_commPlugins[i];    
   }
   return NULL;
}


SYNCML_DM_RET_STATUS_T 
DMPluginManager::MountNode(PDMPlugin & plugin, DMAddNodeProp & oNodeProps)
{
   SYNCML_DM_RET_STATUS_T ret=SYNCML_DM_SUCCESS;

   if( !m_pTree ) return SYNCML_DM_FAIL;
   
   CPCHAR szRootPath = plugin->GetPath();
   DMNode *pParent = m_pTree->FindNodeByURI(szRootPath);

   if ( plugin->IsOveralyPlugin() )
   {
     if ( !pParent  || pParent->getFormat() != SYNCML_DM_FORMAT_NODE || pParent->IsOverlayPI())
      return SYNCML_DM_SUCCESS; // no mount point or invalid type

     DMOverlayPINode* pNewNode = new DMOverlayPINode( plugin, pParent );

     if ( !pNewNode )
        return SYNCML_DM_DEVICE_FULL;
     
     m_pTree->SubstituteNode( pParent, pNewNode );

     delete pParent;
     
     return SYNCML_DM_SUCCESS; 
   }
   
   if ( pParent )
      return SYNCML_DM_SUCCESS; 

   DMURI oURI(TRUE,szRootPath);

   if ( oURI.getBuffer() == NULL )
     return SYNCML_DM_DEVICE_FULL;

   CPCHAR lastSegtment = oURI.getLastSegment();
   CPCHAR parentURI = oURI.getParentURI();
   
   if ( parentURI == NULL )
   {
     XPL_LOG_DM_PLG_Error(("MountNode : unable to get parent\n"));
     return SYNCML_DM_FAIL;
   }

   ret = oNodeProps.set(lastSegtment,NULL,SYNCML_DM_FORMAT_INVALID,NULL,0,NULL,DMNode::enum_NodePlugin);
   

   DMNode* node = new DMPluginRootNode(plugin);

   if ( node == NULL )
   {
     XPL_LOG_DM_PLG_Error(("MountNode : unable allocate memory\n"));
     return SYNCML_DM_DEVICE_FULL;
   }

   pParent = m_pTree->FindNodeByURI( parentURI );

   if ( !pParent ) 
   {
      XPL_LOG_DM_PLG_Error(("MountNode : cannot find node by URI: %s\n",parentURI));
      delete node;
      return SYNCML_DM_FAIL;
   }
  
   /* Add the node to the tree */
   /* the format of the node is unknown here */
   ret = m_pTree->AddNode( &pParent, oNodeProps, node);
   if ( ret != SYNCML_DM_SUCCESS )  
   {
        XPL_LOG_DM_PLG_Error(("Add plugin node failed for %s, ret=%d\n", szRootPath, ret));
   }     

   return ret;
}



SYNCML_DM_RET_STATUS_T 
DMPluginManager::Init( CEnv* env, DMTree* tree )
{
   SYNCML_DM_RET_STATUS_T ret_status=SYNCML_DM_SUCCESS; 

   if( !env || !tree ) return SYNCML_DM_FAIL;

    m_pTree = tree;
    
#ifndef DM_STATIC_PLUGINS  
   DMString dstdir = XPL_DM_GetEnv(SYNCML_DM_PLUGINS_PATH);
   if (dstdir == NULL)
      dstdir=".";

   ret_status = LoadDirectory(dstdir);
   if ( ret_status != SYNCML_DM_SUCCESS )
      return ret_status;   
 
   env->GetWFSFullPath("plugins",dstdir);
   LoadDirectory(dstdir);
#else
#ifdef DM_STATIC_FILES 
   UINT32 size;
   UINT8 *pBuffer = (UINT8*)XPL_DM_GetPluginsConfig(&size);
   if ( pBuffer )
       ret_status = LoadPluginFile(pBuffer,size);
#else
   DMString sPluginsINI;

   env->GetMainRFSFullPath(DM_PLUGINS_INI_FILENAME,sPluginsINI);
   LoadPluginFile(sPluginsINI);
#endif
#endif

   return ret_status;

}


SYNCML_DM_RET_STATUS_T 
DMPluginManager::CheckPluginAging(INT32 nAgingTime)
{
    XPL_LOG_DM_PLG_Debug(("CheckPluginAging: nAgingTime: %d\n", nAgingTime));
    XPL_CLK_CLOCK_T currentTime = XPL_CLK_GetClock();
    INT32 j,i = 0;    

    for (j=0; j<MAX_PLUGINTYPES; j++)
    {
        DMPluginVector * pPlugins = GetPlugins(m_nIndex[j]);
        for (i = 0; i < pPlugins->size(); i++) 
        {
            XPL_CLK_CLOCK_T lastAccessed = (*pPlugins)[i]->GetLastAccessedTime();
            if ( (currentTime - lastAccessed) >= (XPL_CLK_CLOCK_T)nAgingTime && (*pPlugins)[i]->IsSymbolsLoaded() ) 
            {
                (*pPlugins)[i]->UnloadSymbols();
            }
        }
   }


    for( DMMap<DMString,XPL_DL_HANDLE_T>::POS pos = m_pluginLibs.end()-1; 
          pos >= m_pluginLibs.begin(); pos-- )    
    {
        BOOLEAN bIsFound = FALSE;
        for (j=0; j<MAX_PLUGINTYPES; j++)
        {
            DMPluginVector * pPlugins = GetPlugins(m_nIndex[j]);

            for (i = 0; i < pPlugins->size(); i++) 
            {
                if ( (*pPlugins)[i]->IsLibReleased(m_pluginLibs.get_value(pos)) == TRUE )
                    bIsFound = TRUE;
            }
        }
        if ( bIsFound == FALSE )
        {
            XPL_DL_Unload(m_pluginLibs.get_value(pos));
            m_pluginLibs.remove(m_pluginLibs.get_key(pos));
        }
    }
          
    return SYNCML_DM_SUCCESS;
}

void 
DMPluginManager::GetPlugins(CPCHAR szPath,
                                       SYNCML_DM_PLUGIN_TYPE_T type,
                                       DMPluginVector& aPlugins )
{
  const DMPluginVector* pArray = GetPlugins(type);

  DMString strURI = szPath;
  for (INT32 i = 0; i < pArray->size(); i++ )
  {
    if ( DmIsParentURI(strURI.c_str(), (*pArray)[i]->GetPath() ) )
    {
      aPlugins.push_back( (*pArray)[i] );
    }
  }
  XPL_LOG_DM_PLG_Debug(("GetPlugins, szURI:%s\n", szPath));
}
