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
 *      The dmtPlugin.cc file contains helper classes implementation
 *      of plug-in API  
 */

#include "jem_defs.hpp"
#include "dmt.hpp"
#include "dmMemory.h"
#include "xpl_Logger.h"
#include "dmtPlugin.hpp"  
#include "dm_tree_class.H" 

//////////////////////////////////////////////////////////////////
// overlay plug-in support
SYNCML_DM_RET_STATUS_T DmtAPIPluginTree::OnAdd( CPCHAR path, DmtOverlayPluginData& data )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtAPIPluginTree::OnDelete( CPCHAR path )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtAPIPluginTree::Synchronize( const char* path, DMVector<DmtOverlayPluginSyncData>& data )
{
   return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

void DmtAPIPluginTree::Release()
{
    m_oAddedNodes.clear();
}


SYNCML_DM_RET_STATUS_T DmtAPIPluginTree::Flush()
{
    m_oAddedNodes.clear();
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtAPIPluginTree::FindAddedNode(CPCHAR path)
{
    INT32 index;
    DMString pathTmp;

    for (index = 0; index < m_oAddedNodes.size(); index++)
    {
         m_oAddedNodes[index]->GetPath(pathTmp);
         if ( pathTmp == path )
             return SYNCML_DM_SUCCESS;
    }   
    
    return SYNCML_DM_NOT_FOUND;

}


SYNCML_DM_RET_STATUS_T DmtAPIPluginTree::FindAddedParentNode(CPCHAR path)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_NOT_FOUND;
    
    if ( path == NULL)
        return dm_stat;

    DMString pStr = path; 
    char * pParent = (char*)DmStrrchr(pStr,'/');
    if ( pParent != 0 )
    {
        *pParent = '\0';
        while ( pParent != NULL )
        {
             dm_stat = FindAddedNode(pStr);
             if ( dm_stat == SYNCML_DM_SUCCESS )
                return dm_stat;
             pParent = (char*)DmStrrchr(pStr,'/');
             if ( pParent )
                 *pParent = '\0';
        }     
    }    
    return dm_stat;

}


SYNCML_DM_RET_STATUS_T DmtAPIPluginTree::SetAddedNode(PDmtNode ptrNode)
{
     m_oAddedNodes.push_back(ptrNode);
     return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtAPIPluginTree::RemoveAddedNode(CPCHAR path)
{
     INT32 index;
     INT32 size = m_oAddedNodes.size();
     INT32 length = DmStrlen(path);
     DMString pathTmp;
        
     for (index = size-1; index >= 0; index--)
     {
          m_oAddedNodes[index]->GetPath(pathTmp);
          if ( DmStrncmp(path,pathTmp,length) == 0 )
               m_oAddedNodes.remove(index);
     }
     return SYNCML_DM_SUCCESS;
     
}

BOOLEAN DmtAPIPluginTree::SetPrincipal(CPCHAR strPrincipal)
{
    m_Principal.assign(strPrincipal);
    return TRUE;
}

DmtPrincipal DmtAPIPluginTree::GetPrincipal() const 
{
    return m_Principal;
}

// this function should be called only from the plug-in "getNode" function;
// returns cached PDs and metaNodeID (or -1 if not set) for the current node.
extern "C" const DmtOPINodeData* DmtGetCachedOPINodeData()
{
    return dmTreeObj.GetCachedOPINodeData();
}

extern "C" SYNCML_DM_RET_STATUS_T DmtSetOPINodeData( CPCHAR szURI, const DmtOverlayPluginData& oData )
{
    return dmTreeObj.SetOPINodeData(szURI, oData);
}



//////////////////////////////////////////////////////////////////
//
// DmtPluginTree
// In this Plugin Tree, we main tain code relationship
//
// All path are local
// node paths are 
// 
// ""    -- root node
// "L1"  -- child node under root node
// "L1/L2" -- sub children
// 
DmtPluginTree::DmtPluginTree()
{
    XPL_LOG_DM_PLG_Debug(("DmtPluginTree::DmtPluginTree()"));
} 

DmtPluginTree::~DmtPluginTree()
{
    Release();
}


SYNCML_DM_RET_STATUS_T DmtPluginTree::Init( CPCHAR rootNodePath ) 
{
    m_strRootPath=rootNodePath;
    if ( rootNodePath && rootNodePath[0] )
    {
        if ( m_strRootPath == NULL )
        {
            XPL_LOG_DM_PLG_Debug(("DmtPluginTree::Init() SYNCML_DM_DEVICE_FULL\n"));
            return SYNCML_DM_DEVICE_FULL;
        }            
    }
    return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T  DmtPluginTree::GetFullPath(CPCHAR path, DMString & fullPath) const
{
    path = DmtGetSafeStrPtr( path );
    fullPath = m_strRootPath;

    if ( path[0] == '/' )
        fullPath += path;
    else
    {
        fullPath += "/";
        fullPath += path;
    }

    if ( path[0] != 0 )
        return SYNCML_DM_SUCCESS;

    // remove trailing '/'
    INT32 nLen = fullPath.length();

    if ( nLen == 0 )
    {
        fullPath = ".";
        return SYNCML_DM_SUCCESS;
    }    

    if ( fullPath[nLen-1] == '/' )
        fullPath.SetAt(nLen-1,0);

    return SYNCML_DM_SUCCESS;
}

//
//DmtPluginTree and DmtPluginNode all uses DmtPluginTree::GetNode 
//
SYNCML_DM_RET_STATUS_T DmtPluginTree::GetNode(CPCHAR path, 
                                               PDmtNode& ptrNode )
{
    DMString strPath(path);

    BOOLEAN result= FALSE;
       
    result= m_Nodes.lookup(strPath, ptrNode);
    XPL_LOG_DM_PLG_Debug(("GetNode lookup for %s=%d\n", path, result));
    if (!result)
        return SYNCML_DM_NOT_FOUND;

    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtPluginTree::GetChildNodeNames( 
                                               CPCHAR path,  
                                               DMStringVector &  aChildren )
{
    SYNCML_DM_RET_STATUS_T dm_stat;

    XPL_LOG_DM_PLG_Debug(("GetChildNodeNames lookup for %s\n", path));
    PDmtNode ptrNode;
    DmtData oData;

    dm_stat = GetNode( path, ptrNode);
    if (dm_stat != SYNCML_DM_SUCCESS)
    {
        XPL_LOG_DM_PLG_Debug(("GetNode err=%d\n", dm_stat));
        return dm_stat;
    }

    dm_stat = ptrNode->GetValue( oData );
    if (dm_stat != SYNCML_DM_SUCCESS)
    {
        XPL_LOG_DM_PLG_Debug(("GetValue err=%d\n", dm_stat));
        return dm_stat;
    }

    if (oData.GetType() != SYNCML_DM_DATAFORMAT_NODE )
        return SYNCML_DM_FAIL;
   
    dm_stat = oData.GetNodeValue( aChildren );
    return dm_stat;
}

SYNCML_DM_RET_STATUS_T DmtPluginTree::DeleteNode( CPCHAR path )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtPluginTree::RenameNode( CPCHAR path, 
                                                       CPCHAR szNewNodeName )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtPluginTree::CreateLeafNode( CPCHAR path, 
                                                          PDmtNode& ptrCreatedNode, 
                                                          const DmtData& value )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}
SYNCML_DM_RET_STATUS_T DmtPluginTree::CreateLeafNode(CPCHAR path,
                              PDmtNode& ptrCreatedNode,
                              const DmtData& value ,
                              BOOLEAN isESN)
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtPluginTree::CreateInteriorNode( CPCHAR path, 
                                                             PDmtNode& ptrCreatedNode )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtPluginTree::Clone(CPCHAR path, 
                                             CPCHAR szNewNodename)
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtPluginTree::GetChildValuesMap( CPCHAR path, 
                                                             DMMap<DMString, DmtData>& mapNodes )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtPluginTree::SetChildValuesMap( CPCHAR path, 
                                                             const DMMap<DMString, DmtData>& mapNodes )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtPluginTree::Flush()
{
    return DmtAPIPluginTree::Flush();
}

BOOLEAN DmtPluginTree::IsAtomic() const 
{
    return FALSE;
}

SYNCML_DM_RET_STATUS_T DmtPluginTree::Begin()
{
   #ifdef DM_ATOMIC_SUPPORTED 
    return SYNCML_DM_SUCCESS;
   #else
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
   #endif
}

SYNCML_DM_RET_STATUS_T DmtPluginTree::Commit()
{
   #ifdef DM_ATOMIC_SUPPORTED 
    return SYNCML_DM_SUCCESS;
   #else
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
   #endif
}

SYNCML_DM_RET_STATUS_T DmtPluginTree::Rollback() 
{
   #ifdef DM_ATOMIC_SUPPORTED 
    return SYNCML_DM_SUCCESS;
   #else
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
   #endif
}

SYNCML_DM_RET_STATUS_T DmtPluginTree::Verify()
{
   //For future expansion. Currently not used
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtPluginTree::GetParameter(CPCHAR name, 
                                                    DMString & value)
{
    DMPluginManager & oPluginManager = dmTreeObj.GetPluginManager();

    PDMPlugin pPlugin = oPluginManager.FindPlugin(SYNCML_DM_DATA_PLUGIN, m_strRootPath.c_str());
    XPL_LOG_DM_PLG_Debug(("GetParameter name=%s\n", name));

    DMStringMap maps=pPlugin->GetParameters();
    DMString strName=name;

    return maps.lookup(strName, value) ? SYNCML_DM_SUCCESS : SYNCML_DM_NOT_FOUND;
}

//
// Helper set Functions for shared library developer to use
//
SYNCML_DM_RET_STATUS_T DmtPluginTree::SetNode(CPCHAR path,    
                                              PDmtNode node)
{
    m_Nodes.put(path, node);
    XPL_LOG_DM_PLG_Debug(("Set Node for %s\n", path));
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtPluginTree::RemoveNode(CPCHAR path)
{
    m_Nodes.remove(path);
    XPL_LOG_DM_PLG_Debug(("Remove Node  %s\n", path));
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtPluginTree::ClearNodes()
{
    m_Nodes.clear();
    return SYNCML_DM_SUCCESS;
}

void DmtPluginTree::Release()
{
    DmtAPIPluginTree::Release();
    m_Nodes.clear();
}

//
// DmtPluginNode
//

DmtPluginNode::DmtPluginNode()
{
    m_bLeaf = FALSE;
}


SYNCML_DM_RET_STATUS_T DmtPluginNode::InitAttributes(SYNCML_DM_DATAFORMAT_T type)
{

    if (type == SYNCML_DM_DATAFORMAT_NODE)
        m_bLeaf = FALSE;
    else
        m_bLeaf = TRUE;
 
    DMString strFormat;
   if (type<0 ||DMTree::ConvertFormat(type, strFormat) != SYNCML_DM_SUCCESS) 
    {
       strFormat = "node";
    }

    return m_oAttr.Set(m_strName,strFormat.c_str(),"","text/plain",0,0,JemDate(),DmtAcl()); 
}



SYNCML_DM_RET_STATUS_T DmtPluginNode::Init(PDmtPluginTree ptrTree, 
                                        CPCHAR path)
{
    DMString fullPath; // DP: since this string is used all over the function ('name' pointer), moved declaration to the up
    
    m_ptrTree=ptrTree;
    m_strName = NULL;
    m_strPath = NULL;
    m_bESN = FALSE;
    if ( path && path[0] )
    {
        m_strPath=path;
        if ( m_strPath == NULL )
            return SYNCML_DM_DEVICE_FULL;
    }
    m_bLeaf=FALSE;
   
   //Name starts with relative PATH ONLY ?
   if ( path ) 
   {
        SYNCML_DM_RET_STATUS_T dm_stat;
        CPCHAR name = DmStrrchr(m_strPath.c_str(), '/');
     
        if (name != NULL)
            name++;
        else 
        {
            if (path[0] != 0)
                name=m_strPath.c_str();
            else // relative path is NULL, use last part of root node name
            {
                dm_stat = ptrTree->GetFullPath(path,fullPath);
                if ( dm_stat == SYNCML_DM_SUCCESS )
                {
                    name=DmStrrchr(fullPath.c_str(), '/');
                    if (name != NULL)
                        name++;
                }    
            }
        }    
        if ( name )
        {
            m_strName = name;
            if ( m_strName == NULL )
                return SYNCML_DM_DEVICE_FULL;
        }    
   }

   return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T DmtPluginNode::Init(PDmtPluginTree ptrTree, 
                                           CPCHAR path,
                                           BOOLEAN isleaf)
{
    SYNCML_DM_RET_STATUS_T dm_stat;

    dm_stat = Init(ptrTree,path);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;

    XPL_LOG_DM_PLG_Debug(("DmtPluginNode::Init, %s, type=%d\n", m_strPath.c_str(), m_oData.GetType()));  

    if (isleaf)  // derived class needs to override this for special formats (INT, BINARY, etc)
        return InitAttributes(SYNCML_DM_DATAFORMAT_STRING);
    else
        return InitAttributes(SYNCML_DM_DATAFORMAT_NODE);

}

SYNCML_DM_RET_STATUS_T DmtPluginNode::Init(PDmtPluginTree ptrTree, 
                                      CPCHAR path,
                                      const DmtData & oData,
                                      BOOLEAN isESN)
{
    SYNCML_DM_RET_STATUS_T dm_stat;
    INT32 dataSize = 0;
    SYNCML_DM_DATAFORMAT_T type = oData.GetType();

    dm_stat = Init(ptrTree,path);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;

    m_bESN = isESN;

    if(m_bESN)
    {
        if (type == SYNCML_DM_DATAFORMAT_NULL ||  
            type == SYNCML_DM_DATAFORMAT_INT ||
            type == SYNCML_DM_DATAFORMAT_UNDEFINED)
        type = SYNCML_DM_DATAFORMAT_STRING;
    }

    dm_stat = m_oData.Set(oData);

    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
    XPL_LOG_DM_PLG_Debug(("DmtPluginNode::Init, %s, type=%d\n", m_strPath.c_str(), m_oData.GetType()));  
    dm_stat = InitAttributes(type);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;

    // Get data size
   if(!m_bESN)
   {
    dm_stat = m_oData.GetSize(dataSize);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
   }
   m_oAttr.SetSize(dataSize);
  return dm_stat;
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::Init(PDmtPluginTree ptrTree, 
                                        CPCHAR path, 
                                        const DMStringVector & childNodeNames) 
{

    SYNCML_DM_RET_STATUS_T dm_stat;

    dm_stat = Init(ptrTree,path);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;


    dm_stat = m_oData.SetNodeValue(childNodeNames);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat; 

    XPL_LOG_DM_PLG_Debug(("DmtPluginNode::Init, %s, type=%d\n", m_strPath.c_str(), m_oData.GetType()));

    return InitAttributes(m_oData.GetType());
   
}


DmtPluginNode::~DmtPluginNode()
{
//    XPL_LOG_DM_PLG_Debug(("DmtPluginNode::~DmtPluginNode, %s, type=%d\n", m_strPath.c_str(), m_oData.GetType()));
    m_ptrTree =NULL;
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::GetTree( PDmtTree& ptrTree ) const
{
    ptrTree = PDmtTree(m_ptrTree.GetPtr());
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::GetPath(DMString & path) const
{
    path = m_strPath;
    if ( m_strPath != NULL && path == NULL )
        return SYNCML_DM_DEVICE_FULL;
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::GetAttributes( DmtAttributes& oAttr ) const
{
    return oAttr.Set(m_oAttr);        
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::SetTitle( CPCHAR szTitle )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::SetAcl( const DmtAcl& oAcl )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::GetValue( DmtData& oData ) const
{
    XPL_LOG_DM_PLG_Debug(("DmtPluginNode::GetValue, this=0x%x %s, type=%d\n", this, m_strPath.c_str(), m_oData.GetType()));
    return oData.Set(m_oData);
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::SetValue( const DmtData& value )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

BOOLEAN DmtPluginNode::IsLeaf() const
{
    return m_bLeaf;
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::GetChildNode( CPCHAR szPath, PDmtNode& ptrNode )
{
    if ( m_bLeaf )
        return SYNCML_DM_FEATURE_NOT_SUPPORTED;
    
    DMString oChildPath(m_strPath);
    oChildPath +="/";
    oChildPath += DmtGetSafeStrPtr( szPath );
    return m_ptrTree->GetNode(oChildPath.c_str(), ptrNode);
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::Execute( CPCHAR strData, DMString& result )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::GetNodeName(DMString & name) const 
{ 
    name = m_strName;
    if ( m_strName != NULL && name == NULL )
        return SYNCML_DM_DEVICE_FULL;
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::Rename( CPCHAR szNewName )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::GetChildNodes( DMVector<PDmtNode>& oChildren ) const
{
    XPL_LOG_DM_PLG_Debug(("GetChildNodes for %s\n", m_strPath.c_str()));
  
    oChildren.clear(); // remove all previous items from array

    if ( m_bLeaf )
        return SYNCML_DM_FAIL;
  
    DmtData oData;

    SYNCML_DM_RET_STATUS_T dm_stat = GetValue( oData );

    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;

    const DMStringVector & aChildren = oData.GetNodeValue();

    XPL_LOG_DM_PLG_Debug(("aChildren.size()=%d\n", aChildren.size()));

    for ( INT32 i = 0; i < aChildren.size(); i++ )
    {
        DMString strChildPath = m_strPath;

        if (strChildPath.length() != 0)
            strChildPath += "/";
        strChildPath += aChildren[i];

        PDmtNode ptrNode;

        XPL_LOG_DM_PLG_Debug(("Get child[%d], path=%s\n", i, strChildPath.c_str()));
        dm_stat = m_ptrTree->GetNode( strChildPath.c_str(), ptrNode );
        if ( dm_stat != SYNCML_DM_SUCCESS )
            return dm_stat;

        oChildren.push_back( ptrNode );
    }

    return dm_stat;
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::GetFirstChunk(DmtDataChunk&  dmtChunkData) 
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::GetNextChunk(DmtDataChunk& dmtChunkData)
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::SetFirstChunk(DmtDataChunk& dmtChunkData)  
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::SetNextChunk(DmtDataChunk& dmtChunkData)  
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtPluginNode::SetLastChunk(DmtDataChunk& dmtChunkData)  
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}
