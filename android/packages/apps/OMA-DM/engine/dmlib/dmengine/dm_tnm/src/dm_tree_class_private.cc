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
//   Module Name: dm_tree_class_private.cc
//
//   General Description:Contains the implementations of the methods of
//                       DMNode class.
//------------------------------------------------------------------------

#include "dm_tree_util.h"
#include "dm_tree_plugin_root_node_class.H"
#include "xpl_Logger.h"
#include "dm_uri_utils.h"
#include "SyncML_DM_Archive.H"
#include "dmprofile.h"
#include "dm_tree_plugin_util.H"
#include "dm_tree_default_interior_node_class.H"
#include "dm_tree_class.H"

//------------------------------------------------------------------------
//                          GLOBAL VARIABLES
//------------------------------------------------------------------------
DMTree               dmTreeObj;

//------------------------------------------------------------------------
//
// FUNCTION        : GetLastSegmentOfURI
//
// DESCRIPTION     : This function will return the starting location
//                   of the last segment of the URI.
//
// ARGUMENTS PASSED: UINT8 *pbURI
// RETURN VALUE    : UINT8 *
//                       - Pointer to starting location of last segment
// PRE-CONDITIONS  : Input parameter SHOULD NOT BE NULL.
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   : N/A The methods in this file are private methods
//                   internal to DMTNM implementation
//------------------------------------------------------------------------
BOOLEAN DMTree::GetLastSegmentOfURI( DMString& strURI, DMString& strLastSegment )
{
    if (!strURI.GetBuffer())
        return FALSE;

  char* szLastSlash = DmStrrchr( strURI.GetBuffer(), SYNCML_DM_FORWARD_SLASH );

  if ( !szLastSlash )
    return FALSE;

  *szLastSlash = 0;

  strLastSegment = ++szLastSlash;
  return TRUE;
}

//------------------------------------------------------------------------
//
// FUNCTION        : IsValidServer
//
// DESCRIPTION     : This function will check the access type and the
//                   ACL permissions of the node.
//
//
// ARGUMENTS PASSED:

// RETURN VALUE    : SYNCML_DM_RET_STATUS_T
//                    - It returns SYNCML_DM_SUCCESS if the AccessType is set
//                      and the ACL permisssion is there for the requested
//                      operation.
//                    - It returns SYNCML_DM_COMMAND_NOT_ALLOWED if the
//                      Accesstype is not for the requested operation.
//                    - It returns SYNCML_DM_PERMISSION_FAILED if the ACL
//                      permission is not for the requested operation
//                    - It returns SYNCML_DM_COMMAND_NOT_IMPLEMENTED if
//                      the command is not implemented by the client.
// PRE-CONDITIONS  :
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   : ESR-DMTNM00xx-m in SYNCML-DM-ENGINE-FSRS-02101010
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMTree::IsValidServer(CPCHAR pURI,
                                         SYNCML_DM_ACCESS_TYPE_T accessType,
                                         SYNCML_DM_REQUEST_TYPE_T eRequestType,
                                         BOOLEAN bCheckAccess,
                                         BOOLEAN bIsCheckLocal,
                                         DMMetaPCharVector* pChildDependNodes /*=NULL*/)
{

  if (pURI == NULL)
  {
      //XPL_LOG_DM_TMN_Debug(("DMTree::IsValidServer: pURI is null \n"));
      return SYNCML_DM_COMMAND_FAILED;
  }

  if ( eRequestType != SYNCML_DM_REQUEST_TYPE_SERVER )
  {
     //XPL_LOG_DM_TMN_Debug(("DMTree::IsValidServer: Request type is not SYNCML_DM_REQUEST_TYPE_SERVER \n"));
     bIsCheckLocal = FALSE;
  }

  if ( !m_oACLObj.IsPermitted( pURI,
                               GetServerId(),
                               (SYNCML_DM_ACL_PERMISSIONS_T)accessType,
                               bIsCheckLocal) )
  {
     XPL_LOG_DM_TMN_Debug(("DMTree::IsValidServer: permission failed \n"));
     return SYNCML_DM_PERMISSION_FAILED;
  }

  if ( accessType !=  SYNCML_DM_GET_ACCESS_TYPE && bCheckAccess )
  {
      if( !m_oMDFObj.VerifyAccessType(pURI,accessType, pChildDependNodes) )
      {
        XPL_LOG_DM_TMN_Debug(("DMTree::IsValidServer: access not allowed \n"));
        return (SYNCML_DM_COMMAND_NOT_ALLOWED);
      }
  }

  return SYNCML_DM_SUCCESS;
}


//------------------------------------------------------------------------
//
// FUNCTION        : GetChildren
//
// DESCRIPTION     : This function gives the list of childs of the node
//
//
// ARGUMENTS PASSED: DMNode *psNode
//                   DMString & strChildren
//                       - contains childs each child seperated by '/'
//
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T
//                     - It returns SYNCML_DM_SUCCESS if the function return
//                       the childs.
//                     - It returns SYNCML_DM_COMMAND_FAILED if the
//                       pointer psNode is NULL.
// PRE-CONDITIONS  :
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   : ESR-DMTNM00xx-m in SYNCML-DM-ENGINE-FSRS-02101010
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMTree::GetChildren(
    CPCHAR szURI,
    const DMNode *psNode,
    SYNCML_DM_REQUEST_TYPE_T eRequestType,
    DMString& strChildren ) const
{
    DMMetaDataManager & m_oMDFObj =  dmTreeObj.GetMetaDataManager();
    BOOLEAN bIsLocal = FALSE;

    DMString strChildrenOPI;

    strChildren = NULL;

    if( !psNode )
    return SYNCML_DM_COMMAND_FAILED;

    psNode = psNode->pcFirstChild;

    while(psNode)
    {
        bIsLocal = FALSE;
        if ( eRequestType ==  SYNCML_DM_REQUEST_TYPE_SERVER )
        {
              DMString szChildURI(szURI);

              szChildURI += "/";
              szChildURI += psNode->abNodeName;
              bIsLocal = m_oMDFObj.IsLocal(szChildURI);
        }
        if ( !bIsLocal )
        {
            strChildren += psNode->abNodeName;

            if ( psNode->pcNextSibling )
                strChildren += "/";
        }
        strChildrenOPI += psNode->abNodeName;

        if ( psNode->pcNextSibling )
            strChildrenOPI += "/";

        psNode = psNode->pcNextSibling;
  }

  // DP: Overlay plugin support
  if ( m_ptrCacheOPI != NULL )
  {
    m_oMDFObj.UpdateChildrenList( szURI, strChildrenOPI );
  }

  return SYNCML_DM_SUCCESS;
}



//------------------------------------------------------------------------
//
// FUNCTION        : GetChildrenCount
//
// DESCRIPTION     : This function gives the number of childs of the node
//
//
// ARGUMENTS PASSED: DMNode *psNode
//
// RETURN VALUE    : childs count
// PRE-CONDITIONS  :
// POST-CONDITIONS :
// IMPORTANT NOTES :
//------------------------------------------------------------------------
UINT16 DMTree::GetChildrenCount(const DMNode *psNode)const
{
  int childCount = 0;

  if( !psNode )
    return 0;

  psNode = psNode->pcFirstChild;

  while(psNode)
  {
     childCount++;
     psNode = psNode->pcNextSibling;
  }

  return childCount;
}

//------------------------------------------------------------------------
//
// FUNCTION        : FindNodeByURI
//
// DESCRIPTION     : This functions returns the pointer to DMNode of the
//                   the last segment of the URI. This function will use
//                   both Depth First Search and Breadth First Search
//                   traversal algorithms to search.
//
//
// ARGUMENTS PASSED: UINT8* pbURI - Contails the URI
// RETURN VALUE    : DMNode*
//                      - It returns NULL if the node was NOT found
//                        in the tree.
//                      - It returns Pointer to the DMNode if it finds
//                        the node in the tree.
// PRE-CONDITIONS  :
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   : ESR-DMTNM00xx-m in SYNCML-DM-ENGINE-FSRS-02101010
//------------------------------------------------------------------------
DMNode* DMTree::FindNodeByURI(CPCHAR pbURI)
{
  XPL_LOG_DM_TMN_Debug(("DMTree::FindNodeByURI: Enter. pbURI=%s\n",pbURI));
  DMString strURI = pbURI;
  char *szURI = strURI.GetBuffer();

  const char *pbURISegment = NULL;
  DMNode *psFindNode  = NULL;
  DMNode *pParentCopy = NULL;

#ifdef LOB_SUPPORT
  // Is a cached ESD
  psFindNode = GetESN(pbURI);
  if (psFindNode != NULL)
  {
    XPL_LOG_DM_TMN_Debug(("DMTree::FindNodeByURI: this is a cached ESD\n"));
    return psFindNode;
  }
#endif
  // last access time for pArchive
  XPL_CLK_CLOCK_T currentTime = m_currentTime; // use cached value if available
  if (currentTime==0)
    currentTime = XPL_CLK_GetClock();

  // reset OPI cache
  m_ptrCacheOPI = NULL;
  m_oOPICacheData.metaNodeID = -1; // metanode ID is set later only for PI node

  while ( m_oOPICacheData.aPD.size() )
    m_oOPICacheData.aPD.remove(0);    // function "remove" does not free buffer in contrast to "removeall"

  psFindNode  = m_psRoot;

  pbURISegment = GetURISegment(&szURI);
  while(psFindNode != NULL && pbURISegment)
  {
    //XPL_LOG_DM_TMN_Debug(("DMTree::FindNodeByURI: szURI=%s, pbURISegment=%s\n",szURI, pbURISegment));
    //XPL_LOG_DM_TMN_Debug(("DMTree::FindNodeByURI: m_psRoot.abNodeName=%s\n",m_psRoot->abNodeName.GetBuffer()));
    // This functions will seach for the segment in the siblings linked
    // list
    pParentCopy = psFindNode;

    psFindNode = FindNodeInNextSiblingsList(psFindNode, pbURISegment);
    if(psFindNode == NULL) {
      pParentCopy = pParentCopy->pcParentOfNode;
      if ( pParentCopy && LoadSkeletonParentArchive(pParentCopy) ) {
        psFindNode = FindNodeInNextSiblingsList(pParentCopy->pcFirstChild, pbURISegment);
      }

      if ( !psFindNode )
      {
        if (szURI == NULL && m_ptrCacheOPI != NULL )
       {
               psFindNode = GetOPINode( pbURI );
#ifdef LOB_SUPPORT
            if(psFindNode != NULL && m_oMDFObj.IsESN( pbURI ))
            SetESNCache(pbURI ,psFindNode);
#endif
                XPL_LOG_DM_TMN_Debug(("DMTree::FindNodeByURI: find node from OPI cache \n"));
        return psFindNode;
        }
        XPL_LOG_DM_TMN_Debug(("DMTree::FindNodeByURI: can not find the node \n"));
        return (NULL);
      }
    }

    // OPI
    //XPL_LOG_DM_TMN_Debug(("DMTree::FindNodeByURI: Check if the node is a OverlayPI \n"));
    if ( psFindNode->IsOverlayPI() )
    {
      m_ptrCacheOPI = psFindNode->getOverlayPI();
      // reset data again for case overlapped opi
      while ( m_oOPICacheData.aPD.size() )
        m_oOPICacheData.aPD.remove(0);    // function "remove" does not free buffer in contrast to "removeall"
    }

    if ( psFindNode->IsOverlayPIData() && psFindNode->getOverlayPIData())
      m_oOPICacheData.aPD.push_back( *psFindNode->getOverlayPIData());

    // OPI synchronization support
    if ( m_ptrCacheOPI != NULL && !psFindNode->opiSyncNotNeeded() )
      CheckOpiSync( psFindNode, strURI.GetBuffer(), pbURISegment );

    pbURISegment = GetURISegment(&szURI);
    // If the pbURISegment is NULL means then it has processed the
    // full URI and got the DMNode pointer for the last segment of
    // the URI.
    // For plugin Proxy node, use the proxy node for ALL subtree.
    if ((pbURISegment == NULL) || (pbURISegment[0] == '\0') || (psFindNode->isPlugin())) {

        if ( psFindNode->IsSkeletonNode() && psFindNode->pArchive )
          psFindNode->pArchive->deserialize(this);

        // e50024
        // Update last access time of the archive
        DMNode * pNode = psFindNode;

        while ( pNode ) {
            if ( pNode->pArchive ) {
                pNode->pArchive->setLastAccessedTime(currentTime);
                break;
            }
            pNode = pNode->pcParentOfNode;
        }
#ifdef LOB_SUPPORT
    if(psFindNode != NULL && m_oMDFObj.IsESN( pbURI ))
            SetESNCache(pbURI ,psFindNode);
#endif

        return (psFindNode);
    }

    // If the URI segment is NOT NULL then we have look into
    // the childs list of the current node because previous URI
    // segment will become the parent of the current URI segment.
    if ( !psFindNode->pcFirstChild )
      LoadSkeletonParentArchive( psFindNode );

    psFindNode = psFindNode->pcFirstChild;
  }

  if (szURI == NULL && m_ptrCacheOPI != NULL )
 {     psFindNode = GetOPINode( pbURI );

#ifdef LOB_SUPPORT
    if(psFindNode != NULL && m_oMDFObj.IsESN( pbURI ))
            SetESNCache(pbURI ,psFindNode);
#endif
    return psFindNode;
  }
  return (NULL);
}

//------------------------------------------------------------------------
//
// FUNCTION        : ParseACL
//
// DESCRIPTION     : This function will parse the ACL according the BNF
//                   defined in FSRS.
//
// ARGUMENTS PASSED: UINT8* pACL
//                      - ACL value
//
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T
//                      - It returns SYNCML_DM_COMMAND_FAILED in case
//                        if there is any failure.
//                      - It returns SYNCML_DM_SUCCESS in case if the ACL value
//                        parsed successfully.
//                      - It returns SYNCML_DM_DEVICE_FULL in case if the
//                        device is out of memory.
//
// PRE-CONDITIONS  :
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   : ESR-DMTNM00xx-m in SYNCML-DM-ENGINE-FSRS-02101010
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMTree::ParseACL(UINT8* pACL)
{
  UINT8 *pbStartingPosOfCmd = NULL;
  UINT8 *pbACL = NULL, *pNew;

  // oCommandsInACL variable is used to decide whether to add "&" to the
  // ACL value or not while parsing the ACL. Assume that if the server
  // has fired an ACL value like ACL = Add=ServerA&Get=ServerB then after
  // processing "ADD".
  // BOOLEAN oCommandsInACL = FALSE;

  SYNCML_DM_RET_STATUS_T wReturnCode = SYNCML_DM_SUCCESS;

  XPL_LOG_DM_TMN_Debug(("Entered DMTree::ParseACL \n"));

  if(pACL == NULL)
    return (SYNCML_DM_SUCCESS);

  if(pACL[0] == '\0')
    return (SYNCML_DM_COMMAND_FAILED);

  pNew = pbACL = (UINT8*) DmAllocMem((sizeof(UINT8) * (DmStrlen((const char*)pACL) + 1)));
  if (!pbACL)
      return SYNCML_DM_DEVICE_FULL;

  DmStrcpy((char*)pbACL, (const char*)pACL);
  pbStartingPosOfCmd = (UINT8*)pbACL;

  while(*pbACL != '\0')
  {
    pbACL = (UINT8*)DmStrchr((CPCHAR)pbACL, SYNCML_DM_EQUAL_TO);

    if(pbACL == NULL)
    {
      XPL_LOG_DM_TMN_Debug(("Invalid ACL String \n"));
      DmFreeMem(pNew);
      return (SYNCML_DM_COMMAND_FAILED);
    }
    *pbACL = '\0';

    // Here we are checking for the first character to reduce
    // the time.
    if(pbStartingPosOfCmd[0] == SYNCML_DM_ASCII_CAPITAL_R)
    {
      // We have already compared "R" and now we have to check if it is
      // "eplace".
      if(DmStrcmp((CPCHAR)&pbStartingPosOfCmd[1], SYNCML_DM_EPLACE))
          wReturnCode = SYNCML_DM_COMMAND_FAILED;
      else
      {
          pbStartingPosOfCmd += REPLACE_CMD_LENGTH_IN_ACL;
          pbStartingPosOfCmd = (UINT8*) DmStrchr((CPCHAR)pbStartingPosOfCmd, SYNCML_DM_AMPERSAND);
          if ( !pbStartingPosOfCmd )
            break;
          pbStartingPosOfCmd++;
      }
    }
    else
    if(pbStartingPosOfCmd[0] == SYNCML_DM_ASCII_CAPITAL_G)
    {
      if(DmStrcmp((CPCHAR)&pbStartingPosOfCmd[1], SYNCML_DM_ET))
          wReturnCode = SYNCML_DM_COMMAND_FAILED;
      else
      {
          pbStartingPosOfCmd += GET_CMD_LENGTH_IN_ACL;
          pbStartingPosOfCmd = (UINT8*) DmStrchr((CPCHAR)pbStartingPosOfCmd, SYNCML_DM_AMPERSAND);
          if ( !pbStartingPosOfCmd )
            break;
          pbStartingPosOfCmd++;
      }
    }
    else
    if(pbStartingPosOfCmd[0] == SYNCML_DM_ASCII_CAPITAL_A)
    {
      if(DmStrcmp((CPCHAR)&pbStartingPosOfCmd[1], SYNCML_DM_DD))
          wReturnCode = SYNCML_DM_COMMAND_FAILED;
      else
      {
          pbStartingPosOfCmd += ADD_CMD_LENGTH_IN_ACL;
          pbStartingPosOfCmd = (UINT8*) DmStrchr((CPCHAR)pbStartingPosOfCmd, SYNCML_DM_AMPERSAND);
          if ( !pbStartingPosOfCmd )
            break;
          pbStartingPosOfCmd++;
      }
    }
    else
    if(pbStartingPosOfCmd[0] == SYNCML_DM_ASCII_CAPITAL_D)
    {
        if(DmStrcmp((CPCHAR)&pbStartingPosOfCmd[1], SYNCML_DM_ELETE))
            wReturnCode = SYNCML_DM_COMMAND_FAILED;
        else
        {
            pbStartingPosOfCmd += DELETE_CMD_LENGTH_IN_ACL;
            pbStartingPosOfCmd = (UINT8*) DmStrchr((CPCHAR)pbStartingPosOfCmd, SYNCML_DM_AMPERSAND);
            if ( !pbStartingPosOfCmd )
                break;
            pbStartingPosOfCmd++;
        }
    }
    else
    if(pbStartingPosOfCmd[0] == SYNCML_DM_ASCII_CAPITAL_E)
    {
      if(DmStrcmp((CPCHAR)&pbStartingPosOfCmd[1], SYNCML_DM_XEC))
          wReturnCode = SYNCML_DM_COMMAND_FAILED;
      else
      {
          pbStartingPosOfCmd += EXEC_CMD_LENGTH_IN_ACL;
          pbStartingPosOfCmd = (UINT8*) DmStrchr((CPCHAR)pbStartingPosOfCmd, SYNCML_DM_AMPERSAND);
          if ( !pbStartingPosOfCmd )
            break;
          pbStartingPosOfCmd++;
      }
    }
    else
    {
        wReturnCode = SYNCML_DM_COMMAND_FAILED;
    }

    pbACL = pbStartingPosOfCmd;
  }

  DmFreeMem(pNew);
  return wReturnCode;
}



//------------------------------------------------------------------------
//
// FUNCTION        : FindNodeInNextSiblingsList
//
// DESCRIPTION     : This function will search for the URI Segment in
//                   the siblings list. If the node was found then it
//                   returns the DMNode pointer otherwise it returns NULL.
//
//
// ARGUMENTS PASSED: const DMNode *psFindNode
//                      - Starting position of the siblings linked list.
//                   const UINT8 *pbURISegment
//                      - Contains the URI segment.
// RETURN VALUE    : DMNode*
//                      - It returns DMNode pointer if the node was found.
//                      - NULL if the node was NOT found.
//
// PRE-CONDITIONS  : pbURISegment SHOULD NOT BE NULL.
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   : ESR-DMTNM00xx-m in SYNCML-DM-ENGINE-FSRS-02101010
//------------------------------------------------------------------------
DMNode* DMTree::FindNodeInNextSiblingsList(DMNode *psFindNode,
                                           CPCHAR pbURISegment) const
{
  DMNode *psCurNode = psFindNode;

  while(psCurNode != NULL)
  {
    if(DmStrcmp(psCurNode->abNodeName, pbURISegment) == 0)
      return (psCurNode);

    psCurNode = psCurNode->pcNextSibling;
  }

  return (NULL);
}

//------------------------------------------------------------------------
//
// FUNCTION        : InsertNodeIntoNextSiblingsList
//
// DESCRIPTION     : This function will insert the node into the siblings
//                   list and before inserting the node it checks if the
//                   node already existed or NOT. if the node already
//                   existed then it will add the node into the siblings
//                   list.
//
//
// ARGUMENTS PASSED: DMNode *psNextSiblingStartNode
//                      - Starting position of the siblings linked list.
//                   DMNode *psInsertNode
//                      - Inserting node pointer.
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T
//                      - It returns SYNCML_DM_SUCCESS if the functions inserts
//                        the node successfully into the siblings list.
//                      - It returns SYNCML_DM_TARGET_ALREADY_EXISTS the
//                        node already existed in the siblings list.
//
// PRE-CONDITIONS  : Input parameters SHOULD NOT BE NULL.
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   : ESR-DMTNM00xx-m in SYNCML-DM-ENGINE-FSRS-02101010
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMTree::InsertNodeIntoNextSiblingsList(
                               DMNode *psNextSiblingStartNode,
                               DMNode *psInsertNode)
{
  XPL_LOG_DM_TMN_Debug(("Entered DMTree::InsertNodeIntoNextSiblingsList \n"));
  while(psNextSiblingStartNode->pcNextSibling != NULL)
  {
    if(DmStrcmp((CPCHAR)psNextSiblingStartNode->abNodeName,
              (CPCHAR)psInsertNode->abNodeName) == 0)
    {
      XPL_LOG_DM_TMN_Debug(("Node Already exists in the Tree \n"));
      return (SYNCML_DM_TARGET_ALREADY_EXISTS);
    }

    psNextSiblingStartNode = psNextSiblingStartNode->pcNextSibling;
  }

  if(DmStrcmp((CPCHAR)psNextSiblingStartNode->abNodeName,
            (CPCHAR)psInsertNode->abNodeName) == 0)
  {
    XPL_LOG_DM_TMN_Debug(("Node Already exists in the Tree \n"));
    return (SYNCML_DM_TARGET_ALREADY_EXISTS);
  }

  psInsertNode->pcParentOfNode = psNextSiblingStartNode->pcParentOfNode;
  psNextSiblingStartNode->pcNextSibling = psInsertNode;
  return (SYNCML_DM_SUCCESS);
}

//------------------------------------------------------------------------
//
// FUNCTION        : IsValidSegment
//
// DESCRIPTION     : This function checks if the URI segment is valid.
//
//
// ARGUMENTS PASSED: UINT8 *pbSegment
//                      - Pointer to the URI segment.
//                   UINT8 bSegmentLength
//                      - Length of the pbSegment.
// RETURN VALUE    : BOOLEAN
//                      - It returns TRUE if the URI segment is valid
//                        otherwise it returns FALSE.
// PRE-CONDITIONS  :
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   :
//------------------------------------------------------------------------
BOOLEAN DMTree::IsValidSegment(CPCHAR pbSegment, UINT8 bSegmentLength)
{
  if((pbSegment == NULL) ||
     (bSegmentLength == 0) ||
     (bSegmentLength > GetMaxPathSegmentLength()) )
    return (FALSE);

  while (bSegmentLength != 0)
  {
    bSegmentLength--;  //decrementing to indicate character processed

    if ((pbSegment[0] == '/') || (pbSegment[0] == '?') || (pbSegment[0] == 0))
      return FALSE;

    pbSegment++;
  }

  return TRUE;
}

//------------------------------------------------------------------------
//
// FUNCTION        : DeleteNode
//
// DESCRIPTION     : This function performs ACL check for children nodes of deleting node
//                   This function will use DFS algorithm to traverse the
//                   tree.
//
// ARGUMENTS PASSED:
//                   psDeletingNode - Pointer to the deleting node.
//                   pbURI -  Pointer the deleting node URI.
//
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T
//
// PRE-CONDITIONS  : psDeletingNode pointer SHOULD NEVER be NULL.
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #     :
//------------------------------------------------------------------------

SYNCML_DM_RET_STATUS_T
DMTree::CheckDeleteForNode(DMNode *psDeletingNode,
                                        CPCHAR pbURI)
{

    XPL_LOG_DM_TMN_Debug(("Entered DMTree::CheckDeleteForNode \n"));

    if( !psDeletingNode )
        return SYNCML_DM_FAIL;

    DMBuffer aRelativeBuffer;
    DMBuffer aAbsoluteBuffer;

    char * abRelativeURI = (char*)aRelativeBuffer.allocate(GetMaxTotalPathLength());
    if(abRelativeURI == NULL)
        return SYNCML_DM_DEVICE_FULL;

    char * abAbsoluteURI = (char*)aAbsoluteBuffer.allocate(GetMaxTotalPathLength());
    if(abAbsoluteURI == NULL)
        return SYNCML_DM_DEVICE_FULL;

    UINT16 wRelativeURILength = 0;
    BOOLEAN bFlag = TRUE;
    DMNode *psTemp = NULL;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    wRelativeURILength = (UINT16)DmSprintf(abRelativeURI, "%s",pbURI);

    psTemp = psDeletingNode;
    psTemp = psTemp->pcFirstChild;

    if( !psTemp )
        return dm_stat;

    while(psTemp != psDeletingNode)
    {
        if(bFlag == TRUE)
        {
            if(psTemp->pcFirstChild)
            {
                wRelativeURILength = (UINT16)DmSprintf(abRelativeURI,"%s/%s",abRelativeURI,(CPCHAR)psTemp->abNodeName);
                psTemp = psTemp->pcFirstChild;
            }
            else
                bFlag = FALSE;
        }
        else
        {
            DmSprintf(abAbsoluteURI, "%s/%s",abRelativeURI,(CPCHAR)psTemp->abNodeName);

            if(psTemp->pcNextSibling)
            {
                psTemp = psTemp->pcNextSibling;
                bFlag = TRUE;
            }
            else
            {
                while((abRelativeURI[wRelativeURILength] != SYNCML_DM_FORWARD_SLASH) &&
                         (wRelativeURILength > 0))
                {
                    --wRelativeURILength;
                }
                abRelativeURI[wRelativeURILength] = '\0';
                psTemp = psTemp->pcParentOfNode;
            }

            dm_stat = IsValidServer((CPCHAR)abAbsoluteURI,
                                               SYNCML_DM_DELETE_ACCESS_TYPE,
                                               SYNCML_DM_REQUEST_TYPE_SERVER,
                                               FALSE,
                                               FALSE);

            if ( dm_stat != SYNCML_DM_SUCCESS)
                break;

       }
    }

    return dm_stat;

}


//------------------------------------------------------------------------
//
// FUNCTION        : DeleteNode
//
// DESCRIPTION     : This function will perform deleting of subtree
//
// ARGUMENTS PASSED:
//                   psDeletingNode - Pointer to the deleting node.
//                   pbURI - Pointer the deleting node URI.
//                   aDeletedChildren - Vector of deleted children nodes
//
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T
//
// PRE-CONDITIONS  : psDeletingNode pointer SHOULD NEVER be NULL.
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   :
//------------------------------------------------------------------------

SYNCML_DM_RET_STATUS_T
DMTree::DeleteNode(DMNode *psDeletingNode,
                             CPCHAR pbURI,
                             DMStringVector & aDeletedChildren)
{
    XPL_LOG_DM_TMN_Debug(("Entered DMTree::DeleteNode \n"));

    if( !psDeletingNode )
        return SYNCML_DM_FAIL;

    DMBuffer aRelativeBuffer;
    DMBuffer aAbsoluteBuffer;

    char * abRelativeURI = (char*)aRelativeBuffer.allocate(GetMaxTotalPathLength());
    if(abRelativeURI == NULL)
        return SYNCML_DM_DEVICE_FULL;

    char * abAbsoluteURI = (char*)aAbsoluteBuffer.allocate(GetMaxTotalPathLength());
    if(abAbsoluteURI == NULL)
        return SYNCML_DM_DEVICE_FULL;

    UINT16 wRelativeURILength = 0;
    BOOLEAN bFlag = TRUE;
    // TRUE  1st exploration of the node, continue with 1st subnode
    // FALSE node completely explored, should contine with sibling

    DMNode *psTemp = NULL;
    DMNode *psRemovingNode = NULL;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    wRelativeURILength = (UINT16)DmSprintf(abRelativeURI, "%s",pbURI);
    psTemp = psDeletingNode;
    psTemp = psTemp->pcFirstChild;

    // When the processing of all the nodes is over then psTemp will
    // become NULL.
    if (psTemp)
    {
        // The processing of subtree is over when psTemp equals to
        // psDeletingNode
        while(psTemp != psDeletingNode)
        {
            // exploration of the node

            if(bFlag == TRUE)
            {
                if(psTemp->pcFirstChild)
                {
                    // In case if the bOperation is SYNCML_TNM_DELETE_NODE then we
                    // have to build URI. This will become relative URI
                    // because there are childs for current node. For the
                    // Childs it will become the relative URI.

                    wRelativeURILength = (UINT16)DmSprintf(abRelativeURI,"%s/%s",abRelativeURI,(CPCHAR)psTemp->abNodeName);

                    // Here we are doing depth first search
                    psTemp = psTemp->pcFirstChild;
                }   // End of if (psTemp->pcFirstChild)
                else
                {
                    // 1st exploration of current node is finished
                    // go to 2nd exploration of the current node
                    bFlag = FALSE;
                }
            }   // End of if (bFlag == TRUE)
            else
            {
                // If it has come to this condition means psTemp does NOT
                // have any childs. So we can process this node and
                // move to the next sibling if there are any otherwise
                // we can move the pointer to the Parent.

                DmSprintf(abAbsoluteURI, "%s/%s",abRelativeURI,(CPCHAR)psTemp->abNodeName);

                //exploration of the node, should continue with sibling
                if(psTemp->pcNextSibling)
                {
                    psRemovingNode = psTemp;
                    psTemp = psTemp->pcNextSibling;
                    // goto 1st exploration of the sibling (now current)
                    bFlag = TRUE;
                }
                else
                {
                    // exploration of the node, should continue if
                    // there is no sibling

                    // If there are no siblings we are moving to parent
                    // so we can remove the last node segment in the Relative
                    // URI because we have process psTemp and curent psTemp
                    // does NOT have any childs and we have to move the pointer
                    // to the parent because there could be some more siblings
                    // on the parent siblings list.
                    while((abRelativeURI[wRelativeURILength] != SYNCML_DM_FORWARD_SLASH) &&
                             (wRelativeURILength > 0))
                    {
                        --wRelativeURILength;
                    }
                    abRelativeURI[wRelativeURILength] = '\0';

                    psRemovingNode = psTemp;
                    psTemp = psTemp->pcParentOfNode;
                }// End of else of if(psTemp->pcNextSibling)

                dm_stat = m_oMDFObj.VerifyDeleteParameters(psRemovingNode,(CPCHAR)abAbsoluteURI);
                if(dm_stat  != SYNCML_DM_SUCCESS)
                    return dm_stat;
#ifdef LOB_SUPPORT
                LogDeleteForESN(psRemovingNode);
#endif

                // In this case first we have to call plug-in.Delete
                // if this plug-in.Delete return SYNCML_DM_SUCCESS then
                // it will delete the node from the tree.
                dm_stat = psRemovingNode->Delete(abAbsoluteURI);

                if(dm_stat != SYNCML_DM_SUCCESS)
                    return dm_stat;

                psRemovingNode->pcParentOfNode->pcFirstChild = psRemovingNode->pcNextSibling;
                delete psRemovingNode;
                psRemovingNode = NULL;
                aDeletedChildren.push_back(DMString(abAbsoluteURI));

                m_oACLObj.Delete(abAbsoluteURI);
        }// End of else of if (bFlag == TRUE)
      }// End of while (psTemp != psDeletingNode)
    }// End of if(psTemp)

    // Contains pointer to the parent of the deleting node.
    DMNode *psParentNode = NULL;
    // Contains previous node pointer to the directly referenced deleting node.
    DMNode *psPrevNode = NULL;
    // Contais pointer to the first child in the deleting node silings list.
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
        XPL_LOG_DM_TMN_Debug(("Deleting node: %s \n", pbURI));
        dm_stat = m_oMDFObj.VerifyDeleteParameters(psDeletingNode,(CPCHAR)pbURI);
        if(dm_stat != SYNCML_DM_SUCCESS)
            return dm_stat;

#ifdef LOB_SUPPORT
        LogDeleteForESN(psDeletingNode);
#endif
        dm_stat = psDeletingNode->Delete( pbURI );

        if (dm_stat != SYNCML_DM_SUCCESS)
            return dm_stat;

        if (psPrevNode != NULL)
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
            psParentNode->pcFirstChild = psDeletingNode->pcNextSibling;
        }
        delete psDeletingNode;
        psDeletingNode = NULL;
    }

#ifndef DM_IGNORE_TSTAMP_AND_VERSION
    if (psParentNode != NULL)
    {
       // Set timestamp and version number for parent Node
       // Use parentURI's URI.
       psParentNode->wTStamp=XPL_CLK_GetClock();
       psParentNode->wVerNo ++;
    }
#endif

    return dm_stat;
}



//------------------------------------------------------------------------
//
// FUNCTION        : DeleteNodesFromTree
//
// DESCRIPTION     : This function will delete all the nodes in the tree.
//
//
// ARGUMENTS PASSED:
//
//
// RETURN VALUE    :
//
// PRE-CONDITIONS  :
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   :
//------------------------------------------------------------------------
void
DMTree::DeleteNodesFromTree(DMNode * psStartNode)
{


  BOOLEAN oFlag = TRUE; // This variable is used to explore the child

  DMNode *psCurrent = psStartNode;

  XPL_LOG_DM_TMN_Debug(("Entered DMTree::DeleteNodesFromTree \n"));

  if (psStartNode != m_psRoot)
  {
      //detach the node.
      DetachNodeFromTree(psStartNode);
  }

  if (psCurrent != NULL)
  {
    psCurrent = psCurrent->pcFirstChild;
    if (psCurrent)
    {
      while (psCurrent != psStartNode)
      {
        // exploration of the node
        if (oFlag == TRUE)
        {
          if (psCurrent->pcFirstChild)
          {
            //Here we are doing depth first search
            psCurrent = psCurrent->pcFirstChild;
          }
          else
          {
            // exploration of node is finished
            // start the exploration of the next node
            oFlag = FALSE;
          }
        }// End of if(oFlag == TRUE)
        else
        {
          //exploration of the node, should continue with sibling
          if (psCurrent->pcNextSibling)
          {
            DMNode* psTemp = psCurrent;
            psCurrent = psCurrent->pcNextSibling;
            delete psTemp;
            psTemp = NULL;
            // goto 1st exploration of the sibling (now current)
            oFlag = TRUE;
          }
          else
          {
            // exploration of the node, should continue if
            // there is no sibling
            DMNode* psTemp = psCurrent;
            psCurrent = psCurrent->pcParentOfNode;
            delete psTemp;
            psTemp = NULL;
          }
        }// End of else
      }// End of while(psCurrent)

      //Now delete psStartNode
      {
         XPL_LOG_DM_TMN_Debug(("Delete root %s\n", (const char*)psStartNode->abNodeName));
         delete psStartNode;
         //Fix the node links for parent nodes etc.
      }
    }// End of if(psCurrent)
    else
    {
      XPL_LOG_DM_TMN_Debug(("Delete root %s\n", (const char*)psStartNode->abNodeName));
      delete psStartNode;
      //Fix the node links for parent nodes etc.
    }
  }// End of if(psCurrent != NULL)

  if (psStartNode==m_psRoot)
  {
      m_psRoot = NULL;
  }
}

SYNCML_DM_RET_STATUS_T
DMTree::CheckDeleteForPluginNodes(SYNCML_DM_REQUEST_TYPE_T eRequestType,
                                                                   CPCHAR pbURI,
                                                                   DMNode *psProxyNode,
                                                                   DMStringVector & aChildren)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    if (!psProxyNode->isPlugin())
        return SYNCML_DM_PERMISSION_FAILED;

    PDmtAPIPluginTree tree;
    dm_stat = ((DMPluginRootNode*)psProxyNode)->GetTree(tree);

    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;

    CPCHAR pPluginURI= ((DMPluginRootNode*)psProxyNode)->GetPluginURI(pbURI);

   // Proxy root node should not be deleted
    if(DmStrlen(pPluginURI) == 0)
        return SYNCML_DM_PERMISSION_FAILED;

    PDmtNode ptrNode;

    dm_stat = tree->GetNode(pPluginURI, ptrNode);
    if (dm_stat == SYNCML_DM_SUCCESS)
           dm_stat = CheckDeleteForPluginNode(eRequestType,pbURI, ptrNode, aChildren, TRUE);

    return dm_stat;
}

SYNCML_DM_RET_STATUS_T
DMTree::CheckDeleteForPluginNode(SYNCML_DM_REQUEST_TYPE_T eRequestType,
                                                 CPCHAR pbURI,
                                                 PDmtNode ptrNode,
                                                 DMStringVector & aChildren,
                                                 BOOLEAN bIsParent)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    if ( !bIsParent )
        aChildren.push_back(DMString(pbURI));
    if (ptrNode->IsLeaf())
    {
         dm_stat = IsValidServer( pbURI,
                                           SYNCML_DM_DELETE_ACCESS_TYPE,
                                           eRequestType,
                                           FALSE,
                                           TRUE);
    }
    else
    {
        DMVector<PDmtNode> vecNodes;

        dm_stat = ptrNode->GetChildNodes(vecNodes);
        if ( dm_stat == SYNCML_DM_SUCCESS )
        {
            for (INT32 i = 0; i < vecNodes.size(); i++)
            {
                DMString nodeName;

                dm_stat = vecNodes[i]->GetNodeName(nodeName);
                if( dm_stat != SYNCML_DM_SUCCESS )
                    return dm_stat;

                DMString uri = pbURI;
                uri += "/";
                uri += nodeName;
                dm_stat = CheckDeleteForPluginNode(eRequestType,uri.c_str(),vecNodes[i],aChildren,FALSE);
                if( dm_stat != SYNCML_DM_SUCCESS )
                    return dm_stat;
            }
        }
    }
    return dm_stat;
}

BOOLEAN DMTree::GetPluginURI(CPCHAR szURI, DMString& strURI, DMString& strShortURI ) const
{
  if ( !szURI )
    return FALSE;

  if ( szURI[0] != '.' )
    strURI = "./";
  else
    strURI = NULL;

  strURI += szURI;

  const char* szShortURI = strURI;
  const char* szQuestionPos = DmStrrchr( szShortURI, SYNCML_DM_FORWARD_SLASH);

  if( !szQuestionPos )
    szQuestionPos = DmStrrchr( szShortURI, SYNCML_DM_QUESTION_MARK);
  else
    szQuestionPos = DmStrrchr( szQuestionPos, SYNCML_DM_QUESTION_MARK);

  if( szQuestionPos )
    strShortURI.assign( szShortURI, szQuestionPos - szShortURI );
  else
    strShortURI = strURI;

  return TRUE;
}

void DMTree::UnloadArchive(DMNode * psStartNode )
{
  ResetOPICache();
#ifdef LOB_SUPPORT
  ResetESNCache();
#endif
  // unload archive performed in 2 steps:
  // 1. find all child archives and converting path to them to "skeleton"
  // 2. unload remaining stuff.
  DMNode* pNode = psStartNode->pcFirstChild;

  if ( !pNode ){
    DeleteNodesFromTree( psStartNode );
    return;
  }

  // traverse sub-tree
  while ( pNode != psStartNode ){
    BOOLEAN bSkipChildren = FALSE;
    if ( pNode->pArchive ) {
      bSkipChildren = TRUE;
      pNode->ConvertPathToSkeleton( psStartNode );
    }

    if ( !bSkipChildren && pNode->pcFirstChild )
      pNode = pNode->pcFirstChild;
    else if ( pNode->pcNextSibling )
      pNode = pNode->pcNextSibling;
    else {
      while ( pNode != psStartNode ) {
        pNode = pNode->pcParentOfNode;

        if ( pNode != psStartNode && pNode->pcNextSibling ){
          pNode = pNode->pcNextSibling;
          break;
        }
      }
    }
  }


  // traverse sub-tree again and free all "non-skeleton" nodes:
  pNode = psStartNode->pcFirstChild;

  while ( pNode != psStartNode ){
    BOOLEAN bSkipChildren = FALSE;
    BOOLEAN bDelete = FALSE;
    if ( pNode->pArchive ) {
      bSkipChildren = TRUE;
    } else if ( !pNode->IsSkeletonNode() )
      bDelete = TRUE;

    DMNode *pNodeDelete = pNode;

    if ( !bSkipChildren && !bDelete && pNode->pcFirstChild )
      pNode = pNode->pcFirstChild;
    else if ( pNode->pcNextSibling )
      pNode = pNode->pcNextSibling;
    else{
      while ( pNode != psStartNode ) {
        pNode = pNode->pcParentOfNode;

        if ( pNode != psStartNode && pNode->pcNextSibling ){
          pNode = pNode->pcNextSibling;
          break;
        }
      }
    }

    if ( bDelete ) {
      DeleteNodesFromTree( pNodeDelete );
    }
  }

  psStartNode->m_nFlags |= DMNode::enum_NodeSkeleton;
}

BOOLEAN DMTree::LoadSkeletonParentArchive( DMNode* pNode )
{
  while ( pNode ) {
    if ( pNode->pArchive ) {
      if ( pNode->IsSkeletonNode() )
        return ( (pNode->pArchive->deserialize(this) == SYNCML_DM_SUCCESS) ? TRUE : FALSE );

      return FALSE; // parent node already loaded
    }
    pNode = pNode->pcParentOfNode;
  }
  return FALSE;
}


/*
This function returns fantom node for OPI if confirmed by MDF
*/
DMNode* DMTree::GetOPINode( CPCHAR szURI )
{
  CPCHAR szID = NULL;
  SYNCML_DM_ACCESS_TYPE_T  wAccessType = 0;
  SYNCML_DM_FORMAT_T nNodeFormat = 0;

  if ( !m_oMDFObj.VerifyOPINode(szURI, szID, wAccessType, nNodeFormat) )
    return NULL;

  if ( szID )
    m_oOPICacheData.metaNodeID = DmAtol(szID);

  if ( m_pOPINode && m_pOPINode->GetPlugin() != m_ptrCacheOPI ){
    delete m_pOPINode;
    m_pOPINode = NULL;
  }

  if ( !m_pOPINode )
    m_pOPINode = new DMOverlayDataPluginNode( m_ptrCacheOPI );

  if ( m_pOPINode )
    m_pOPINode->SetNodeAttributes( wAccessType, nNodeFormat );

  return m_pOPINode;
}


void DMTree::ResetOPICache()
{
  delete m_pOPINode;
  m_pOPINode = NULL;
  m_ptrCacheOPI = NULL;
}

SYNCML_DM_RET_STATUS_T  DMTree::OnOPiDelete( CPCHAR szURI )
{
  // in case the overlay plugin library is not loaded
  // to account for edge case where the plugin ini file does not have entry or has
  // incorrect entry for the overlay plugin library
  if (m_ptrCacheOPI == NULL) {
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
  }

  // notify opi about "data" node to be deleted
  PDmtAPIPluginTree ptrPluginTree;
  CPCHAR szPath = m_ptrCacheOPI->GetPath();

  SYNCML_DM_RET_STATUS_T dm_stat = m_ptrCacheOPI->GetTree(szPath, ptrPluginTree);

  if ( dm_stat != SYNCML_DM_SUCCESS )
    return dm_stat;

  if ( ptrPluginTree != NULL )
  {
    dm_stat = ptrPluginTree->OnDelete( szURI );

    return dm_stat;
  }

  return SYNCML_DM_FAIL; // unlikely case, since if tree is null, plug-in should return an error
}

SYNCML_DM_RET_STATUS_T  DMTree::OnOPiAdd( CPCHAR szURI, DmtOverlayPluginData& data )
{

  // in case the overlay plugin library is not loaded
  // to account for edge case where the plugin ini file does not have entry or has
  // incorrect entry for the overlay plugin library
  if (m_ptrCacheOPI == NULL) {
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
  }

  // notify opi about "data" node to be deleted
  PDmtAPIPluginTree ptrPluginTree;
  CPCHAR szPath = m_ptrCacheOPI->GetPath();

  SYNCML_DM_RET_STATUS_T dm_stat = m_ptrCacheOPI->GetTree(szPath, ptrPluginTree);

  if ( dm_stat != SYNCML_DM_SUCCESS )
    return dm_stat;

  if ( ptrPluginTree != NULL )
  {
    dm_stat = ptrPluginTree->OnAdd( szURI, data );

    return dm_stat;
  }

  return SYNCML_DM_FAIL; // unlikely case, since if tree is null, plug-in should return an error
}


void DMTree::CheckOpiSync( DMNode* pNode, char* szURI, const char* szLastSegment )
{
  if ( pNode->opiSyncMayNeeded() )
  {
    // sync state unknown - mdf data required
    // restore URI
    char* s = szURI;

    while ( s < szLastSegment )
    {
      if ( *s == 0 )
        *s = SYNCML_DM_FORWARD_SLASH;
      s++;
    }

    if ( !m_oMDFObj.IsOPiDataParent( szURI ) )
    {
      pNode->addFlags( DMNode::enum_NodeOPISyncNotNeeded);
      return;
    }
    pNode->addFlags( DMNode::enum_NodeOPISyncNeeded);
  }

  if ( pNode->opiInSync() )
    return; // up-to-date

  // perform sync
  OpiSync( pNode, szURI );

  pNode->addFlags( DMNode::enum_NodeOPISyncUptodate );
}

void DMTree::OpiSync( DMNode* pNode, const char* szURI )
{
  // in case the overlay plugin library is not loaded
  // to account for edge case where the plugin ini file does not have entry or has
  // incorrect entry for the overlay plugin library
  if (m_ptrCacheOPI == NULL) {
    return;
  }

  // get tree
  PDmtAPIPluginTree ptrPluginTree;
  CPCHAR szPath = m_ptrCacheOPI->GetPath();

  SYNCML_DM_RET_STATUS_T dm_stat = m_ptrCacheOPI->GetTree(szPath, ptrPluginTree);

  if ( dm_stat != SYNCML_DM_SUCCESS || ptrPluginTree == NULL )
    return ;

  // fill in sync data
  DMVector<DmtOverlayPluginSyncData> data;

  DMNode* pChild = pNode->pcFirstChild;

  while ( pChild )
  {
    data.push_back(DmtOverlayPluginSyncData());

    DmtOverlayPluginSyncData& item = data[data.size()-1];

    item.m_nStatus = DmtOverlayPluginSyncData::enum_StatusUnchanged;
    item.m_strNodeName = pChild->abNodeName;

    if ( pChild->getOverlayPIData() )
      item.m_oData = *pChild->getOverlayPIData();

    pChild = pChild->pcNextSibling;
  }

  dm_stat = ptrPluginTree->Synchronize( szURI, data );

  if ( dm_stat != SYNCML_DM_SUCCESS )
    return;

  for ( int i = 0; i < data.size(); i++ )
  {
    if ( data[i].m_nStatus == DmtOverlayPluginSyncData::enum_StatusUnchanged )
      continue;

    if ( data[i].m_nStatus == DmtOverlayPluginSyncData::enum_StatusAdded )
    {
      // create a node
      DMNode* pNewNode = new DMOverlayPINode( m_ptrCacheOPI, data[i].m_strNodeName,
          data[i].m_oData );

      if ( !pNewNode )
        return; // sync failed

      pNewNode->pcParentOfNode = pNode;

      if(pNode->pcFirstChild)
      {
        // if the current addind node is NOT the first child of the
        // parent then insert the node in the siblings list.
        InsertNodeIntoNextSiblingsList(pNode->pcFirstChild,pNewNode);
      }
      else
      {
          // if the current adding node is the first child of the
          // parent then assign adding node pointer tp parent first
          // child pointer.
        pNode->pcFirstChild = pNewNode;;
      }
    }

    if ( data[i].m_nStatus == DmtOverlayPluginSyncData::enum_StatusDeleted )
    { // find and delete the node
      pChild = pNode->pcFirstChild;

      while ( pChild )
      {
        if ( pChild->abNodeName == data[i].m_strNodeName )
        {
          DeleteNodesFromTree( pChild );
          break;
        }

        pChild = pChild->pcNextSibling;
      }

    }
  }
}
#ifdef LOB_SUPPORT
void  DMTree::ResetESNCache()
{
    m_pESN = NULL;
    m_strESNPath= NULL;
}
void  DMTree::SetESNCache( CPCHAR szURI , DMNode* pESN)
{
    m_strESNPath = szURI;
    m_pESN = pESN;
}
DMNode*  DMTree::GetESN( CPCHAR szURI )
{
    if(m_pESN != NULL)
    {
        if (DmStrcmp((CPCHAR)m_strESNPath.c_str(), szURI) == 0 )
            return     m_pESN;
    }
    return NULL;
}
void  DMTree::RemoveESNCache(CPCHAR szURI)
{
    if(m_pESN != NULL)
    {    if (DmStrncmp((CPCHAR)m_strESNPath.c_str(), szURI, DmStrlen(szURI)) == 0 )
            ResetESNCache();
    }
}
//------------------------------------------------------------------------
//
// FUNCTION        : LogDeleteForESN
//
// DESCRIPTION     : This function logs delete command for an ESN.
//
//
// ARGUMENTS PASSED: DMNode - node to be removed

//
// RETURN VALUE    : error code if failed, "DM_SUCCESS" otherwise
// PRE-CONDITIONS  :
// POST-CONDITIONS :
// IMPORTANT NOTES :
//------------------------------------------------------------------------
void  DMTree::LogDeleteForESN(DMNode *psDeletingNode)
{
  BOOLEAN  IsESN = psDeletingNode->IsESN();
   if(IsESN)
   {
        DMDefaultESN *tempESN = reinterpret_cast< DMDefaultESN *>(psDeletingNode);
        SyncML_DM_Archive * pArchive=GetArchive(psDeletingNode);
        if(pArchive != NULL)
        {    SyncML_Commit_Log *commitLog = pArchive->GetCommitLogHandler();
            if(commitLog != NULL)
            {
                  DMString targetFileName  = tempESN->GetOriginalInternalFileName();
                   if(targetFileName != NULL)
                    commitLog->logCommand(SYNCML_DM_DELETE, NULL, targetFileName.c_str());
            }
        }
   }
}
#endif

//------------------------------------------------------------------------
//
// FUNCTION        : SetOPINodeData
//
// DESCRIPTION     : This function sets Overlay plug-in data for multi-node
//
//
// ARGUMENTS PASSED: szURI - node uri
//            oData - new data
//
// RETURN VALUE    : error code if failed, "DM_SUCCESS" otherwise
// PRE-CONDITIONS  :
// POST-CONDITIONS :
// IMPORTANT NOTES :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMTree::SetOPINodeData( CPCHAR szURI, const DmtOverlayPluginData& oData )
{
  DMNode* pNode = FindNodeByURI(szURI);

  if ( !pNode || !pNode->IsOverlayPIData() || !pNode->getOverlayPIData() )
    return SYNCML_DM_INVALID_PARAMETER;

  *pNode->getOverlayPIData() = oData;
  return SYNCML_DM_SUCCESS;
}


//------------------------------------------------------------------------
// FUNCTION        : CheckForIndirectUpdates
// DESCRIPTION     : This function checks all child/depend nodes
//                   and for all of them ponting to current uri
//                   issues "indirect" update
// ARGUMENTS PASSED: szURI - currently updated uri
//                   asChildDepend - list of "child/depend" constraints for
//                                   parent nodes
//                   inNode - currently updated node
// RETURN VALUE    : void
// PRE-CONDITIONS  : called from tree update function
// POST-CONDITIONS : none
// IMPORTANT NOTES :
// REQUIREMENT #   :
//------------------------------------------------------------------------
void DMTree::CheckForIndirectUpdates( CPCHAR szURI, const DMMetaPCharVector& asChildDepend, DMNode* inNode )
{
  // check if provided nodes
  for ( int i = 0; i < asChildDepend.size(); i++ )
  {
    DMToken oChild(TRUE,asChildDepend[i],SYNCML_DM_COMMA);

    if ( !oChild.getBuffer() )
        return ; //low memory condition

    const char* szChild = oChild.nextSegment();

    while ( szChild )
    {
 //     if ( m_oPluginManager.IsCommitPluginMounted(szChild) == TRUE )
      CheckURIForIndirectUpdates( szChild, szURI, inNode);

      szChild = oChild.nextSegment();
    }
  }
}

//------------------------------------------------------------------------
// FUNCTION        : CheckURIForIndirectUpdates
// DESCRIPTION     : This function checks one child/depend uri
//                   and if it ponts to the current uri
//                   issues "indirect" update
// ARGUMENTS PASSED: szURI - currently updated uri
//                   szChild - "child/depend" constraint
//                   inNode - currently updated node
// RETURN VALUE    : void
// PRE-CONDITIONS  : called from CheckForIndirectUpdates function
// POST-CONDITIONS : none
// IMPORTANT NOTES :
// REQUIREMENT #   :
//------------------------------------------------------------------------
void DMTree::CheckURIForIndirectUpdates( CPCHAR szChild, CPCHAR szURI, DMNode* inNode )
{
  // node can have '*' inside, so we have to traverse all posible nodes to look for corresponding values
  CPCHAR szStarPos = DmStrstr(szChild, "*");
  if ( !szStarPos )
  {
    DMGetData  oReturnData;
    SYNCML_DM_RET_STATUS_T res = Get(szChild, oReturnData, SYNCML_DM_REQUEST_TYPE_INTERNAL);

    if ( res == SYNCML_DM_SUCCESS  && DmStrstr( szURI, oReturnData.getCharData() ) != NULL )
    {
         LogEvent(SYNCML_DM_EVENT_INDIRECT, szChild);
    }

    return;
  }

  if ( szStarPos == szChild )
    return; // incorect uri - '*' first symbol

  DMString strParent( szChild, szStarPos-szChild-1);

  DMGetData  oData;
  SYNCML_DM_RET_STATUS_T res = Get(strParent, oData, SYNCML_DM_REQUEST_TYPE_INTERNAL);

  if ( res != SYNCML_DM_SUCCESS  )
    return; // node probably does not exist

  DMToken oChild(FALSE, oData.getCharData(), '/' );

  const char* szNextChild = oChild.nextSegment();

  while ( szNextChild )
  {
    DMString strURI( szChild, szStarPos-szChild); // 'parent' with '/' at the end
    strURI += szNextChild;
    strURI += (szStarPos+1); // the rest after '*'

    CheckURIForIndirectUpdates( strURI, szURI, inNode );

    szNextChild = oChild.nextSegment();
  }

}


//------------------------------------------------------------------------
// FUNCTION        : IsUriEnabled
// DESCRIPTION     : This function checks if URI is blocked
// ARGUMENTS PASSED: szURI - currently updated uri
// RETURN VALUE    : TRUE/FALSE
// PRE-CONDITIONS  : called from Add/Replace/Delete/Get functions
// POST-CONDITIONS : none
// IMPORTANT NOTES :
// REQUIREMENT #   :
//------------------------------------------------------------------------
BOOLEAN
DMTree::IsUriEnabled(CPCHAR szURI) const
{
     BOOLEAN bIsEnabled;
     bIsEnabled = m_oTreeMountObj.IsMountPointEnabled(szURI);
     if ( bIsEnabled == FALSE )
          return FALSE;

     bIsEnabled =  m_oPluginManager.IsMountPointEnabled(szURI);
    if ( bIsEnabled == FALSE )
          return FALSE;

    if ( DmIsDMAccNodePath(szURI) ==  SYNCML_DM_FEATURE_NOT_SUPPORTED )
        return FALSE;


    if ( szURI!=NULL && DmStrstr( szURI, "/DevDetail/Ext/OMA_PoC" )!=NULL )
    {
        CPCHAR bit = ::XPL_DM_GetEnv(SYNCML_DM_FEATURE_ID_POC_PROVISION_VIA_OMADM);
        if (bit!=NULL && bit[0]=='0' && bit[1]==0)
        {
            const DMString & sessionName = GetServerId();
            DMString dmProfleNodeName;
            if ((sessionName != "DM_OMACP") && (sessionName != "localhost") &&
                ((DMTree*)this)->GetParentOfKeyValue (sessionName,
                                                  ::XPL_DM_GetEnv( SYNCML_DM_NODENAME_SERVERID ),
                                                  ::XPL_DM_GetEnv( SYNCML_DM_DMACC_ROOT_PATH ),
                                                  dmProfleNodeName))
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOLEAN
DMTree::VerifyArchiveReadAccess(CPCHAR szURI)
{
    BOOLEAN result = TRUE;
    SyncML_DM_Archive* archive = this->GetArchiver().getArchiveByURI(szURI);
    if (archive != NULL) {
        result = archive->verifyPermission(XPL_FS_RDONLY_MODE);
    }
    return result;
}

BOOLEAN
DMTree::VerifyArchiveWriteAccess(CPCHAR szURI)
{
    BOOLEAN result = TRUE;
    SyncML_DM_Archive* archive = this->GetArchiver().getArchiveByURI(szURI);
    if (archive != NULL) {
        result = archive->verifyPermission(XPL_FS_RDWR_MODE);
    }
    return result;
}
