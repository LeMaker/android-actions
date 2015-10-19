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
//   Module Name: dm_tree_node_class.cc
//
//   General Description:Contains the implementations of the methods of
//                       DMNode class.
//------------------------------------------------------------------------

#include "dmdefs.h"
#include "dm_tree_node_class.H"   
#include "dm_tree_class.H" 
#include "SyncML_DM_Archive.H"

// For the mime type optimization
#define DEFAULT_MIME_TYPE "text/plain"
#define DEFAULT_MIME_TYPE_INTERNAL ""

class DMTree;

// Constructor that sets class UID to the object while creating the object
DMNode::DMNode(BOOLEAN bPlugin): 
  pcParentOfNode(NULL), 
  pcFirstChild(NULL),
  pcNextSibling(NULL)
{
  bFormat = 0;
  m_nFlags = bPlugin ? enum_NodePlugin : 0;
  
  //Only root of the archive has it set. Others will be NULL.
  this->pArchive=NULL;
#ifndef DM_IGNORE_TSTAMP_AND_VERSION
  wTStamp = 0;
  wVerNo = 0;
#endif
}

//Destructor of the DMNode class
DMNode::~DMNode()
{
  //Only root of the archive has it set. Others will be NULL.
  if (this->pArchive != NULL)
  {
      if ( this->pArchive->getRootNode() == this )
      {
         this->pArchive->setRootNode(NULL);
      }
  }



}


SYNCML_DM_RET_STATUS_T DMNode::GetName(CPCHAR pbUri, DMString& strName )
{
    strName = abNodeName;
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DMNode::SetName(CPCHAR pbUri, CPCHAR pbNewName)
{
    SYNCML_DM_RET_STATUS_T sRetStatus = SYNCML_DM_SUCCESS;

    this->abNodeName = pbNewName;
    return sRetStatus;
}

SYNCML_DM_RET_STATUS_T DMNode::GetTitle(CPCHAR pbUri,  DMString& ppbTitle)
{
    ppbTitle = m_strTitle;
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DMNode::SetTitle(CPCHAR pbUri, CPCHAR pbNewTitle)
{
    m_strTitle = pbNewTitle;
    return SYNCML_DM_SUCCESS;
}

#ifndef DM_IGNORE_TSTAMP_AND_VERSION

SYNCML_DM_RET_STATUS_T DMNode::SetTStamp(CPCHAR pbUri, XPL_CLK_CLOCK_T timeStamp) 
{ 
    wTStamp = timeStamp; 
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DMNode::SetVerNo(CPCHAR pbUri, UINT16 verNo)
{
    if (verNo == 0xffff) 
    {
        wVerNo = 0;
    }
    else
        wVerNo = verNo;
    return SYNCML_DM_SUCCESS;
}
#endif

DMNode* DMNode::GetChildByName( CPCHAR szName ) const
{
  DMNode *pNode = pcFirstChild;

  while ( pNode && pNode->abNodeName != szName )
    pNode = pNode->pcNextSibling;

  return pNode;
}

DMNode* DMNode::GetNextSerializeItem() 
{
  DMNode *pItem = this;

  while ( pItem ) {
    if ( !pItem->isPlugin() && !pItem->pArchive )
      return pItem;

    pItem = pItem->pcNextSibling;
  }
  return NULL;  
}

void DMNode::ConvertPathToSkeleton( DMNode* psStartNode ) 
{
  DMNode *pNode = pcParentOfNode;

  while ( pNode && pNode != psStartNode ){
    pNode->m_nFlags |= enum_NodeSkeleton;
    pNode = pNode->pcParentOfNode;
  }
  
  if ( pNode )
    pNode->m_nFlags |= enum_NodeSkeleton;
}

CPCHAR DMNode::getType() const
{
    if (psType_ == DEFAULT_MIME_TYPE_INTERNAL) {
        return DEFAULT_MIME_TYPE;
    } else {
        return psType_.c_str();
    }
}

SYNCML_DM_RET_STATUS_T DMNode::setType(CPCHAR strType)
{
    if ( strType )
    {
        if ( DmStrcmp(strType,DEFAULT_MIME_TYPE) == 0 ) 
        {
            psType_ = DEFAULT_MIME_TYPE_INTERNAL;
        } else {
            psType_ = strType;
            if ( psType_ == NULL && strType[0] )
               return SYNCML_DM_DEVICE_FULL;
        }
    }    
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DMNode::set(const DMGetData * pData)
{
   SYNCML_DM_RET_STATUS_T res;

   bFormat= pData->m_nFormat == SYNCML_DM_FORMAT_NODE_PDATA ? SYNCML_DM_FORMAT_NODE : pData->m_nFormat;

   CPCHAR pMimeType = ( pData->m_oMimeType.getSize() ?  pData->getType() : NULL ); 
   
   res = setType(pMimeType);
   if ( res != SYNCML_DM_SUCCESS )
      return res; 
   
   if ( pData->m_oData.getSize() && getData())
   {
      *getData() = pData->m_oData;
      if ( getData()->getBuffer() == 0 )
        return SYNCML_DM_DEVICE_FULL;
   }
   else 
	  if  ( getData() ) 	
	   {
	      // Set boolean string value if it is omitted, otherwise size 0
	      // will be returned for DM API call.
	      if (!pData->m_oData.getSize() )
		{
		     if (bFormat == SYNCML_DM_FORMAT_BOOL) 
	      	    {
	         	    (*getData()).assign("false");
	      	    }
	      	    else
		   {
			   (*getData()).clear();
		   }		   
	      }
	   }	  

   return SYNCML_DM_SUCCESS;
}   


SYNCML_DM_RET_STATUS_T DMNode::set(CPCHAR strName, CPCHAR strTitle, const DMGetData * pData)
{

   if ( strName )
   {
      abNodeName = strName;
      if ( abNodeName == NULL && strName[0] ) 
        return SYNCML_DM_DEVICE_FULL;
   }
   else
      return SYNCML_DM_FAIL;
   
 
   if ( strTitle )
   {
      m_strTitle = strTitle;
      if ( m_strTitle == NULL && strTitle[0] )
        return SYNCML_DM_DEVICE_FULL; 
   }   

   return set(pData);

}


SYNCML_DM_RET_STATUS_T DMNode::set(const DMAddNodeProp * pNodeProp)
{

   m_nFlags = pNodeProp->m_nFlags;
#ifndef DM_IGNORE_TSTAMP_AND_VERSION
   wTStamp = pNodeProp->m_nTStamp;
   wVerNo = pNodeProp->m_nVerNo;
#endif   

   CPCHAR pTitle = ( pNodeProp->m_oTitle.getSize() ?  pNodeProp->getTitle() : NULL ); 
   CPCHAR pName = ( pNodeProp->m_oName.getSize() ?  pNodeProp->getName() : NULL ); 

   if ( getOverlayPIData() )
   {
      getOverlayPIData()->set_size( pNodeProp->m_oOPiData.getSize() );
      if ( getOverlayPIData()->size() != pNodeProp->m_oOPiData.getSize() )
        return SYNCML_DM_DEVICE_FULL;
      memcpy( getOverlayPIData()->get_data(), pNodeProp->m_oOPiData.getBuffer(), 
        getOverlayPIData()->size() );
   }
   
   return set(pName,pTitle,(DMGetData*)pNodeProp);

}

#ifdef LOB_SUPPORT
SYNCML_DM_RET_STATUS_T	DMNode::IsESN(CPCHAR pbUri, BOOLEAN& bESN) 
{ 
    bESN = (m_nFlags & enum_NodeESN) != 0; 
   return SYNCML_DM_SUCCESS;
}
#endif
