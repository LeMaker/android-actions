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

#include "dmt.hpp"
#include "plugin/dmtPlugin.hpp"
#include "DmtJavaPluginNode.h"
#include "DmtJavaPluginTree.h"
#include "DmtJavaPluginCommon.h"

DmtJavaPluginNode::DmtJavaPluginNode(PDmtJavaPluginTree ptrTree, const char* path, 
             const DmtData& data):DmtRWPluginNode()
{
    Init(static_cast<PDmtPluginTree>(ptrTree), path, data); 
    m_javaPluginTree = ptrTree;
}

DmtJavaPluginNode::~DmtJavaPluginNode()
{
    m_javaPluginTree = NULL;
}

DmtJavaPluginNode::DmtJavaPluginNode(PDmtJavaPluginTree ptrTree, const char* path, 
             const DMStringVector& childNodeNames):DmtRWPluginNode()
{
    Init(static_cast<PDmtPluginTree>(ptrTree), path, childNodeNames); 
    m_javaPluginTree = ptrTree;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginNode::GetValue(DmtData& oData) const 
{
    DmtJavaPlugin_Debug("Inside DmtJavaPluginNode::GetValue: oData-->\n"); 
    DmtJavaPlugin_Debug("m_strPath.c_str() is %s\n", m_strPath.c_str());
    if(!IsLeaf())
    {
        DmtJavaPlugin_Debug("This is a interior node!\n");  
        return DmtPluginNode::GetValue(oData);
    }
    return m_javaPluginTree->GetNodeValueInternal(m_strPath.c_str(), oData);
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginNode::SetValue(const DmtData& oData)
{
     DmtJavaPlugin_Debug("Inside DmtJavaPluginNode::SetValue:\n");
     if(!IsLeaf())
     {
        DmtJavaPlugin_Debug("This is a interior node!\n");  
        return DmtRWPluginNode::SetValue(oData);
     }
     SYNCML_DM_RET_STATUS_T res = DmtRWPluginNode::SetValue(oData);
     if(res != SYNCML_DM_SUCCESS)
     {    
        DmtJavaPlugin_Debug("update memory tree failed...\n"); 
        return res;    
     }
     DmtJavaPlugin_Debug("m_strPath.c_str() is %s\n", m_strPath.c_str());
     return m_javaPluginTree->SetNodeValueInternal(m_strPath.c_str(), oData);
}
