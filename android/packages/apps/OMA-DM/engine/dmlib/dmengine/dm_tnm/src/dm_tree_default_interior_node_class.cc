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
//   Module Name: dm_default_interior_node_class.cpp
//
//   General Description: Contains the implementations of the methods of
//                       DMDefaultInteriorNode class.
//------------------------------------------------------------------------

#include "dm_tree_default_interior_node_class.H" //header file for class defn

#define ADD_INT_NODES_DYNAMICALLY_FOR_TESTING 1

//------------------------------------------------------------------------
// FUNCTION        : Add
// DESCRIPTION     : This function sets the ACCESS type and format
//                   properties for an Interior node added in the tree.
//                   No data involved ,so synchronous Non-blocking call
//                   callback function pointers will be ignored
// ARGUMENTS PASSED: *psAdd,
//                 
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : status code
// PRE-CONDITIONS  : 1.URI has already been validated
//                   2.The node object of the plug-in class has been
//                     created and a reference has been given to DMTNM
// POST-CONDITIONS : Following property values are set by the function
//                   Format = SYNCML_DM_FORMAT_NODE
// IMPORTANT NOTES : The default Interior node class grants all Access
//                   rights to the Node in Add
//                   If a Plug-in needs to implement a class for interior
//                   nodes in which the access type needs to be plug-in
//                   specific Ex: that plug-in does not want to give
//                   delete access type, then the plug-in SHALL implement
//                   it's own Add accordingly.
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultInteriorNode::Add(DMAddData & oAddData)
{
    XPL_LOG_DM_TMN_Debug(("Entered DMDefaultInteriorNode::Add \n"));
#ifdef ADD_INT_NODES_DYNAMICALLY_FOR_TESTING
    return SYNCML_DM_SUCCESS;
#else
    return SYNCML_DM_COMMAND_NOT_ALLOWED;
#endif
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
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultInteriorNode :: Delete(CPCHAR pbUri)
{
    XPL_LOG_DM_TMN_Debug(("Entered DMDefaultInteriorNode::Delete \n"));
    return SYNCML_DM_SUCCESS;
}

//------------------------------------------------------------------------
// FUNCTION        : Get
// DESCRIPTION     : This function should not actually not get called.
//                   since interior node has no value to return.It is a
//                   pure virtual function in DMNode,hence is implemented
//                   and returns SYNCML_DM_SUCCESS
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
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultInteriorNode::Get(CPCHAR pbUri, DMGetData & oReturnData)
{
#ifdef LOB_SUPPORT
    oReturnData.SetESN(FALSE);
#endif
    return SYNCML_DM_SUCCESS;
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
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultInteriorNode::GetFormat(CPCHAR pbUri,
        SYNCML_DM_FORMAT_T *dwpRetPropertyData)
{
    XPL_LOG_DM_TMN_Debug(("Entered DMDefaultInteriorNode::GetFormat \n"));
    *dwpRetPropertyData = SYNCML_DM_FORMAT_NODE;
    return SYNCML_DM_SUCCESS;
}

//------------------------------------------------------------------------
// FUNCTION        : GetType
// DESCRIPTION     : This function returns SYNCML_DM_COMMAND_NOT_ALLOWED
//                   since TYPE of the interior node is not present
// ARGUMENTS PASSED: waitMsgForGetFormat,
//                   replyGetFormatCback,
//                   *pbUri,
//                   **pRetPropertyData
// RETURN VALUE    : status code,type is NULL for interior nodes 
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES : 
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultInteriorNode::GetType(CPCHAR pbUri,
        DMString& strType)
{
    strType = NULL;
    return SYNCML_DM_SUCCESS;
}

//------------------------------------------------------------------------
// FUNCTION        : GetSize
// DESCRIPTION     : This function returns SIZE of the node.Interior node
//                   has no data,hence there is no Size property value
// ARGUMENTS PASSED: waitMsgForGetFormat,
//                   replyGetFormatCback,
//                   *pbUri,
//                   **pRetPropertyData
// RETURN VALUE    : ALWAYS returns SYNCML_DM_COMMAND_NOT_ALLOWED
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultInteriorNode::GetSize(CPCHAR pbUri,
        UINT32 *dwpRetPropertyData)
{
    return SYNCML_DM_COMMAND_NOT_ALLOWED;
}

//------------------------------------------------------------------------
// FUNCTION        : Rename
// DESCRIPTION     : This function is called when a node's name has
//                   been changed
// ARGUMENTS PASSED: waitMsgForStatus,
//                   replyStatusCback,
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
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultInteriorNode::Rename(CPCHAR pbUri, CPCHAR ppNewNodeName)
{
    XPL_LOG_DM_TMN_Debug(("Entered DMDefaultInteriorNode::Rename\n"));
    return SYNCML_DM_SUCCESS;
}

//------------------------------------------------------------------------
// FUNCTION        : Replace
//
// DESCRIPTION     : This function is called when a node's value has
//                   to be replaced.It returns
//                   SYNCML_DM_COMMAND_NOT_ALLOWED
// ARGUMENTS PASSED: waitMsgForStatus,
//                   replyStatusCback,
//                   dwCommandId,
//                   bItemNumber,
//                   *pReplace,
//                   oMoreData
//                   oIsThisAtomic
// RETURN VALUE    : ALWAYS returns SYNCML_DM_COMMAND_NOT_ALLOWED
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultInteriorNode::Replace(DMAddData & oReplace)
{
    XPL_LOG_DM_TMN_Debug(("Entered DMDefaultInteriorNode::Replace \n"));
    return SYNCML_DM_COMMAND_NOT_ALLOWED;
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
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultInteriorNode::Rollback(SYNCML_DM_COMMAND_T dmCommand, CPCHAR pbUri)
{
    XPL_LOG_DM_TMN_Debug(("Entered DMDefaultInteriorNode::Rollback \n"));
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}
