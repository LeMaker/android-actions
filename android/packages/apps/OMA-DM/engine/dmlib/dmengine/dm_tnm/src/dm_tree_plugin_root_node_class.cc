//------------------------------------------------------------------------
//
//   Module Name: dm_tree_plugin_root_node_class.cpp
//
//YXU ADD
//This is the adapter class that redirect calls back to 
// plugin API
//
//   General Description:Contains the implementations of the methods of
//                       DMPluginRootNode class.
//------------------------------------------------------------------------
// Revision History:
//                     Modification   Tracking
// Author (core ID)       Date         Number    Description of Changes
// -----------------  ------------   ----------  -------------------------
// Denis Makarenko    02/19/2007     LIBll07144  Getting rid of global objects to fix deinitialization problems    
// ehb005             01/09/2007      libkk95578 added SetPrincipal on plugin tree
// cdp180              03/16/2007    LIBll55345   Removing ACL check for internal calls                                  
//
// Portability: This module is portable to other compilers. 
//------------------------------------------------------------------------
//                          INCLUDE FILES
//------------------------------------------------------------------------

#include "dm_tree_plugin_root_node_class.H" 
#include "dm_tree_util.h"                   
#include "xpl_Logger.h"
#include "dm_tree_plugin_util.H"


//------------------------------------------------------------------------
//                     LOCAL FUNCTION PROTOTYPES
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//                          LOCAL CONSTANTS
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//               LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//                            LOCAL MACROS
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//                           LOCAL VARIABLES
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//                          GLOBAL VARIABLES
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//                           LOCAL FUNCTIONS
//------------------------------------------------------------------------


//------------------------------------------------------------------------
//                          GLOBAL FUNCTIONS
//------------------------------------------------------------------------

DMPluginRootNode::DMPluginRootNode(PDMPlugin pPlugin ):DMNode(TRUE)
{
  m_ptrPlugin = pPlugin;
}

DMPluginRootNode::~DMPluginRootNode()
{
}

BOOLEAN  DMPluginRootNode::IsGetAccess(CPCHAR pURI) const
{
    return dmTreeObj.GetMetaDataManager().VerifyAccessType(pURI,SYNCML_DM_GET_ACCESS_TYPE);
}

//------------------------------------------------------------------------
// FUNCTION        : Add
// DESCRIPTION     : This function sets the ACCESS type and format
//                   properties for an Interior node added in the tree.
//                   No data involved ,so synchronous Non-blocking call
//                   callback function pointers will be ignored
// ARGUMENTS PASSED: *psAdd,
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : status code
// PRE-CONDITIONS  : 1.URI has already been validated
//                   2.The node object of the plug-in class has been
//                     created and a reference has been given to DMTNM
// POST-CONDITIONS : Following property values are set by the function
//                   Format = SYNCML_DM_FORMAT_NODE
// IMPORTANT NOTES : The default Leaf node class grants all Access
//                   rights to the Node in Add
//                   If a Plug-in needs to implement a class for interior
//                   nodes in which the access type needs to be plug-in
//                   specific Ex: that plug-in does not want to give
//                   delete access type, then the plug-in SHALL implement
//                   it's own Add accordingly.
// REQUIREMENT #   : ESR-DMTNM0042-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMPluginRootNode::Add(DMAddData & oAddData)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    
    PDmtNode ptrNode;

    CPCHAR pPluginPath = GetPluginURI(oAddData.getURI());

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;

    if (oAddData.m_nFormat == SYNCML_DM_FORMAT_NODE) 
    {
        dm_stat = m_ptrPluginTree->CreateInteriorNode(pPluginPath,ptrNode);
    }
    else 
    {
        DmtData oData;

        dm_stat = dmBuildData(oAddData.m_nFormat, oAddData.m_oData, oData);
        if ( dm_stat != SYNCML_DM_SUCCESS )
            return dm_stat;
        
#ifdef LOB_SUPPORT        
        dm_stat = m_ptrPluginTree->CreateLeafNode(pPluginPath,ptrNode,oData, oAddData.IsESN());
#else  
	dm_stat = m_ptrPluginTree->CreateLeafNode(pPluginPath,ptrNode,oData);
#endif

    }

    if ( dm_stat == SYNCML_DM_SUCCESS )
    {
        if ( ptrNode == NULL )
            SetAddedNode(oAddData.getURI());
        else    
            m_ptrPluginTree->SetAddedNode(ptrNode);
    }    
    
    XPL_LOG_DM_TMN_Debug(("DMPluginRootNode::Add, dm_stat=%d\n", dm_stat));
    return dm_stat;
}

//------------------------------------------------------------------------
// FUNCTION        : Delete
// DESCRIPTION     : This function should not actually not get called.
//                   since DMTNM deletes the node object.It is a pure
//                   virtual function in DMNode,hence is implemented and
//                   returns SYNCML_DM_SUCCESS
// ARGUMENTS PASSED: waitMsgForStatus
//                   replyStatusCback
//                   dwCommandId,
//                   bItemNumber,
//                   *pbUri,
//                   oIsThisAtomic,
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : ALWAYS returns
//                   SYNCML_DM_COMMAND_NOT_ALLOWED
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS : DMTNM actually deletes the node object
// IMPORTANT NOTES :
// REQUIREMENT #   : ESR-DMTNM0025-m to ESR-DMTNM0027-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMPluginRootNode::Delete(CPCHAR pURI)
{

   SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

   CPCHAR pPluginPath = GetPluginURI(pURI);

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;

   dm_stat = m_ptrPluginTree->DeleteNode(pPluginPath);

   if ( dm_stat == SYNCML_DM_SUCCESS )
   {
        m_ptrPluginTree->RemoveAddedNode(pPluginPath); 
   }  
   return dm_stat;   

}


//------------------------------------------------------------------------
// FUNCTION        : Get
// DESCRIPTION     : 
// ARGUMENTS PASSED: *pbUri  - full path name
//                   **ppsReturnData
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : ALWAYS returns
//                   SYNCML_DM_COMMAND_NOT_ALLOWED
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS : 
// IMPORTANT NOTES : The DMTNM will return the list of child names(if 
//                   the interior node has no children it will return 
//                   an empty list
// REQUIREMENT #   : ESR-DMTNM0028-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMPluginRootNode::Get(CPCHAR pURI, 
                                                                          DMGetData &oReturnData)
{
   SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
   PDmtNode ptrNode;
  
   CPCHAR pPluginPath = GetPluginURI(pURI);

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;

   oReturnData.clear(); 
   dm_stat = m_ptrPluginTree->GetNode( pPluginPath, ptrNode );

   if ( dm_stat != SYNCML_DM_SUCCESS )
   {
     XPL_LOG_DM_TMN_Debug(("Can't Get Node for %s, error=%d\n", pPluginPath, dm_stat));
     return dm_stat;
   }  
   
   if (ptrNode->IsLeaf())
   {  
      DmtData data;
#ifdef LOB_SUPPORT
	if(ptrNode->IsExternalStorageNode() && oReturnData.chunkData != NULL)
	{
		if(oReturnData.m_chunkOffset ==0)
			dm_stat = ptrNode->GetFirstChunk(*oReturnData.chunkData);
		else
			dm_stat = ptrNode->GetNextChunk(*oReturnData.chunkData);
	}
	else
#endif
	{
      dm_stat = ptrNode->GetValue(data);

      XPL_LOG_DM_TMN_Debug(("Get Attributes error=%d\n", dm_stat));

      if ( dm_stat != SYNCML_DM_SUCCESS )
      {
         XPL_LOG_DM_TMN_Debug(("Can't Get Value for %s, error=%d\n", pPluginPath, dm_stat));
         return dm_stat;
      }

      if ( data.GetType() == SYNCML_DM_DATAFORMAT_UNDEFINED )
        return SYNCML_DM_DEVICE_FULL;

      DmtAttributes oAttr;
      dm_stat = ptrNode->GetAttributes( oAttr );

      if ( dm_stat != SYNCML_DM_SUCCESS )
      {
        XPL_LOG_DM_TMN_Debug(("Can't Get Attributes for %s, error=%d\n", pPluginPath, dm_stat));
        return dm_stat;
      }

      dm_stat = oReturnData.set(data,oAttr.GetType());
      if ( dm_stat != SYNCML_DM_SUCCESS )
      {
        return dm_stat;
      }
	oReturnData.m_nFormat = DMTree::ConvertFormatStr(oAttr.GetFormat());
#ifdef LOB_SUPPORT
	if(ptrNode->IsExternalStorageNode())
	{
		oReturnData.m_TotalSize = oAttr.GetSize();
	}
#endif		
	}   } 
   else
   {  
      DMStringVector mapNodeNames;
      DMString strVal;
      dm_stat = m_ptrPluginTree->GetChildNodeNames( pPluginPath,mapNodeNames);

      if ( dm_stat != SYNCML_DM_SUCCESS )
      {
         XPL_LOG_DM_TMN_Debug(("Can't Get Child Node Names for %s, error=%d\n", pPluginPath, dm_stat));
         return dm_stat;
      }   
            
      // Make sure that there are no duplicate child node names. It's a huck for GetStruct. 
      // In future should be fixed in the GetStruct 
      DMStringVector aChildren;
      for ( int i = 0; i < mapNodeNames.size(); i++ )
      {
          DMString strChild = mapNodeNames[i];
          if ( aChildren.find( strChild ) == -1 )
          {
              aChildren.push_back( strChild );
              if (i > 0)
                  strVal += "/";
              strVal += strChild;
          }
      }

      dm_stat = oReturnData.set(SYNCML_DM_FORMAT_NODE,strVal,strVal.length(),"text/plain");
   } 
#ifdef LOB_SUPPORT
   // Set ESN flag
   oReturnData.SetESN(ptrNode->IsExternalStorageNode());
#endif
   return dm_stat;   
   
}


//------------------------------------------------------------------------
// FUNCTION        : SetAddedNode
// DESCRIPTION     : 
// ARGUMENTS PASSED: *pUri  - full path name
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS : 
// IMPORTANT NOTES : will add URI of interior node into internal vector 
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMPluginRootNode::SetAddedNode(CPCHAR pURI)
{

    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    CPCHAR pPluginPath = GetPluginURI(pURI);

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;

    PDmtPluginNode ptrNode = new DmtPluginNode();

    if ( ptrNode == NULL )
        return SYNCML_DM_DEVICE_FULL;

    DmtPluginTree *pTree =   (DmtPluginTree *)m_ptrPluginTree.GetPtr();
    
    dm_stat = ptrNode->Init(PDmtPluginTree(pTree), pPluginPath);
    if ( dm_stat != SYNCML_DM_SUCCESS )
    {
        return dm_stat;
    }
    
    m_ptrPluginTree->SetAddedNode(PDmtNode(ptrNode)); 

    return SYNCML_DM_SUCCESS;  
}


//------------------------------------------------------------------------
// FUNCTION        : RemoveAddedNode
// DESCRIPTION     : 
// ARGUMENTS PASSED: *pUri  - full path name
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS : 
// IMPORTANT NOTES : will remove URI of interior node from internal vector 
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMPluginRootNode::RemoveAddedNode(CPCHAR pURI)
{

    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS; 
    CPCHAR pPluginPath = GetPluginURI(pURI);
   
    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;
   
    m_ptrPluginTree->RemoveAddedNode(pPluginPath); 

    return SYNCML_DM_SUCCESS;  
}


//------------------------------------------------------------------------
// FUNCTION        : Find
// DESCRIPTION     : 
// ARGUMENTS PASSED: *pUri  - full path name
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : ALWAYS returns
//                   SYNCML_DM_COMMAND_NOT_ALLOWED
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS : 
// IMPORTANT NOTES : The DMTNM will return the list of child names(if 
//                   the interior node has no children it will return 
//                   an empty list
// REQUIREMENT #   : ESR-DMTNM0028-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMPluginRootNode::Find(CPCHAR pURI)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    PDmtNode ptrNode;
  
    CPCHAR pPluginPath = GetPluginURI(pURI);

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;

    dm_stat = m_ptrPluginTree->FindAddedNode(pPluginPath);
    if ( dm_stat == SYNCML_DM_SUCCESS )
        return dm_stat;


    dm_stat = m_ptrPluginTree->FindAddedParentNode(pPluginPath);
    if ( dm_stat == SYNCML_DM_SUCCESS )
        return SYNCML_DM_NOT_FOUND;

    dm_stat = m_ptrPluginTree->GetNode( pPluginPath, ptrNode );

    if ( dm_stat == SYNCML_DM_SUCCESS )
    {
       DmtData data;
       dm_stat = ptrNode->GetValue(data);
    }   

    return dm_stat;
   
}


//------------------------------------------------------------------------
// FUNCTION        : GetFormat
//
// DESCRIPTION     : This function returns FORMAT of the node
// ARGUMENTS PASSED: 
//                   *pbUri,
//                   **pRetPropertyData
// RETURN VALUE    : ALWAYS returns SYNCML_DM_FORMAT_NODE
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   : ESR-DMTNM0033-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMPluginRootNode::GetFormat(CPCHAR pURI,
                                                      SYNCML_DM_FORMAT_T *dwpRetPropertyData) 
{
    PDmtNode ptrNode;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    CPCHAR pPluginPath = GetPluginURI(pURI);

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;

    dm_stat = m_ptrPluginTree->GetNode( pPluginPath, ptrNode );
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat; 
   
    DmtAttributes oAttr;
    dm_stat = ptrNode->GetAttributes( oAttr );
    if ( dm_stat == SYNCML_DM_SUCCESS ) 
    {
        *dwpRetPropertyData=DMTree::ConvertFormatStr(oAttr.GetFormat());
    }
   
    return dm_stat;
}

//------------------------------------------------------------------------
// FUNCTION        : GetType
// DESCRIPTION     : This function returns MIME type
// ARGUMENTS PASSED: 
//                   *pbUri,
//                   **pRetPropertyData
// RETURN VALUE    : status code,type is NULL for interior nodes 
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES : 
// REQUIREMENT #   : ESR-DMTNM0033-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMPluginRootNode::GetType(CPCHAR pURI, DMString& strType)
{
    PDmtNode ptrNode;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    CPCHAR pPluginPath = GetPluginURI(pURI);

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;

    dm_stat = m_ptrPluginTree->GetNode( pPluginPath, ptrNode );
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat; 
    
    DmtAttributes oAttr;
    dm_stat = ptrNode->GetAttributes( oAttr );
    if ( dm_stat == SYNCML_DM_SUCCESS )
    {
        const DMString & type = oAttr.GetType();
        strType = type;
        if ( type.length() && strType == NULL )
            return SYNCML_DM_DEVICE_FULL;
    }    
      
    return dm_stat;
}

SYNCML_DM_RET_STATUS_T DMPluginRootNode::GetName(CPCHAR pURI, DMString& strName )
{
    CPCHAR szName=DmStrrchr(pURI, '/');

    if ( szName )
        strName = (szName + 1 );
    else
        strName = pURI;

   return SYNCML_DM_SUCCESS;
}

//------------------------------------------------------------------------
// FUNCTION        : GetSize
// DESCRIPTION     : This function returns SIZE of the node.
// ARGUMENTS PASSED: waitMsgForGetFormat,
//                   replyGetFormatCback,
//                   *pbUri,
//                   **pRetPropertyData
// RETURN VALUE    : 
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   : 
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMPluginRootNode ::GetSize(CPCHAR pURI, UINT32 *dwpRetPropertyData)
{
    PDmtNode ptrNode;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;


    CPCHAR pPluginPath = GetPluginURI(pURI);

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;

    *dwpRetPropertyData = 0;
    
    dm_stat = m_ptrPluginTree->GetNode( pPluginPath, ptrNode );
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;

    DmtAttributes oAttr;
    dm_stat = ptrNode->GetAttributes( oAttr );
    if ( dm_stat == SYNCML_DM_SUCCESS )
        *dwpRetPropertyData=oAttr.GetSize();
   
    return dm_stat;
}

//------------------------------------------------------------------------
// FUNCTION        : Rename
// DESCRIPTION     : This function is called when a node's name has
//                   been changed
// ARGUMENTS PASSED: 
//                   dwCommandId,
//                   bItemNumber,
//                   *pbUri,
//                   pNewNodeName
//                   oIsThisAtomic
// RETURN VALUE    : ALWAYS returns SYNCML_DM_SUCCESS
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES : The node Name will be renamed by the DMTNM.It informs
//                   the plug-in about this to allow the plug-in to change
//                   the name in the data-base correspondingly.Since
//                   Interior nodes in this class have no database this
//                   method simply returns SYNCML_DM_SUCCESS.This means that
//                   name of node will be renamed by DMTNM,and plug-in
//                   does not have anything specific to do.
// REQUIREMENT #   : 
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMPluginRootNode::Rename(CPCHAR pURI, CPCHAR psNewNodeName)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    CPCHAR pPluginPath = GetPluginURI(pURI);

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;
   
    dm_stat = m_ptrPluginTree->RenameNode(pPluginPath, psNewNodeName);
    return dm_stat;
}
//------------------------------------------------------------------------
// FUNCTION        : Replace
//
// DESCRIPTION     : This function is called when a node's value has
//                   to be replaced.
// ARGUMENTS PASSED: waitMsgForStatus,
//                   replyStatusCback,
//                   dwCommandId,
//                   bItemNumber,
//                   *pReplace,
//                   oMoreData
//                   oIsThisAtomic
// RETURN VALUE    : 
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   : ESR-DMTNM0024-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMPluginRootNode::Replace(DMAddData & oReplace)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    PDmtNode ptrNode;
   
    CPCHAR pPluginPath = GetPluginURI(oReplace.getURI());

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;

    dm_stat = m_ptrPluginTree->GetNode( pPluginPath, ptrNode );
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
    
    if (ptrNode->IsLeaf()) 
    {
        DmtData oData;
#ifdef LOB_SUPPORT
	if(ptrNode->IsExternalStorageNode() && oReplace.chunkData != NULL)
	{
		if(oReplace.m_chunkOffset ==0)
			dm_stat = ptrNode->SetFirstChunk(*oReplace.chunkData);
		else
			if(oReplace.IsLastChunk())
				dm_stat = ptrNode->SetLastChunk(*oReplace.chunkData);
			else
				dm_stat = ptrNode->SetNextChunk(*oReplace.chunkData);
	}
	else
#endif
	{
        dm_stat = dmBuildData(oReplace.m_nFormat, oReplace.m_oData, oData);
        if ( dm_stat != SYNCML_DM_SUCCESS )
            return dm_stat;
            
        dm_stat = ptrNode->SetValue(oData); 
	}
    }
    else 
        dm_stat = SYNCML_DM_COMMAND_NOT_ALLOWED;
 
    return dm_stat;
}

//------------------------------------------------------------------------
// FUNCTION        : Rollback
//
// DESCRIPTION     : This function is called when a node's value has
//                   needs to be rolledback. NOT SUPPORTED FOR PHASE 1
// ARGUMENTS PASSED: waitMsgForStatus,
//                   replyStatusCback,
//                   dwCommandId,
//                   bItemNumber,
//                   dmCommand
//                   *pbUri,
// RETURN VALUE    : returns SYNCML_DM_FEATURE_NOT_SUPPORTED
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   : ESR-DMTNM0037-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMPluginRootNode :: Rollback(SYNCML_DM_COMMAND_T /*dmCommand*/, CPCHAR /*pURI*/)
{
   SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;

   return m_ptrPluginTree->Rollback();
}


SYNCML_DM_RET_STATUS_T DMPluginRootNode::SetName(CPCHAR pURI, CPCHAR pbNewName)
{

   SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
   CPCHAR pPluginPath = GetPluginURI(pURI);

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;

   return m_ptrPluginTree->RenameNode(pPluginPath, pbNewName);
}

#ifdef LOB_SUPPORT
SYNCML_DM_RET_STATUS_T  DMPluginRootNode::IsESN(CPCHAR pbUri,  BOOLEAN& bESN)
{
    PDmtNode ptrNode;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    CPCHAR pPluginPath = GetPluginURI(pbUri);

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;

    dm_stat = m_ptrPluginTree->GetNode( pPluginPath, ptrNode );
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
    bESN = ptrNode->IsExternalStorageNode();
    return 	SYNCML_DM_SUCCESS;
	
}
#endif
SYNCML_DM_RET_STATUS_T DMPluginRootNode::GetTitle(CPCHAR pURI, DMString& ppbTitle)
{
    PDmtNode ptrNode;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    CPCHAR pPluginPath = GetPluginURI(pURI);

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;

    dm_stat = m_ptrPluginTree->GetNode( pPluginPath, ptrNode );
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;

    DmtAttributes oAttr;
    dm_stat = ptrNode->GetAttributes( oAttr );
    if (dm_stat == SYNCML_DM_SUCCESS)
    {
        const DMString & title = oAttr.GetTitle();
        ppbTitle = title;
        if ( title.length() && ppbTitle == NULL )
            return SYNCML_DM_DEVICE_FULL; 
    }    
  
    return dm_stat;
}

SYNCML_DM_RET_STATUS_T DMPluginRootNode::SetTitle(CPCHAR pURI, CPCHAR pbNewTitle)
{
   PDmtNode ptrNode;
   SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

   CPCHAR pPluginPath = GetPluginURI(pURI);

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;
	
   dm_stat = m_ptrPluginTree->GetNode( pPluginPath, ptrNode );
   if ( dm_stat == SYNCML_DM_SUCCESS )
       dm_stat = ptrNode->SetTitle(pbNewTitle);

   return dm_stat;
}

#ifndef DM_IGNORE_TSTAMP_AND_VERSION

XPL_CLK_CLOCK_T DMPluginRootNode::GetTStamp(CPCHAR pURI)
{
    PDmtNode ptrNode;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    CPCHAR pPluginPath = GetPluginURI(pURI);  

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return 0;

    dm_stat = m_ptrPluginTree->GetNode( pPluginPath, ptrNode );
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return 0;
   
    DmtAttributes oAttr;
    dm_stat = ptrNode->GetAttributes( oAttr );
    if ( dm_stat == SYNCML_DM_SUCCESS )
    {
        XPL_CLK_CLOCK_T timestamp;
        timestamp = (oAttr.GetTimestamp()/1000);
        return timestamp;
    }
   
   return 0;
}

UINT16 DMPluginRootNode::GetVerNo(CPCHAR pURI)
{
    PDmtNode ptrNode;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    CPCHAR pPluginPath = GetPluginURI(pURI);  

    dm_stat = GetTree();
    if ( dm_stat != SYNCML_DM_SUCCESS )
      return 0;

    dm_stat = m_ptrPluginTree->GetNode( pPluginPath, ptrNode );
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return 0;
    
    DmtAttributes oAttr;
    dm_stat = ptrNode->GetAttributes( oAttr );
    if (dm_stat == SYNCML_DM_SUCCESS)
        return oAttr.GetVersion();      
   
    return(0);
}
  
#endif


CPCHAR DMPluginRootNode::GetPluginURI(CPCHAR pURI)  
{

   char * pPluginPath = (char*)pURI;   
   
   if (DmStrstr(pPluginPath, GetPlugin()->GetPath().c_str()) != NULL)  
      pPluginPath += GetPlugin()->GetPath().length();      
   
   if ((*pPluginPath) == '/')                                 
      pPluginPath++;
   return pPluginPath;

}

SYNCML_DM_RET_STATUS_T DMPluginRootNode::GetTree()
{
    if (m_ptrPluginTree != NULL)
    {
	return SYNCML_DM_SUCCESS;
    }

    CPCHAR szPath = GetPlugin()->GetPath();

    XPL_LOG_DM_TMN_Debug(("DMPluginRootNode::GetTree m_ptrPluginTree == NULL, %s\n", szPath));

    SYNCML_DM_RET_STATUS_T dm_stat = GetPlugin()->GetTree(szPath, m_ptrPluginTree);
    if (dm_stat == SYNCML_DM_SUCCESS)
    {
        if (m_ptrPluginTree != NULL)
        {
            // set the principal string for the plugin tree
            m_ptrPluginTree->SetPrincipal(dmTreeObj.GetServerId());
        }
        else
        {
            dm_stat = SYNCML_DM_FAIL;
        }
    }
    else
    {
        m_ptrPluginTree = NULL;
    }
    return dm_stat;
}

SYNCML_DM_RET_STATUS_T DMPluginRootNode::GetTree(PDmtAPIPluginTree & ptrTree) 
{ 
   SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
   dm_stat = GetTree();
   if ( dm_stat == SYNCML_DM_SUCCESS )
   	ptrTree = m_ptrPluginTree; 
   return dm_stat;
}



//////////////////////////////////////////////////////////////
// class DMOverlayDataPluginNode
DMOverlayDataPluginNode::DMOverlayDataPluginNode(PDMPlugin pPlugin) :
  DMPluginRootNode( pPlugin )
{
}


SYNCML_DM_RET_STATUS_T DMOverlayDataPluginNode::GetFormat(CPCHAR , SYNCML_DM_FORMAT_T *pdwRetPropertyData)
{
  *pdwRetPropertyData= m_nNodeFormat;
  return SYNCML_DM_SUCCESS;
}


