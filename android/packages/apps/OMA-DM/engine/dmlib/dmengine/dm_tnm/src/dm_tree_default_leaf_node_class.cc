//------------------------------------------------------------------------
//
//   Module Name: dm_tree_default_leaf_node_class.cpp
//
//   General Description:Contains the implementations of the methods of
//                       DMDefaultLeafNode class.
//------------------------------------------------------------------------
// Revision History:
//                     Modification   Tracking
// Author (core ID)       Date         Number    Description of Changes
// -----------------  ----
//                2003-2007                     refactoring  
// cdp180     03/16/2007    LIBll55345   Removing ACL check for internal calls                                  
// Portability: This module is portable to other compilers. 
//------------------------------------------------------------------------
//                          INCLUDE FILES
//------------------------------------------------------------------------
#include "dmdefs.h"
#include "dm_tree_default_leaf_node_class.H" 
#include "dm_tree_util.h"                    

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
//------------------------------------------------------------------------

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
SYNCML_DM_RET_STATUS_T DMDefaultLeafNode::Add(DMAddData & oAddData)
{
   return set(&oAddData);
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
SYNCML_DM_RET_STATUS_T DMDefaultLeafNode::Delete(CPCHAR pbUri)
{
    return(SYNCML_DM_SUCCESS);
}


//------------------------------------------------------------------------
// FUNCTION        : Get
// DESCRIPTION     : 
// ARGUMENTS PASSED: waitMsgForGetdata
//                   waitMsgForGetdata
//                   dwCommandId,
//                   bItemNumber,
//                   *pbUri,
//                   dwStartByte,
//                   dwNBytes
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
SYNCML_DM_RET_STATUS_T DMDefaultLeafNode::Get(CPCHAR pbUri, DMGetData & oReturnData)
{
#ifdef LOB_SUPPORT
  oReturnData.SetESN(IsESN());
#endif
   return oReturnData.set(bFormat,(CPCHAR)psData.getBuffer(),psData.getSize(),getType());
}


//------------------------------------------------------------------------
// FUNCTION        : GetFormat
//
// DESCRIPTION     : This function returns FORMAT of the node
// ARGUMENTS PASSED: waitMsgForGetFormat,
//                   replyGetFormatCback,
//                   *pbUri,
//                   **pRetPropertyData
// RETURN VALUE    : ALWAYS returns SYNCML_DM_FORMAT_NODE
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   : ESR-DMTNM0033-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultLeafNode::GetFormat(CPCHAR pbUri,
                                                       SYNCML_DM_FORMAT_T *dwpRetPropertyData)
{
   *dwpRetPropertyData = this->bFormat;
   return(SYNCML_DM_SUCCESS);
}

//------------------------------------------------------------------------
// FUNCTION        : GetType
// DESCRIPTION     : This function returns MIME type
// ARGUMENTS PASSED: waitMsgForGetFormat,
//                   replyGetFormatCback,
//                   *pbUri,
//                   **pRetPropertyData
// RETURN VALUE    : status code,type is NULL for interior nodes 
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES : 
// REQUIREMENT #   : ESR-DMTNM0033-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultLeafNode::GetType(CPCHAR pbUri,
                                                     DMString& strType)
{
   strType = this->getType();
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
SYNCML_DM_RET_STATUS_T DMDefaultLeafNode ::GetSize(CPCHAR pbUri,
                                                     UINT32 *dwpRetPropertyData)
{
   *dwpRetPropertyData=psData.getSize();
   return(SYNCML_DM_SUCCESS);
}

//------------------------------------------------------------------------
// FUNCTION        : Rename
// DESCRIPTION     : This function is called when a node's name has
//                   been changed
// ARGUMENTS PASSED: *pbUri,
//                   pNewNodeName
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
SYNCML_DM_RET_STATUS_T DMDefaultLeafNode::Rename(CPCHAR pbUri,
                                                    CPCHAR pNewNodeName)
{
   return(SYNCML_DM_SUCCESS);
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
SYNCML_DM_RET_STATUS_T DMDefaultLeafNode::Replace(DMAddData & oReplace)
{
   return set(&oReplace);
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
SYNCML_DM_RET_STATUS_T DMDefaultLeafNode::Rollback(SYNCML_DM_COMMAND_T  dmCommand,
                                                     CPCHAR pbUri)
{
   return(SYNCML_DM_FEATURE_NOT_SUPPORTED);
}

