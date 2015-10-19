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

#include "dmMemory.h"
#include "dmdefs.h"     
#include "dmt.hpp"
#include "SyncML_PlugIn_WBXMLLog.H"
#include "dmtRWPlugin.hpp"
#include "xpl_File.h"
#include "dm_uri_utils.h"
#include "dm_tree_class.H"

#ifdef TEST_DM_RECOVERY
#include <unistd.h>
extern char power_fail_point[];
#endif

DmtRWPluginTree::DmtRWPluginTree()
{
    fpLog = NULL;
    log = NULL;
    m_Playback = FALSE;
   m_strLogPath = NULL;
   m_ESNDirty = FALSE;
   m_bIsAtomic = FALSE;
}

DmtRWPluginTree::~DmtRWPluginTree()
{
    if(fpLog != NULL)
        delete fpLog;

    if(this->log != NULL)
        delete this->log;
}

SYNCML_DM_RET_STATUS_T
DmtRWPluginTree::setLogFileHandle(DMFileHandler* fileHandle)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    if(this->log != NULL)
    {
        delete this->log;
        this->log = NULL;
    }
    if(fpLog != NULL)
      delete fpLog;

    fpLog = fileHandle;
    this->log = new SyncML_PlugIn_WBXMLLog(this,(CPCHAR) m_strRootPath.c_str());
    if(this->log != NULL)
        dm_stat = this->log->setLogFileHandle(fpLog);
    else
        dm_stat = SYNCML_DM_FAIL;

    return dm_stat;
}

SYNCML_DM_RET_STATUS_T 
DmtRWPluginTree::InitLog ()
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    if(this->log  != NULL)
        return SYNCML_DM_SUCCESS;

    if(m_strLogPath == NULL)
    {
        DMString file_name;
        char uniqueStr[20];
        XPL_CLK_LONG_CLOCK_T curTime = XPL_CLK_GetClockMs();
        DmSprintf(uniqueStr, "%lld", curTime);
        if(uniqueStr[0] == '-')
           uniqueStr[0]='L';
   
     dmTreeObj.GetWritableFileSystemFullPath( m_strLogPath );

        CPCHAR pT = m_strLogPath.c_str();
        if (pT[m_strLogPath.length()-1] != '/')
          m_strLogPath += "/";

        XPL_FS_MkDir(m_strLogPath);
        m_strLogPath += uniqueStr;
        m_strLogPath +=  ".log";
    }
    this->log = new SyncML_PlugIn_WBXMLLog(this,(CPCHAR) m_strRootPath.c_str());
    if(this->log == NULL)
       return SYNCML_DM_FAIL;
    dm_stat = this->log->InitLog(m_strLogPath.c_str());
    if(dm_stat != SYNCML_DM_SUCCESS)
            return dm_stat;

    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T 
DmtRWPluginTree::LogCommand(SYNCML_DM_PLUGIN_COMMAND_T type,
                                 CPCHAR pbURI,
                                 SYNCML_DM_PLUGIN_COMMAND_ATTRIBUTE_T attribute,
                                 const DmtNode * inNode)
{
    if(m_Playback == TRUE)
       return SYNCML_DM_SUCCESS;

    SYNCML_DM_RET_STATUS_T  dm_stat = InitLog();
    if(this->log == NULL)
       return dm_stat;

    switch ( type )
    {
       case SYNCML_DM_PLUGIN_ADD:    
           dm_stat = log->logCommand(SYNCML_DM_PLUGIN_DELETE, pbURI, attribute, inNode);
           break;
        
       case SYNCML_DM_PLUGIN_REPLACE:
           dm_stat = log->logCommand(SYNCML_DM_PLUGIN_REPLACE, pbURI, attribute, inNode);
           break;

       case SYNCML_DM_PLUGIN_DELETE:
           if(DmStrlen(pbURI) != 0)
              log->logCommand(SYNCML_DM_PLUGIN_ADD_CHILD, pbURI, attribute, inNode);
           dm_stat = log->logCommand(SYNCML_DM_PLUGIN_ADD, pbURI, attribute, inNode);
           break;
    }
    return dm_stat;
}

SYNCML_DM_RET_STATUS_T DmtRWPluginTree::Verify()
{
   //@@@TODO
    return SYNCML_DM_SUCCESS;
}
SYNCML_DM_RET_STATUS_T DmtRWPluginTree::CreateInteriorNodeInternal( CPCHAR path, PDmtNode& ptrCreatedNode, const DMStringVector & childNodeNames)
{
    PDmtRWPluginNode pNode;
    SYNCML_DM_RET_STATUS_T dm_stat;

    pNode = new DmtRWPluginNode();
    if ( pNode == NULL )
        return SYNCML_DM_DEVICE_FULL;
    
    dm_stat = pNode->Init(this, path, childNodeNames);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
    
    dm_stat = this->SetNode(path, PDmtNode(pNode));
    if ( dm_stat == SYNCML_DM_SUCCESS )
        dm_stat = GetNode( path, ptrCreatedNode );

    return dm_stat;
}

SYNCML_DM_RET_STATUS_T DmtRWPluginTree::CreateLeafNodeInternal( CPCHAR path, PDmtNode& ptrCreatedNode, const DmtData& value )
{
    PDmtRWPluginNode pNode;
    SYNCML_DM_RET_STATUS_T dm_stat;
    
    pNode=new DmtRWPluginNode();
    if ( pNode == NULL )
        return SYNCML_DM_DEVICE_FULL;

     dm_stat = pNode->Init(this, path, value);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
    
    dm_stat = this->SetNode(path, PDmtNode(pNode));
    if ( dm_stat == SYNCML_DM_SUCCESS )
        dm_stat = GetNode( path, ptrCreatedNode );
    
    return dm_stat;
}

SYNCML_DM_RET_STATUS_T DmtRWPluginTree::DeleteSubTree( PDmtNode ptrNode)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    DMString nodePath;
    if(!ptrNode->IsLeaf())
    {
        DMVector<PDmtNode> oChildren;
        dm_stat = ptrNode->GetChildNodes( oChildren );
        if ( dm_stat != SYNCML_DM_SUCCESS )
            return dm_stat;
        
        for ( int i = 0; i < oChildren.size(); i++ )
        {
            if (oChildren[i]->IsLeaf() )
            {
                dm_stat = oChildren[i]->GetPath(nodePath);
                if ( dm_stat == SYNCML_DM_SUCCESS )
                {
                    dm_stat = this->RemoveNode(nodePath);
                    oChildren[i] = NULL;
                }    
            }
            else
               dm_stat =DeleteSubTree(oChildren[i]);
            
            if( dm_stat != SYNCML_DM_SUCCESS)
                return dm_stat;
        }
  }  

  dm_stat = ptrNode->GetPath(nodePath);  
  if ( dm_stat == SYNCML_DM_SUCCESS )
    dm_stat = this->RemoveNode(nodePath);
  ptrNode = NULL;

  return dm_stat;

}

SYNCML_DM_RET_STATUS_T DmtRWPluginTree::DeleteNode( CPCHAR path )
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    PDmtNode ptrNode;
        
    dm_stat = GetNode( path, ptrNode);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;

#ifdef LOB_SUPPORT
    // Create temporary storage for ESN
    dm_stat = BackupESNdata(path);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;

#endif
   
    dm_stat = LogCommand(SYNCML_DM_PLUGIN_DELETE, path, SYNCML_DM_PLUGIN_COMMAND_ON_NODE, ptrNode);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
   
    dm_stat = DeleteSubTree(ptrNode);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
   
    // Is it the root node ?
    if(DmStrlen(path) == 0)
        return dm_stat;

    DMString strURI;
    DMString strKey;
    DmParseURI( path, strURI, strKey );


    DMStringVector oChildren;
    dm_stat = this->GetChildNodeNames(strURI.c_str(), oChildren);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
      
    // Is the plugin root node?
    if(strURI.length() == 0)
    {
        for (INT32 i = 0; i < oChildren.size(); i++ )
        {
            if (!DmStrcmp(oChildren[i].c_str() , path))
                oChildren.remove(i);
        }
    }
    else 
    {
        for ( INT32 i = 0; i < oChildren.size(); i++ )
        {
            if (!DmStrcmp(oChildren[i].c_str() , strKey.c_str()))
                oChildren.remove(i);
        }
    }
   
      // Remove link to parent node
    dm_stat = GetNode( strURI.c_str(), ptrNode);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;

      
    DmtData m_value;
    dm_stat = m_value.SetNodeValue(oChildren);

    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
      
    // Turn off logging temporarilly
    BOOLEAN needLogging= m_Playback;
    m_Playback = TRUE;
    dm_stat = ptrNode->SetValue(m_value);
    m_Playback = needLogging;
  
    return dm_stat;
}
//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtRWPluginNode::RemoveNode
// DESCRIPTION     : Remove a node from memory
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : Returns SYNCML_DM_SUCCESS if success, otherwise fails
//  
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DmtRWPluginTree::RemoveNode(CPCHAR path)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    PDmtNode ptrNode;
   
    dm_stat = GetNode( path, ptrNode);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
#ifdef LOB_SUPPORT
   if(ptrNode->IsExternalStorageNode())
  {
      PDmtRWPluginNode pRWNode = (DmtRWPluginNode *) ((DmtNode *)ptrNode);
      dm_stat = pRWNode->Delete();
     if ( dm_stat != SYNCML_DM_SUCCESS)
         return dm_stat;
  }
#endif
  return DmtPluginTree::RemoveNode(path);

}
SYNCML_DM_RET_STATUS_T DmtRWPluginTree::RenameNode( CPCHAR path, CPCHAR szNewNodeName )
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    PDmtNode ptrNode;
   
    dm_stat = GetNode( path, ptrNode);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
   
    PDmtRWPluginNode pRWNode = (DmtRWPluginNode *) ((DmtNode *)ptrNode);

    return pRWNode->Rename(szNewNodeName);
}

SYNCML_DM_RET_STATUS_T DmtRWPluginTree::LinkToParentNode( CPCHAR path)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    DMString strURI;
    DMString strKey;
    PDmtNode ptrNode;
    DmParseURI( path, strURI, strKey );
    DMStringVector oChildren;

    dm_stat = this->GetChildNodeNames( strURI.c_str(), oChildren);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;

    if(strURI.length() != 0)
        oChildren.push_back(strKey.c_str());
    else
        oChildren.push_back(path);

    DmtData m_value;
    dm_stat = m_value.SetNodeValue(oChildren);

    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
      

    dm_stat = GetNode( strURI.c_str(), ptrNode);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;

    // Turn off logging temporarilly
    BOOLEAN needLogging= m_Playback;
    m_Playback = TRUE;
    dm_stat = ptrNode->SetValue(m_value);
    m_Playback = needLogging;
    return dm_stat;
   
 }
#ifdef LOB_SUPPORT
//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtRWPluginTree::BackupESNdata
// DESCRIPTION     : Create temporary storage for ESN
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : 
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DmtRWPluginTree::BackupESNdata( CPCHAR path)
{
  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
  PDmtNode ptrNode;
  
  dm_stat = GetNode( path, ptrNode);
  if ( dm_stat != SYNCML_DM_SUCCESS)
      return dm_stat;

 if (ptrNode->IsLeaf())
{    
     if(ptrNode->IsExternalStorageNode())
     {
         PDmtRWPluginNode pRWNode = (DmtRWPluginNode *) ((DmtNode *)ptrNode);
         
        // Backup file already exist?
        if(pRWNode->GetESNBackupFileName() != NULL)
            return SYNCML_DM_SUCCESS;

        // Backup ESN data
         dm_stat = pRWNode->BackupESNData();
     }
 }
 else
 {    
        DMStringVector mapNodeNames;
      DMString strVal;

        dm_stat = GetChildNodeNames( path, mapNodeNames );

        if ( dm_stat != SYNCML_DM_SUCCESS )
           return dm_stat;
            
        for ( int i = 0; i < mapNodeNames.size(); i++ )
        {
        DMString strVal = path;
        // Is root node?
        if(strVal.length() != 0)
              strVal += "/";
        // Construct child node name
               strVal += mapNodeNames[i];
        dm_stat = BackupESNdata(strVal.c_str());
            if ( dm_stat != SYNCML_DM_SUCCESS )
            return dm_stat;
        }
    }
    return dm_stat;
}
//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtRWPluginTree::CommitESN
// DESCRIPTION     : Commit changes for ESN
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : 
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DmtRWPluginTree::CommitESN( CPCHAR path)
{
  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
  PDmtNode ptrNode;

  // Any ESN data modified?
  if(!m_ESNDirty)
      return dm_stat;
  
  dm_stat = GetNode( path, ptrNode);
  if ( dm_stat != SYNCML_DM_SUCCESS)
      return dm_stat;

 if (ptrNode->IsLeaf())
{    
     if(ptrNode->IsExternalStorageNode())
     {
         PDmtRWPluginNode pRWNode = (DmtRWPluginNode *) ((DmtNode *)ptrNode);
         
        // Backup ESN data
         dm_stat = pRWNode->Commit();
     }
 }
 else
 {    
        DMStringVector mapNodeNames;
      DMString strVal;

        dm_stat = GetChildNodeNames( path, mapNodeNames );

        if ( dm_stat != SYNCML_DM_SUCCESS )
           return dm_stat;
            
        for ( int i = 0; i < mapNodeNames.size(); i++ )
        {
        DMString strVal = path;
        // Is root node?
        if(strVal.length() != 0)
                  strVal += "/";
        // Construct child node name
               strVal += mapNodeNames[i];
        dm_stat = CommitESN(strVal.c_str());
            if ( dm_stat != SYNCML_DM_SUCCESS )
            return dm_stat;
        }
    }
    return dm_stat;
}
//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtRWPluginTree::IsESNSetComplete
// DESCRIPTION     : Check if all the ESN setting are done
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : 
//--------------------------------------------------------------------------------------------
BOOLEAN DmtRWPluginTree::IsESNSetComplete(CPCHAR pbURI)
{
  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
  PDmtNode ptrNode;
  // Any ESN data modified?
  if(!m_ESNDirty)
      return TRUE;
  
  dm_stat = GetNode( pbURI, ptrNode);
  if ( dm_stat != SYNCML_DM_SUCCESS)
      return TRUE;

 if (ptrNode->IsLeaf())
{    
     PDmtRWPluginNode pRWNode = (DmtRWPluginNode *) ((DmtNode *)ptrNode);
     if(ptrNode->IsExternalStorageNode() && !pRWNode->IsESNSetComplete())
         return FALSE;
     else
         return TRUE;
 }
 else
 {    
        DMStringVector mapNodeNames;
      DMString strVal;

        dm_stat = GetChildNodeNames( pbURI, mapNodeNames );

        if ( dm_stat != SYNCML_DM_SUCCESS )
           return TRUE;
            
        for ( int i = 0; i < mapNodeNames.size(); i++ )
        {
        DMString strVal = pbURI;
              strVal += "/";
        // Construct child node name
               strVal += mapNodeNames[i];
        if(IsESNSetComplete(strVal.c_str()) == FALSE)
            return FALSE;
        }
    }
    return TRUE;
}
#endif

SYNCML_DM_RET_STATUS_T DmtRWPluginTree::CreateLeafNodeInternal( CPCHAR path, 
                                                             PDmtNode& ptrCreatedNode, 
                                                             const DmtData& value ,
                                                             BOOLEAN isESN)
{
    PDmtRWPluginNode pNode;
    SYNCML_DM_RET_STATUS_T dm_stat;
    
    pNode=new DmtRWPluginNode();
    if ( pNode == NULL )
        return SYNCML_DM_DEVICE_FULL;

     dm_stat = pNode->Init(this, path, value, isESN);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
    
    dm_stat = this->SetNode(path, PDmtNode(pNode));
    if ( dm_stat == SYNCML_DM_SUCCESS )
        dm_stat = GetNode( path, ptrCreatedNode );
    
    return dm_stat;
}
SYNCML_DM_RET_STATUS_T DmtRWPluginTree::CreateLeafNode(CPCHAR path,
                              PDmtNode& ptrCreatedNode,
                              const DmtData& value ,
                              BOOLEAN isESN)
{
    SYNCML_DM_RET_STATUS_T dm_stat;

    dm_stat = LogCommand(SYNCML_DM_PLUGIN_ADD, path, SYNCML_DM_PLUGIN_COMMAND_ON_NODE, NULL);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
    
    dm_stat = this ->CreateLeafNodeInternal(path, ptrCreatedNode, value, isESN );
    if (dm_stat != SYNCML_DM_SUCCESS)
    {
        if ( dm_stat == SYNCML_DM_FEATURE_NOT_SUPPORTED )
            return SYNCML_DM_SUCCESS;
        else
            return dm_stat;
    }    
    
    dm_stat = LinkToParentNode(path);
    return dm_stat;    
}

SYNCML_DM_RET_STATUS_T DmtRWPluginTree::CreateLeafNode( CPCHAR path, 
                                                            PDmtNode& ptrCreatedNode, 
                                                            const DmtData& value )
{
    SYNCML_DM_RET_STATUS_T dm_stat;

    dm_stat = LogCommand(SYNCML_DM_PLUGIN_ADD, path, SYNCML_DM_PLUGIN_COMMAND_ON_NODE, NULL);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
    
    dm_stat = this ->CreateLeafNodeInternal(path, ptrCreatedNode, value );
    if (dm_stat != SYNCML_DM_SUCCESS)
    {
        if ( dm_stat == SYNCML_DM_FEATURE_NOT_SUPPORTED )
            return SYNCML_DM_SUCCESS;
        else
            return dm_stat;
    }    
    
    dm_stat = LinkToParentNode(path);
    return dm_stat;    
}

SYNCML_DM_RET_STATUS_T DmtRWPluginTree::CreateInteriorNode( CPCHAR path, PDmtNode& ptrCreatedNode )
{
    SYNCML_DM_RET_STATUS_T dm_stat;
    dm_stat = LogCommand(SYNCML_DM_PLUGIN_ADD, path, SYNCML_DM_PLUGIN_COMMAND_ON_NODE, NULL);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
    
    DMStringVector oChildren;
    dm_stat  = this->CreateInteriorNodeInternal(path, ptrCreatedNode, oChildren);
    if (dm_stat != SYNCML_DM_SUCCESS)
    {
        if ( dm_stat == SYNCML_DM_FEATURE_NOT_SUPPORTED )
            return SYNCML_DM_SUCCESS;
        else
            return dm_stat;
    }   
    
    dm_stat = LinkToParentNode(path);
    return dm_stat;
}

SYNCML_DM_RET_STATUS_T DmtRWPluginTree::Begin()
{
  #ifdef DM_ATOMIC_SUPPORTED   
  SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_SUCCESS;
    m_bIsAtomic = TRUE;
    ret_stat = this->Flush();
    return ret_stat;
  #else 
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
  #endif
  }

SYNCML_DM_RET_STATUS_T DmtRWPluginTree::Commit()
{
   #ifdef DM_ATOMIC_SUPPORTED 
     SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_SUCCESS;

    ret_stat = this->Flush();
    m_bIsAtomic = FALSE;
    return ret_stat;
   #else 
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
   #endif

}

BOOLEAN DmtRWPluginTree::IsAtomic() const
{
    return m_bIsAtomic;
}    

SYNCML_DM_RET_STATUS_T DmtRWPluginTree::Flush() 
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
#ifdef LOB_SUPPORT
    // Verify if all ESN setting are complete.
    if(IsESNSetComplete("") ==  FALSE)
     return     SYNCML_DM_ESN_SET_NOT_COMPLETE;
#endif
    if(this->log != NULL)
    {   
      this->log->RemoveLog();

        delete this->log;
        this->log = NULL;

    }
   if(m_strLogPath != NULL)
       m_strLogPath = NULL;

#ifdef LOB_SUPPORT
    // Commit all  ESN changes.
     dm_stat = CommitESN("");
     m_ESNDirty = FALSE;
#endif
    return dm_stat;
}

SYNCML_DM_RET_STATUS_T DmtRWPluginTree::Rollback() 
{
   #ifdef DM_ATOMIC_SUPPORTED
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    m_Playback = TRUE;

#ifdef TEST_DM_RECOVERY
    if ((power_fail_point != NULL) && (DmStrcmp(power_fail_point, "PLUGIN_PF2") == 0)) 
    {
        printf("Type Ctrl-C to simulate Power Fail ...\n");
        sleep(30);
    }
#endif
    if(this->log != NULL)
    {
    if(fpLog == NULL)
            dm_stat = log->playLog(m_strLogPath.c_str());
    else
    {    dm_stat = log->playLog();
        this->log->RemoveLog();
        fpLog = NULL;
    }
        delete this->log;
        this->log = NULL;
    }
   if(m_strLogPath != NULL)
     m_strLogPath = NULL;

#ifdef TEST_DM_RECOVERY
    if ((power_fail_point != NULL) && (DmStrcmp(power_fail_point, "PLUGIN_PF3") == 0)) 
    {
        printf("Type Ctrl-C to simulate Power Fail ...\n");
        sleep(30);
    }
#endif

    m_Playback = FALSE;
    m_ESNDirty = FALSE;
    m_bIsAtomic = FALSE;
    m_oAddedNodes.clear();    
    return dm_stat;
   #else
   return SYNCML_DM_FEATURE_NOT_SUPPORTED;
   #endif
}


BOOLEAN DmtRWPluginTree::IsPlaybackMode()
{
    return m_Playback;
}

SYNCML_DM_RET_STATUS_T DmtRWPluginTree::SetPlaybackMode(boolean bPlayback)
{
     m_Playback = bPlayback;
     return SYNCML_DM_SUCCESS;
}


DmtRWPluginNode::~DmtRWPluginNode()
{
}

DmtRWPluginNode::DmtRWPluginNode()
{
#ifdef LOB_SUPPORT
  m_LobComplete = TRUE;
  m_LobDirty = FALSE;
  m_LobLogging = FALSE;
  abStorageName = NULL;
#endif  
}

SYNCML_DM_RET_STATUS_T DmtRWPluginNode::SetTitle( CPCHAR szTitle )
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    PDmtRWPluginTree ptrTree = (DmtRWPluginTree *) ((DmtPluginTree *)m_ptrTree);
   
    dm_stat = ptrTree->LogCommand(SYNCML_DM_PLUGIN_REPLACE, 
                                  m_strPath.c_str(), 
                                  SYNCML_DM_PLUGIN_COMMAND_ON_TITLE_PROPERTY, 
                                  (const DmtNode*)this);

    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
 
    DmtAttributes oAttr;
    dm_stat =this->GetAttributes( oAttr );

    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
    
    return m_oAttr.SetTitle(szTitle);
}

SYNCML_DM_RET_STATUS_T DmtRWPluginNode::SetValueInternal( const DmtData& value )
{
    m_oData = value;
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtRWPluginNode::SetValue( const DmtData& value )
{
    XPL_LOG_DM_PLG_Debug(("Enter DmtRWPluginNode::SetValue..."));
    PDmtRWPluginTree ptrTree = (DmtRWPluginTree *) ((DmtPluginTree *)m_ptrTree);
    SYNCML_DM_RET_STATUS_T dm_stat; 
    
    dm_stat= ptrTree->LogCommand(SYNCML_DM_PLUGIN_REPLACE,
                                 m_strPath.c_str(), 
                                 SYNCML_DM_PLUGIN_COMMAND_ON_NODE, 
                                (const DmtNode *)this);
    XPL_LOG_DM_PLG_Debug(("LogCommand returns dm_stat=%d", dm_stat));
    
    if(dm_stat == SYNCML_DM_SUCCESS)
    {  
    INT32 dataSize;
      m_oData = value; 

      // Get data size
      dm_stat = m_oData.GetSize(dataSize);
    XPL_LOG_DM_PLG_Debug(("DmtRWPluginNode::SetValue m_oData.getSize() returns dataSize=%d dm_stat=%d", dataSize, dm_stat));
      if ( dm_stat != SYNCML_DM_SUCCESS )
          return dm_stat;
  
    XPL_LOG_DM_PLG_Debug(("calling m_oAttr.SetSize(%d)", dataSize));
      m_oAttr.SetSize(dataSize);
    }
    
    XPL_LOG_DM_PLG_Debug(("DmtRWPluginNode::SetValue returns dm_stat=%d", dm_stat));
    return dm_stat;
}

SYNCML_DM_RET_STATUS_T DmtRWPluginNode::Rename( CPCHAR szNewName )
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    PDmtRWPluginTree ptrTree = (DmtRWPluginTree *) ((DmtPluginTree *)m_ptrTree);
    
    DMString strURI;
    DMString strKey;
    DmParseURI( m_strPath.c_str(), strURI, strKey );

    DMString strNewURI = strURI;
    if(strNewURI.length() != 0)
        strNewURI += "/";
    strNewURI += szNewName;

    dm_stat = ptrTree->LogCommand(SYNCML_DM_PLUGIN_REPLACE,
                                  (CPCHAR)strNewURI.c_str(), 
                                  SYNCML_DM_PLUGIN_COMMAND_ON_NAME_PROPERTY,
                                  (const DmtNode*)this); 

    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
    
    DMStringVector oChildren;

    dm_stat = m_ptrTree->GetChildNodeNames( strURI.c_str(), oChildren);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
    
    // Is the plugin root node?
    if(strURI.length() == 0)
    {
        for ( INT32 i = 0; i < oChildren.size(); i++ )
        {
            if (!DmStrcmp(oChildren[i].c_str() , m_strPath.c_str()))
            {
                oChildren.remove(i);
                break;
            }
        }
    }
    else 
    {
        for ( INT32 i = 0; i < oChildren.size(); i++ )
        {
            if (!DmStrcmp(oChildren[i].c_str() , strKey.c_str()))
            {
                oChildren.remove(i);
                break;
            }
        }
    }
            
    oChildren.push_back(szNewName);
    PDmtNode ptrParentNode;
    DmtData m_value;

    dm_stat = m_value.SetNodeValue(oChildren);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;

    dm_stat = m_ptrTree->GetNode( strURI.c_str(), ptrParentNode);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;

    PDmtRWPluginNode pRWParentNode = (DmtRWPluginNode *) ((DmtNode *)ptrParentNode);

    DmtData oData;
    dm_stat = pRWParentNode->GetValue( oData );
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;

    // Turn off logging temporarilly
    BOOLEAN needLogging= ptrTree->IsPlaybackMode();
    ptrTree->SetPlaybackMode(TRUE);
    dm_stat = pRWParentNode->SetValue(m_value);
    ptrTree->SetPlaybackMode(needLogging);

    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
    
    dm_stat = pRWParentNode->GetValue( oData );

    m_strName = szNewName;
    if ( szNewName && szNewName[0] )
    {
        if ( m_strName == NULL )
            return SYNCML_DM_DEVICE_FULL;
    }
    
    DmtAttributes oAttr;
    dm_stat = this->GetAttributes( oAttr );
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
   
    dm_stat = m_oAttr.SetName(szNewName);

    if (dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;

    dm_stat = RenameChildNodes(strURI.c_str(), szNewName);
    return dm_stat;
}

SYNCML_DM_RET_STATUS_T DmtRWPluginNode::RenameChildNodes( CPCHAR szParentPath, CPCHAR szNodeName )
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    DMString strURI = szParentPath;
    
    if(strURI.length() != 0)
        strURI += "/";
    strURI += szNodeName;
    
    dm_stat = m_ptrTree->RemoveNode(m_strPath.c_str());
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
    
    
    dm_stat = m_ptrTree->SetNode(strURI.c_str(), this);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;

    if ( m_bLeaf )
    {
        m_strPath = strURI;
        return SYNCML_DM_SUCCESS;
    }


    DMStringVector oChildren;
    dm_stat = m_ptrTree->GetChildNodeNames( strURI.c_str(), oChildren);
    if ( dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
   
    for ( int i = 0; i < oChildren.size(); i++ )
    {
        DMString strChildPath = m_strPath;

        if (strChildPath.length() !=0)
            strChildPath += "/";
        
        strChildPath += oChildren[i];

        PDmtNode ptrNode;

        dm_stat = m_ptrTree->GetNode( strChildPath.c_str(), ptrNode );
        if ( dm_stat != SYNCML_DM_SUCCESS)
            return dm_stat;
        
        PDmtRWPluginNode pRWNode = (DmtRWPluginNode *) ((DmtNode *)ptrNode);

        dm_stat = pRWNode->RenameChildNodes(strURI.c_str(), oChildren[i].c_str());
        if ( dm_stat != SYNCML_DM_SUCCESS)
            return dm_stat;
    }
    m_strPath = strURI;

    return dm_stat;
}

#ifdef LOB_SUPPORT
//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtRWPluginNode::GetFirstChunk
// DESCRIPTION     : 
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : Returns SYNCML_DM_SUCCESS if success, otherwise fails
//
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DmtRWPluginNode::GetFirstChunk(DmtDataChunk&  dmtChunkData) 
{
   // Only for ESN 
    if(!m_bESN)
    return SYNCML_DM_COMMAND_NOT_ALLOWED;
    if(m_LobComplete == FALSE)
        return SYNCML_DM_ESN_SET_NOT_COMPLETE;

    return SYNCML_DM_SUCCESS;
}

//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtRWPluginNode::GetNextChunk
// DESCRIPTION     : 
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : Returns SYNCML_DM_SUCCESS if success, otherwise fails
//
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DmtRWPluginNode::GetNextChunk(DmtDataChunk& dmtChunkData)
{
  // Only for ESN 
   if(!m_bESN)
     return SYNCML_DM_COMMAND_NOT_ALLOWED;
    if(m_LobComplete == FALSE)
        return SYNCML_DM_ESN_SET_NOT_COMPLETE;

    return SYNCML_DM_SUCCESS;
}

//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtRWPluginNode::SetFirstChunk
// DESCRIPTION     : 
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : Returns SYNCML_DM_SUCCESS if success, otherwise fails
//  
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DmtRWPluginNode::SetFirstChunk(DmtDataChunk& dmtChunkData)  
{
  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
  PDmtRWPluginTree ptrTree = (DmtRWPluginTree *) ((DmtPluginTree *)m_ptrTree);
  // Only for ESN 
   if(!m_bESN)
       return SYNCML_DM_COMMAND_NOT_ALLOWED;

  // Need to log the command ?
  if(!ptrTree->IsPlaybackMode())
  {
   if(!m_LobLogging)
   {
         dm_stat = BackupESNData();
         if ( dm_stat != SYNCML_DM_SUCCESS)
           return dm_stat;

          dm_stat = ptrTree->LogCommand(SYNCML_DM_PLUGIN_REPLACE, 
                                    m_strPath.c_str(), 
                                    SYNCML_DM_PLUGIN_COMMAND_ON_LOB_PROPERTY, 
                                    (const DmtNode*)this);
      
          if ( dm_stat != SYNCML_DM_SUCCESS)
          return dm_stat;
    m_LobLogging = TRUE;
   }
  
  m_LobComplete = FALSE;
  m_LobDirty = TRUE;
  //Set flag in the plugin tree
  ptrTree->SetESNDirty();
  }
  return dm_stat;
}

//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtRWPluginNode::SetNextChunk
// DESCRIPTION     : 
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : Returns SYNCML_DM_SUCCESS if success, otherwise fails
//  
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DmtRWPluginNode::SetNextChunk(DmtDataChunk& dmtChunkData)  
{
  PDmtRWPluginTree ptrTree = (DmtRWPluginTree *) ((DmtPluginTree *)m_ptrTree);
 // Only for ESN and SetFirstChunk() is invoked.
 if(!m_bESN )
      return SYNCML_DM_COMMAND_NOT_ALLOWED;
 // Need to log the command ?
 if(!ptrTree->IsPlaybackMode())
 {
     if(!m_LobDirty ||m_LobComplete)
      return SYNCML_DM_INCOMPLETE_COMMAND;
 }
    return SYNCML_DM_SUCCESS;
}

//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtRWPluginNode::SetLastChunk
// DESCRIPTION     : 
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : Returns SYNCML_DM_SUCCESS if success, otherwise fails
//  
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DmtRWPluginNode::SetLastChunk(DmtDataChunk& dmtChunkData)  
{
   PDmtRWPluginTree ptrTree = (DmtRWPluginTree *) ((DmtPluginTree *)m_ptrTree);
  // Only for ESN and SetFirstChunk() is invoked.
   if(!m_bESN )
        return SYNCML_DM_COMMAND_NOT_ALLOWED;

   if(!ptrTree->IsPlaybackMode())
   {    
       if(!m_LobDirty ||m_LobComplete)
        return SYNCML_DM_INCOMPLETE_COMMAND;

       m_LobComplete = TRUE;
   }
   return SYNCML_DM_SUCCESS;
}

//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtRWPluginNode::Commit
// DESCRIPTION     : Commit changes for an ESN 
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : Returns SYNCML_DM_SUCCESS if success, otherwise fails
// 
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DmtRWPluginNode::Commit()
{
   SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
   // Only for ESN 
   if(m_bESN)
   {  
       m_LobComplete = TRUE;
       m_LobDirty = FALSE;
       m_LobLogging = FALSE;
    // Remove temporary storage file
    if(abStorageName.length() != 0) 
    {    
        DMFileHandler sourceHandle(abStorageName);
        dm_stat = sourceHandle.open(XPL_FS_FILE_RDWR);
        if(dm_stat == SYNCML_DM_SUCCESS)
                sourceHandle.deleteFile();
        abStorageName = NULL;
    }
   }
   return SYNCML_DM_SUCCESS;
}

//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtRWPluginNode::Delete
// DESCRIPTION     : Delete the node 
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : Returns SYNCML_DM_SUCCESS if success, otherwise fails
// 
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DmtRWPluginNode::Delete()
{
   return SYNCML_DM_SUCCESS;
}

//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtRWPluginNode::BackupESNData
// DESCRIPTION     : Backup ESN data to temporary file
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : Returns SYNCML_DM_SUCCESS if success, otherwise fails
//  
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DmtRWPluginNode::BackupESNData()
{
 SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
 PDmtRWPluginTree ptrTree = (DmtRWPluginTree *) ((DmtPluginTree *)m_ptrTree);
 DmtDataChunk chunkData;
 UINT32 getLen;
 UINT8 *bufp;

  if(!m_bESN || m_LobLogging)
    return SYNCML_DM_SUCCESS;

  DmtAttributes oAttr;
  dm_stat =this->GetAttributes( oAttr );
  // Is the ESN empty
  if(oAttr.GetSize() == 0)
  {
    abStorageName = NULL;
    return SYNCML_DM_SUCCESS;
  }
  
 // No internal file created yet
 if(abStorageName.length() == 0) {
       dm_stat = DMFileHandler::createTempESNFileName(abStorageName);
       if(dm_stat != SYNCML_DM_SUCCESS)
           return dm_stat;
 }
 DMFileHandler tempLogFileHandler(abStorageName);
 dm_stat = tempLogFileHandler.open(XPL_FS_FILE_WRITE);
 if(dm_stat != SYNCML_DM_SUCCESS)
     return    SYNCML_DM_IO_FAILURE;

 // Allocate chunk buffer
 if(chunkData.AllocateChunkBuffer() != SYNCML_DM_SUCCESS)
    return  SYNCML_DM_DEVICE_FULL;
 
 dm_stat = GetFirstChunk(chunkData);  
 if( dm_stat != SYNCML_DM_SUCCESS)
     return dm_stat;
 chunkData.GetReturnLen(getLen);     
 chunkData.GetChunkData(&bufp);  // the chunk data is available  

  while (true) 
 {
      // Get the last chunk of data
      if (getLen == 0)
              break;
      if(tempLogFileHandler.seek(XPL_FS_SEEK_END, 0) != SYNCML_DM_SUCCESS) 
      {
         dm_stat =    SYNCML_DM_IO_FAILURE;
        break;
      }
      if(tempLogFileHandler.write((CPCHAR)bufp, getLen) != SYNCML_DM_SUCCESS) 
      {
        dm_stat =    SYNCML_DM_IO_FAILURE;
        break;
      }
      dm_stat = GetNextChunk(chunkData);
      if( dm_stat != SYNCML_DM_SUCCESS)
          break;
      chunkData.GetReturnLen(getLen); 
      chunkData.GetChunkData(&bufp);
  }
 
  if(dm_stat != SYNCML_DM_SUCCESS)
  {     tempLogFileHandler.deleteFile();
      abStorageName = NULL;
  }
  else
      tempLogFileHandler.close();

 //Set flag in the plugin tree
 ptrTree->SetESNDirty();
 return  dm_stat;
}
//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtRWPluginNode::RestoreESNData
// DESCRIPTION     : Restore ESN data from temporary file
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : Returns SYNCML_DM_SUCCESS if success, otherwise fails
//  
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DmtRWPluginNode::RestoreESNData( CPCHAR szBackupFileName )
{
 SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
 UINT32 totalSize;
 DmtDataChunk chunkData;     
 int setLen = 0;
 int offset = 0;
 bool isFirstChunk = true;
 bool isLastChunk = false;
 UINT8 *bufp;

 if(!m_bESN )
     return SYNCML_DM_SUCCESS;

 int chunksize = chunkData.GetChunkSize();

  m_LobComplete = TRUE;
  m_LobDirty = FALSE;
  m_LobLogging = FALSE;

  // No internal storage file
  if(DmStrlen(szBackupFileName) == 0)
  {
      dm_stat = SetFirstChunk(chunkData);  
     if(dm_stat != SYNCML_DM_SUCCESS)
         return dm_stat;
      dm_stat = SetLastChunk(chunkData);  
     abStorageName = NULL;

     return dm_stat;
  }

  if(!XPL_FS_Exist(szBackupFileName))
    return SYNCML_DM_SUCCESS;


 DMFileHandler tempFileHandler(szBackupFileName);
 dm_stat = tempFileHandler.open(XPL_FS_FILE_RDWR);
 if(dm_stat != SYNCML_DM_SUCCESS)
     return    SYNCML_DM_IO_FAILURE;
 totalSize = tempFileHandler.size();

 // Allocate chunk buffer
 if(chunkData.AllocateChunkBuffer() != SYNCML_DM_SUCCESS)
    return  SYNCML_DM_DEVICE_FULL;

 chunkData.GetChunkData(&bufp);  // the chunk data is available  


 while(!isLastChunk)
 {     setLen =  totalSize- offset;
     if(setLen > 0)  
     {
         if(setLen > chunksize)
             setLen = chunksize;
     }
     else
         isLastChunk = true;

     if(tempFileHandler.seek(XPL_FS_SEEK_SET, offset) != SYNCML_DM_SUCCESS) 
         return  SYNCML_DM_IO_FAILURE;
     if(tempFileHandler.read(bufp, setLen) != SYNCML_DM_SUCCESS) 
         return  SYNCML_DM_IO_FAILURE;
 
     dm_stat = chunkData.SetChunkData((const UINT8 *)bufp, setLen);
     if(dm_stat != SYNCML_DM_SUCCESS)
         return    dm_stat;

     if(isFirstChunk)
     {     
         dm_stat = SetFirstChunk(chunkData);  
         isFirstChunk = false;
     }
     else
     {     if(!isLastChunk)
             dm_stat = SetNextChunk(chunkData);     
         else
             dm_stat = SetLastChunk(chunkData);     
     }
     if(dm_stat != SYNCML_DM_SUCCESS)
         return    dm_stat;
 
        offset += setLen;
    }

    totalSize = tempFileHandler.deleteFile();
    abStorageName = NULL;
    return dm_stat;
}

#endif
