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

//------------------------------------------------------------------------
//
//   Module Name: dm_tree_node_class_public.cc
//
//   General Description: Contains the implementations of the Public
//                        methods of DMNTree class.
//------------------------------------------------------------------------

#include "dmprofile.h"
#include "dm_tree_util.h"
#include "dmLockingHelper.h"
#include "dm_tree_class.H"
#include "SyncML_DM_Archiver.H"
#include "dm_tree_default_leaf_node_class.H"
#ifdef LOB_SUPPORT
#include "dm_tree_default_ESN_class.H"
#endif
#include "dm_tree_default_interior_node_class.H"
#include "dm_uri_utils.h"
#include "xpl_Logger.h"
#include "dmtTreeImpl.hpp"
#include "dm_tree_plugin_root_node_class.H"
#include "dm_tree_plugin_util.H"
#include "dmtoken.h"
#include "xpl_dm_Manager.h"

//------------------------------------------------------------------------
//                           LOCAL VARIABLES
//------------------------------------------------------------------------
const UINT8 *DMTree::m_pDataFormatTable[] =
{
    (UINT8 *)"null", // SYNCML_DM_FORMAT_NULL
    (UINT8 *)"chr",  //SYNCML_DM_FORMAT_CHR,
    (UINT8 *)"int",  //SYNCML_DM_FORMAT_INT,
    (UINT8 *)"bool", //SYNCML_DM_FORMAT_BOOL,
    (UINT8 *)"bin",  //SYNCML_DM_FORMAT_BIN,
    (UINT8 *)"node", //SYNCML_DM_FORMAT_NODE,
    (UINT8 *)"b64",  //SYNCML_DM_FORMAT_B64,
    (UINT8 *)"xml",  //SYNCML_DM_FORMAT_XML,
    (UINT8 *)"date", //SYNCML_DM_FORMAT_DATE,
    (UINT8 *)"test", //SYNCML_DM_FORMAT_TEST,
    (UINT8 *)"time", //SYNCML_DM_FORMAT_TIME,
    (UINT8 *)"float",//SYNCML_DM_FORMAT_FLOAT,
    (UINT8 *)NULL,   //SYNCML_DM_FORMAT_INVALID
};

//------------------------------------------------------------------------
// FUNCTION         :   LogEvent
// DESCRIPTION      :   Logging the DELETE command which was executed successfully
// ARGUMENTS PASSED :   pbURI - updated node
//                                  aDeletedChildren - deleted children nodes
// RETURN VALUE     :   SYNCML_DM_RET_STATUS_T
//                      SYNCML_DM_SUCCESS or SYNCML_DM_FAIL if some
//                      failure in logCommand occured
// PRE-CONDITIONS   :   The command was successful
//                      Tree was updated accordingly
// POST-CONDITIONS  :   the Archiver validates the log and will use it
//                      while serializing the tree
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T
DMTree::LogEvent(CPCHAR pbURI, const DMStringVector & aDeletedChildren)
{
    SyncML_DM_Archive * pArchive = m_oArchiver.getArchiveByURI(pbURI);

    if (pArchive == NULL)
        return SYNCML_DM_FAIL;

    pArchive->setDirty(TRUE);

    return m_oEvtObj.OnNodeDeleted(pArchive, pbURI, aDeletedChildren);
}

//------------------------------------------------------------------------
// FUNCTION         :   LogEvent
// DESCRIPTION      :   Logging the command which was executed
//                      successfully in the log file
//                      sets the command type, URI and pointer to a copy
//                      of the node and calls the logCommand method of
//                      archiver
// ARGUMENTS PASSED :   commandType ADD/REPLACE/DELETE/RENAME/INDIRECT UPDATE
//                                  pbURI - updated node
//                                  szNewName - new name
// RETURN VALUE     :   SYNCML_DM_RET_STATUS_T
//                      SYNCML_DM_SUCCESS or SYNCML_DM_FAIL if some
//                      failure in logCommand occured
// PRE-CONDITIONS   :   The command was successful
//                      Tree was updated accordingly
// POST-CONDITIONS  :   the Archiver validates the log and will use it
//                      while serializing the tree
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T
DMTree::LogEvent(SYNCML_DM_EVENT_ACTION_T eEvent,
                               CPCHAR pbURI,
                               CPCHAR szNewName)
{
    SyncML_DM_Archive * pArchive = m_oArchiver.getArchiveByURI(pbURI);

    if (pArchive == NULL)
        return SYNCML_DM_FAIL;

    pArchive->setDirty(TRUE);

    return m_oEvtObj.OnNodeChanged( pArchive, pbURI, eEvent, szNewName );
}

//------------------------------------------------------------------------
// FUNCTION         :   LogESNCommandForArchiver
// DESCRIPTION     :   Logging the command which was executed
//                      successfully in the log file
//                      sets the command type, URI and pointer to a copy
//                      of the node and calls the logCommand method of
//                      archiver
// ARGUMENTS PASSED :   pbURI - updated node
//                                  inNode - updated node
// RETURN VALUE     :   SYNCML_DM_RET_STATUS_T
//                      SYNCML_DM_SUCCESS or SYNCML_DM_FAIL if some
//                      failure in logCommand occured
// PRE-CONDITIONS   :   The command was successful
//                      Tree was updated accordingly
// POST-CONDITIONS  :   the Archiver validates the log and will use it
//                      while serializing the tree
//------------------------------------------------------------------------
#ifdef LOB_SUPPORT
SYNCML_DM_RET_STATUS_T
DMTree::LogESNCommandForArchiver(CPCHAR pbURI, DMNode * inNode)
{
    SyncML_DM_Archive * pArchive = m_oArchiver.getArchiveByURI(pbURI);
    SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;

    if (pArchive == NULL)
        return SYNCML_DM_FAIL;

    DMDefaultESN  *tempESN = reinterpret_cast< DMDefaultESN *>(inNode);;
    SyncML_Commit_Log *commitLog =  pArchive->GetCommitLogHandler();
    if(commitLog != NULL && tempESN->NeedLogging())
       retStatus = commitLog->logCommand(SYNCML_DM_REPLACE, tempESN->GetOriginalInternalFileName() , tempESN->GetInternalStorageFileName());

    return retStatus;
}
#endif

SYNCML_DM_RET_STATUS_T DMTree::ConvertFormat(SYNCML_DM_FORMAT_T format,
        DMString & strFormat)
{
    strFormat = format >= sizeof(m_pDataFormatTable)/sizeof(int) ?
                NULL : (CPCHAR)m_pDataFormatTable[format];

    if ( strFormat == NULL && format != SYNCML_DM_FORMAT_NULL)
        return SYNCML_DM_DEVICE_FULL;
    else
        return SYNCML_DM_SUCCESS;
}

SYNCML_DM_FORMAT_T DMTree::ConvertFormatStr(const DMString& formatStr)
{
   SYNCML_DM_FORMAT_T bFormat = SYNCML_DM_FORMAT_NODE;

   if (formatStr == "chr" || formatStr == "string")
   {
      bFormat = SYNCML_DM_FORMAT_CHR;
   } else
   if (formatStr == "int" )
   {
      bFormat = SYNCML_DM_FORMAT_INT;
   } else
   if (formatStr == "bool" )
   {
      bFormat = SYNCML_DM_FORMAT_BOOL;
   } else
   if (formatStr == "b64" )
   {
      bFormat = SYNCML_DM_FORMAT_B64;
   } else
   if (formatStr == "bin" )
   {
      bFormat = SYNCML_DM_FORMAT_BIN;
   } else
   if (formatStr == "node" )
   {
      bFormat = SYNCML_DM_FORMAT_NODE;
   } else
   if (formatStr == "xml" )
   {
      bFormat = SYNCML_DM_FORMAT_XML;
   } else
   if (formatStr == "date" )
   {
      bFormat = SYNCML_DM_FORMAT_DATE;
   } else
   if (formatStr == "time" )
   {
      bFormat = SYNCML_DM_FORMAT_TIME;
   } else
   if (formatStr == "float" )
   {
      bFormat = SYNCML_DM_FORMAT_FLOAT;
   } else
   if (formatStr == "null" )
   {
      bFormat = SYNCML_DM_FORMAT_NULL;
   }
   else
   {
      bFormat = SYNCML_DM_FORMAT_CHR;  //Cover unknown Meta data format
   }

   return bFormat;
}

BOOLEAN DMTree::ParentExistsForPluginNode(CPCHAR pURI, DMNode *psNode) const
{
    DMString strParentURI = pURI, strLastSegment;

    if ( !GetLastSegmentOfURI(strParentURI, strLastSegment) )
      return FALSE;

    if ( strParentURI == "." )
      return TRUE;

    DMGetData getData;

    if (psNode->Get(strParentURI, getData) != SYNCML_DM_SUCCESS)
      return FALSE;

    return TRUE;
}


void DMTree::DetachNodeFromTree(DMNode *psDeletingNode)
{
    DMNode *psParentNode = NULL;
    DMNode *psPrevNode = NULL;
    DMNode *psListNode = NULL;

    psParentNode = psDeletingNode->pcParentOfNode;
    psListNode = psParentNode->pcFirstChild;

    // This while stores the previous pointer to the deleting node.
    while(psListNode)
    {
       if(psListNode == psDeletingNode)
         break;
      psPrevNode = psListNode;
      psListNode = psListNode->pcNextSibling;
    }

    if(psListNode == psDeletingNode)
    {
       if(psPrevNode != NULL)
       {
         // If previous node is not equals to NULL then we will
         // set the deleting node next pointer to the next node
         // pointer of previous node
         psPrevNode->pcNextSibling = psDeletingNode->pcNextSibling;
       }
       else
       {
         // If the previous node is NOT there mean then deleting
         // node is first child in the siblings list.
         psParentNode->pcFirstChild =  psDeletingNode->pcNextSibling;
       }
       psDeletingNode->pcParentOfNode=NULL;
       psDeletingNode->pcNextSibling=NULL;

    }// End of if(psListNode == psDeletingNode)

}

// DMTree constructor
DMTree::DMTree()
  : m_init_status( SYNCML_DM_FAIL ),
    m_psRoot( NULL ),
    m_wMaxDepth( SYNCML_DM_URI_MAX_DEPTH ),
    m_wMaxTotLen( SYNCML_DM_URI_MAX_TOTAL_LENGTH ),
    m_wMaxSegLen( SYNCML_DM_URI_MAX_SEGMENT_LENGTH ),
    m_nRefCount( 0 ),
    m_currentTime( 0 )
{
  m_strPrincipal = "";

  m_oOPICacheData.metaNodeID = -1;
  m_pOPINode = NULL;
  m_bVersion_1_2 = FALSE;
#ifdef LOB_SUPPORT
  m_pESN = NULL;
  m_strESNPath= "";
#endif
}

// DMTree destructor
DMTree::~DMTree()
{
    DeInit(TRUE);
}

/*==================================================================================================
FUNCTION        : DMTree::SetServerId

DESCRIPTION     : This function will set the server Id.
ARGUMENT PASSED : p_ServerId
OUTPUT PARAMETER:
RETURN VALUE    : SYNCML_DM_SUCCESS if success,
                  SYNCML_DM_FAIL if allocating memory failed.
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMTree::SetServerId (CPCHAR p_ServerId)
{
  m_strPrincipal = p_ServerId;
  return SYNCML_DM_SUCCESS;
}

/*==================================================================================================
FUNCTION        : DMTree::SetConRef

DESCRIPTION     : This function will set the ConRef
ARGUMENT PASSED : p_ConRef
OUTPUT PARAMETER:
RETURN VALUE    : SYNCML_DM_SUCCESS if success,
                  SYNCML_DM_FAIL if allocating memory failed.
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMTree::SetConRef (CPCHAR p_ConRef)
{
  //m_strPrincipal = p_ConRef;
  m_strConRef = p_ConRef;
  return SYNCML_DM_SUCCESS;
}

BOOLEAN DMTree::IsInitialized() const
{
    return (SYNCML_DM_SUCCESS == m_init_status);
}

//------------------------------------------------------------------------
// FUNCTION        : Init
// DESCRIPTION     : The UA calls this method once the syncml task is up
//                   The DMTNM initializes it's data members and also
//                   creates a timer
// ARGUMENTS PASSED: None
// RETURN VALUE    : void
// PRE-CONDITIONS  : syncml task is ready
// POST-CONDITIONS : the Tree obtains a timer handle and resets it's flags
// IMPORTANT NOTES :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMTree::Init()
{
#ifndef DM_NO_LOCKING
  DMSingleLock oLock( m_csInitLock );
#endif

  XPL_LOG_Startup();

  SYNCML_DM_RET_STATUS_T result = SYNCML_DM_SUCCESS;

  if ( 0 == m_nRefCount )
  {
    result = InternalInit();
    m_init_status = result;
  }

  if( SYNCML_DM_SUCCESS == result )
  {
    m_nRefCount++;
    DM_MEMORY_STATISTICS_WRITE("Initialize\n");
  }
  else
  {
    XPL_LOG_DM_TMN_Error(("Initializing failed\n"));

    // DM: reverting to uninitialized state partially initialized object
    InternalDeInit();
  }

  return result;
}

SYNCML_DM_RET_STATUS_T DMTree::InternalInit()
{
  SYNCML_DM_RET_STATUS_T result = SYNCML_DM_SUCCESS;

  for (;;)
  {
    DM_PROFILE( "DmMnInit" );


    if( IsInitialized() )
    {
      XPL_LOG_DM_TMN_Error(("Already initialized\n"));
      result = SYNCML_DM_FAIL;
      break;
    }

    XPL_LOG_DM_TMN_Debug(("Initializing XPL...\n"));
    result = XPL_DM_Init();
    if( SYNCML_DM_SUCCESS != result ) break;

    XPL_LOG_DM_TMN_Debug(("Initializing environment...\n"));
    if( !m_oEnv.Init() )
    {
      result = SYNCML_DM_FAIL;
      break;
    }

    CPCHAR dm_ver = XPL_DM_GetEnv(SYNCML_DM_VERSION);
    m_bVersion_1_2 = ( dm_ver && DmStrcmp(dm_ver,SYNCML_REP_PROTOCOL_VERSION_1_2) == 0 );

    XPL_LOG_DM_TMN_Debug(("DM version is %s\nMounting tree...\n", m_bVersion_1_2 ? "1.2" : "1.1.2" ));
    DM_PERFORMANCE(DM_INITIALIZE_MOUNT);
    result = m_oTreeMountObj.MountTree( &m_oEnv, this );
    if( SYNCML_DM_SUCCESS != result ) break;

    XPL_LOG_DM_TMN_Debug(("Initializing MDF and Plugin Manager...\n"));
    DM_PERFORMANCE(DM_INITIALIZE_MDF);
    result = m_oMDFObj.Init( &m_oEnv, this );
    if( SYNCML_DM_SUCCESS != result ) break;

    DM_PERFORMANCE(DM_INITIALIZE_PLUGIN);
    result = m_oPluginManager.Init( &m_oEnv, this );
    if( SYNCML_DM_SUCCESS != result ) break;

    XPL_LOG_DM_TMN_Debug(("Initializing Archiver and File Manager...\n"));
    DM_PERFORMANCE(DM_INITIALIZE_ACRHIVER);
    result = m_oArchiver.initArchives( &m_oEnv, this );
    if( SYNCML_DM_SUCCESS != result ) break;

    DM_PERFORMANCE(DM_INITIALIZE_FILE);
    result = m_oFileManager.Init( this );
    if( SYNCML_DM_SUCCESS != result ) break;

    XPL_LOG_DM_TMN_Debug(("Initializing Lock Context Manager and ACL...\n"));
    DM_PERFORMANCE(DM_INITIALIZE_LOCK);
    result = m_oLockContextManager.Init( this, &m_oFileManager );
    if( SYNCML_DM_SUCCESS != result ) break;

    DM_PERFORMANCE(DM_INITIALIZE_ACL);
    result = m_oACLObj.Init( &m_oEnv, this );
    if( SYNCML_DM_SUCCESS != result ) break;

    DM_PERFORMANCE(DM_INITIALIZE_EVENT);
    result = m_oEvtObj.Init( &m_oEnv, this );
    if( SYNCML_DM_SUCCESS != result ) break;

    DM_PERFORMANCE(DM_INITIALIZE_LOAD);
    // LoadMaxValues();

    if ( DmGetMemFailedFlag() ) // device runs out of memory
    {
      result = SYNCML_DM_DEVICE_FULL;
      break;
    }

    result = SYNCML_DM_SUCCESS;
    break;
  }

  if( SYNCML_DM_SUCCESS == result )
  {
    XPL_LOG_DM_TMN_Debug(("Initialized successfully\n"));
  }
  else
  {
    XPL_LOG_DM_TMN_Error(( "Initialization faled\n" ));
  }

  return result;
}

UINT16 DMTree::GetMaxPathDepth() const
{
  return m_wMaxDepth;
}

UINT16 DMTree::GetMaxTotalPathLength() const
{
  return m_wMaxTotLen;
}

UINT16 DMTree::GetMaxPathSegmentLength() const
{
  return m_wMaxSegLen;
}

void DMTree::LoadMaxValues()
{
    // special case for some nodes
    int nLock = 0;
    {
     DMLockingHelper oLock( 0, "./DevDetail/URI", "localhost", SYNCML_DM_LOCK_TYPE_SHARED, FALSE );
     nLock = oLock.GetID();

     if ( oLock.IsLockedSuccessfully() )
     {
        DMNode* pNode = FindNodeByURI("./DevDetail/URI/MaxDepth");

        m_wMaxDepth = readOneWordFromTree(pNode, SYNCML_DM_URI_MAX_DEPTH);

        pNode = FindNodeByURI("./DevDetail/URI/MaxTotLen");

        m_wMaxTotLen = readOneWordFromTree(pNode, SYNCML_DM_URI_MAX_TOTAL_LENGTH);

        pNode = FindNodeByURI("./DevDetail/URI/MaxSegLen");

        m_wMaxSegLen = readOneWordFromTree(pNode, SYNCML_DM_URI_MAX_SEGMENT_LENGTH);

    }
   }

   ReleaseLock( nLock );
}

SYNCML_DM_RET_STATUS_T DMTree::DeInit(BOOLEAN bIsDestructor)
{
#ifndef DM_NO_LOCKING
  DMSingleLock oLock( m_csInitLock );
#endif

  if ( m_nRefCount < 1 )
  {
     if (bIsDestructor == FALSE)
     {
        XPL_LOG_DM_TMN_Error(("Invalid ref count = %i\n", m_nRefCount ));
     }
     return SYNCML_DM_FAIL;

  }

  m_nRefCount--;

  XPL_LOG_DM_TMN_Debug(("Uninitializing, ref count = %i\n", m_nRefCount ));

  if ( ( 0 == m_nRefCount ) && IsInitialized() )
  {
    InternalDeInit();

    DM_MEMORY_STATISTICS_REPORT_LEAKS
  }

  XPL_LOG_Shutdown();

  return SYNCML_DM_SUCCESS;
}

void DMTree::InternalDeInit()
{
  XPL_LOG_DM_TMN_Debug(("Uninitializing, releasing locks...\n" ));
  m_oLockContextManager.ReleaseAll();

  XPL_LOG_DM_TMN_Debug(("Uninitializing ACL...\n" ));
  m_oACLObj.DeInit();

  XPL_LOG_DM_TMN_Debug(("Uninitializing lock context manager...\n" ));
  m_oLockContextManager.Destroy();

  XPL_LOG_DM_TMN_Debug(("Uninitializing file manager...\n" ));
  m_oFileManager.DeInit();

  XPL_LOG_DM_TMN_Debug(("Uninitializing Archiver...\n" ));
  m_oArchiver.deinitArchives();

  XPL_LOG_DM_TMN_Debug(("Uninitializing, reseting cache...\n" ));
#ifdef LOB_SUPPORT
  ResetESNCache();
#endif
  ResetOPICache();
  m_oOPICacheData.aPD.clear();

  XPL_LOG_DM_TMN_Debug(("Uninitializing, deleting nodes...\n" ));
  if( m_psRoot )
  {
    DeleteNodesFromTree( m_psRoot );
    m_psRoot = NULL;
  }

 XPL_LOG_DM_TMN_Debug(("Uninitializing Plugin manager...\n" ));
 m_oPluginManager.DeInit();

  XPL_LOG_DM_TMN_Debug(("Uninitializing MDF...\n" ));
  m_oMDFObj.DeInit();

  XPL_LOG_DM_TMN_Debug(("Uninitializing: unmounting trees...\n" ));
  m_oTreeMountObj.UnMountTree();

  XPL_LOG_DM_TMN_Debug(("Uninitializing environment...\n" ));
  m_oEnv.Done();

  XPL_LOG_DM_TMN_Debug(("Uninitializing XPL...\n" ));
  XPL_DM_DeInit();

  m_strPrincipal = "";
  m_init_status = SYNCML_DM_FAIL;
  XPL_LOG_DM_TMN_Debug(("Uninitializing : succeeded\n" ));

}

// force to unload all loaded sub-trees and plug-ins
SYNCML_DM_RET_STATUS_T DMTree::Flush()
{
  XPL_LOG_DM_TMN_Debug(("DMTree::Flush\n"));
  ResetOPICache();
  m_oOPICacheData.aPD.clear();

  if( m_psRoot )
  {
    DeleteNodesFromTree( m_psRoot );
  }

  m_oPluginManager.DeInit();
  return m_oPluginManager.Init( &m_oEnv, this );
}

void DMTree::GetTreeMountEntry (CPCHAR &p_Uri,  CPCHAR& p_TreePath,  UINT16   index) const
{
  m_oTreeMountObj.GetTreeMountEntry( p_Uri, p_TreePath, index );
}

DMPluginManager & DMTree::GetPluginManager()
{
  return m_oPluginManager;
}

SyncML_DM_Archiver& DMTree::GetArchiver()
{
  return m_oArchiver;
}

DMLockContextManager& DMTree::GetLockContextManager()
{
  return m_oLockContextManager;
}

DMSubscriptionManager &  DMTree::GetSubscriptionManager()
 {
    return m_oEvtObj;
 }

SYNCML_DM_RET_STATUS_T DMTree::ReleaseLock( INT32 nLockID ,SYNCML_DM_COMMAND_T command /* = SYNCML_DM_RELEASE*/)
{
  return m_oLockContextManager.ReleaseID( nLockID, command );
}


void DMTree::CheckMemoryAging()
{
  m_oLockContextManager.CheckMemoryAging();
}


SYNCML_DM_RET_STATUS_T DMTree::SaveFile(SYNCML_DM_FILE_TYPE_T eFileType)
{
    if ( eFileType == SYNCML_DM_FILE_ACL )
        return m_oACLObj.Serialize();
    else
        return m_oEvtObj.Serialize();

}

SYNCML_DM_RET_STATUS_T DMTree::RevertFile(SYNCML_DM_FILE_TYPE_T eFileType)
{
    if ( eFileType == SYNCML_DM_FILE_ACL )
        return m_oACLObj.Revert();
    else
        return m_oEvtObj.Revert();
}


CPCHAR DMTree::GetWritableFileSystemFullPath( DMString & path )
{
  return m_oEnv.GetWFSFullPath( NULL, path );
}


SYNCML_DM_RET_STATUS_T
DMTree::SetACL(CPCHAR szURI)
{
    //This variable stores the length of the ACL
    UINT16   totalLength = 0;
    UINT16   length = 0;
    DMBuffer oACL;

    length = m_strPrincipal.length();

    totalLength = ADD_CMD_LENGTH_IN_ACL + length + 1 +
                       GET_CMD_LENGTH_IN_ACL + length + 1 +
                       REPLACE_CMD_LENGTH_IN_ACL + length + 1 +
                       DELETE_CMD_LENGTH_IN_ACL + length + 1 +
                       EXEC_CMD_LENGTH_IN_ACL + length + 1;

    if ( oACL.allocate(totalLength) == NULL )
        return(SYNCML_DM_DEVICE_FULL);


    oACL.assign(ADD_CMD_IN_ACL);
    oACL.append((UINT8*) m_strPrincipal.GetBuffer(),length);
    oACL.append((UINT8*)"&",1);

    oACL.append((UINT8*)GET_CMD_IN_ACL,GET_CMD_LENGTH_IN_ACL);
    oACL.append((UINT8*)m_strPrincipal.GetBuffer(),length);
    oACL.append((UINT8*)"&",1);

    oACL.append((UINT8*)REPLACE_CMD_IN_ACL,REPLACE_CMD_LENGTH_IN_ACL);
    oACL.append((UINT8*)m_strPrincipal.GetBuffer(),length);
    oACL.append((UINT8*)"&",1);

    oACL.append((UINT8*)DELETE_CMD_IN_ACL,DELETE_CMD_LENGTH_IN_ACL);
    oACL.append((UINT8*)m_strPrincipal.GetBuffer(),length);
    oACL.append((UINT8*)"&",1);

    oACL.append((UINT8*)EXEC_CMD_IN_ACL,EXEC_CMD_LENGTH_IN_ACL);
    oACL.append((UINT8*)m_strPrincipal.GetBuffer(),length);

    m_oACLObj.SetACL(szURI, (CPCHAR)oACL.getBuffer());

    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T
DMTree::AddAutoNodes(CPCHAR szURI,
                                 SYNCML_DM_REQUEST_TYPE_T eRequestType,
                                 DMToken & oAutoNodes)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    if ( oAutoNodes.getBuffer() == NULL )
        return SYNCML_DM_SUCCESS;

    DMString autoNodeURI;
    DMAddData oAddChildren;
    CPCHAR sSegment = NULL;

    while ( (sSegment = oAutoNodes.nextSegment()) != NULL )
    {
        autoNodeURI = szURI;
        autoNodeURI += "/";
        autoNodeURI += sSegment;

        dm_stat = m_oMDFObj.SetAutoNodeProperty(autoNodeURI,oAddChildren);
        if ( dm_stat != SYNCML_DM_SUCCESS )
            return dm_stat;

        dm_stat = Add( oAddChildren,eRequestType);
        if ( dm_stat != SYNCML_DM_SUCCESS )
            return dm_stat;
   }

   return dm_stat;
}

//------------------------------------------------------------------------
// FUNCTION        : Add
// DESCRIPTION     : The UA calls this method to add a node/data in the
//                   tree.The UA need not call URIValidateAndParse
//                   for ADD operation.DMTNM will internally call before
//                   adding
// ARGUMENTS PASSED: SYNCML_DM_PLUGIN_ADD_T *pAdd - pointer to adding node
//                                                  information.

//If default value then pAdd->bFormat=SYNCML_DM_FORMAT_INVALID
//                                          contains value TRUE.
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T
//                              - It returns SYNCML_DM_SUCCESS if the plug-in
//                                adds the node succefully.
//                              - It returns SYNCML_DM_DEVICE_FULL if the
//                                device is out of memory.
//                              - It returns SYNCML_DM_COMMAND_NOT_ALLOWED
//                                if the commad is NOT allowed because
//                                Access type and etc.
//                              - It returns SYNCML_DM_TARGET_ALREADY_EXISTS
//                                if the node already exists.
//                              - It returns SYNCML_DM_INVALID_URI
//                                if the node path is not valid for current DMT version
// PRE-CONDITIONS  : The Tree is in memory
// POST-CONDITIONS : the node is added in the tree only if the plug-in
//                   returns SYNCML_DM_SUCCESS
//                   else the object is deleted.
//                   after adding the node successfully,the DMTNM sets the
//                   Name and ACL and Access Type properties.It uses the
//                   access type sent by plug-in.
// IMPORTANT NOTES : PHASE 1 implementation
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMTree::Add( DMAddData & oAddData,
                                    SYNCML_DM_REQUEST_TYPE_T eRequestType)
{
  XPL_LOG_DM_TMN_Debug(("Enter DMTree::Add(DMAddData&, SYNCML_DM_REQUEST_TYPE_T)"));
    SYNCML_DM_RET_STATUS_T wReturnStatusCode = AddNodeInternal(oAddData, eRequestType);

    if ( wReturnStatusCode == SYNCML_DM_SUCCESS || wReturnStatusCode != SYNCML_DM_NOT_FOUND )
    {
       return wReturnStatusCode;
    }

    // Add interior node(s) in the chain
    DMString strURI = oAddData.getURI();
    DMString strLastInteriorNodeAdded = NULL;
    DMVector<DMAddData *> interiorNodeList;
    DMAddData *pAddInteriorNodeData;
    while ( GetLastSegmentOfURI(strURI, strLastInteriorNodeAdded ) )
    {
       pAddInteriorNodeData = new DMAddData();
       if ( NULL == pAddInteriorNodeData )
       {
          wReturnStatusCode = SYNCML_DM_DEVICE_FULL;
          break;
       }
       pAddInteriorNodeData->m_oURI.assign(strURI.c_str());
       pAddInteriorNodeData->m_nFormat = SYNCML_DM_FORMAT_NODE;
       wReturnStatusCode = AddNodeInternal(*pAddInteriorNodeData, eRequestType);
       if ( wReturnStatusCode == SYNCML_DM_SUCCESS )
       {
          strLastInteriorNodeAdded = strURI;
          XPL_LOG_DM_TMN_Debug(("DMTree::Add: set strLastInteriorNodeAdded=%s\n", strLastInteriorNodeAdded.c_str() ));
          pAddInteriorNodeData->clear();
          delete pAddInteriorNodeData;
          break;
       }
       interiorNodeList.push_back(pAddInteriorNodeData);
    }

    while ( interiorNodeList.size() > 0 )
    {
       int idx = interiorNodeList.size() - 1;
       pAddInteriorNodeData = interiorNodeList[idx];
       if ( wReturnStatusCode == SYNCML_DM_SUCCESS )
       {
          wReturnStatusCode = AddNodeInternal(*pAddInteriorNodeData, eRequestType);
       }
       pAddInteriorNodeData->clear();
       delete pAddInteriorNodeData;
       interiorNodeList.remove(idx);
    }

    // Add target node again
    if ( wReturnStatusCode == SYNCML_DM_SUCCESS )
    {
       wReturnStatusCode = AddNodeInternal(oAddData, eRequestType);
    }

    if ( wReturnStatusCode != SYNCML_DM_SUCCESS && strLastInteriorNodeAdded != NULL )
    {
       XPL_LOG_DM_TMN_Debug(("DMTree::Add: deleting last added node \n"));
       Delete(strLastInteriorNodeAdded.c_str(),SYNCML_DM_REQUEST_TYPE_INTERNAL);
    }
    return wReturnStatusCode;
}

SYNCML_DM_RET_STATUS_T DMTree::AddNodeInternal( DMAddData & oAddData,
                                                              SYNCML_DM_REQUEST_TYPE_T eRequestType)
{
  m_oLockContextManager.OnTreeAccessed();

  DMString strURI;
  DMString strPluginURI;  // uri without ?xxx stuff for plugins and so on
  SYNCML_DM_RET_STATUS_T wReturnStatusCode;

  if ( !GetPluginURI( oAddData.getURI(), strURI, strPluginURI ) )
  {
    XPL_LOG_DM_TMN_Debug(("DMTree::Add: GetPluginURI path:%s\n",oAddData.getURI() ));
    return SYNCML_DM_COMMAND_FAILED;
  }

  XPL_LOG_DM_TMN_Debug(("DMTree::Add: Validating URI:%s\n",strURI.c_str()));
  SYNCML_DM_URI_RESULT_T wURIValidateRetCode = URIValidateAndParse( strURI );

  // if wURIValidateRetCode != SYNCML_DM_COMMAND_ON_NODE
  // means the server has fired adding the following commands
  // example: ./TestAdd?prop=Format, ./Adding?prop=Type etc.
  switch(wURIValidateRetCode)
  {
    case SYNCML_DM_COMMAND_URI_TOO_LONG:
         return(SYNCML_DM_URI_TOO_LONG);
    case SYNCML_DM_COMMAND_INVALID_URI:
         return(SYNCML_DM_COMMAND_FAILED);
    case SYNCML_DM_COMMAND_ON_NODE :
          //do nothing here,proceed
         break;
    default:
         return(SYNCML_DM_COMMAND_NOT_ALLOWED);
  }

  XPL_LOG_DM_TMN_Debug(("DMTree::Add: Get Node name from:%s\n",strURI.c_str()));
  // The last segment of URI contains the node name
  DMString strLastSegment;
  if ( !GetLastSegmentOfURI(strURI, strLastSegment ) )
    return(SYNCML_DM_COMMAND_FAILED);

  // FindNodeByURI() function returns the node pointer
  // of the last segment of the node name.
  XPL_LOG_DM_TMN_Debug(("DMTree::Add: Check permission to write:%s\n",strURI.c_str()));
  // check permission to write
  if (!VerifyArchiveWriteAccess(strURI)) {
    XPL_LOG_DM_TMN_Error(("archive has no write access for uri %s\n", strURI.c_str()));
    return (SYNCML_DM_COMMAND_NOT_ALLOWED);
  }

  XPL_LOG_DM_TMN_Debug(("DMTree::Add: Get Parent Node from:%s\n",strURI.c_str()));
  DMNode  *psParentNodeOftheAddingNode = FindNodeByURI(strURI);
  BOOLEAN bInPlugin = FALSE;

  if (psParentNodeOftheAddingNode &&
      psParentNodeOftheAddingNode->isPlugin())
  {
    bInPlugin = TRUE;
  }
  else if ( m_ptrCacheOPI != NULL )
  {  // OPI support:
      DMNode *pNode = FindNodeByURI( strPluginURI );

      if ( pNode && pNode->isPlugin() )
      {
        psParentNodeOftheAddingNode = pNode;
        bInPlugin = TRUE;
      }
  }

  // If the Parent doesn't exists then FindNodeByURI will return
  // NULL and If the parent of the Addind node is a leaf node then we
  // SHOULD NOT to create a node under a leaf node.

  if(!bInPlugin &&
     ((psParentNodeOftheAddingNode == NULL) ||
     (psParentNodeOftheAddingNode->bFormat != SYNCML_DM_FORMAT_NODE)))
  {
    XPL_LOG_DM_TMN_Debug(("Either the Parent does not exists or the server \
                 is trying to add a node under a leaf node \n"));
    return psParentNodeOftheAddingNode == NULL ? SYNCML_DM_NOT_FOUND : SYNCML_DM_COMMAND_NOT_ALLOWED;
  }

  XPL_LOG_DM_TMN_Debug(("DMTree::Add: Parent Node got successfully\n Check isEnabled\n"));
  BOOLEAN bIsEnabled;
  bIsEnabled = IsUriEnabled(oAddData.getURI());
  if ( bIsEnabled == FALSE )
       return SYNCML_DM_FEATURE_NOT_SUPPORTED;


  XPL_LOG_DM_TMN_Debug(("DMTree::Add: check if the new node already exists\n"));
  DMNode *psPluginNode = NULL;
   // The following condition is used to checks if the node already
   // exists
  if (bInPlugin)
  {
      psPluginNode = psParentNodeOftheAddingNode;

      if (psPluginNode->Find(oAddData.getURI()) == SYNCML_DM_SUCCESS) {
           XPL_LOG_DM_TMN_Debug(("DMTree::Add: pluginNode already exists \n"));
           return(SYNCML_DM_TARGET_ALREADY_EXISTS);
      }
  }
  else
  {
      if(FindNodeInNextSiblingsList(psParentNodeOftheAddingNode->pcFirstChild,strLastSegment) != NULL) {
          XPL_LOG_DM_TMN_Debug(("DMTree::Add: node already exists \n"));
          return(SYNCML_DM_TARGET_ALREADY_EXISTS);
      }
  }


  XPL_LOG_DM_TMN_Debug(("DMTree::Add: check access type and ACL permissions\n"));
  DMMetaPCharVector asChildDepend;  // array of child/dependend nodes for indirect updates
  // check the Access type and ACL permissions.
  wReturnStatusCode = IsValidServer(strURI,
                                                   SYNCML_DM_ADD_ACCESS_TYPE,
                                                   eRequestType,
                                                   TRUE,
                                                   TRUE,
                                                   &asChildDepend);

  if(wReturnStatusCode != SYNCML_DM_SUCCESS)
  {
    XPL_LOG_DM_TMN_Debug(("DMTree::Add: IsValidServer uri:%s %d\n",strURI.c_str(), wReturnStatusCode));
     return(wReturnStatusCode);
  }
  UINT16 count = GetChildrenCount(psParentNodeOftheAddingNode);
  BOOLEAN bOPiDataParent = FALSE; // indicator that plug-in wants to store some data inside

  BOOLEAN bESN = FALSE; // Is an ESN

#ifdef LOB_SUPPORT
    bESN = m_oMDFObj.IsESN(oAddData.getURI() );
    oAddData.SetESN(bESN);
#endif
  if ( !m_oMDFObj.VerifyChildrenMultiNodesCount(strURI,count, bOPiDataParent) )
    return SYNCML_DM_DEVICE_FULL;

  DMNode *psAddingNode = NULL;

  if (!bInPlugin)
  {
      XPL_LOG_DM_TMN_Debug(("DMTree::Add: not plugin, create node\n"));
      psAddingNode = CreateNodeObj( bOPiDataParent ? SYNCML_DM_FORMAT_NODE_PDATA : oAddData.m_nFormat, bESN, NULL);

      if(psAddingNode == NULL)
      {
        XPL_LOG_DM_TMN_Debug(("CreateNodeObj returned NULL pointer \n"));
        return(SYNCML_DM_COMMAND_FAILED);
      }

      if ( bOPiDataParent && psAddingNode->getOverlayPIData())
      {
        wReturnStatusCode = OnOPiAdd(oAddData.getURI(), *psAddingNode->getOverlayPIData());

        if(wReturnStatusCode != SYNCML_DM_SUCCESS)
        {
          XPL_LOG_DM_TMN_Debug(("DMTree::Add: OnOPiAdd uri:%s %d\n",oAddData.getURI(), wReturnStatusCode));
          delete(psAddingNode);
          return(wReturnStatusCode);
        }

      }
  }

  XPL_LOG_DM_TMN_Debug(("DMTree::Add: check if server has replace permission\n"));
  // This function checks if the server has replace permissions
  SYNCML_DM_RET_STATUS_T wLocalRetStatusCode;
  wLocalRetStatusCode = IsValidServer(strURI,
                                                     SYNCML_DM_REPLACE_ACCESS_TYPE,
                                                     eRequestType,
                                                     FALSE,
                                                     TRUE);

  if (!bInPlugin) {
      psAddingNode->abNodeName = strLastSegment;
  }

  // If the current connected doesn't have ACL permissions on
  // replace command then we have to give all the ACL command
  // permissions to connected server.

  if(wLocalRetStatusCode == SYNCML_DM_PERMISSION_FAILED)
  {
        wLocalRetStatusCode = SetACL(oAddData.getURI());
        if ( wLocalRetStatusCode != SYNCML_DM_SUCCESS )
        {
             delete(psAddingNode);
             return(wLocalRetStatusCode);
        }
  }

  DMNode *psTheNode = (bInPlugin) ? psPluginNode : psAddingNode;
  DMToken aAutoNodes(SYNCML_DM_COMMA);

  BOOLEAN bNodeGetAccess;

  XPL_LOG_DM_TMN_Debug(("DMTree::Add: verify parameters\n"));
  wReturnStatusCode = m_oMDFObj.VerifyAddParameters(psTheNode,oAddData,aAutoNodes,bNodeGetAccess);

  if(wReturnStatusCode != SYNCML_DM_SUCCESS)
  {
    XPL_LOG_DM_TMN_Debug(("MDF check returned failure status code \n"));
    delete psAddingNode;
    return(wReturnStatusCode);
  }

  wReturnStatusCode = psTheNode->Add(oAddData);

  if(wReturnStatusCode != SYNCML_DM_SUCCESS)
  {
    XPL_LOG_DM_TMN_Debug(("Plug-in Add returned failure status code \n"));
    delete psAddingNode;
    return wReturnStatusCode;
  }

  XPL_LOG_DM_TMN_Debug(("DMTree::Add: setup the links of the node before adding the node into the tree\n"));
  if (!bInPlugin)
  {
        // Setup the links of the node before adding the node into the tree
        if ( bNodeGetAccess == FALSE )
            psAddingNode->addFlags(DMNode::enum_NodeNoGetAccess);
        psAddingNode->pcParentOfNode = psParentNodeOftheAddingNode;
        psAddingNode->pcFirstChild = NULL;
        psAddingNode->bFormat = oAddData.m_nFormat;

        if(psParentNodeOftheAddingNode->pcFirstChild)
        {
          // if the current addind node is NOT the first child of the
          // parent then insert the node in the siblings list.

          wReturnStatusCode = InsertNodeIntoNextSiblingsList
                              (psParentNodeOftheAddingNode->pcFirstChild,
                               psAddingNode);
        }
        else
        {
          // if the current adding node is the first child of the
          // parent then assign adding node pointer tp parent first
          // child pointer.

          psParentNodeOftheAddingNode->pcFirstChild = psAddingNode;
        }

#ifndef DM_IGNORE_TSTAMP_AND_VERSION
        if ( psParentNodeOftheAddingNode )
        {
           // Set timestamp and version number for parent Node
           // Use parentURI's URI.
           psParentNodeOftheAddingNode->SetTStamp(strURI.c_str(),XPL_CLK_GetClock());
           psParentNodeOftheAddingNode->SetVerNo(strURI.c_str(),psParentNodeOftheAddingNode->GetVerNo(strURI)+1);
        }
#endif

  }

  XPL_LOG_DM_TMN_Debug(("DMTree::Add: set time stamp and version number, add auto nodes\n"));
#ifndef DM_IGNORE_TSTAMP_AND_VERSION
  if (psAddingNode)
  {
        // Set timestamp and version number for currently added Node
        psAddingNode->SetTStamp(strPluginURI.c_str(),XPL_CLK_GetClock());
        psAddingNode->SetVerNo(strPluginURI.c_str(),0);
  }
#endif

  wReturnStatusCode = AddAutoNodes(oAddData.getURI(), eRequestType, aAutoNodes);
  if ( wReturnStatusCode != SYNCML_DM_SUCCESS )
  {
    Delete(oAddData.getURI(),SYNCML_DM_REQUEST_TYPE_INTERNAL);
    return wReturnStatusCode;
   }

#ifdef LOB_SUPPORT
     if(bESN)
          SetESNCache(oAddData.getURI(), psTheNode);
#endif
   XPL_LOG_DM_TMN_Debug(("DMTree::Add: log command and send event\n"));
   //Logging the command which was executed successfully in the log file
   LogEvent(SYNCML_DM_EVENT_ADD, oAddData.getURI());
   if ( asChildDepend.size() > 0 )
      CheckForIndirectUpdates( oAddData.getURI(), asChildDepend, psTheNode );

   if ( bInPlugin == FALSE &&  oAddData.m_nFormat == SYNCML_DM_FORMAT_NODE &&
        m_ptrCacheOPI )
   {
      DMNode *psPluginFantomNode = new DMPluginRootNode( m_ptrCacheOPI );

      if ( psPluginFantomNode == NULL )
        return SYNCML_DM_DEVICE_FULL;

      psPluginFantomNode->SetAddedNode(oAddData.getURI());

      DMNode::operator delete  (psPluginFantomNode);
   }

  XPL_LOG_DM_TMN_Debug(("DMTree::Add: returing\n"));

  return(wReturnStatusCode);
}

//------------------------------------------------------------------------
// FUNCTION        : Delete
//
// DESCRIPTION     : This function deletes the node incase if the directly
//                   referenced node is an interior node then it deletes
//                   it childs also. Before deleting the node first it
//                   checks the ACL incase if the directly referenced node
//                   is an interior node then it checks the ACL of childs
//                   also. In case if the ACL permissions are then then
//                   it calls plug-in.Delete function. If
//                   plug-in.Delete function returns SUCCESS then this
//                   function deletes the node from the tree.
//
//
// ARGUMENTS PASSED: UINT32 commandId
//                   UINT8 itemNumber
//                   UINT8 *pbURI
//                        -- URI of the deleting node.
//                   BOOLEAN isThisAtomic
//
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T
//                     - It returns SYNCML_DM_URI_TOO_LONG incase if the
//                       URI is too long.
//                     - It returns SYNCML_DM_COMMAND_FAILED incase if the
//                       URI is invalid like ?prop=Name.
//                     - It returns SYNCML_DM_NOT_FOUND incase if the
//                       node does not exists.
//                     - It returns SYNCML_DM_COMMAND_NOT_ALLOWED incase
//                       if the referenced node is a permanent node.
//                     - It returns 206 incase if the this function
//                       deletes only part of the directly referenced node.
//                       This mostly happens if the referenced node is
//                       an interior node.
//                     - It returns SYNCML_DM_SUCCESS incase this function
//                       deletes node succefully from the tree.
//                     - It returns SYNCML_DM_INVALID_URI
//                       if the node path is not valid for current DMT version
//
// PRE-CONDITIONS  :
// POST-CONDITIONS :
// IMPORTANT NOTES :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMTree::Delete(CPCHAR pbURI,
                                                                 SYNCML_DM_REQUEST_TYPE_T eRequestType)
{
  m_oLockContextManager.OnTreeAccessed();

  DMString strPluginURI;  // uri without ?xxx stuff for plugins and so on
  DMString strURI;
  SYNCML_DM_RET_STATUS_T wReturnStatusCode;
  DMNode *psDeletingNode = NULL;
  SYNCML_DM_RET_STATUS_T wURIValidateRetCode;
  BOOLEAN bInPlugin = FALSE;

  if ( !GetPluginURI( pbURI, strURI, strPluginURI ) )
    return SYNCML_DM_COMMAND_FAILED;

  // We are maintaing a local of the URI send by the UA
  // because we are going to modify the URI value.
  // This checks for a relative URI if so, force the ./

  wURIValidateRetCode = URIValidateAndParse( strURI );
  if(wURIValidateRetCode != SYNCML_DM_COMMAND_ON_NODE)
  {
    if(wURIValidateRetCode == SYNCML_DM_COMMAND_URI_TOO_LONG)
      return(SYNCML_DM_URI_TOO_LONG);
    else
    {
      XPL_LOG_DM_TMN_Debug(("Invalid URI \n"));
      return(SYNCML_DM_COMMAND_FAILED);
    }
  }
  // check permission to write
  if (!VerifyArchiveWriteAccess(strURI)) {
    XPL_LOG_DM_TMN_Error(("archive has no write access for uri %s\n", strURI.c_str()));
    return (SYNCML_DM_COMMAND_NOT_ALLOWED);
  }

  psDeletingNode = FindNodeByURI(strURI);
  BOOLEAN bIsEnabled;
  bIsEnabled = IsUriEnabled(strURI);

  // In Node or Parent does NOT exists
  if(psDeletingNode == NULL)
  {
    XPL_LOG_DM_TMN_Debug(("Node not exists in the Tree \n"));

    if ( bIsEnabled == FALSE )
          return SYNCML_DM_FEATURE_NOT_SUPPORTED;
    else
         return SYNCML_DM_NOT_FOUND;
  }
  else
  {
    if ( bIsEnabled == FALSE )
      return SYNCML_DM_FEATURE_NOT_SUPPORTED;
  }

  bInPlugin = psDeletingNode->isPlugin();

  // Is this the correct position to check this condition
  if(psDeletingNode->isPermanent())
  {
    XPL_LOG_DM_TMN_Debug(("Deleting node is a PERMANENT node \n"));
    return(SYNCML_DM_COMMAND_NOT_ALLOWED);
  }

  //YXU if Node is .
  if(psDeletingNode->pcParentOfNode == NULL && !bInPlugin)
  {
    XPL_LOG_DM_TMN_Debug(("Deleting . node is a PERMANENT node \n"));
    return(SYNCML_DM_COMMAND_NOT_ALLOWED);
  }

  wReturnStatusCode = IsValidServer(strURI,
                                                   SYNCML_DM_DELETE_ACCESS_TYPE,
                                                   eRequestType,
                                                   TRUE,
                                                   TRUE);

  if(wReturnStatusCode != SYNCML_DM_SUCCESS)
  {
    return(wReturnStatusCode);
  }


  //This function is checking for the ACL permissions of
  //an interior.
  DMStringVector aChildren;
  if(!bInPlugin &&
     (psDeletingNode->bFormat == SYNCML_DM_FORMAT_NODE) &&
     (psDeletingNode->pcFirstChild != NULL))
  {
     if ( eRequestType == SYNCML_DM_REQUEST_TYPE_SERVER )
         wReturnStatusCode = CheckDeleteForNode(psDeletingNode,strURI);
  }

  if (bInPlugin )
  {
    wReturnStatusCode = CheckDeleteForPluginNodes(eRequestType,strURI, psDeletingNode, aChildren);
  }

  if(wReturnStatusCode != SYNCML_DM_SUCCESS)
     return wReturnStatusCode;

  //If the all the nodes the has ACL permissions then
  //we have to delete the nodes from the tree after
  //calling Delete of each node. In case of atomic
  //we are not deleting any node from the tree here
  //we are deleting the tree in DataCommit or
  //RollBack function.

  SYNCML_DM_FORMAT_T format = psDeletingNode->bFormat;
  if (!bInPlugin)
  {
        if ( psDeletingNode->IsOverlayPIData() && m_ptrCacheOPI != NULL ) // notify plug-in first
        {
          wReturnStatusCode = OnOPiDelete( strURI );
        }

        if ( wReturnStatusCode == SYNCML_DM_SUCCESS )
          wReturnStatusCode = DeleteNode(psDeletingNode,strURI,aChildren);

  } else
        wReturnStatusCode = psDeletingNode->Delete( strURI );

  if ( wReturnStatusCode != SYNCML_DM_SUCCESS )
    return wReturnStatusCode;

  if ( bInPlugin == FALSE &&
       format == SYNCML_DM_FORMAT_NODE &&
       m_ptrCacheOPI )
  {

         DMNode* psPluginFantomNode = new DMPluginRootNode( m_ptrCacheOPI );

         if ( psPluginFantomNode == NULL )
            return SYNCML_DM_DEVICE_FULL;

         psPluginFantomNode->RemoveAddedNode(strURI);

         DMNode::operator delete  (psPluginFantomNode);
  }


  m_oACLObj.Delete(strURI);
  LogEvent(pbURI,aChildren);

#ifdef LOB_SUPPORT
   RemoveESNCache(pbURI);
#endif

  return(wReturnStatusCode);
}


//------------------------------------------------------------------------
// FUNCTION        : Exec
//
// DESCRIPTION     : This function execute the node by fist check permission
//                     - It returns SYNCML_DM_INVALID_URI
//                       if the node path is not valid for current DMT version
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMTree::Exec(CPCHAR pbURI,
                                     CPCHAR pExecData,
                                     DMString & oExecResult,
                                     CPCHAR szCorrelator)
{
  SYNCML_DM_RET_STATUS_T wReturnStatusCode;

  m_oLockContextManager.OnTreeAccessed();

  XPL_LOG_DM_TMN_Debug(("Entered DMTree::Exec \n"));

  if(pbURI == NULL)
  {
    XPL_LOG_DM_TMN_Debug(("DMTree::Exec: pbURI is null\n"));
    return(SYNCML_DM_COMMAND_FAILED);
  }

  char *abTempURI = (char*) DmAllocMem(GetMaxTotalPathLength() + 1);
  if(abTempURI == NULL)
  {
     XPL_LOG_DM_TMN_Debug(("DMTree::Exec: abTempURI allocate memory failed\n"));
     return SYNCML_DM_DEVICE_FULL;
  }

  abTempURI[0] = '\0';

  XPL_LOG_DM_TMN_Debug(("Server fired <Exec> on: %s \n", pbURI));

  UINT16 bStringLength = DmStrlen(pbURI);
  // We are maintaing a local of the URI send by the UA
  // because we are going to modify the URI value.
  // This checks for a relative URI if so, force the ./

  if(pbURI[0] != SYNCML_DM_DOT)
  {
    abTempURI[0] = SYNCML_DM_DOT;
    abTempURI[1] = SYNCML_DM_FORWARD_SLASH;
    abTempURI[2] = '\0';
    bStringLength += SYNCML_DM_RELATIVE_URI_OFFSET_LENGTH; // For "./"
  }

  if(bStringLength > GetMaxTotalPathLength() )
  {
      XPL_LOG_DM_TMN_Debug(("URI Too long \n"));
      DmFreeMem(abTempURI);
      return(SYNCML_DM_URI_TOO_LONG);
  }

  DmStrcat(abTempURI, pbURI);

  SYNCML_DM_RET_STATUS_T wURIValidateRetCode = URIValidateAndParse(abTempURI);

  if(wURIValidateRetCode != SYNCML_DM_COMMAND_ON_NODE)
  {
    if(wURIValidateRetCode == SYNCML_DM_COMMAND_URI_TOO_LONG)
    {
      XPL_LOG_DM_TMN_Debug(("URI Too long \n"));
      DmFreeMem(abTempURI);
      return(SYNCML_DM_URI_TOO_LONG);
    }
    else
    {
      XPL_LOG_DM_TMN_Debug(("Invalid URI \n"));
      DmFreeMem(abTempURI);
      return(SYNCML_DM_COMMAND_FAILED);
    }
  }

  DMNode *psExecNode = FindNodeByURI(abTempURI);

  // In Node or Parent does NOT exists
  BOOLEAN bIsEnabled;
  bIsEnabled = IsUriEnabled(abTempURI);

  if(psExecNode == NULL)
  {
    XPL_LOG_DM_TMN_Debug(("Node not exists in the Tree \n"));
    DmFreeMem(abTempURI);
    if ( bIsEnabled == FALSE )
       return SYNCML_DM_FEATURE_NOT_SUPPORTED;
    else
       return SYNCML_DM_NOT_FOUND;
  }
  else
  {
    if ( bIsEnabled == FALSE )
    {
        DmFreeMem(abTempURI);
        XPL_LOG_DM_TMN_Debug(("DMTree::Exec: Node not enabled! \n"));
        return SYNCML_DM_FEATURE_NOT_SUPPORTED;
    }
  }

  wReturnStatusCode = IsValidServer(abTempURI,
                                    SYNCML_DM_EXEC_ACCESS_TYPE,
                                    SYNCML_DM_REQUEST_TYPE_SERVER,
                                    TRUE,
                                    TRUE);

  if(wReturnStatusCode != SYNCML_DM_SUCCESS)
  {
    DmFreeMem(abTempURI);
    XPL_LOG_DM_TMN_Debug(("DMTree::Exec: Not valid server! \n"));
    return(wReturnStatusCode);
  }
  XPL_LOG_DM_TMN_Debug(("DMTree::Exec: Execute plugin... \n"));
  if ( pExecData )
     wReturnStatusCode=DmExecutePlugin(abTempURI,pExecData,szCorrelator,oExecResult);
  else
     wReturnStatusCode=DmExecutePlugin(abTempURI,"",szCorrelator,oExecResult);

  DmFreeMem(abTempURI);

  XPL_LOG_DM_TMN_Debug(("Exec: exec plugin for uri %s, data %s, correlator %s returned %d\n",
                        abTempURI,
                        pExecData ? pExecData : "<NULL>",
                        szCorrelator,
                        wReturnStatusCode));

  return(wReturnStatusCode);
}


//------------------------------------------------------------------------
// FUNCTION        : Get
// DESCRIPTION     : Get Node value
// ARGUMENTS PASSED: the URI(target to get)
//                   the pointer to the structure to return after filling in the value
// RETURN VALUE    : status code indicating result of the operation
//                   and the structure filled with the values
// PRE-CONDITIONS  : ONLY THE DM ENGINE INVOKES THIS,it is an INTERNAL
//                   METHOD
// POST-CONDITIONS : The UA is able to retrieve the value through the
//                   return structure
// IMPORTANT NOTES :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMTree::Get(CPCHAR pbUri,
                                            DMGetData & oReturnData,
                                            SYNCML_DM_REQUEST_TYPE_T eRequestType)
{

  m_oLockContextManager.OnTreeAccessed();

  DMNode *psGetNode = NULL;
  SYNCML_DM_URI_RESULT_T  wURIValidateRetCode;
  SYNCML_DM_RET_STATUS_T wReturnStatusCode = SYNCML_DM_SUCCESS;
  DMString strPluginURI;  // uri without ?xxx stuff for plugins and so on
  DMString strURI;

  if ( !GetPluginURI( pbUri, strURI, strPluginURI ) )
    return SYNCML_DM_COMMAND_FAILED;

  wURIValidateRetCode = URIValidateAndParse( strURI );
  switch(wURIValidateRetCode)
  {
    case SYNCML_DM_COMMAND_INVALID_URI:
          return(SYNCML_DM_COMMAND_NOT_ALLOWED);

    case SYNCML_DM_COMMAND_ON_UNKNOWN_PROPERTY:
           return(SYNCML_DM_FEATURE_NOT_SUPPORTED);

    default :
             //not processed immedietely,do nothing
    break;
  }

  XPL_LOG_DM_TMN_Debug(("Get %s, %s, %s\n", pbUri, strURI.c_str(), strPluginURI.c_str()));
  // check permission to read
  if (!VerifyArchiveReadAccess(strPluginURI)) {
    XPL_LOG_DM_TMN_Error(("archive has no read access for uri %s\n", strPluginURI.c_str()));
    return (SYNCML_DM_COMMAND_NOT_ALLOWED);
  }

  psGetNode = FindNodeByURI( strPluginURI );

  BOOLEAN bIsEnabled;
  bIsEnabled = IsUriEnabled(strPluginURI);
  if(psGetNode == NULL) // If this node doesn't exist
  {
    if ( bIsEnabled == FALSE )
        return SYNCML_DM_FEATURE_NOT_SUPPORTED;
    else
            return SYNCML_DM_NOT_FOUND;

  }
  else
  {
    if ( bIsEnabled == FALSE )
       return SYNCML_DM_FEATURE_NOT_SUPPORTED;
  }

  if ( eRequestType != SYNCML_DM_REQUEST_TYPE_INTERNAL )
  {

    if( psGetNode->IsGetAccess(strPluginURI) == FALSE )
      return     SYNCML_DM_COMMAND_NOT_ALLOWED;

    wReturnStatusCode = IsValidServer(strPluginURI,
                                                    SYNCML_DM_GET_ACCESS_TYPE,
                                                    eRequestType,
                                                    TRUE,
                                                    TRUE);

    if(wReturnStatusCode != SYNCML_DM_SUCCESS)
      return(wReturnStatusCode);
  }

  DMString strData;
  SYNCML_DM_FORMAT_T bNodeFormat = SYNCML_DM_FORMAT_CHR;
#ifdef LOB_SUPPORT
  oReturnData.SetESN(psGetNode->IsESN());
#endif

  switch(wURIValidateRetCode)
  {
    case SYNCML_DM_COMMAND_ON_NODE:
      if(psGetNode->bFormat != SYNCML_DM_FORMAT_NODE)
      {
        return psGetNode->Get(strPluginURI, oReturnData);
      }
      else
      {
        wReturnStatusCode = GetChildren(strPluginURI, psGetNode, eRequestType, strData );
        bNodeFormat = SYNCML_DM_FORMAT_NODE;
      }
    break;

    case SYNCML_DM_COMMAND_ON_ACL_PROPERTY:
      m_oACLObj.GetACL(strPluginURI, strData);
      break;

    case SYNCML_DM_COMMAND_ON_FORMAT_PROPERTY:
    {
      SYNCML_DM_FORMAT_T dwFormat = 0;

      if(psGetNode->bFormat == SYNCML_DM_FORMAT_NODE)
      {
        dwFormat = SYNCML_DM_FORMAT_NODE;
        wReturnStatusCode = SYNCML_DM_SUCCESS;
      }
      else
      {
        wReturnStatusCode = psGetNode->GetFormat(strPluginURI,&dwFormat);
      }

      if(wReturnStatusCode == SYNCML_DM_SUCCESS)
        wReturnStatusCode = ConvertFormat(dwFormat,strData);
    }
    break;

    case SYNCML_DM_COMMAND_ON_TYPE_PROPERTY:
      wReturnStatusCode = psGetNode->GetType(strPluginURI, strData);
    break;

#ifdef LOB_SUPPORT
    case SYNCML_DM_COMMAND_ON_ESN_PROPERTY:
       {
          char szSize[4];
    BOOLEAN  bESN = FALSE;
    wReturnStatusCode = psGetNode->IsESN(strPluginURI, bESN);
    if(bESN)
              DmSprintf(szSize, "%s", "yes");
    else
              DmSprintf(szSize, "%s", "no");
          strData = szSize;
        }
     break;
#endif

    case SYNCML_DM_COMMAND_ON_SIZE_PROPERTY:
    {
      UINT32 dwSize = 0;
      char szSize[ SIZE_LENGTH + 1 ];

      wReturnStatusCode = psGetNode->GetSize(strPluginURI,&dwSize);

      if(wReturnStatusCode == SYNCML_DM_SUCCESS)
      {
          DmSprintf(szSize, "%d", dwSize);
          strData = szSize;
      }
    }
    break;

    case SYNCML_DM_COMMAND_ON_NAME_PROPERTY:
       wReturnStatusCode = psGetNode->GetName(strPluginURI, strData);
    break;

    case SYNCML_DM_COMMAND_ON_TITLE_PROPERTY:
      wReturnStatusCode = psGetNode->GetTitle(strPluginURI, strData);
    break;

    case SYNCML_DM_COMMAND_ON_TSTAMP_PROPERTY:
#ifndef DM_IGNORE_TSTAMP_AND_VERSION
    {
      char szTimeStamp[ TSTAMP_LENGTH+1 ];
      XPL_CLK_CLOCK_T timestamp=psGetNode->GetTStamp(strPluginURI); //In seconds
      //OK, interface uses milleseconds %lld while internal TStamp uses seconds
      DmSprintf(szTimeStamp, "%lld", (XPL_CLK_LONG_CLOCK_T)timestamp * 1000L);
      strData = szTimeStamp;
    }
#else
      wReturnStatusCode = SYNCML_DM_FEATURE_NOT_SUPPORTED;
#endif
    break;

    case SYNCML_DM_COMMAND_ON_VERNO_PROPERTY:
#ifndef DM_IGNORE_TSTAMP_AND_VERSION
    {
      char szVer[ VERSION_LENGTH + 1 ];
      DmSprintf(szVer, "%d", psGetNode->GetVerNo(strPluginURI));
      strData = szVer;
    }
#else
      wReturnStatusCode = SYNCML_DM_FEATURE_NOT_SUPPORTED;
#endif
    break;

    default:
      wReturnStatusCode = SYNCML_DM_FEATURE_NOT_SUPPORTED;
    break;
  }// End of switch case

  if(wReturnStatusCode != SYNCML_DM_SUCCESS)
    return wReturnStatusCode;

  if(strData[0] == 0 && (psGetNode->bFormat != SYNCML_DM_FORMAT_NODE))
    bNodeFormat = SYNCML_DM_FORMAT_NULL;
  return oReturnData.set(bNodeFormat,strData,strData.length(),NULL);
}

//------------------------------------------------------------------------
// FUNCTION        : InternalGetAttributes
// DESCRIPTION     : This method is called INTERNALLY by the DM Engine(UA)
//                   to mimic the DM GET operation to get the node attributes.
// ARGUMENTS PASSED:
//                   pbURI -- URI of the node.
//                   attrs -- reference to DmtAttributes object.
//                   bInternalGet -- whether this is an internal call or not.
// RETURN VALUE    : status code indicating result of the operation
//                   and the structure filled with the values
// PRE-CONDITIONS  : ONLY THE DM ENGINE INVOKES THIS,it is an INTERNAL
//                   METHOD
// POST-CONDITIONS : The UA is able to retrieve the value through the
//                   return structure
// IMPORTANT NOTES :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMTree::GetAttributes(CPCHAR pbUri,
                                                      DmtAttributes& attrs,
                                                      SYNCML_DM_REQUEST_TYPE_T eRequestType)
{
  m_oLockContextManager.OnTreeAccessed();

  DMNode *psGetNode = NULL;
  SYNCML_DM_RET_STATUS_T wURIValidateRetCode;
  SYNCML_DM_RET_STATUS_T wReturnStatusCode = SYNCML_DM_SUCCESS;
  DMString strPluginURI;  // uri without ?xxx stuff for plugins and so on
  DMString strURI;

  if ( !GetPluginURI( pbUri, strURI, strPluginURI ) )
    return SYNCML_DM_COMMAND_FAILED;

  wURIValidateRetCode = URIValidateAndParse( strURI );
  switch(wURIValidateRetCode)
  {
    case SYNCML_DM_COMMAND_INVALID_URI:
       return(SYNCML_DM_COMMAND_NOT_ALLOWED);

    case SYNCML_DM_COMMAND_ON_UNKNOWN_PROPERTY:
       return(SYNCML_DM_FEATURE_NOT_SUPPORTED);

    default :
             //not processed immedietely,do nothing
    break;
  }

  psGetNode = FindNodeByURI( strPluginURI );
  if(psGetNode == NULL)
  {
    BOOLEAN bIsEnabled;
    bIsEnabled =  IsUriEnabled(strPluginURI);
    if ( bIsEnabled == FALSE )
        return SYNCML_DM_FEATURE_NOT_SUPPORTED;
   else
        return SYNCML_DM_NOT_FOUND;

  }


  if ( eRequestType != SYNCML_DM_REQUEST_TYPE_INTERNAL ) {
    wReturnStatusCode = IsValidServer(strPluginURI,
                                                     SYNCML_DM_GET_ACCESS_TYPE,
                                                     eRequestType,
                                                     TRUE,
                                                     TRUE);

    if(wReturnStatusCode != SYNCML_DM_SUCCESS)
      return(wReturnStatusCode);
  }

  DMString strName, strFormat, strTitle, strType, strAcl;
  UINT32 nVer = 0, nSize = 0;
  INT64 timestamp =0;

  if (wURIValidateRetCode == SYNCML_DM_COMMAND_ON_NODE)
  {
     wReturnStatusCode = psGetNode->GetName(strPluginURI, strName);

     if (wReturnStatusCode != SYNCML_DM_SUCCESS)
        return(wReturnStatusCode);

    SYNCML_DM_FORMAT_T dwFormat = 0;
    if(psGetNode->bFormat == SYNCML_DM_FORMAT_NODE)
    {
       dwFormat = SYNCML_DM_FORMAT_NODE;
       wReturnStatusCode = SYNCML_DM_SUCCESS;
    }
    else
        wReturnStatusCode = psGetNode->GetFormat(strPluginURI, &dwFormat);

    if(wReturnStatusCode == SYNCML_DM_SUCCESS)
      wReturnStatusCode = ConvertFormat(dwFormat,strFormat);
    else
      return(wReturnStatusCode);

    m_oACLObj.GetACL(strPluginURI, strAcl);
    wReturnStatusCode = psGetNode->GetType(strPluginURI, strType);
    wReturnStatusCode = psGetNode->GetTitle(strPluginURI, strTitle);
    wReturnStatusCode = psGetNode->GetSize(strPluginURI, &nSize);

#ifndef DM_IGNORE_TSTAMP_AND_VERSION
    timestamp=(JemDate)(psGetNode->GetTStamp(strPluginURI) * 1000L);
    nVer = psGetNode->GetVerNo(strPluginURI);
#else
    return(SYNCML_DM_FEATURE_NOT_SUPPORTED);
#endif

  }
  else
     return(SYNCML_DM_FEATURE_NOT_SUPPORTED);

  return attrs.Set(strName, strFormat, strTitle, strType, nVer, nSize, timestamp, DmtAcl(strAcl));

}

BOOLEAN DMTree::NeedCheckParent( const DMString&         strURI,
                                 DMNode                  *psReplacingNode,
                                 BOOLEAN                 bInPlugin,
                                 SYNCML_DM_FORMAT_T  bFormat,
                                 SYNCML_DM_REQUEST_TYPE_T eRequestType)
{
  BOOLEAN bCheckParent = FALSE;
  BOOLEAN bContinueChecking = TRUE;

  if( bFormat == SYNCML_DM_FORMAT_NODE )
  {
    bContinueChecking = ( IsValidServer( strURI,
                                                  SYNCML_DM_REPLACE_ACCESS_TYPE,
                                                  eRequestType,
                                                  FALSE,
                                                  TRUE) != SYNCML_DM_SUCCESS );
  }

  if( bContinueChecking )
  {
    if (!bInPlugin)
    {
      if(psReplacingNode->pcParentOfNode )
      {
        bCheckParent = TRUE;
      }
    }
    else
    {
      if( ParentExistsForPluginNode(strURI, psReplacingNode ) )
      {
        bCheckParent = TRUE;
      }
    }
  }

  return bCheckParent;
}

SYNCML_DM_RET_STATUS_T DMTree::ReplaceACLProperty( const DMString&  strURI,
                                                   BOOLEAN          bInPlugin,
                                                   const DMString&  strPluginURI,
                                                   DMAddData&       oReplaceData,
                                                   DMNode*          psReplacingNode )
{
  SYNCML_DM_RET_STATUS_T wReturnStatusCode = SYNCML_DM_SUCCESS;

  if ( strURI == "." )
     return (SYNCML_DM_COMMAND_NOT_ALLOWED);

  if((oReplaceData.m_oData.getSize() == 0))
  {
    m_oACLObj.Delete(strURI);
    LogEvent(SYNCML_DM_EVENT_REPLACE,oReplaceData.getURI());
  }
  else
  {
    char *pACL = NULL;
    oReplaceData.m_oData.copyTo(&pACL);
    if( pACL == NULL)
      return SYNCML_DM_DEVICE_FULL;

    wReturnStatusCode = ParseACL((UINT8*)pACL);
    if(wReturnStatusCode == SYNCML_DM_SUCCESS)
    {
       LogEvent(SYNCML_DM_EVENT_REPLACE,oReplaceData.getURI());
       if( oReplaceData.m_oURI.compare(".?prop=ACL") )
       {
          XPL_LOG_DM_TMN_Debug(("Server fired <Replace> on the ACL of a Root node \n"));
          UINT8 *pbADDLoc = NULL;

          pbADDLoc = (UINT8*)DmStrstr(pACL, "Add=");
          if(pbADDLoc != NULL)
          {
             pbADDLoc += ADD_CMD_LENGTH_IN_ACL;
             if((pbADDLoc != NULL) && (pbADDLoc[0] != SYNCML_DM_STAR))
             {
               XPL_LOG_DM_TMN_Debug(("Root node Replacing ACL value does NOT have \
                            Add permissions to ALL servers it should Add=* \n"));

               if (pACL)
                   DmFreeMem(pACL);
               return (SYNCML_DM_COMMAND_NOT_ALLOWED);
             }
          }
          else
          {
              XPL_LOG_DM_TMN_Debug(("Root node Replacing ACL value does NOT have \
                           Add permissions to. This is NOT allowed according to spec \n"));

              if (pACL)
                  DmFreeMem(pACL);
              return (SYNCML_DM_COMMAND_NOT_ALLOWED);
          }
      }

      m_oACLObj.SetACL(strURI, pACL);
    }

    DmFreeMem(pACL);
  }

#ifndef DM_IGNORE_TSTAMP_AND_VERSION
  if ( (wReturnStatusCode == SYNCML_DM_SUCCESS) && !bInPlugin && psReplacingNode )
  {
    psReplacingNode->SetTStamp(strPluginURI, XPL_CLK_GetClock());
    psReplacingNode->SetVerNo(strPluginURI, psReplacingNode->GetVerNo(strPluginURI) + 1);
  }
#endif

  return wReturnStatusCode;
}

SYNCML_DM_RET_STATUS_T DMTree::ReplaceTitleProperty( const DMString&  strURI,
                                                     BOOLEAN          bInPlugin,
                                                     const DMString&  strPluginURI,
                                                     DMAddData&       oReplaceData,
                                                     DMNode*          psReplacingNode )
{
  if( oReplaceData.m_oData.getSize() == 0)
  {
    psReplacingNode->m_strTitle = "";
    LogEvent(SYNCML_DM_EVENT_REPLACE,oReplaceData.getURI());
  }
  else
  {
    if ( oReplaceData.m_oData.getSize() > SYNCML_DM_MAX_TITLE_LENGTH )
    {
     oReplaceData.m_oData.setSize(SYNCML_DM_MAX_TITLE_LENGTH);
    }

    psReplacingNode->SetTitle( strURI, oReplaceData.getCharData());
    LogEvent(SYNCML_DM_EVENT_REPLACE,oReplaceData.getURI());
  }

#ifndef DM_IGNORE_TSTAMP_AND_VERSION
  // Update timestamp and increment version number !!
  if( !bInPlugin && psReplacingNode)
  {
     psReplacingNode->SetTStamp(strPluginURI.c_str(),XPL_CLK_GetClock());
     psReplacingNode->SetVerNo(strPluginURI.c_str(),psReplacingNode->GetVerNo(strPluginURI.c_str()) + 1);
  }
#endif

  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DMTree::ReplaceNodeInternal( const DMString&        strURI,
                                                    DMAddData&             oReplaceData,
                                                    DMNode*                psReplacingNode,
                                                    DMMetaPCharVector&     asChildDepend )
{
  SYNCML_DM_RET_STATUS_T  wReturnStatusCode = m_oMDFObj.VerifyReplaceParameters(psReplacingNode,
                                                                                strURI,
                                                                                oReplaceData,
                                                                                NULL);
  if(wReturnStatusCode != SYNCML_DM_SUCCESS)
      return(wReturnStatusCode);

  wReturnStatusCode = psReplacingNode->Replace( oReplaceData );

  if(wReturnStatusCode == SYNCML_DM_SUCCESS)
  {
    LogEvent(SYNCML_DM_EVENT_REPLACE,oReplaceData.getURI());

#ifdef LOB_SUPPORT
    if(psReplacingNode->IsESN())
        LogESNCommandForArchiver(oReplaceData.getURI(),psReplacingNode);
#endif

    if ( asChildDepend.size() > 0 )
      CheckForIndirectUpdates( strURI, asChildDepend, psReplacingNode );
  }

  return wReturnStatusCode;
}

SYNCML_DM_RET_STATUS_T DMTree::PrepareNamePropertyReplace( const DMString&        strURI,
                                                           BOOLEAN                bInPlugin,
                                                           DMAddData&             oReplaceData,
                                                           DMNode*                psReplacingNode )
{
  if(psReplacingNode->isPermanent())
  {
    XPL_LOG_DM_TMN_Debug(("Server fired <Replace> on the name of a permanent node which is NOT allowed \n"));
    return SYNCML_DM_COMMAND_NOT_ALLOWED;
  }

  // For a Replace command on the name of a node, the DMTNM shall
  // first verify that the result of the operation will not cause
  // any inconsistencies in the tree, before it executes the command.

  // The following condition is used to check if the node name
  // already existing or not

  if( oReplaceData.m_oData.getSize() == 0  )
      return(SYNCML_DM_COMMAND_FAILED);

  if( oReplaceData.m_oData.getSize() > GetMaxPathSegmentLength() )
  {
     XPL_LOG_DM_TMN_Debug(("Replacing Name is too long \n"));
     return(SYNCML_DM_REQUEST_ENTITY_TOO_LARGE);
  }

  if(IsValidSegment((CPCHAR)oReplaceData.m_oData.getBuffer(),oReplaceData.m_oData.getSize()) == FALSE)
     return(SYNCML_DM_COMMAND_FAILED);

  BOOLEAN noParent = FALSE;
  if (bInPlugin) {
     if (!ParentExistsForPluginNode(strURI, psReplacingNode))
          noParent = TRUE;
  } else
      if (psReplacingNode->pcParentOfNode == NULL)
          noParent = TRUE;

  if (noParent)
     return(SYNCML_DM_COMMAND_FAILED);

  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DMTree::CheckReplacingNodeName( DMAddData& oReplaceData,
                                                       DMNode*    psReplacingNode )
{
  SYNCML_DM_RET_STATUS_T  wReturnStatusCode = SYNCML_DM_SUCCESS;

  if ( oReplaceData.m_oData.compare(psReplacingNode->abNodeName) )
  {
    if((psReplacingNode->pcParentOfNode != NULL) &&
        psReplacingNode->pcParentOfNode->abNodeName == "." )
    {
         if( oReplaceData.m_oData.compare("DevDetail") ||
             oReplaceData.m_oData.compare("SyncML") ||
             oReplaceData.m_oData.compare("DevInfo") )
         {
            wReturnStatusCode = SYNCML_DM_COMMAND_NOT_ALLOWED;
         }
         else
         {
           wReturnStatusCode = SYNCML_DM_SUCCESS;
         }
    }
    else
    {
       wReturnStatusCode = SYNCML_DM_SUCCESS;
    }
 }
 else
 {
   wReturnStatusCode = SYNCML_DM_COMMAND_NOT_ALLOWED;
 }

  return wReturnStatusCode;
}

SYNCML_DM_RET_STATUS_T DMTree::FixACL( const DMString& strURI,
                                        DMAddData&      oReplaceData )
{
  DMString dacl;
  m_oACLObj.GetACL(strURI, dacl);
  if (dacl != "")
  {
    DMString strParentURI = strURI, strLastSegment;

    if ( !GetLastSegmentOfURI(strParentURI, strLastSegment) )
      return SYNCML_DM_DEVICE_FULL;

    strParentURI += "/";
    strParentURI += (CPCHAR)oReplaceData.getCharData();

    m_oACLObj.Delete(strURI);
    m_oACLObj.SetACL( strParentURI, dacl);
  }

  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DMTree::ReplaceNameProperty( const DMString&         strURI,
                                                     BOOLEAN                bInPlugin,
                                                     const DMString&        strPluginURI,
                                                     DMAddData&             oReplaceData,
                                                     DMNode*                psReplacingNode,
                                                     SYNCML_DM_RET_STATUS_T wURIValidateRetCode )
{
  XPL_LOG_DM_TMN_Error(("ReplaceNameProperty() called \n"));
  SYNCML_DM_RET_STATUS_T  wReturnStatusCode = PrepareNamePropertyReplace( strURI,
                                                                          bInPlugin,
                                                                          oReplaceData,
                                                                          psReplacingNode );
  XPL_LOG_DM_TMN_Error(("From ReplaceNameProperty(): Call to PrepareNamePropertyReplace() returns %d \n", wReturnStatusCode));

  if( SYNCML_DM_SUCCESS != wReturnStatusCode ) return wReturnStatusCode;

  if(!bInPlugin && (FindNodeInNextSiblingsList((psReplacingNode->pcParentOfNode)->pcFirstChild,
                                                oReplaceData.getCharData()) != NULL))
  {
    wReturnStatusCode = CheckReplacingNodeName( oReplaceData,
                                                psReplacingNode );
    XPL_LOG_DM_TMN_Error(("From ReplaceNameProperty(): Call to CheckReplacingNodeName() returns %d \n", wReturnStatusCode));
  }
  else
  {
    DMString origName;

    if((wURIValidateRetCode == SYNCML_DM_COMMAND_ON_NAME_PROPERTY) && !bInPlugin)
    {
      origName = psReplacingNode->abNodeName;
      psReplacingNode->abNodeName = oReplaceData.getCharData();
    }

    DMString strParentURI = strURI, strLastSegment;

    if ( !GetLastSegmentOfURI(strParentURI, strLastSegment) )
      return SYNCML_DM_DEVICE_FULL;

    strParentURI += "/";
    strParentURI += oReplaceData.getCharData();

    wReturnStatusCode = m_oMDFObj.VerifyReplaceParameters(psReplacingNode,
                                                         strParentURI,
                                                         oReplaceData,
                                                         origName);

    XPL_LOG_DM_TMN_Error(("From ReplaceNameProperty(): Call to VerifyReplaceParameters() returns %d \n", wReturnStatusCode));

    if(wURIValidateRetCode == SYNCML_DM_COMMAND_ON_NAME_PROPERTY)
    {
        if (!bInPlugin)
            psReplacingNode->abNodeName = origName;
    }

    if(wReturnStatusCode != SYNCML_DM_SUCCESS)
        return(wReturnStatusCode);


    wReturnStatusCode = psReplacingNode->Rename(strPluginURI,oReplaceData.getCharData());
    XPL_LOG_DM_TMN_Error(("From ReplaceNameProperty(): Call to psReplacingNode->Rename() returns %d \n", wReturnStatusCode));
  }

  if(wReturnStatusCode == SYNCML_DM_SUCCESS)
  {
    if (!bInPlugin) {
        psReplacingNode->SetName( oReplaceData.getURI(), oReplaceData.getCharData());
    }

    LogEvent(SYNCML_DM_EVENT_RENAME,strURI,oReplaceData.getCharData());
    wReturnStatusCode = FixACL( strURI, oReplaceData );
    XPL_LOG_DM_TMN_Error(("From ReplaceNameProperty(): Call to  FixACL() returns %d \n", wReturnStatusCode));
  }

  return wReturnStatusCode;
}

SYNCML_DM_RET_STATUS_T DMTree::ReplaceProperty( const DMString&         strURI,
                                                BOOLEAN                bInPlugin,
                                                const DMString&        strPluginURI,
                                                DMAddData&             oReplaceData,
                                                DMNode*                psReplacingNode,
                                                SYNCML_DM_RET_STATUS_T wURIValidateRetCode )
{
  SYNCML_DM_RET_STATUS_T  wReturnStatusCode = SYNCML_DM_SUCCESS;

  switch( wURIValidateRetCode )
  {
    case SYNCML_DM_COMMAND_ON_NAME_PROPERTY:
    {
      wReturnStatusCode = ReplaceNameProperty( strURI,
                                               bInPlugin,
                                               strPluginURI,
                                               oReplaceData,
                                               psReplacingNode,
                                               wURIValidateRetCode );
      break;
    }
    case SYNCML_DM_COMMAND_ON_ACL_PROPERTY:
    {
      wReturnStatusCode = ReplaceACLProperty( strURI,
                                              bInPlugin,
                                              strPluginURI,
                                              oReplaceData,
                                              psReplacingNode );
      break;
    }
    case SYNCML_DM_COMMAND_ON_TITLE_PROPERTY:
    {
      wReturnStatusCode = ReplaceTitleProperty( strURI,
                                                bInPlugin,
                                                strPluginURI,
                                                oReplaceData,
                                                psReplacingNode );
      break;
    }
    case SYNCML_DM_COMMAND_ON_FORMAT_PROPERTY:
    case SYNCML_DM_COMMAND_ON_SIZE_PROPERTY:
    case SYNCML_DM_COMMAND_ON_TYPE_PROPERTY:
    case SYNCML_DM_COMMAND_ON_TSTAMP_PROPERTY:
    case SYNCML_DM_COMMAND_ON_VERNO_PROPERTY:
    {
      wReturnStatusCode = SYNCML_DM_COMMAND_NOT_ALLOWED;
      break;
    }
    default:
    {
      wReturnStatusCode = SYNCML_DM_FEATURE_NOT_SUPPORTED;
      break;
    }
  }

  return wReturnStatusCode;
}

//------------------------------------------------------------------------
//
// FUNCTION        : Replace
//
// DESCRIPTION     : This function first will check the ACL and access type
//                   and after that if the both permissions are there then
//                   it will call respective plug-in.Replace.
//
// ARGUMENTS PASSED: UINT32 commandId
//                   UINT8 itemNumber
//                   SYNCML_DM_PLUGIN_ADD_T *pReplace
//                      -- Contains the replacing dat and it's format and
//                         type.
//                   BOOLEAN moreData
//                   BOOLEAN isThisAtomic
//
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T
//                      -- It returns SYNCML_DM_COMMAND_FAILED in case if
//                         pReplace is NULL or incase of invalid name if
//                         the server fire replace on ?prop=Name
//                      -- It returns SYNCML_DM_COMMAND_INVALID_URI incase
//                         if the input URI is invalid.
//                      - It returns SYNCML_DM_INVALID_URI
//                        if the node path is not valid for current DMT version
//                      -- It returns SYNCML_DM_URI_TOO_LONG incase
//                         if the input URI is long.
//                      -- It returns SYNCML_DM_DEVICE_FULL incase
//                         if the device is out of memory.
//                      -- It returns SYNCML_DM_FEATURE_NOT_SUPPORTED
//                         incase if the server fire replace on ?prop=Title
//                      -- It returns SYNCML_DM_COMMAND_NOT_ALLOWED incase
//                         if the command is not allowed on the target.
//                      -- It returns SYNCML_DM_REQUEST_ENTITY_TOO_LARGE
//                         incase if the replacing name is large.
//                      -- It returns SYNCML_DM_SUCCESS incase if the operation
//                         is success.
// PRE-CONDITIONS  :
// POST-CONDITIONS :
// IMPORTANT NOTES :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMTree::Replace( DMAddData & oReplaceData,
                                                                     SYNCML_DM_REQUEST_TYPE_T eRequestType)
{
  m_oLockContextManager.OnTreeAccessed();

  if(oReplaceData.getURI() == NULL)
    return(SYNCML_DM_COMMAND_INVALID_URI);

  SYNCML_DM_RET_STATUS_T  wReturnStatusCode = SYNCML_DM_SUCCESS;
  DMString strPluginURI;  // uri without ?xxx stuff for plugins and so on
  DMString strURI;

  if ( !GetPluginURI( oReplaceData.getURI(), strURI, strPluginURI ) )
    return SYNCML_DM_COMMAND_FAILED;

  SYNCML_DM_RET_STATUS_T wURIValidateRetCode = URIValidateAndParse( strURI );

  if((wURIValidateRetCode != SYNCML_DM_COMMAND_ON_NODE) &&
     (wURIValidateRetCode != SYNCML_DM_COMMAND_ON_ACL_PROPERTY) &&
     (wURIValidateRetCode != SYNCML_DM_COMMAND_ON_NAME_PROPERTY) &&
     (wURIValidateRetCode != SYNCML_DM_COMMAND_ON_TITLE_PROPERTY)
     )    //TStamp VerNo goes here.
    return(SYNCML_DM_COMMAND_NOT_ALLOWED);


  //This if statement checks for ?prop attribute such that
  // '?' is set to NULL.This is because before sending to plugins
  //?prop=<property_name> must be removed from the URI
  //example if ./SyncML?prop=ACL,we need to send only ./SyncML

  if(wURIValidateRetCode != SYNCML_DM_COMMAND_ON_NODE)
    strURI = strPluginURI;

  // check permission to write
  if (!VerifyArchiveWriteAccess(strURI)) {
    XPL_LOG_DM_TMN_Error(("archive has no write access for uri %s\n", strURI.c_str()));
    return (SYNCML_DM_COMMAND_NOT_ALLOWED);
  }

  DMNode *psReplacingNode = FindNodeByURI(strURI);

  BOOLEAN bIsEnabled;
  bIsEnabled = IsUriEnabled(strURI);
  if(psReplacingNode == NULL) // In this parent doesn't exists
  {

    if ( bIsEnabled == FALSE )
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
    else
        return SYNCML_DM_NOT_FOUND;

  }
  else
  {
    if ( bIsEnabled == FALSE )
        return SYNCML_DM_FEATURE_NOT_SUPPORTED;

  }

  BOOLEAN bInPlugin = FALSE;

  if (psReplacingNode->isPlugin())
    bInPlugin = TRUE;

  SYNCML_DM_FORMAT_T bFormat;
  psReplacingNode->GetFormat(strURI, &bFormat);

  // C23495 Verify replace parameters using DDF information
  //modify the URI sent to plugin by truncating ?prop
  if( ( wURIValidateRetCode != SYNCML_DM_COMMAND_ON_NODE) &&
      ( wURIValidateRetCode != SYNCML_DM_COMMAND_ON_FORMAT_PROPERTY)) //So far engine does not replacing FORMAT
  {
    oReplaceData.m_nFormat = bFormat;
  }

  //if (bInPlugin)
    //pReplace->bFormat = bFormat;

  //Added this check to first see if REPLACE command is allowed on the
  //TARGET node,only then ACL is checked

  DMMetaPCharVector asChildDepend;  // array of child/dependend nodes for indirect updates

  if( !m_oMDFObj.VerifyAccessType(strURI,
                                                 SYNCML_DM_REPLACE_ACCESS_TYPE,
                                                 &asChildDepend) )
  {
    XPL_LOG_DM_TMN_Debug(("<Replace> Access type is NOT there \n"));
    return (SYNCML_DM_COMMAND_NOT_ALLOWED);
  }

  /**
   * if ACL is being set for a leaf node, then ACL of the parent node
   * (or the parent's parent and so on) is used to enforce the replace operation
   *
   * If ACL is being set for an interior node, then the ACL of that node is
   * checked first and if it does not have replace permission, then the parent
   * node's ACL is checked for enforcement.
   */
  BOOLEAN bCheckParent = FALSE;

  if (wURIValidateRetCode == SYNCML_DM_COMMAND_ON_ACL_PROPERTY)
  {
    bCheckParent = NeedCheckParent( strURI,
                                    psReplacingNode,
                                    bInPlugin,
                                    bFormat,
                                    eRequestType);
  }

  if( bCheckParent )
  {
    DMString strParentURI = strURI, strLastSegment;

    if ( !GetLastSegmentOfURI(strParentURI, strLastSegment) )
      return SYNCML_DM_DEVICE_FULL;

    wReturnStatusCode = IsValidServer(strParentURI,
                                                     SYNCML_DM_REPLACE_ACCESS_TYPE,
                                                     eRequestType,
                                                     FALSE,
                                                     TRUE);
  }
  else
  {
    wReturnStatusCode = IsValidServer(strURI,
                                                     SYNCML_DM_REPLACE_ACCESS_TYPE,
                                                     eRequestType,
                                                     FALSE,
                                                     TRUE);
  }

  if(wReturnStatusCode != SYNCML_DM_SUCCESS)
    return(wReturnStatusCode);

  // The following condition is used check if the server is trying
  //  replace the value of the interior node. which is not possible.

  if((bFormat == SYNCML_DM_FORMAT_NODE) &&
     (wURIValidateRetCode == SYNCML_DM_COMMAND_ON_NODE))
  {
    XPL_LOG_DM_TMN_Debug(("Server is trying to replace the value of an interior node which is NOT allowed \n"));
     return(SYNCML_DM_COMMAND_NOT_ALLOWED);
  }

  // that means the URI in the Replace command doesn't have
  // any property "?prop=..

  if((bFormat != SYNCML_DM_FORMAT_NODE) &&
     (wURIValidateRetCode == SYNCML_DM_COMMAND_ON_NODE))
  {
    wReturnStatusCode = ReplaceNodeInternal( strURI,
                                             oReplaceData,
                                             psReplacingNode,
                                             asChildDepend );
  }
  else
  {
    wReturnStatusCode = ReplaceProperty( strURI,
                                         bInPlugin,
                                         strPluginURI,
                                         oReplaceData,
                                         psReplacingNode,
                                         wURIValidateRetCode );
  }

#ifndef DM_IGNORE_TSTAMP_AND_VERSION

  // set timestamp and increment version number
  if ( ( wReturnStatusCode == SYNCML_DM_SUCCESS ) &&
         !bInPlugin &&
         psReplacingNode)
  {
    psReplacingNode->SetTStamp(strPluginURI.c_str(), XPL_CLK_GetClock());
    psReplacingNode->SetVerNo(strPluginURI.c_str(), psReplacingNode->GetVerNo(strPluginURI.c_str()) + 1);
  }
#endif

  return(wReturnStatusCode);
}

//------------------------------------------------------------------------
// FUNCTION        : URIValidateAndParse
// DESCRIPTION     : This method validates URI as per RFC 2 and rules
//                   specified in TreeNode spec
//
// ARGUMENTS PASSED: pbURI :pointer to the URI to be validated
// RETURN VALUE    : SYNCML_DM_URI_RESULT_T indicating validity of URI
// PRE-CONDITIONS  :
// POST-CONDITIONS :
// IMPORTANT NOTES :
//------------------------------------------------------------------------
SYNCML_DM_URI_RESULT_T DMTree::URIValidateAndParse(CPCHAR pbURI) const
{
  if ( !pbURI )
    return SYNCML_DM_COMMAND_INVALID_URI;

  // checks that:
  // length of each segment is less or equal to GetMaxPathSegmentLength()
  // number of segments is equal or less than GetMaxPathDepth()
  int nFullLen = DmStrlen( pbURI );
  const char* szQpos = DmStrchr( pbURI, SYNCML_DM_QUESTION_MARK);
  const char* szPathEnd =  szQpos ? szQpos : pbURI + nFullLen;
  int nSegmentsCount = 1;
  int nSegmentLen = 0;

  if ( (szPathEnd - pbURI) > GetMaxTotalPathLength() )
    return SYNCML_DM_COMMAND_URI_TOO_LONG;

  if ( (szPathEnd - pbURI) == 0 || (*(szPathEnd -1)) == '/' )
    return SYNCML_DM_COMMAND_INVALID_URI;

  while ( pbURI <szPathEnd ) {
    if ( *pbURI == '/' ) { // next segment
      if ( nSegmentLen > GetMaxPathSegmentLength() )
        return SYNCML_DM_COMMAND_URI_TOO_LONG;

      if ( !nSegmentLen )
        return SYNCML_DM_COMMAND_INVALID_URI;

      nSegmentLen = 0;
      nSegmentsCount++;

      if ( nSegmentsCount > GetMaxPathDepth() )
        return SYNCML_DM_COMMAND_URI_TOO_LONG;

    } else
      nSegmentLen++;

    pbURI++;
  }

  // check optional part after '?'
  if ( !szQpos )
    return SYNCML_DM_COMMAND_ON_NODE;


  if( DmStrstr(szQpos, SYNCML_DM_PROP) != NULL) {
    // 6 is the lenth of the "?prop="
    szQpos += SYNCML_DM_PROP_LENGTH;

    if( DmStrcmp( szQpos, "ACL") == 0)
      return SYNCML_DM_COMMAND_ON_ACL_PROPERTY;

#ifdef LOB_SUPPORT
    if( DmStrcmp( szQpos, "ESN") == 0)
      return SYNCML_DM_COMMAND_ON_ESN_PROPERTY;
#endif
    if(DmStrcmp( szQpos, "Format") == 0)
      return SYNCML_DM_COMMAND_ON_FORMAT_PROPERTY;

    if(DmStrcmp( szQpos, "Name") == 0)
      return SYNCML_DM_COMMAND_ON_NAME_PROPERTY;

    if(DmStrcmp( szQpos, "Size") == 0)
      return SYNCML_DM_COMMAND_ON_SIZE_PROPERTY;

    if(DmStrcmp( szQpos, "VerNo") == 0)
      return SYNCML_DM_COMMAND_ON_VERNO_PROPERTY;

    if(DmStrcmp( szQpos, SYNCML_DM_TITLE) == 0)
      return SYNCML_DM_COMMAND_ON_TITLE_PROPERTY;

    if(DmStrcmp( szQpos, SYNCML_DM_TYPE) == 0)
      return SYNCML_DM_COMMAND_ON_TYPE_PROPERTY;

    if(DmStrcmp( szQpos, SYNCML_DM_TSTAMP) == 0)
      return SYNCML_DM_COMMAND_ON_TSTAMP_PROPERTY;

    return SYNCML_DM_COMMAND_ON_UNKNOWN_PROPERTY;
  }

  if (DmStrstr(szQpos, SYNCML_DM_LIST) != NULL) {

    // 6 is the lenth of the "?list="
    szQpos += SYNCML_DM_LIST_LENGTH;

    if(DmStrcmp(szQpos, SYNCML_DM_STRUCT) == 0)
      return SYNCML_DM_COMMAND_LIST_STRUCT;

    if(DmStrcmp(szQpos, SYNCML_DM_STRUCT_DATA) == 0)
      return SYNCML_DM_COMMAND_LIST_STRUCTDATA;

    if(DmStrncmp(szQpos,SYNCML_DM_TNDS, 4) == 0)
#ifdef TNDS_SUPPORT
      return SYNCML_DM_COMMAND_LIST_TNDS;
#else
      return SYNCML_DM_FEATURE_NOT_SUPPORTED;
#endif

    return SYNCML_DM_COMMAND_ON_UNKNOWN_PROPERTY;
  }

  return(SYNCML_DM_COMMAND_INVALID_URI);
}

//------------------------------------------------------------------------
//
// FUNCTION        : readOneWordFromTree
//
// DESCRIPTION     :
//
//
// ARGUMENTS PASSED: None
// RETURN VALUE    :
// PRE-CONDITIONS  :
// POST-CONDITIONS :
// IMPORTANT NOTES :
//------------------------------------------------------------------------
UINT16
DMTree::readOneWordFromTree(DMNode *pNode, UINT16 defaultValue)
{
    UINT16 retData = 0;

    if ( pNode )
    {
        if ( pNode->getData() && !(pNode->getData()->empty()) )
        {
            retData = DmAtoi((CPCHAR)pNode->getData()->getBuffer());
        }
    }

    if(retData == 0)
        retData = defaultValue;

    return retData;
}

// Simplified function for deserialize,
// does not perform lookup, justcreat a child for given node,
// reutrns newly created node.
SYNCML_DM_RET_STATUS_T DMTree::AddNode(DMNode **psNodeObject,
                                       const DMAddNodeProp & oAddNodeProperties,
                                       DMNode* pNewNode )
{
  DMNode *pParent = *psNodeObject;
  BOOLEAN bUpdateNode = TRUE;
  BOOLEAN bESN = FALSE;
  CPCHAR tmpSt = NULL;

  SYNCML_DM_RET_STATUS_T res;

  if ( !m_psRoot )
    return SYNCML_DM_FAIL;  // unexpected condition

#ifdef LOB_SUPPORT
 if((oAddNodeProperties.m_nFlags &  DMNode::enum_NodeESN) != 0)
  {    bESN = TRUE;
    tmpSt =oAddNodeProperties.getESNFileName();
  }
#endif
  // lookup for the node
  DMNode* pChild = pParent ? pParent->GetChildByName(oAddNodeProperties.getName()) : m_psRoot;

  if ( !pChild )
  {
    // node does not exist - create it
    pChild = pNewNode ? pNewNode : DMTree::CreateNodeObj(oAddNodeProperties.m_nFormat, bESN, tmpSt);

    if ( !pChild )
      return SYNCML_DM_DEVICE_FULL;

    // add child:
    pChild->pcParentOfNode = pParent;

    if ( pParent ) {
      pChild->pcNextSibling = pParent->pcFirstChild;
      pParent->pcFirstChild = pChild;
    }
  } else {
    if ( pNewNode )
      delete pNewNode;

    pNewNode = NULL;

    if ( !pChild->IsSkeletonNode() )
      bUpdateNode = FALSE;
  }


  if ( bUpdateNode )
  {
    res = pChild->set(&oAddNodeProperties);
    if ( res != SYNCML_DM_SUCCESS )
        return res;
   }

  *psNodeObject = pChild;
  return SYNCML_DM_SUCCESS;
}


//------------------------------------------------------------------------
// FUNCTION        : GetParentOfKeyValue
// DESCRIPTION     : This method returns the name of a parent whose child
//                   node's name and value of the child node(leaf node)
//                   are passed
// ARGUMENTS PASSED: pbValueOfKey pointer to value of leaf node
//                   pbKey : leaf node's name whose parent's name has to
//                           be found
// RETURN VALUE    : UINT8: name of parent node
//                   NULL if node does not exist
// PRE-CONDITIONS  : If the node does not exist,parent's name will be
//                   returned as NULL
// POST-CONDITIONS :
// IMPORTANT NOTES :This API is meant to be used when there can be many
//                  instances of a particular interior node and the UA
//                  needs to know the name of a parent,given it's child
//                  name and value
//------------------------------------------------------------------------
BOOLEAN DMTree::GetParentOfKeyValue(CPCHAR pbValueOfKey,
                                    CPCHAR pbKey,
                                    CPCHAR pbSubtreeURI,
                                    DMString& strResult )
{
  DMNode *psFindNode = NULL;
  DMNode *psKeyNode = NULL;
  SYNCML_DM_RET_STATUS_T wRetStausCode = SYNCML_DM_SUCCESS;
  DMGetData getData;
  DMString strPluginURI;
  DMString strURI;

  if((pbValueOfKey == NULL) || (pbKey == NULL) || (pbSubtreeURI == NULL))
  {
    XPL_LOG_DM_TMN_Error(("Invalid Parameters\n"));
    return FALSE;
  }

  if ( !GetPluginURI( pbSubtreeURI, strURI, strPluginURI ) )
  {
    XPL_LOG_DM_TMN_Error(("cannot parse URI\n"));
    return FALSE;
  }

  psFindNode = FindNodeByURI( strURI );
  if(psFindNode == NULL)
  {
    XPL_LOG_DM_TMN_Error(("Node isn't found\n"));
    return FALSE;
  }

  psFindNode = psFindNode->pcFirstChild;

  while(psFindNode != NULL)
  {
    strURI = pbSubtreeURI;
    strURI += "/";
    strURI += psFindNode->abNodeName;

    psKeyNode = FindNodeInNextSiblingsList(psFindNode->pcFirstChild,pbKey);
    if(psKeyNode != NULL)
    {
      strURI += "/";
      strURI += pbKey;

      wRetStausCode = psKeyNode->Get(strURI, getData);

      if( wRetStausCode == SYNCML_DM_SUCCESS )
      {
        if( getData.m_oData.compare(pbValueOfKey) )
        {
          strResult = psFindNode->abNodeName;
          return TRUE;
        }
      }
    }
    psFindNode = psFindNode->pcNextSibling;
  }// End of while(psFindNode != NULL)

  XPL_LOG_DM_TMN_Error(("Node isn't found\n"));
  return FALSE;
}

//------------------------------------------------------------------------
//
// FUNCTION        : InitListAndGetListFirstItem
//
// DESCRIPTION     : This function initializes the list and returns first.
//                   item for GetStruct
//
// ARGUMENTS PASSED: UINT8 *pbSegment
//                      - Pointer to the URI segment.
//                   SYNCML_DM_GET_ON_LIST_T bGetOnList
//                      - whether get is ?list=Struct or ?list=StructData
//                   SYNCML_DM_GET_ON_LIST_RET_DATA_T **ppsReturnData
//                      - Contains first item
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T
//                      - It returns SYNCML_DM_COMMAND_FAILED incase if
//                        the URI is NULL or if root node pointer is NULL
//                        or if the URI is invalid.
//                      - It returns SYNCML_DM_DEVICE_FULL if the device
//                        is out of memory.
//                      - It returns SYNCML_DM_SUCCESS if the operation is
//                        success.
//                      - It returns SYNCML_DM_URI_TOO_LONG if the uri is
//                        long.
// PRE-CONDITIONS  :
// POST-CONDITIONS :
// IMPORTANT NOTES :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMTree::InitListAndGetListFirstItem(
                     CPCHAR pbURI,
                     SYNCML_DM_GET_ON_LIST_T bGetOnList,
                     SYNCML_DM_GET_ON_LIST_RET_DATA_T& ppsReturnData)
{
  DMString strFullURI;


  if( !GetPluginURI(pbURI, strFullURI, ppsReturnData.m_strStartURI) || ( m_psRoot == NULL))
  {
    return(SYNCML_DM_COMMAND_FAILED);
  }

  ppsReturnData._pbURI = ppsReturnData.m_strStartURI;
  return GetListItemData( ppsReturnData );
}

SYNCML_DM_RET_STATUS_T DMTree::GetListItemData(SYNCML_DM_GET_ON_LIST_RET_DATA_T& ppsReturnData)
{

  DMGetData * pData = NULL;
  pData = new DMGetData();

  if ( pData == NULL )
    return SYNCML_DM_DEVICE_FULL;

  ppsReturnData.psRetData = pData;
  SYNCML_DM_RET_STATUS_T nRes = Get(ppsReturnData._pbURI, *pData ,SYNCML_DM_REQUEST_TYPE_SERVER);

  if ( nRes == SYNCML_DM_SUCCESS && pData->m_nFormat == SYNCML_DM_FORMAT_NODE )
  {

    DMString strChild( pData->getCharData() );
    char* szSlash = (char*)DmStrchr(strChild, '/');
    if ( szSlash )
      *szSlash = 0;
    if ( strChild.empty() )
      ppsReturnData.m_strNextChild = "";
    else {
      ppsReturnData.m_strNextChild = ppsReturnData._pbURI;
      ppsReturnData.m_strNextChild += "/";
      ppsReturnData.m_strNextChild += strChild;
    }
  } else
    ppsReturnData.m_strNextChild = "";

  return nRes;
}

//------------------------------------------------------------------------
// FUNCTION        : GetListNextItem
// DESCRIPTION     : The UA calls this to get the entire structure of the
//                   subtree.The Tree traverses the subtree and returns
//                   the list of names for which the current server has
//                   ACL permissions.
// ARGUMENTS PASSED: dwCommandId command Id
//                   bItemNumber
//                   pbURI target URI whose subtree structure has to be
//                   returned
//                   ppsReturnData containing linked list of the names
// RETURN VALUE    : ppsReturnData and SYNCML_DM_RET_STATUS_T
//                   SYNCML_DM_SUCCESS if successful
//                   else error specific code indicating failure
// PRE-CONDITIONS  : None
// POST-CONDITIONS : None
// IMPORTANT NOTES : None
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMTree::GetListNextItem(SYNCML_DM_GET_ON_LIST_RET_DATA_T& ppsReturnData)
{

   XPL_LOG_DM_TMN_Debug(("DMTree:  in the  GetListNextItem() for %s\n",  ppsReturnData._pbURI.c_str()));

  // look for the next node -
  // 1. try child
  if ( !ppsReturnData.m_strNextChild.empty() ) {
    ppsReturnData._pbURI = ppsReturnData.m_strNextChild;
    int ret = GetListItemData( ppsReturnData );

    //returns only in case of success; otherwise skip this node and going to take another sibbling (do/while loop)
    if( ret == SYNCML_DM_SUCCESS ){
        return ret;
    }
    XPL_LOG_DM_TMN_Error(("DMTree: The function GetListItemData() (1) for %s returns error = %d \n", ppsReturnData._pbURI.c_str(), ret ));
  }

  do {
    // 2. try next sibling
    DMString strURI = ppsReturnData._pbURI, strLastSegment;

    if ( !GetLastSegmentOfURI( strURI, strLastSegment ) ) // no parent
      return SYNCML_DM_SUCCESS;

    // in case of empty node we should not look for sibling...
    if ( strURI.length() < ppsReturnData.m_strStartURI.length() )
      return SYNCML_DM_SUCCESS; // no more items

    DMGetData getData;

    SYNCML_DM_RET_STATUS_T nRes = Get(strURI, getData,SYNCML_DM_REQUEST_TYPE_SERVER);

    if ( nRes != SYNCML_DM_SUCCESS )
      return nRes;

    DMString strAllChildren( getData.getCharData() ), strChild;

    while ( DmStringParserGetItem( strChild, strAllChildren, '/' ) )
    {
      XPL_LOG_DM_TMN_Debug(("DMTree: strChild =  %s , strAllChildren = %s\n", strChild.c_str(), strAllChildren.c_str() ));

      if ( strChild == strLastSegment ) { // found current node - take next

        while ( DmStringParserGetItem( strChild, strAllChildren, '/' ) )
        {
          ppsReturnData._pbURI = strURI;
          ppsReturnData._pbURI += "/";
          ppsReturnData._pbURI+= strChild;
          int ret = GetListItemData( ppsReturnData );

          //returns only in case of success; otherwise skip this node and going to take another sibbling
          if( ret == SYNCML_DM_SUCCESS ){
               return ret;
          }
          else{
             XPL_LOG_DM_TMN_Error(("DMTree: The function GetListItemData() (2) for %s returns error = %d \n", ppsReturnData._pbURI.c_str(), ret ));
             continue;
          }
        }
        break;
      }
    }

    // 3. take parent
    ppsReturnData._pbURI = strURI;
    if ( strURI.length() <= ppsReturnData.m_strStartURI.length() )
      return SYNCML_DM_SUCCESS; // no more items

    // goto step 2...
  } while ( 1 );
}

//------------------------------------------------------------------------
//
// FUNCTION        : InitSerializationList
//
// DESCRIPTION     : This function will allocate and initialize the values
//                   of the LIST_STRUCT_OR_STRUCT_DATA_INFO_T psListInfo.
//
// ARGUMENTS PASSED:
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T
//                      - It returns SYNCML_DM_COMMAND_FAILED if the
//                        root node is NULL.
//                      - It returns SYNCML_DM_DEVICE_FULL if the device
//                        is out of memory.
//                      - It returns SYNCML_DM_SUCCESS if the operation is
//                        success.
// PRE-CONDITIONS  :
// POST-CONDITIONS :
// IMPORTANT NOTES :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMTree::InitSerializationList(DMNode * serializeRoot)
{
  psListInfo.Clear();
  psListInfo.psCurrent = serializeRoot;
  return(SYNCML_DM_SUCCESS);
}

//------------------------------------------------------------------------
//
// FUNCTION        : GetSerializationListNextItem
//
// DESCRIPTION     : This function will return the properties of a node.
//                   This function will use DFS algorithm to traverse the
//                   tree.
//
//
// ARGUMENTS PASSED: DMNode *psRetNode the node to serialize next
//
// RETURN VALUE    : SYNCML_DM_SERIALIZATION_STATUS_T
//                      -- It returns SYNCML_DM_SERIALIZATION_FAIL
//                         if there is any failure.
//                      -- It returns SYNCML_DM_SERIALIZATION_SUCCESS
//                         if it can retrieve the properties of a node.
//                      -- It returns SYNCML_DM_TREE_TRAVERSING_OVER
//                         in case if the it has visited all the nodes.
//                         in a tree.
// PRE-CONDITIONS  :
// POST-CONDITIONS :
// IMPORTANT NOTES :
//------------------------------------------------------------------------
SYNCML_DM_SERIALIZATION_STATUS_T DMTree::
GetSerializationListNextItem (DMNode **ppsRetNode, INT32& nEndTagsNumber )
{
  DMNode *psCurrent = *ppsRetNode;

  nEndTagsNumber = 0;
  if ( !psCurrent ) {
      *ppsRetNode = psListInfo.psCurrent;
      return SYNCML_DM_SERIALIZATION_SUCCESS;
  }

  // try child first
  DMNode* pChild = psCurrent->pcFirstChild ?
        psCurrent->pcFirstChild->GetNextSerializeItem() : NULL;

  if ( pChild ) {
      *ppsRetNode = pChild;
      return SYNCML_DM_SERIALIZATION_SUCCESS;
  }

  // look up for sibling of the parent including self
  DMNode *pParent = psCurrent;

  while ( pParent != psListInfo.psCurrent ) {
    // try next sibling after that
    DMNode *pNextSibling = pParent->pcNextSibling ?
          pParent->pcNextSibling->GetNextSerializeItem() : NULL;

    nEndTagsNumber++; // even sibling required at least one "end" tag

    if ( pNextSibling ) {
      *ppsRetNode = pNextSibling;
      return SYNCML_DM_SERIALIZATION_SUCCESS;
    }

    // take parent
    pParent = pParent->pcParentOfNode;
  }

  // not found
  return SYNCML_DM_TREE_TRAVERSING_OVER;
}

SyncML_DM_Archive* DMTree::GetArchive(const DMNode * node)
{
   const DMNode * tmpNode=node;
   while (tmpNode != NULL && tmpNode->pArchive == NULL)
   {
      tmpNode=tmpNode->pcParentOfNode;
   }

   return tmpNode ? tmpNode->pArchive : NULL;
}

DMNode* DMTree::CreateNodeObj( SYNCML_DM_FORMAT_T bFormat, BOOLEAN isESN, CPCHAR pbFileName )
{
  switch ( bFormat )
  {
    case SYNCML_DM_FORMAT_NODE:
      return(new DMDefaultInteriorNode);

    case SYNCML_DM_FORMAT_NODE_PDATA:
      return(new DMOverlayPINode);

    default:
#ifdef LOB_SUPPORT
    if(isESN)
        return( new DMDefaultESN(pbFileName));
    else
#endif
      return(new DMDefaultLeafNode);
  }
}

DMNode* DMTree::CreateSkeletonNode( CPCHAR pbURI )
{
  DMString strURI = pbURI;
  char *szURI = strURI.GetBuffer();
  const char *pbURISegment = GetURISegment(&szURI);

  if ( !m_psRoot ) {
    m_psRoot = DMTree::CreateNodeObj(SYNCML_DM_FORMAT_NODE, FALSE, NULL);

    if ( !m_psRoot )
      return NULL; // out of memory

    m_psRoot->abNodeName = pbURISegment; // it should be "."
    m_psRoot->setFlags( DMNode::enum_NodeSkeleton );
  }

  DMNode *psParentNode  = m_psRoot;

  pbURISegment = GetURISegment(&szURI);   // skip dot

  while( pbURISegment ) {

    DMNode *pChild = psParentNode->GetChildByName(pbURISegment);

    if ( !pChild ) {
      pChild = DMTree::CreateNodeObj(SYNCML_DM_FORMAT_NODE, FALSE, NULL);

      if ( !pChild )
        return NULL; // out of memory

      pChild->abNodeName = pbURISegment;
      pChild->setFlags( DMNode::enum_NodeSkeleton );

      pChild->pcParentOfNode = psParentNode;

      pChild->pcNextSibling = psParentNode->pcFirstChild;
      psParentNode->pcFirstChild = pChild;
    }

    psParentNode = pChild;
    pbURISegment = GetURISegment(&szURI);
  }
  return psParentNode;
}

// function "detaches" old node from the tree and inserts new node in the same place.
void DMTree::SubstituteNode( DMNode* pOldNode, DMNode* pNewNode )
{
  // parent
  if ( pOldNode->pcParentOfNode && pOldNode->pcParentOfNode->pcFirstChild == pOldNode )
    pOldNode->pcParentOfNode->pcFirstChild = pNewNode;

  // prev sibling
  DMNode *pNode = pOldNode->pcParentOfNode ? pOldNode->pcParentOfNode->pcFirstChild : NULL;

  while ( pNode )
  {
    if ( pNode->pcNextSibling == pOldNode )
      pNode->pcNextSibling = pNewNode;

    pNode = pNode->pcNextSibling;
  }

  // children
  pNode = pOldNode->pcFirstChild;
  while ( pNode )
  {
    pNode->pcParentOfNode = pNewNode;

    pNode = pNode->pcNextSibling;
  }

}

/**
 * Get the immediate children of a node based on the desired node type.
 *
 * @author Andy
 * @param uri the URI to the node E.g. ./DevInfo
 * @param childrenMap the map that will hold the URI of the children
 *                    and the children
 * @param nodeType the type of nodes that get store in the map either leaf node
 *                 or interior node
 * @return the status of the operation
 */
SYNCML_DM_RET_STATUS_T
DMTree::getChildren( CPCHAR uri,
                                     DMMap<DMString, UINT32>& childrenMap,
                                     DMTNM_NODE_TYPE nodeType,
                                     SYNCML_DM_REQUEST_TYPE_T eRequestType)
{
  SYNCML_DM_RET_STATUS_T eSuccessCode = SYNCML_DM_SUCCESS;
  DMGetData parentNodeData;

  eSuccessCode = Get(uri, parentNodeData,eRequestType);

  if (eSuccessCode != SYNCML_DM_SUCCESS)
    return SYNCML_DM_COMMAND_FAILED;

  DMString remindStr = parentNodeData.getCharData(); // children names
  DMString tmpStr;
  DMString tmpRootPath = uri;
  DMString tmpChildPath;
  DMString sperator = "/";
  DMGetData * tmpNodeData = NULL;

  // loop through children that seperated by forward slash
  while(DmStringParserGetItem(tmpStr, remindStr, '/'))
  {
      tmpChildPath += tmpRootPath;
      tmpChildPath += sperator;
      tmpChildPath += tmpStr;

      tmpNodeData = new DMGetData();
      if ( tmpNodeData == NULL )
      {
         dmFreeGetMap(childrenMap);
         return SYNCML_DM_DEVICE_FULL;
      }

      eSuccessCode = Get(tmpChildPath.c_str(), *tmpNodeData,eRequestType); // get child node
      if (eSuccessCode != SYNCML_DM_SUCCESS)
      {
          dmFreeGetMap(childrenMap);
          return SYNCML_DM_COMMAND_FAILED;
      }

      if (tmpNodeData->m_nFormat == SYNCML_DM_FORMAT_NODE && nodeType == DMTNM_NODE_INTERIOR)
      {
         childrenMap.put(tmpChildPath.c_str(), (UINT32)tmpNodeData);
      }
      else
        if (tmpNodeData->m_nFormat != SYNCML_DM_FORMAT_NODE && nodeType == DMTNM_NODE_LEAF)
        {
           childrenMap.put(tmpChildPath.c_str(), (UINT32)tmpNodeData);
        }
        else // no match
         DMGetData::operator delete (tmpNodeData);

     tmpChildPath = NULL;
  }
  return eSuccessCode;
}

/**
 * Get the immediate leaf children
 *
 * @author Andy
 * @param uri the URI to the node E.g. ./DevInfo
 * @param childrenMap the map that will hold the URI of the children
 *                    and the children
 * @return the status of the operation
 */
SYNCML_DM_RET_STATUS_T
DMTree::getLeafChildren( CPCHAR uri,
                                             DMMap<DMString,
                                             UINT32>& childrenMap,
                                             SYNCML_DM_REQUEST_TYPE_T eRequestType)
{
    return getChildren(uri, childrenMap, DMTNM_NODE_LEAF,eRequestType);
}

/**
 * Get the immediate interior children
 *
 * @author Andy
 * @param uri the URI to the node E.g. ./DevInfo
 * @param childrenMap the map that will hold the URI of the children
 *                    and the children
 * @return the status of the operation
 */
SYNCML_DM_RET_STATUS_T
DMTree::getInteriorChildren( CPCHAR uri,
                                                DMMap<DMString,
                                                UINT32>& childrenMap,
                                                SYNCML_DM_REQUEST_TYPE_T eRequestType)
{
    return getChildren(uri, childrenMap, DMTNM_NODE_INTERIOR,eRequestType);
}

/**
 * Set the leaf children with the new set of leaf children.  If the children already
 * exist, replace them.  If the children are new, add them.  If the children were not
 * in the new set of leaf children, remove them.  Since this is a atomic operation,
 * it will roll back to the original state if any of above operations failed,
 *
 * @author Andy
 * @param uri the URI to the node E.g. ./DevInfo
 * @param childrenMap the new set of leaf children
 * @return the status of the operation
 */
SYNCML_DM_RET_STATUS_T DMTree::setLeafChildren( CPCHAR uri, DMMap<DMString, UINT32>& childrenMap )
{

    DMMap<DMString, UINT32> oldChildrenMap;
    SYNCML_DM_RET_STATUS_T status = SYNCML_DM_SUCCESS;

    status = getLeafChildren(uri,
                                 (DMMap<DMString, UINT32>&)oldChildrenMap,
                                 SYNCML_DM_REQUEST_TYPE_API);

    if (status != SYNCML_DM_SUCCESS) {
        dmFreeGetMap((DMMap<DMString, UINT32>&)oldChildrenMap);
        return status;
    }

    int numOfNewChildren = childrenMap.size();

    DMVector<INT8> deleteList;  // mark the delete on a child for rollback
    deleteList.set_size(numOfNewChildren);
    for (int i = 0; i < numOfNewChildren; i++)
    {
        deleteList[i] = CLEAN;
    }

    int numOfOldChildren = oldChildrenMap.size();
    // mark the action (add or replace) on a child for rollback
    DMVector<INT8> actionList;
    actionList.set_size(numOfOldChildren);

    if (numOfNewChildren != 0)
    {
        for (int i = 0; i < numOfOldChildren; i++)
        {
            actionList[i] = CLEAN;
        }
        status = handleNewChildren(oldChildrenMap,  (DMMap<DMString, UINT32>&)childrenMap,
                                   (INT8*)actionList.get_data(), (INT8*)deleteList.get_data());
    }
    else
    {
        for (int i = 0; i < numOfOldChildren; i++)
        {
            actionList[i] = DIRTY;  //mark DIRTY to delete all old leaf children
        }
    }

    if (status == SYNCML_DM_SUCCESS)
    {
        status = handleOldChildren(oldChildrenMap,  (DMMap<DMString, UINT32>&)childrenMap,
                                   (INT8*)actionList.get_data(), (INT8*)deleteList.get_data());
    }

    dmFreeGetMap((DMMap<DMString, UINT32>&)oldChildrenMap);
    return status;
}

/**
 * Loop through the new set of children and compare them with the old set of children
 * If exist in the old set of children, replace them with new value from the new children.
 * If not exist, add them.  Roll back when any of those operation failed.
 *
 * @author Andy
 * @param oldChildrenMap the old set of leaf children
 * @param newChildrenMap the new set of leaf children
 * @param actionList a list that match with the order of the old children in the map. It
 *                   marks REPLACED if the child was replaced and marks DIRTY otherwise.
 *                   This list will be use to replace back the old value when rollback.
 *
 * @param deleteList a list that match with the order of the new children in the map and
 *                   marks DELETED if the child is added.  This list will be use to delete newly
 *                   added node when rollback.
 * @return the status of the operation
 */
SYNCML_DM_RET_STATUS_T DMTree::handleNewChildren( DMMap<DMString, UINT32> oldChildrenMap,
                                            DMMap<DMString, UINT32>& newChildrenMap,
                                            INT8 actionList[],
                                            INT8 deleteList[] )
{
    SYNCML_DM_RET_STATUS_T status = SYNCML_DM_SUCCESS;
    INT32 oldIndex = 0;
    INT32 newIndex = 0;
    BOOLEAN found = FALSE;

    for ( DMMap<DMString, UINT32>::POS  newIt= newChildrenMap.begin(); newIt < newChildrenMap.end(); newIt++ )
    {
      DMAddData *newData = (DMAddData*)newChildrenMap.get_value( newIt );

      // compare the new child with list of old children
      for ( DMMap<DMString, UINT32>::POS oldIt = oldChildrenMap.begin(); oldIt < oldChildrenMap.end(); oldIt++ )
      {

         if (actionList[oldIndex] == CLEAN || actionList[oldIndex] != REPLACED)
         {    // if not replaced yet
            const DMString & oldNodeURI = oldChildrenMap.get_key( oldIt );
            if ( oldNodeURI == newData->getURI() ) // new child has the same name as the old child
            {
               status = Replace( *newData, SYNCML_DM_REQUEST_TYPE_API );
               if (status != SYNCML_DM_SUCCESS)
               {
                  SYNCML_DM_RET_STATUS_T rollbackStatus = rollback(oldChildrenMap, newChildrenMap, actionList, deleteList);
                  if (rollbackStatus == SYNCML_DM_SUCCESS)
                     return status;
                  else
                     return rollbackStatus;
               }
               actionList[oldIndex] = REPLACED;     // mark replece for roll back
               found = TRUE;
               break;
            }
            else
            {
              actionList[oldIndex] = DIRTY;         // mark visited
            }
         }
         oldIndex++;
      }

      if (!found)
      {
        status = Add( *newData, SYNCML_DM_REQUEST_TYPE_API );
        if (status != SYNCML_DM_SUCCESS)
        {
           SYNCML_DM_RET_STATUS_T rollbackStatus = rollback(oldChildrenMap, newChildrenMap, actionList, deleteList);
           if (rollbackStatus == SYNCML_DM_SUCCESS)
              return status;
           else
              return rollbackStatus;
        }
        deleteList[newIndex] = DELETED;  // mark delete for roll back
      }
      else
      {
        deleteList[newIndex] = DIRTY;    // mark visited
      }
      newIndex++;
      oldIndex = 0;
      found = FALSE;
   }
   return status;
}

/**
 * Loop through the old set of children and check if it has been replaced.
 * If false, delete the child.  Roll back when fail to delete.
 *
 * @author Andy
 * @param oldChildrenMap the old set of leaf children
 * @param newChildrenMap the new set of leaf children
 * @param actionList a list that match with the order of the old children in the map. It
 *                   marks ADDED if the child was added. This list will be use to add back
 *                   the old value when rollback.
 *
 * @param deleteList a list that match with the order of the new children in the map and
 *                   will be use to delete newly added node when rollback.
 * @return the status of the operation
 */
SYNCML_DM_RET_STATUS_T DMTree::handleOldChildren( DMMap<DMString, UINT32> oldChildrenMap,
                                            DMMap<DMString, UINT32>& newChildrenMap,
                                            INT8 actionList[],
                                            INT8 deleteList[] )
{
   SYNCML_DM_RET_STATUS_T status = SYNCML_DM_SUCCESS;
   SYNCML_DM_RET_STATUS_T rollbackStatus = SYNCML_DM_SUCCESS;
   int oldIndex = 0;

   for ( DMMap<DMString, UINT32>::POS oldIt = oldChildrenMap.begin(); oldIt < oldChildrenMap.end(); oldIt++ )
   {
      if (actionList[oldIndex] == DIRTY)
      {    // if has not been replaced means the child is not in the new list
          const DMString & oldNodeURI = oldChildrenMap.get_key( oldIt );
          status = Delete( oldNodeURI, SYNCML_DM_REQUEST_TYPE_API );
          if (status != SYNCML_DM_SUCCESS)
          {
             rollbackStatus = rollback(oldChildrenMap, newChildrenMap, actionList, deleteList);
             if (rollbackStatus == SYNCML_DM_SUCCESS)
                return status;
             else
                return rollbackStatus;
          }
          actionList[oldIndex] = ADDED;  // mark for add back on roll back
      }
      oldIndex++;
   }
   return status;
}

/**
 * Rollback operation for SetLeafChildren.  It will replace the replaced children
 * with original values.  It will delete the added children.  It will add back
 * the delete children.
 *
 * @author Andy
 * @param oldChildrenMap the old set of leaf children
 * @param newChildrenMap the new set of leaf children
 * @param actionList a list that match with the order of the old children in the map. It marks
 *                   the children with REPLACED means replace with the children with old children
 *                   It marks children with ADDED means add back the old children to the parent.
 *
 * @param deleteList a list that match with the order of the new children in the map.
 *                   DELETED mark mean delete the child
 * @return the status of the operation
 */
SYNCML_DM_RET_STATUS_T DMTree::rollback( DMMap<DMString, UINT32> oldChildrenMap,
                                     DMMap<DMString, UINT32>& newChildrenMap,
                                     INT8 actionList[],
                                     INT8 deleteList[] )
{
   int i = 0;
   SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;
   DMAddData oldData;

   // unreplace and add back
   for ( DMMap<DMString, UINT32>::POS oldIt = oldChildrenMap.begin(); oldIt < oldChildrenMap.end(); oldIt++ )
   {
       if (actionList[i] != CLEAN)
       {
           oldData.clear();
           DMGetData* oldGetData = (DMGetData*)oldChildrenMap.get_value( oldIt );
           retStatus = oldData.set(oldChildrenMap.get_key(oldIt),
                                   oldGetData->m_nFormat,
                                   (CPCHAR)oldGetData->m_oData.getBuffer(),
                                   oldGetData->m_oData.getSize(),
                                   oldGetData->getType());
           if (retStatus != SYNCML_DM_SUCCESS)
                   return retStatus;

           if (actionList[i] == REPLACED)
           {  // replace back
              retStatus = Replace( oldData, SYNCML_DM_REQUEST_TYPE_API );
              if (retStatus != SYNCML_DM_SUCCESS)
                 return retStatus;
           }
           else
             if (actionList[i] == ADDED)
             { // add back
                retStatus = Add( oldData, SYNCML_DM_REQUEST_TYPE_API );
                if (retStatus != SYNCML_DM_SUCCESS)
                   return retStatus;
             }
       }
       else
       {
         break;    // didn't touch the node, so stop over here
       }
       i++;
   }

   i = 0;
   // delete added nodes
   for ( DMMap<DMString, UINT32>::POS newIt = newChildrenMap.begin(); newIt < newChildrenMap.end(); newIt++ )
   {
     if (deleteList[i] != CLEAN)
     {
       if (deleteList[i] == DELETED)
       {
         retStatus = Delete(((DMAddData*)newChildrenMap.get_value( newIt ))->getURI(),
                            SYNCML_DM_REQUEST_TYPE_API);
         if (retStatus != SYNCML_DM_SUCCESS)
            return retStatus;
       }
     }
     else
     {
       break; // didn't touch the node, so stop over here
     }
     i++;
   } // end for loop
   return SYNCML_DM_SUCCESS;
}



/*==================================================================================================
Function:    GetSubNodeValue

Description: This method is called to retrieve the a value from beneath the DMAcc node for the
             current Server.)
ARGUMENT PASSED : pParentName
                  pSubNode

OUTPUT PARAMETER: ppDmaccData
RETURN VALUE    : SYNCML_DM_SUCCESS or SYNCML_DM_FAIL
IMPORTANT NOTES : This method assumes the DM Tree is locked and the management session is in
                  progress.

==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMTree::GetAccNodeValue (CPCHAR       pParentName,
                        CPCHAR       pSubNode,
                        DMGetData &  oAccData)
{
  return Get( ::XPL_DM_GetEnv( SYNCML_DM_DMACC_ROOT_PATH ) +
                       DMString( DM_STR_SLASH ) +
                       pParentName +
                       DM_STR_SLASH +
                       pSubNode,
                       oAccData,
                       SYNCML_DM_REQUEST_TYPE_INTERNAL );

}


/**
 * Get the default server address information
 *
 * @param pAccProfileName the DMAcc node name
 * @param oAddr the object that will hold the server address
 * @param oAddrType the object that will hold the server address type, valud values are "URI", "IPv4"or "IPv6"
 * @param oPortNbr the object that will hold the port address, pPortNbr.oData.getSize() is zero if PortNbr node not found or not set
 * @return the status of the operation
 **/
SYNCML_DM_RET_STATUS_T
DMTree::GetDefAccountAddrInfo(CPCHAR pAccProfileName,
                              DMGetData & oAddr,
                              DMGetData & oAddrType,
                              DMGetData & oPortNbr)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_FAIL;

    if (  m_bVersion_1_2 == TRUE )
    {
      DMString strNodeURI = ::XPL_DM_GetEnv( SYNCML_DM_DMACC_ROOT_PATH ) +
                            DMString( DM_STR_SLASH ) +
                            pAccProfileName +
                            DM_STR_SLASH +
                            DM_APPADDR;

        DMMap<DMString, UINT32> addrNodesMap;

        dm_stat = getInteriorChildren( strNodeURI, addrNodesMap, SYNCML_DM_REQUEST_TYPE_INTERNAL);

        if (dm_stat != SYNCML_DM_SUCCESS)
      {
           dmFreeGetMap((DMMap<DMString, UINT32>&)addrNodesMap);
           return SYNCML_DM_FAIL;
        }

        if( addrNodesMap.size() == 0 )
        {
          dmFreeGetMap((DMMap<DMString, UINT32>&)addrNodesMap);
          return SYNCML_DM_NOT_FOUND;
        }

        while ( true )
      {
           DMMap<DMString, UINT32>::POS  iter = addrNodesMap.begin();
           DMString addrNodeName = addrNodesMap.get_key(iter);

         addrNodeName += DM_STR_SLASH;

           dm_stat = Get( addrNodeName + DM_ADDR, oAddr,  SYNCML_DM_REQUEST_TYPE_INTERNAL);
           if ( dm_stat != SYNCML_DM_SUCCESS ) {
              break;
           }

           // Get the Addr Type
           dm_stat = Get(addrNodeName + DM_ADDRTYPE, oAddrType,  SYNCML_DM_REQUEST_TYPE_INTERNAL);
           if ( dm_stat != SYNCML_DM_SUCCESS )
           {
                // session will set to default (1=http) if null
               oAddrType.set(SYNCML_DM_FORMAT_CHR, "1", 1, NULL);
           }

           // Get the portNbrs if any is specified
           DMMap<DMString, UINT32>portNbrNodesMap;
           dm_stat = getInteriorChildren( addrNodeName + DM_PORT, portNbrNodesMap, SYNCML_DM_REQUEST_TYPE_INTERNAL);

           // if there are no child nodes, but this is OK since portNbr is optional
           if ( dm_stat != SYNCML_DM_SUCCESS )
         {
              DMString dummyStr = NULL;
              oPortNbr.set(SYNCML_DM_FORMAT_NULL, dummyStr, 0, NULL);
              dmFreeGetMap((DMMap<DMString, UINT32>&)addrNodesMap);
              return SYNCML_DM_SUCCESS;
           }

           // if portNbr node is present, retrieve its value
           iter = portNbrNodesMap.begin();
           DMString portNbrName = portNbrNodesMap.get_key(iter);

           dm_stat = Get(portNbrName + DM_STR_SLASH + DM_PORTNBR, oPortNbr,  SYNCML_DM_REQUEST_TYPE_INTERNAL);

           if ( dm_stat != SYNCML_DM_SUCCESS )
         {
              DMString dummyStr = NULL;
              oPortNbr.set(SYNCML_DM_FORMAT_NULL, dummyStr, 0, NULL);
           }

           dmFreeGetMap((DMMap<DMString, UINT32>&)addrNodesMap);
           dmFreeGetMap((DMMap<DMString, UINT32>&)portNbrNodesMap);
           return SYNCML_DM_SUCCESS;
        }

        // Got here because something went wrong
        dmFreeGetMap((DMMap<DMString, UINT32>&)addrNodesMap);
    }
    else
    {
        /* Fill the pNodeUri with the string "./SyncML/DMAcc/<pAccProfileName>/AppAddr" */
        DMString strNodeURI = ::XPL_DM_GetEnv( SYNCML_DM_DMACC_ROOT_PATH ) +
                              DMString( DM_STR_SLASH ) +
                              pAccProfileName +
                              DM_STR_SLASH;

        dm_stat = Get(strNodeURI + DM_ADDR, oAddr, SYNCML_DM_REQUEST_TYPE_INTERNAL);

        if ( dm_stat == SYNCML_DM_SUCCESS )
        {
             dm_stat = Get( strNodeURI + DM_ADDRTYPE, oAddrType, SYNCML_DM_REQUEST_TYPE_INTERNAL);
             if ( dm_stat == SYNCML_DM_SUCCESS )
             {
                 dm_stat = Get( strNodeURI + DM_PORTNBR, oPortNbr, SYNCML_DM_REQUEST_TYPE_INTERNAL);

                 if ( SYNCML_DM_NOT_FOUND == dm_stat )
                 {
                     // DM: optional, set to default if not found
                     DMString dummyStr = NULL;
                     oPortNbr.set(SYNCML_DM_FORMAT_NULL, dummyStr, 0, NULL);
                     dm_stat = SYNCML_DM_SUCCESS;
                 }
             }
         }
    }

    return dm_stat;
}

/**
 * Get the server authentication information in the specified DMAcc profile
 *
 * @param pAccProfileName the DMAcc node name
 * @param oAuthType the object that will hold the authentication type
 * @param oAuthName the object that will hold the authentication name
 * @param oAuthSecret the object that will hold the authentication secret
 * @param oAuthData the object that will hold the authentication data
 * @param sAuthDataUri the DMString object that will hold the full server AAuthData (nonce) URI
 * @return the status of the operation
 **/
SYNCML_DM_RET_STATUS_T
DMTree::GetServerAuthInfo(CPCHAR pAccProfileName,
                          CPCHAR pAuthType,
                          DMGetData& oAuthName,
                          DMGetData& oAuthSecret,
                          DMGetData& oAuthData,
                          DMString& oAuthDataUri)
{
    DMGetData oAuthType; // dummy parameter

    return this->GetAuthInfo(pAccProfileName,
                             DM_AUTHLEVEL_SRVCRED,
                             pAuthType,
                             oAuthType,
                             oAuthName,
                             oAuthSecret,
                             oAuthData,
                             oAuthDataUri);
}

/**
 * Get the HTTP authentication information in the specified DMAcc profile
 *
 * @param pAccProfileName the DMAcc node name
 * @param oAuthType the object that will hold the authentication type
 * @param oAuthName the object that will hold the authentication name
 * @param oAuthSecret the object that will hold the authentication secret
 * @param oAuthData the object that will hold the authentication data
 * @return the status of the operation
 **/
SYNCML_DM_RET_STATUS_T
DMTree::GetHttpAuthInfo(CPCHAR pAccProfileName,
                          DMGetData& oAuthType,
                          DMGetData& oAuthName,
                          DMGetData& oAuthSecret,
                          DMGetData& oAuthData,
                          DMString& oAuthDataUri)
{
    return this->GetAuthInfo(pAccProfileName,
                             DM_AUTHLEVEL_HTTP,
                             NULL,
                             oAuthType,
                             oAuthName,
                             oAuthSecret,
                             oAuthData,
                             oAuthDataUri);
}

/**
 * Get the client authentication information matching the specified authentication type
 *
 * @param pAccProfileName the DMAcc node name
 * @param pAuthType the required client authentication type
 * @param oAuthName the object that will hold the authentication name
 * @param oAuthSecret the object that will hold the authentication secret
 * @param oAuthData the object that will hold the authentication data
 * @param sAuthDataUri the DMString object that will hold the full client AAuthData (nonce) URI
 * @return the status of the operation
 **/
SYNCML_DM_RET_STATUS_T
DMTree::GetClientAuthInfo(CPCHAR pAccProfileName,
                          CPCHAR pAuthType,
                          DMGetData& oAuthName,
                          DMGetData& oAuthSecret,
                          DMGetData& oAuthData,
                          DMString& oAuthDataUri,
                          DMGetData& oAuthType)
{
    return this->GetAuthInfo(pAccProfileName,
                             DM_AUTHLEVEL_CLCRED,
                             pAuthType,
                             oAuthType,
                             oAuthName,
                             oAuthSecret,
                             oAuthData,
                             oAuthDataUri);
}

/**
 * Get the authentication information matching the specified authentication level and type
 *
 * @param pAccProfileName the DMAcc node name
 * @param pAuthLevel the required authentication level, should be either "CLCRED","SRVCRED","OBEX" or "HTTP"
 * @param pAuthType the required authentication type, should be either "HTTP-BASIC", "HTTP-DIGEST", "TRANSPORT", "HMAC", "DIGEST" or "BASIC". NULL if no specific type is required, then the first node with matching authentication level is returned
 * @param oAuthType the object that will hold the authentication type
 * @param oAuthName the object that will hold the authentication name
 * @param oAuthSecret the object that will hold the authentication secret
 * @param oAuthData the object that will hold the authentication data
 * @param sAuthDataUri the DMString object that will hold the full AAuthData (nonce) URI
 * @return the status of the operation
 **/
SYNCML_DM_RET_STATUS_T
DMTree::GetAuthInfo(CPCHAR pAccProfileName,
                    CPCHAR pAuthLevel,
                    CPCHAR pAuthType,
                    DMGetData& oAuthType,
                    DMGetData& oAuthName,
                    DMGetData& oAuthSecret,
                    DMGetData& oAuthData,
                    DMString& oAuthDataUri)
{
  SYNCML_DM_RET_STATUS_T  dm_stat = SYNCML_DM_FAIL;
  CPCHAR                  szDMAccRootPath = ::XPL_DM_GetEnv( SYNCML_DM_DMACC_ROOT_PATH );

  oAuthDataUri = NULL;

  if ( m_bVersion_1_2 == TRUE )
  {
    /* Fill the pNodeUri with the string "./SyncML/DMAcc/<pAccProfileName>/AppAuth" */
    DMString  strAPPAUTHNodeUri = szDMAccRootPath +
                                   DMString( DM_STR_SLASH ) +
                                   pAccProfileName +
                                   DM_STR_SLASH +
                                   DM_APPAUTH;

    DMMap<DMString, UINT32> authNodesMap;

    dm_stat = getInteriorChildren( strAPPAUTHNodeUri, authNodesMap,SYNCML_DM_REQUEST_TYPE_INTERNAL);
    // if there are no child nodes, no client authentication nodes are found
    if ( dm_stat != SYNCML_DM_SUCCESS )
    {
      dmFreeGetMap((DMMap<DMString, UINT32>&)authNodesMap);
      return SYNCML_DM_FAIL;
    }

    for ( DMMap<DMString, UINT32>::POS iter= authNodesMap.begin();
          iter < authNodesMap.end();
          iter++ )
    {
      dm_stat = SYNCML_DM_FAIL;

      DMString  authNodeName = authNodesMap.get_key(iter);
      DMString  strAPPAUTHInstPath = authNodeName + DM_STR_SLASH;
      DMString  authLevelUri(strAPPAUTHInstPath + DM_AAUTHLEVEL);
      DMString  authTypeUri(strAPPAUTHInstPath + DM_AAUTHTYPE);
      DMString  authNameUri(strAPPAUTHInstPath + DM_AAUTHNAME);
      DMString  authSecretUri(strAPPAUTHInstPath + DM_AAUTHSECRET);
      DMString  authDataUri(strAPPAUTHInstPath + DM_AAUTHDATA);
      DMGetData authLevel;

      dm_stat = Get(authLevelUri.c_str(), authLevel,  SYNCML_DM_REQUEST_TYPE_INTERNAL);

      if ( ( dm_stat == SYNCML_DM_SUCCESS ) &&
            ( DmStrcmp(authLevel.getCharData(), pAuthLevel) == 0 ) )
      {
        dm_stat = Get(authTypeUri.c_str(), oAuthType,  SYNCML_DM_REQUEST_TYPE_INTERNAL);

        if ( pAuthType == NULL || strlen(pAuthType) == 0 ||
             (oAuthType.getCharData() != NULL && DmStrcmp( oAuthType.getCharData(), pAuthType) == 0) )
        {
          dm_stat = Get(authNameUri.c_str(), oAuthName,  SYNCML_DM_REQUEST_TYPE_INTERNAL);

          if ( dm_stat == SYNCML_DM_SUCCESS )
          {
            dm_stat = Get(authSecretUri.c_str(), oAuthSecret,  SYNCML_DM_REQUEST_TYPE_INTERNAL);

            if ( dm_stat == SYNCML_DM_SUCCESS )
            {
              dm_stat = Get(authDataUri.c_str(), oAuthData,  SYNCML_DM_REQUEST_TYPE_INTERNAL);
            }
          }

          oAuthDataUri = strAPPAUTHInstPath;
          break;
        }
      }
    }

    if (0 == oAuthDataUri.length())
    {
      // DM: no matching AUTHINFO is found
      dm_stat = SYNCML_DM_FAIL;
    }

    dmFreeGetMap((DMMap<DMString, UINT32>&)authNodesMap);
  }
  else
  {
        DMString strNodeURI = szDMAccRootPath +
                          DMString( DM_STR_SLASH ) +
                          pAccProfileName +
                          DM_STR_SLASH;

        DMString authNameUri(strNodeURI);
        DMString authSecretUri(strNodeURI);
        DMString authDataUri(strNodeURI);
        DMString authTypeUri(strNodeURI);
        BOOLEAN bClientAuth = ( DmStrcmp(pAuthLevel,DM_AUTHLEVEL_CLCRED) == 0 );

        if ( bClientAuth )
        {
              authTypeUri += DM_AUTHPREF;
              authNameUri += DM_USERNAME;
              authSecretUri += DM_CLIENTPW;
              authDataUri += DM_CLIENTNONCE;

              dm_stat = Get(authTypeUri.c_str(), oAuthType,  SYNCML_DM_REQUEST_TYPE_INTERNAL);
        }
        else
        {
              authNameUri += DM_NAME;
              authSecretUri += DM_SERVERPW;
              authDataUri += DM_SERVERNONCE;

              dm_stat = SYNCML_DM_SUCCESS;
        }

        if ( dm_stat == SYNCML_DM_SUCCESS  || dm_stat == SYNCML_DM_NOT_FOUND )
        {
            dm_stat = Get(authNameUri.c_str(), oAuthName,  SYNCML_DM_REQUEST_TYPE_INTERNAL);

            if ( dm_stat == SYNCML_DM_SUCCESS )
            {
                dm_stat = Get(authSecretUri.c_str(), oAuthSecret,  SYNCML_DM_REQUEST_TYPE_INTERNAL);
                if ( dm_stat == SYNCML_DM_SUCCESS )
                {
                    dm_stat = Get(authDataUri.c_str(), oAuthData,  SYNCML_DM_REQUEST_TYPE_INTERNAL);
                }
            }
        }

        oAuthDataUri = strNodeURI;
  }

  return dm_stat;
}

/**
 * Get the authentication type used by the last DM session
 *
 * @param pAccProfileName the DMAcc node name
 * @param oClientAuthType the object that will hold the client authentication type
 * @return the status of the operation
 **/
SYNCML_DM_RET_STATUS_T
DMTree::GetLastClientAuthType(CPCHAR pAccProfileName,
                              DMGetData& oClientAuthType)
{
  return Get( ::XPL_DM_GetEnv( SYNCML_DM_DMACC_ROOT_PATH ) +
                                    DMString( DM_STR_SLASH ) +
                                    pAccProfileName +
                                    DM_STR_SLASH +
                                    DM_EXT +
                                    DM_STR_SLASH +
                                    DM_LASTCLIENTAUTHTYPE,
              oClientAuthType,
              SYNCML_DM_REQUEST_TYPE_INTERNAL);
}
/**
 * Set the authentication type used by the last DM session
 *
 * @param pAccProfileName the DMAcc node name
 * @param pClientAuthType the client authentication type value to set
 * @return the status of the operation
 **/
SYNCML_DM_RET_STATUS_T
DMTree::SetLastClientAuthType(CPCHAR pAccProfileName,
                              CPCHAR pClientAuthType)
{
    DMAddData               oNodeData;
    SYNCML_DM_RET_STATUS_T  dm_stat = SYNCML_DM_SUCCESS;

    DMString strEXTURI = ::XPL_DM_GetEnv( SYNCML_DM_DMACC_ROOT_PATH ) +
                          DMString( DM_STR_SLASH ) +
                          pAccProfileName +
                          DM_STR_SLASH +
                          DM_EXT;

    DMString strLastClientAuthURI = strEXTURI +
                                    DM_STR_SLASH +
                                    DM_LASTCLIENTAUTHTYPE;

    dm_stat = oNodeData.set( strEXTURI,
                             SYNCML_DM_FORMAT_NODE,
                             NULL,
                             0,
                             "text/plain");

    if ( dm_stat == SYNCML_DM_SUCCESS )
    {
      Add(oNodeData, SYNCML_DM_REQUEST_TYPE_INTERNAL);
    }

    dm_stat = oNodeData.set(strLastClientAuthURI,
                            SYNCML_DM_FORMAT_CHR,
                            pClientAuthType,
                            DmStrlen(pClientAuthType),
                            "text/plain" );

    if ( dm_stat == SYNCML_DM_SUCCESS )
    {
        dm_stat = Replace(oNodeData, SYNCML_DM_REQUEST_TYPE_INTERNAL);

        if ( dm_stat == SYNCML_DM_NOT_FOUND )
        {
          dm_stat = Add(oNodeData, SYNCML_DM_REQUEST_TYPE_INTERNAL);
        }
    }

    return dm_stat;
}

#ifdef LOB_SUPPORT
/*==============================================================================
FUNCTION        : IsESN

DESCRIPTION     : Is it a External Storage Node

ARGUMENT PASSED : pNodeValue,
                  nodeLength,
                  pNode
                  pNodeName
OUTPUT PARAMETER:
RETURN VALUE    :
IMPORTANT NOTES :
==============================================================================*/
SYNCML_DM_RET_STATUS_T
DMTree::IsESN(CPCHAR pbURI, BOOLEAN &isESN)
{
    DMString strPluginURI;    // uri without ?xxx stuff for plugins and so on
    DMString strURI;

    if ( !GetPluginURI( pbURI, strURI, strPluginURI ) )
      return SYNCML_DM_COMMAND_FAILED;

    isESN = m_oMDFObj.IsESN( strPluginURI );
    return SYNCML_DM_SUCCESS;
}
#endif

DMMetaDataManager& DMTree::GetMetaDataManager()
{
    return m_oMDFObj;
}
