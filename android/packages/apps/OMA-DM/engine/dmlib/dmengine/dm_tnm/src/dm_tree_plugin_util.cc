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

/*
 *  DESCRIPTION:
 *      The dm_tree_plugin_util.cc file contains helper functions
 *      for plug-in support
 */

#include "xpl_Logger.h"
#include "dmtTreeImpl.hpp"
#include "dm_tree_plugin_root_node_class.H"
#include "dm_tree_plugin_util.H"
#include "dm_tree_util.h"  
#include "SyncML_DM_WBXMLArchive.H"
#include "SyncML_PlugIn_WBXMLLog.H"
#include "dm_uri_utils.h"

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

SYNCML_DM_RET_STATUS_T
DmCheckNodeConstraint(const PDMPlugin & pPlugin) ;

SYNCML_DM_RET_STATUS_T 
DmCheckMultiNodeConstraint(const PDMPlugin & pPlugin, CPCHAR path, CPCHAR remain, const PDmtTree & tree);

SYNCML_DM_RET_STATUS_T DmRemoveTempfile(DMString &dirName)
{
  
    SYNCML_DM_RET_STATUS_T ret_code = SYNCML_DM_SUCCESS;
    XPL_FS_SHANDLE_T search_handle;
    DMString strFileNameBuffer;
    char *file_name = strFileNameBuffer.AllocateBuffer(XPL_FS_MAX_FILE_NAME_LENGTH); 

    if (file_name == NULL)
      return SYNCML_DM_DEVICE_FULL;

    search_handle = XPL_FS_StartSearch(dirName, DMFileHandler::MIDDLE_FILE_EXTENSION, TRUE, NULL);
    if ( search_handle == XPL_FS_SHANDLE_INVALID)
        return SYNCML_DM_IO_FAILURE;
  
   //Loop through all .temp files in the directory
    memset(file_name,0,XPL_FS_MAX_FILE_NAME_LENGTH);
    while ( XPL_FS_GetSearchResult(search_handle,file_name) != XPL_FS_RET_NOT_FOUND ) 
    {

	DMFileHandler fileHandler(file_name);
	ret_code = fileHandler.open(XPL_FS_FILE_RDWR);
	// Remove temporary ESN file
	if(ret_code == SYNCML_DM_SUCCESS)
		fileHandler.deleteFile();
     }
     XPL_FS_EndSearch(search_handle);
    return ret_code;
}

SYNCML_DM_RET_STATUS_T DMTree::ReadCommandFromFile( DMFileHandler    *fileHandle,
                                                    DMBuffer&        cmdURI )
{
  SYNCML_DM_RET_STATUS_T  ret_code = SYNCML_DM_FAIL;
  SyncML_DM_WBXMLReader*  reader = new SyncML_DM_WBXMLReader( fileHandle );

  if( reader ) for( ; ; )
  {
    ret_code = SYNCML_DM_IO_FAILURE;
    if(fileHandle->seek(XPL_FS_SEEK_SET, sizeof(INT32) + 2) != XPL_FS_RET_SUCCESS) break;

    // Read URI
    ret_code = reader->readOpaque( &cmdURI );
    if( SYNCML_DM_SUCCESS != ret_code ) break;

    //Read END_TAG
    UINT8  bYte = 0;
    ret_code = reader->readByte( &bYte );
    if( SYNCML_DM_SUCCESS != ret_code ) break;

    ret_code = SYNCML_DM_IO_FAILURE;
    if( bYte != SyncML_DM_WBXMLArchive::END_TAG ) break;

    ret_code = SYNCML_DM_SUCCESS;
    break;
  }

  if( reader )
  {
    delete reader;
    reader = NULL;
  }

  return ret_code;
}

SYNCML_DM_RET_STATUS_T DMTree::RecoverPluginFromFile( const DMString&  file_bak_name )
{
  SYNCML_DM_RET_STATUS_T  ret_code = SYNCML_DM_FAIL;
  DMFileHandler           *fileHandle = new DMFileHandler( file_bak_name );

  if( fileHandle )
  {
    BOOLEAN delete_file = FALSE;

    if(fileHandle->open(XPL_FS_FILE_RDWR) != SYNCML_DM_SUCCESS)
    {
      // DM: Ignore files that cannot be opened ?
      ret_code = SYNCML_DM_SUCCESS;
    }
    else
    {
      delete_file = TRUE;

      DMBuffer cmdURI;
      ret_code = ReadCommandFromFile( fileHandle, cmdURI );

      if( ret_code == SYNCML_DM_SUCCESS )
      {
        PDMPlugin pPlugin = m_oPluginManager.FindPlugin(SYNCML_DM_DATA_PLUGIN, (CPCHAR)cmdURI.getBuffer());
       
        if (pPlugin != NULL)
        {
          PDmtAPIPluginTree  pluginTree;
          ret_code=pPlugin->GetTree((CPCHAR) cmdURI.getBuffer(), pluginTree);

          if(ret_code == SYNCML_DM_SUCCESS)
          {
            PDmtRWPluginTree ptrRWTree = (DmtRWPluginTree *) ((DmtTree *)pluginTree);
            ret_code = ptrRWTree->setLogFileHandle(fileHandle);

            if( ret_code == SYNCML_DM_SUCCESS)
            {
              ret_code = ptrRWTree->Rollback();

              // DM: hands-off from file
              fileHandle = NULL;
            }
          }    
        }
      }    
    }

    if( fileHandle )
    {
      if( delete_file ) 
      {
        fileHandle->deleteFile();
      }

      delete fileHandle;
      fileHandle = NULL;
    }
  }

  return ret_code;
}
  
SYNCML_DM_RET_STATUS_T DMTree::RecoverPlugin()
{

  SYNCML_DM_RET_STATUS_T  ret_code = SYNCML_DM_SUCCESS;
  DMString                strFileNameBuffer;
  DMString                strFileBakNameBuffer;

  char                    *file_name = strFileNameBuffer.AllocateBuffer(XPL_FS_MAX_FILE_NAME_LENGTH); 
  char                    *file_bak_name = strFileBakNameBuffer.AllocateBuffer(XPL_FS_MAX_FILE_NAME_LENGTH); 

  if ( !file_name || !file_bak_name )
    return SYNCML_DM_DEVICE_FULL;
  
  DMString logtdir;
  m_oEnv.GetWFSFullPath( NULL, logtdir );

  XPL_FS_SHANDLE_T search_handle = XPL_FS_StartSearch(logtdir, DMFileHandler::LOG_FILE_EXTENSION, TRUE, NULL);
  if ( search_handle == XPL_FS_SHANDLE_INVALID)
      return SYNCML_DM_IO_FAILURE;

 //Loop through all .log files in the directory
  memset(file_name, 0, XPL_FS_MAX_FILE_NAME_LENGTH );
  memset(file_bak_name, 0, XPL_FS_MAX_FILE_NAME_LENGTH );

  int nExtLength = DmStrlen( DMFileHandler::LOG_FILE_EXTENSION );

  while ( XPL_FS_GetSearchResult(search_handle, file_name) != XPL_FS_RET_NOT_FOUND ) 
  {
    int nFileNameLen = DmStrlen(file_name);

    if ( file_name[0] == 0 || nFileNameLen < 5 ) continue;

    DmStrncpy(file_bak_name, file_name, nFileNameLen - nExtLength );
    DmStrcat(file_bak_name, DMFileHandler::MIDDLE_FILE_EXTENSION);

    if( XPL_FS_RET_SUCCESS != XPL_FS_Rename( file_name, file_bak_name ) )
    {
      ret_code = SYNCML_DM_IO_FAILURE;
      continue;
    }    

    ret_code = RecoverPluginFromFile( file_bak_name );
    // DM: continue even if there is an error ( exactly as in original version of code )
  }

  XPL_FS_EndSearch(search_handle);
  return ret_code;
  
}

SYNCML_DM_RET_STATUS_T 
DmExecutePlugin(CPCHAR path, 
                                CPCHAR args, 
                                CPCHAR szCorrelator, 
                                DMString & oResult)
{
   SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_FAIL;
   DMPluginManager & oPluginManager = dmTreeObj.GetPluginManager();

   XPL_LOG_DM_TMN_Debug((" DmExecutePlugin find plugin\n")); 
   PDMPlugin pPlugin = oPluginManager.FindPlugin(SYNCML_DM_EXECUTE_PLUGIN, path);
     
   if (pPlugin == NULL)
   {
        XPL_LOG_DM_TMN_Debug((" DmExecutePlugin plugin not found\n"));
        return SYNCML_DM_FEATURE_NOT_SUPPORTED;
   }

   PDmtTree ptrTree( new DmtTreeImpl(FALSE) ); // readonly version of the tree

   XPL_LOG_DM_TMN_Debug((" DmExecutePlugin plugin being executed...\n"));
   return pPlugin->Execute(path,args,szCorrelator,ptrTree,oResult);
}

SYNCML_DM_RET_STATUS_T 
DmCallPluginFunction(CPCHAR szSessionRootURI,
                                        FILESETTYPE nFileSet,
                                        SYNCML_DM_COMMAND_T type)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    DMPluginManager & oPluginManager = dmTreeObj.GetPluginManager();

    DMPluginVector  plugins;
    PDmtAPIPluginTree  pluginTree;

    oPluginManager.GetPlugins(szSessionRootURI, SYNCML_DM_DATA_PLUGIN, plugins);

    INT32 size = plugins.size();
    for ( INT32 i=0; i<size; i++ )
    {
        PDMPlugin  plugin = plugins[i];
        if(plugin->IsTreeEmpty())
            continue;

        dm_stat=plugin->GetTree(plugin->GetPath(), pluginTree);
        if(dm_stat == SYNCML_DM_SUCCESS)
        {
            switch(type) 
            {
                case SYNCML_DM_RELEASE:
                    dm_stat = pluginTree->Flush();
                    break;
                
                case SYNCML_DM_ROLLBACK:
                {
                    FILESETTYPE sets = 0;

                    SyncML_DM_Archiver&  archiver = dmTreeObj.GetArchiver();
                    sets = archiver.getArchivesByURI(plugin->GetPath());
                    sets &= nFileSet;
                    
                    if ( sets != 0 )
                    {
                        dm_stat = pluginTree->Rollback();
                    }
                }
                    break;

                case SYNCML_DM_COMMIT:
                    dm_stat = pluginTree->Commit();
                    break;
                
                case SYNCML_DM_ATOMIC:
                    dm_stat = pluginTree->Begin();
                    break;
           }
         }

         if (dm_stat != SYNCML_DM_SUCCESS)
         {
            if((dm_stat == SYNCML_DM_FEATURE_NOT_SUPPORTED)&&(type == SYNCML_DM_ATOMIC))
                return dm_stat;
            else
               dm_stat = SYNCML_DM_SUCCESS;
        }
     }         

   XPL_LOG_DM_TMN_Debug((" DmCallPluginFunction retStat=%d\n", dm_stat));
   return dm_stat;
}

SYNCML_DM_RET_STATUS_T 
DmCheckConstraint(CPCHAR szSessionRootURI, 
                                 FILESETTYPE * pFileSet,
                                 FILESETTYPE * pFileSetToRollback)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    SYNCML_DM_RET_STATUS_T dm_const_stat = SYNCML_DM_SUCCESS;

    SyncML_DM_Archiver&  archiver = dmTreeObj.GetArchiver();
    FILESETTYPE  numArchives = archiver.getNumArchives();
    FILESETTYPE  i = 0;
    FILESETTYPE  set = 1;


    if ( pFileSet == NULL || pFileSetToRollback == NULL )
        return SYNCML_DM_FAIL;

    XPL_LOG_DM_TMN_Debug(("--- DmCheckConstraint for %s, pFileSet=0x%x, numArchives=%d\n", szSessionRootURI, *pFileSet, numArchives));
 
    if ( !archiver.IsDirty(pFileSet) )
    {
        XPL_LOG_DM_TMN_Debug(("No archives are dirty\n"));
        return dm_stat;
    }

    DMPluginVector  aSessionPlugins;
   
    DMPluginManager & oPluginManager = dmTreeObj.GetPluginManager();
    oPluginManager.GetPlugins(szSessionRootURI, SYNCML_DM_CONSTRAINT_PLUGIN, aSessionPlugins);
   
    INT32 size = aSessionPlugins.size();
    for (INT32 index=0; index< size; index++)
    {
        FILESETTYPE sets = 0;
  
        CPCHAR strRootPath=(aSessionPlugins[index])->GetPath();

        sets = archiver.getArchivesByURI(strRootPath);
        sets = sets & (*pFileSet);
        if ( sets )
        {
            dm_stat = DmCheckNodeConstraint(aSessionPlugins[index]);
            if ( dm_stat != SYNCML_DM_SUCCESS ) 
            {
                for (i=0, set=1; i< numArchives;  i++, set = set <<1)  
                {
                    if ( (sets & set) != 0 && ((*pFileSetToRollback) & set) == 0 )
                    {
                        (*pFileSetToRollback) |= set;
                        (*pFileSet) &= ~ set; 
                    }
                }
                dm_const_stat = dm_stat;
            }
        }
    }	  

    if ( dm_const_stat != SYNCML_DM_SUCCESS )
        dm_stat = dm_const_stat;

    XPL_LOG_DM_TMN_Debug((" DmCheckConstraint retStat=%d\n", dm_stat));
    return dm_stat;
}



SYNCML_DM_RET_STATUS_T
DmCheckNodeConstraint(const PDMPlugin & pPlugin) 
{
      SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
      PDmtTree tree( new DmtTreeImpl(true) );

      CPCHAR strPluginPath=pPlugin->GetPath();

      if (  (DmStrchr(strPluginPath, '*')) == NULL)
      {
            dm_stat = (pPlugin)->CheckConstraint(strPluginPath,tree);
      }
      else
      {
            dm_stat = DmCheckMultiNodeConstraint(pPlugin,"",strPluginPath,tree);
      }

      if (dm_stat != SYNCML_DM_SUCCESS)
            dm_stat = SYNCML_DM_CONSTRAINT_FAIL;

     return dm_stat;

}


SYNCML_DM_RET_STATUS_T
DmCheckMultiNodeConstraint(const PDMPlugin & pPlugin, 
                                  CPCHAR path, 
                                  CPCHAR remain, 
                                  const PDmtTree & tree)       
{
    SYNCML_DM_RET_STATUS_T dm_stat;
    const char * ptr;    //point to remaining stuff after *

    XPL_LOG_DM_TMN_Debug(("CheckMultiNode %s:%s\n", path, remain));

    ptr=remain;
    while ( * ptr != '*' && * ptr != '\0')
        ptr ++;

    if ( *ptr == '*' )
    {
        DMString strPath=path;
        strPath += DMString(remain, (int) ptr - (int)remain -1);

        ptr++;

        DMStringVector mapNodeNames;
        dm_stat = tree->GetChildNodeNames(strPath.c_str(),mapNodeNames);
        if ( dm_stat != SYNCML_DM_SUCCESS )
            return dm_stat;
      
        for (INT32 i=0; i<mapNodeNames.size(); i++)
        {
            DMString childPath;
            childPath =strPath;  
            childPath += "/";
            childPath += mapNodeNames[i];

            dm_stat = DmCheckMultiNodeConstraint(pPlugin,childPath.c_str(),ptr,tree);
            if ( dm_stat != SYNCML_DM_SUCCESS)
                return dm_stat;
        }
    } 
    else
    {
        DMString strPath=path;
        strPath += remain;
        dm_stat=(pPlugin)->CheckConstraint(strPath.c_str(),tree);
    }


   return dm_stat;
}
