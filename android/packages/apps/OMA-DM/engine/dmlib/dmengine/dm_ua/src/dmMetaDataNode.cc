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

    Source Name: dmMetaDataNode.cc

    General Description: Implementation of the DMMetaDataNode class 

==================================================================================================*/

#include <limits.h>
#include "dm_uri_utils.h"
#include "dmStringUtil.h"
#include "dmMetaDataNode.h" 
#include "xpl_Logger.h"

DMMetaDataNode::DMMetaDataNode()
{
    Init();
}


DMConstraints * 
DMMetaDataNode::GetConstraints() const
{
    if ( m_nNumConstraints )
      return (DMConstraints*)&m_oConstraints;
    else
      return NULL;
}

void DMMetaDataNode::Init() 
{
    m_psName = NULL;  
    m_szID = NULL;
    m_wAccessType = SYNCML_DM_GET_ACCESS_TYPE | SYNCML_DM_EXEC_ACCESS_TYPE;
    m_nNodeFormat = SYNCML_DM_FORMAT_INVALID;
    m_nMimeType = SYNCML_DM_DDF_MIME_TYPE_TEXTPLAIN;
    m_nNumConstraints = 0;
    m_nMaxChildrenMultiNodes = 0;
    m_bIsHasMultiChildren = FALSE;
    m_bIsMultiNode = FALSE;
    m_bStoresPD = FALSE;
    m_bPluginNode = FALSE;
    m_bOPiDataParent = FALSE;
#ifdef LOB_SUPPORT
    m_bESN = FALSE;
    m_bProgressBarNeeded = FALSE;
#endif
}


void 
DMMetaDataNode::CheckHasMultiNode(DMMetaDataBuffer oBuffer)
{
   
    UINT16 nodeType;
    
    
    oBuffer.SetOffset(oBuffer.ReadUINT32() + sizeof(UINT32));
    nodeType = oBuffer.ReadUINT16();
    if ( (nodeType & 0x80) == 0x80 ) 
        m_bIsHasMultiChildren = TRUE;
} 



SYNCML_DM_RET_STATUS_T 
DMMetaDataNode::Read(DMMetaDataBuffer oBuffer, 
                                  BOOLEAN bReadConstraints)   
{

    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;
    UINT16 nodeType;
    
    Init();

    m_psName = oBuffer.ReadString();
    nodeType = oBuffer.ReadUINT16();
    if ( (nodeType & nodeMultiNode) == nodeMultiNode) 
        m_bIsMultiNode = TRUE;

#ifdef LOB_SUPPORT
    if ( (nodeType & nodeESN) == nodeESN) 
        m_bESN = TRUE;

    if ( (nodeType & nodeProgressBar) == nodeProgressBar) 
        m_bProgressBarNeeded = TRUE;
#endif
    m_nNodeFormat = (UINT8)(nodeType & nodeTypeMask );

    if ( (nodeType & nodeHasID) != 0 )
      m_szID = oBuffer.ReadString();

    m_bPluginNode = ((nodeType & nodePluginNode) != 0);
    m_bStoresPD = ((nodeType & nodeStoresPD) != 0);
    
    m_wAccessType = oBuffer.ReadUINT8();
    m_nMimeType = oBuffer.ReadUINT8();
    m_nNumChildren = oBuffer.ReadUINT16();

    m_nOffsetChildren = oBuffer.GetOffset();

    if ( m_nNumChildren == 1 )
        CheckHasMultiNode(oBuffer);
  
   if ( bReadConstraints )
   {
        oBuffer.IncOffset(m_nNumChildren*sizeof(UINT32)); 
        m_nNumConstraints = oBuffer.ReadUINT8();
        ret_status = m_oConstraints.Read(&oBuffer, m_nNumConstraints, nodeType);
   }
   return ret_status;
   

}


SYNCML_DM_RET_STATUS_T 
DMMetaDataNode::SetChildrenOffset(DMMetaDataBuffer * pBuffer, 
                                                  UINT8 index) 
{ 
    if ( index > m_nNumChildren ) 
        return SYNCML_DM_SUCCESS;

    pBuffer->SetOffset(m_nOffsetChildren + index*sizeof(UINT32));
    
    UINT32 offset = pBuffer->ReadUINT32();
    pBuffer->SetOffset(offset);

    return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T 
DMMetaDataNode::GetMaxMultiNodeChildren(DMMetaDataBuffer oBuffer)   
{

    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;
    
    if ( m_bIsHasMultiChildren )
    {
        oBuffer.SetOffset(m_nOffsetChildren);
        UINT32 offset = oBuffer.ReadUINT32();
        oBuffer.SetOffset(offset);
        DMMetaDataNode oChildren;
        DMConstraints * pConstraints = NULL;

        ret_status = oChildren.Read(oBuffer,TRUE);
        if ( ret_status != SYNCML_DM_SUCCESS )
            return ret_status;

        m_bOPiDataParent = oChildren.m_bStoresPD;
        pConstraints = oChildren.GetConstraints();
        if ( pConstraints )
           m_nMaxChildrenMultiNodes = pConstraints->m_nMaxMultiNodes;
   }
   return ret_status;
}


SYNCML_DM_RET_STATUS_T 
DMMetaDataNode::ReadName(DMMetaDataBuffer oBuffer)   
{

    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;
    
    Init();
    m_psName = oBuffer.ReadString();
    return ret_status;
}    


BOOLEAN
DMMetaDataNode::VerifyAccessType(SYNCML_DM_ACCESS_TYPE_T accessType) const
{
    XPL_LOG_DM_TMN_Debug(("DMMetaDataNode::VerifyAccessType: node m_wAccesstype is:%x, passed accessType is %x\n", m_wAccessType,accessType));
    return ( (m_wAccessType & accessType) == accessType );
}
 
BOOLEAN DMMetaDataNode::VerifyMimeType(CPCHAR mimeType) const
{
    return TRUE;
}


void DMMetaDataNode::GetMimeType(DMString & strType)
{
    strType = "text/plain";
}

void DMMetaDataNode::SetAccessType(SYNCML_DM_ACCESS_TYPE_T accessType)
{
    m_wAccessType = accessType;

}


BOOLEAN DMMetaDataNode::IsLocal()
{
    return ( (m_wAccessType & SYNCML_DM_LOCAL_ACCESS_TYPE) ==  SYNCML_DM_LOCAL_ACCESS_TYPE );

}

BOOLEAN DMMetaDataNode::IsLeaf()
{
    if ( m_nNodeFormat  != SYNCML_DM_FORMAT_NODE &&
         m_nNodeFormat  != SYNCML_DM_FORMAT_NODE_PDATA )
         return TRUE;
    else
        return FALSE;
}

SYNCML_DM_RET_STATUS_T DMMetaDataNode::SetPath(CPCHAR szPath)
{
    if ( m_oPath.assign(szPath) == NULL )
        return SYNCML_DM_DEVICE_FULL;
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DMMetaDataNode::AllocatePath(UINT32 size)
{
     if ( m_oPath.allocate(size) == NULL )
        return SYNCML_DM_DEVICE_FULL;
     return  SetPath(".");
}

SYNCML_DM_RET_STATUS_T DMMetaDataNode::AppendSegment() 
{
     m_oPath.append((UINT8*)"/", 1);
     m_oPath.append((UINT8*)m_psName,DmStrlen(m_psName));
     return SYNCML_DM_SUCCESS;
}

BOOLEAN 
DMMetaDataNode::VerifyChildrenMultiNodesCount(UINT16 count, 
                                                                   BOOLEAN& bOPiDataParent ) const
{
    bOPiDataParent = m_bOPiDataParent;
    if ( m_nMaxChildrenMultiNodes > 0 )
    {
        return ( (count < m_nMaxChildrenMultiNodes) );
    }    
    else    
        return TRUE;
}

BOOLEAN 
DMMetaDataNode::VerifyOPINode( CPCHAR& szID,
                                               SYNCML_DM_ACCESS_TYPE_T&  wAccessType,
                                              SYNCML_DM_FORMAT_T& nNodeFormat )  const
{
  szID = m_szID;
  wAccessType = m_wAccessType;
  nNodeFormat = m_nNodeFormat;

  return m_bPluginNode;
}
