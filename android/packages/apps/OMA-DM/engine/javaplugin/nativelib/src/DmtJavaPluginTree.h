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

#ifndef __DMTJAVAPLUGINTREE_H__
#define __DMTJAVAPLUGINTREE_H__

#include "dmt.hpp"
#include "jem_defs.hpp"
#include "plugin/dmtRWPlugin.hpp"

class DmtJavaPluginManager;

class DmtJavaPluginTree : public DmtRWPluginTree
{
public:
    DmtJavaPluginTree(const char* rootPath, DMMap<DMString, DMString>& mapParameters);
    virtual SYNCML_DM_RET_STATUS_T DeleteNode(const char* path);
    virtual SYNCML_DM_RET_STATUS_T CreateInteriorNode(const char* path, PDmtNode& ptrCreatedNode);
    virtual SYNCML_DM_RET_STATUS_T CreateLeafNode(const char* path, PDmtNode& ptrCreatedNode, const DmtData& value);
    virtual SYNCML_DM_RET_STATUS_T CreateLeafNode(const char* path, PDmtNode& ptrCreatedNode, const DmtData& value, BOOLEAN isESN);
    virtual SYNCML_DM_RET_STATUS_T CreateInteriorNodeInternal(const char* path, PDmtNode& ptrCreatedNode, const DMStringVector& childNodeNames);
    virtual SYNCML_DM_RET_STATUS_T CreateLeafNodeInternal(const char* path, PDmtNode& ptrCreatedNode, const DmtData& value);
    virtual SYNCML_DM_RET_STATUS_T CreateLeafNodeInternal(const char* path, PDmtNode& ptrCreatedNode, const DmtData& value, BOOLEAN isESN);
    virtual SYNCML_DM_RET_STATUS_T RenameNode(const char* path, const char* szNewNodeName);
    virtual SYNCML_DM_RET_STATUS_T GetNode(const char* path, PDmtNode& ptrNode);
    SYNCML_DM_RET_STATUS_T BuildPluginTree();
    DMMap<DMString, DMString>& GetParameters();
    DMString& GetRootPath();
    SYNCML_DM_RET_STATUS_T GetNodeValueInternal(const char* path, DmtData& value);
    SYNCML_DM_RET_STATUS_T SetNodeValueInternal(const char* path, const DmtData& value);
    virtual BOOLEAN IsAtomic();
    virtual SYNCML_DM_RET_STATUS_T Flush();
    virtual SYNCML_DM_RET_STATUS_T Commit();
    virtual SYNCML_DM_RET_STATUS_T Begin();
    virtual SYNCML_DM_RET_STATUS_T Rollback();

protected:
    virtual ~DmtJavaPluginTree();

private:
    JemSmartPtr<DmtJavaPluginManager> m_pluginManager;
    DMMap<DMString, DMString> m_parameters;
    BOOLEAN mIsAtomic;
};

typedef JemSmartPtr<DmtJavaPluginTree> PDmtJavaPluginTree;

#endif //__DMTJAVAPLUGINTREE_H__
