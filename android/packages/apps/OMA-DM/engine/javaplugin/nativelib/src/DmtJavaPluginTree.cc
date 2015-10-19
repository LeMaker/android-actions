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

#include "DmtJavaPluginTree.h"
#include "DmtJavaPluginNode.h"
#include "DmtJavaPluginManager.h"
#include "DmtJavaPluginCommon.h"

DmtJavaPluginTree::DmtJavaPluginTree(const char* rootPath, DMMap<DMString, DMString>& mapParameters)
   : DmtRWPluginTree(), mIsAtomic(FALSE)
{
    DmtJavaPlugin_Debug("Inside: DmtJavaPluginTree constructor...\n");
    Init(rootPath);
    DmtJavaPlugin_Debug("Finish init rootPath, and begin create new instance of DmtJavaPluginManager..\n");
    m_pluginManager = new DmtJavaPluginManager(rootPath, mapParameters);
    DmtJavaPlugin_Debug("Finish create new instance of DmtJavaPluginManager..\n");
    m_parameters = mapParameters;
}

DmtJavaPluginTree::~DmtJavaPluginTree()
{
    m_pluginManager = NULL;
    m_parameters.clear();
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginTree::DeleteNode(const char* path)
{
    DmtJavaPlugin_Debug("Inside: DmtJavaPluginTree::DeleteNode, path = %s\n", path);
    if (m_pluginManager == NULL)
    {
        DmtJavaPlugin_Debug("Plugin manager is not created\n");
        return SYNCML_DM_FAIL;
    }

    SYNCML_DM_RET_STATUS_T res = DmtRWPluginTree::DeleteNode(path);
    DmtJavaPlugin_Debug("DmtJavaPluginTree::DeleteNode: DmtRWPluginTree::DeleteNode res = %d\n", res);
    if (res == SYNCML_DM_SUCCESS)
    {
        res = m_pluginManager->DeleteNode(path);
    }
    DmtJavaPlugin_Debug("Leave: DmtJavaPluginTree::DeleteNode, res = %d\n", res);
    return res;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginTree::CreateInteriorNode(const char* path, PDmtNode& ptrCreatedNode)
{
    DmtJavaPlugin_Debug("Inside: DmtJavaPluginTree::CreateInteriorNode, path = %s \n", path);

    if (m_pluginManager == NULL)
    {
        DmtJavaPlugin_Debug("Plugin manager is not created\n");
        return SYNCML_DM_FAIL;
    }

    SYNCML_DM_RET_STATUS_T res = DmtRWPluginTree::CreateInteriorNode(path, ptrCreatedNode);
    DmtJavaPlugin_Debug("Leave: DmtJavaPluginTree::CreateInteriorNode, res = %d\n", res);
    return res;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginTree::CreateInteriorNodeInternal(const char* path,
                                                                     PDmtNode& ptrCreatedNode,
                                                                     const DMStringVector& childNodeNames)
{
    DmtJavaPlugin_Debug("Inside: DmtJavaPluginTree::CreateInteriorNodeInternal, path = %s\n", path);

    SYNCML_DM_RET_STATUS_T res = m_pluginManager->CreateInteriorNode(path);
    if (res == SYNCML_DM_SUCCESS)
    {
        PDmtJavaPluginNode pNode = new DmtJavaPluginNode(this, path, childNodeNames);
        res = this->SetNode(path, static_cast<PDmtNode>(pNode));
        if (res != SYNCML_DM_SUCCESS)
        {
            m_pluginManager->DeleteNode(path);
        }
        ptrCreatedNode = pNode;
    }
    DmtJavaPlugin_Debug("Leave: DmtJavaPluginTree::CreateInteriorNodeInternal, path = %s\n", path);
    return res;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginTree::CreateLeafNode(const char* path, PDmtNode& ptrCreatedNode, const DmtData& value)
{
    DmtJavaPlugin_Debug("Inside: DmtJavaPluginTree::CreateLeafNode, path = %s\n", path);
    if (m_pluginManager == NULL)
    {
        DmtJavaPlugin_Debug("Plugin manager is not created\n");
        return SYNCML_DM_FAIL;
    }

    SYNCML_DM_RET_STATUS_T res = DmtRWPluginTree::CreateLeafNode(path, ptrCreatedNode, value);
    DmtJavaPlugin_Debug("Leave: DmtJavaPluginTree::CreateLeafNode, res = %d\n", res);
    return res;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginTree::CreateLeafNode(const char* path, PDmtNode& ptrCreatedNode, const DmtData& value, BOOLEAN isESN)
{
    DmtJavaPlugin_Debug("Inside: DmtJavaPluginTree::CreateLeafNode, path = %s\n", path);
    if (m_pluginManager == NULL)
    {
        DmtJavaPlugin_Debug("Plugin manager is not created\n");
        return SYNCML_DM_FAIL;
    }

    SYNCML_DM_RET_STATUS_T res = DmtRWPluginTree::CreateLeafNode(path, ptrCreatedNode, value, isESN);
    DmtJavaPlugin_Debug("Leave: DmtJavaPluginTree::CreateLeafNode, res = %d\n", res);
    return res;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginTree::CreateLeafNodeInternal(const char* path, PDmtNode& ptrCreatedNode, const DmtData& value)
{
    DmtJavaPlugin_Debug("Inside: DmtJavaPluginTree::CreateLeafNodeInternal, path = %s\n", path);

    SYNCML_DM_RET_STATUS_T res = m_pluginManager->CreateLeafNode(path, value);
    if (res == SYNCML_DM_SUCCESS)
    {
        PDmtJavaPluginNode pNode = new DmtJavaPluginNode(this, path, value);
        res = this->SetNode(path, static_cast<PDmtNode>(pNode));
        if (res != SYNCML_DM_SUCCESS)
        {
            m_pluginManager->DeleteNode(path);
        }
        ptrCreatedNode = pNode;
    }
    DmtJavaPlugin_Debug("Leave: DmtJavaPluginTree::CreateLeafNodeInternal, path = %s\n", path);
    return res;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginTree::CreateLeafNodeInternal(const char* path, PDmtNode& ptrCreatedNode, const DmtData& value, BOOLEAN isESN)
{
    DmtJavaPlugin_Debug("Inside: DmtJavaPluginTree::CreateLeafNodeInternal, path = %s, isESN = %d\n", path, isESN);
    return CreateLeafNodeInternal(path, ptrCreatedNode, value);
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginTree::RenameNode(const char* path, const char* szNewNodeName)
{
    DmtJavaPlugin_Debug("Inside: DmtJavaPluginTree::RenameNode, path = %s\n", path);

    if (m_pluginManager == NULL)
    {
        DmtJavaPlugin_Debug("Plugin manager is not created\n");
        return SYNCML_DM_FAIL;
    }

    // This method is not supported by DmtPluginTree so we don't call java plug-in
    // SYNCML_DM_RET_STATUS_T res = m_pluginManager->RenameNode(path, szNewNodeName);
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginTree::GetNode(const char* szPath, PDmtNode& ptrNode)
{
    if (!szPath)
    {
        szPath = "";
    }
    DmtJavaPlugin_Debug("DmtJavaPluginTree::GetNode(%s)\n", szPath);
    return DmtPluginTree::GetNode(szPath, ptrNode);
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginTree::BuildPluginTree()
{
    DmtJavaPlugin_Debug("Inside: DmtJavaPluginTree::BuildPluginTree\n");
    if (m_pluginManager == NULL)
    {
        DmtJavaPlugin_Debug("Plugin manager is not created\n");
        return SYNCML_DM_FAIL;
    }

    SYNCML_DM_RET_STATUS_T res = m_pluginManager->BuildPluginTree(this);
    DmtJavaPlugin_Debug("Leave: DmtJavaPluginTree::BuildPluginTree, res = %d\n", res);
    return res;
}

DMMap<DMString, DMString>& DmtJavaPluginTree::GetParameters()
{
    return m_parameters;
}

DMString& DmtJavaPluginTree::GetRootPath()
{
    return m_strRootPath;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginTree::GetNodeValueInternal(const char* path, DmtData& value)
{
    DmtJavaPlugin_Debug("Inside: DmtJavaPluginTree::GetNodeValueInternal, path = %s\n", path);
    if (m_pluginManager == NULL)
    {
        DmtJavaPlugin_Debug("Plugin manager is not created\n");
        return SYNCML_DM_FAIL;
    }

    SYNCML_DM_RET_STATUS_T res = m_pluginManager->GetNodeValue(path, value);
    DmtJavaPlugin_Debug("Leave: DmtJavaPluginTree::GetNodeValueInternal, res = %d\n", res);
    return res;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginTree::SetNodeValueInternal(const char* path, const DmtData& value)
{
    DmtJavaPlugin_Debug("Inside: DmtJavaPluginTree::SetNodeValueInternal, path = %s\n", path);
    if (m_pluginManager == NULL)
    {
        DmtJavaPlugin_Debug("Plugin manager is not created\n");
        return SYNCML_DM_FAIL;
    }

    SYNCML_DM_RET_STATUS_T res = m_pluginManager->SetNodeValue(path, value);
    DmtJavaPlugin_Debug("Leave: DmtJavaPluginTree::SetNodeValueInternal, res = %d\n", res);
    return res;
}

BOOLEAN DmtJavaPluginTree::IsAtomic()
{
    DmtJavaPlugin_Debug("DmtJavaPluginTree::IsAtomic\n");
    return mIsAtomic;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginTree::Flush()
{
    DmtJavaPlugin_Debug("Inside: DmtJavaPluginTree::Flush\n");

    //TODO: Workaround. Commit will be called by engine in any case.
    Commit();

    // ehb005:should not set this NULL because tree could be accessed again after flush
    //m_pluginManager = NULL;

    SYNCML_DM_RET_STATUS_T res = DmtRWPluginTree::Flush();

    DmtJavaPlugin_Debug("Leave: DmtJavaPluginTree::Flush, res = %d\n", res);

    return res;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginTree::Commit()
{
    DmtJavaPlugin_Debug("Inside: DmtJavaPluginTree::Commit\n");
    if (m_pluginManager == NULL)
    {
        DmtJavaPlugin_Debug("Plugin manager is not created\n");
        return SYNCML_DM_FAIL;
    }

    mIsAtomic = FALSE;

    SYNCML_DM_RET_STATUS_T res = m_pluginManager->Commit();
    DmtJavaPlugin_Debug("Leave: DmtJavaPluginTree::Commit, res = %d\n", res);
    return res;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginTree::Begin()
{
    DmtJavaPlugin_Debug("DmtJavaPluginTree::Begin\n");
    mIsAtomic = TRUE;
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginTree::Rollback()
{
    DmtJavaPlugin_Debug("DmtJavaPluginTree::Rollback\n");
    mIsAtomic = FALSE;
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}
