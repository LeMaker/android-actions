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

#ifndef __DMTJAVAPLUGINNODE_H__
#define __DMTJAVAPLUGINNODE_H__

#include "jem_defs.hpp"
#include "dmt.hpp"
#include "plugin/dmtRWPlugin.hpp"

class DmtJavaPluginTree;

class DmtJavaPluginNode : public DmtRWPluginNode
{
public:
    DmtJavaPluginNode(JemSmartPtr<DmtJavaPluginTree> ptrTree, const char* path, const DmtData& data);
    DmtJavaPluginNode(JemSmartPtr<DmtJavaPluginTree> ptrTree, const char* path, const DMStringVector & childNodeNames);
    virtual SYNCML_DM_RET_STATUS_T GetValue(DmtData& oData) const;
    virtual SYNCML_DM_RET_STATUS_T SetValue(const DmtData& value);

protected:
    virtual ~DmtJavaPluginNode();
    JemSmartPtr<DmtJavaPluginTree> m_javaPluginTree;
private:

};

typedef JemSmartPtr<DmtJavaPluginNode> PDmtJavaPluginNode;

#endif //__DMTJAVAPLUGINNODE_H__
