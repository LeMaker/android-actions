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

    Header Name: SYNCML_DM_TreeMount.cc

    General Description: Implementation of the and SYNCML_DM_TreeMount  classes.

==================================================================================================*/

#include "dm_uri_utils.h"
#include "SyncML_DM_WBXMLArchive.H"
#include "dmBufferReader.h"
#include "xpl_dm_Manager.h"
#include "dm_tree_class.H"
#include "SYNCML_DM_TreeMount.H"

SYNCML_DM_TreeMount::SYNCML_DM_TreeMount()
  : m_pTree( NULL )
{
}

SYNCML_DM_RET_STATUS_T 
SYNCML_DM_TreeMount::TreeAddToMountList (CPCHAR pUri, CPCHAR pTreePath)
{
    if( !m_pTree ) return SYNCML_DM_FAIL;
    
    /* Validate the URI */
    SYNCML_DM_URI_RESULT_T  dm_uri_result = m_pTree->URIValidateAndParse(pUri);

    if (( dm_uri_result == SYNCML_DM_COMMAND_ON_UNKNOWN_PROPERTY ) ||
        ( dm_uri_result == SYNCML_DM_COMMAND_INVALID_URI) ||
        ( dm_uri_result == SYNCML_DM_COMMAND_URI_TOO_LONG ))
    {
        return (SYNCML_DM_FAIL);
    }

    /* Check if the entry is existed in the list already */
    if ( m_astrURIs.find( pUri ) >= 0 )
        return SYNCML_DM_ENTRY_EXIST;

    CPCHAR loc = DmStrrchr(pTreePath, '.');

    // Compare the last six chars for a ".wbxml" extension
    if(loc == NULL || DmStrcmp(loc, SyncML_DM_WBXMLArchive::FILE_EXTENSION) != 0)
        return SYNCML_DM_FAIL;

    BOOLEAN bFeatureEnabled = TRUE;
    bFeatureEnabled = DmIsEnabledURI(pUri);

    m_astrURIs.push_back( pUri );
    if ( bFeatureEnabled ) 	
        m_astrPaths.push_back( pTreePath );
    else
        m_astrPaths.push_back( NULL );
    
    return (SYNCML_DM_SUCCESS);
}
 
void 
SYNCML_DM_TreeMount::GetTreeMountEntry(CPCHAR &p_Uri,  
                                                              CPCHAR& p_TreePath,  
                                                              UINT16 index) const 
{
  /* If the list is empty, set OUTPUT parameters as NULL and return. */
    if ( index >= m_astrURIs.size() ) {
        p_Uri = NULL;
        p_TreePath = NULL;
        return;
    }
    p_Uri = m_astrURIs[index].c_str();
    p_TreePath = m_astrPaths[index].c_str();
}

BOOLEAN 
SYNCML_DM_TreeMount::IsMountPointEnabled(CPCHAR pUri) const 
{
    INT32 size =  m_astrURIs.size();
      
    for (INT32 index = 0; index < size; index++)
    {
        const DMString & str = m_astrURIs[index];
        if ( DmIsParentURI( str.c_str(), pUri ) )
        {
            if ( m_astrPaths[index] == NULL )
                 return FALSE;
            else
                 return TRUE;
        }

   }  
   return TRUE;
}
  
void 
SYNCML_DM_TreeMount::UnMountTree(void)
{
    m_astrURIs.clear();
    m_astrPaths.clear();
    m_pTree = NULL;
}


#ifndef DM_STATIC_FILES
SYNCML_DM_RET_STATUS_T
SYNCML_DM_TreeMount::MountTree( CEnv* env, DMTree* tree )
{
    if( !env || !tree ) return SYNCML_DM_FAIL;

    m_pTree = tree;
    
    DMString strLineBuffer;
    char *line = strLineBuffer.AllocateBuffer(DM_MAX_CONFIG_LINE);
    SYNCML_DM_RET_STATUS_T dm_result =  SYNCML_DM_SUCCESS;
    DMString strFstabFileName;

   if ( !line )
    return SYNCML_DM_DEVICE_FULL;
    
    env->GetMainRFSFullPath(DM_FSTAB_FILENAME,strFstabFileName);
    
    DMFileHandler fp( strFstabFileName );
    
    dm_result = fp.open(XPL_FS_FILE_READ);

    if ( dm_result != SYNCML_DM_SUCCESS ) 
        return dm_result;
 
    INT32 numMountedFiles =0;
    char * uri = NULL;
    char * filepath = NULL;
    while( !fp.iseof() )
    {
        fp.fgets((char *)line, DM_MAX_CONFIG_LINE);
        if( line[0] == '#' || line[0] == '\0' )
            continue;

        uri = line;
        filepath = (char*)DmStrchr(line,' ');

        if ( filepath == NULL )
            break;
        
        *filepath ='\0';
        filepath++;

        while ( *filepath !='\0' && *filepath == ' ' )
           filepath ++;
         
        if (filepath == '\0')
           break;
            
        //Now add to the mount table
        dm_result = TreeAddToMountList(uri, filepath);
        if ( dm_result  == SYNCML_DM_SUCCESS)
            numMountedFiles++;
    }    
    
    fp.close();      

    //If no files are there, bail out.
    if (numMountedFiles==0)
       return SYNCML_DM_IO_FAILURE;
   
    return dm_result;
}
#else
SYNCML_DM_RET_STATUS_T
SYNCML_DM_TreeMount::MountTree( CEnv* env, DMTree* tree )
{
    SYNCML_DM_RET_STATUS_T dm_result =  SYNCML_DM_SUCCESS;

    UINT8 * pBuffer;
    UINT32  size;
    char * line;

    pBuffer = (UINT8*)XPL_DM_GetFstab(&size);

    if ( !pBuffer )
        return SYNCML_DM_FAIL;

    DMBufferReader fp(pBuffer,size);
    INT32 numMountedFiles =0;
    DMString uri;
    char * filepath = NULL;
    while( !fp.iseof() )
    {
        line = fp.fgets();

        filepath = (char*)DmStrchr(line,' ');
        
        if ( filepath == NULL)
           break;

        uri.assign(line,filepath-line);    
        filepath++;
        
        while ( *filepath !='\0' && *filepath ==' ' )
           filepath++;
            
        dm_result = TreeAddToMountList(uri, filepath);
        if ( dm_result  == SYNCML_DM_SUCCESS)
           numMountedFiles++;
    }    

    //If no files are there, bail out.
    if (numMountedFiles==0)
        return SYNCML_DM_IO_FAILURE;
   
    return dm_result;
}

#endif
