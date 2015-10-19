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

#include "plugin/dmtPlugin.hpp"
#include "DmtJavaPluginTree.h"
#include "DmtJavaPluginManager.h"
#include "DmtJavaPluginCommon.h"

// Support exec plugin
extern "C"
SYNCML_DM_RET_STATUS_T DMT_PluginLib_Execute2(
    const char*                pPath,
    DMMap<DMString, DMString>& mapParameters,
    const char*                pArgs,
    const char*                pCorrelator,
    PDmtTree                   pTree,
    DMString&                  results
)
{
    DmtJavaPlugin_Debug("Enter DMT_PluginLib_Execute2...\n");
    DmtJavaPlugin_Debug("Plugin path = %s\n", pPath);

    results = "";

    PDmtJavaPluginManager pJavaPluginManager(new DmtJavaPluginManager(pPath, mapParameters));
    if (pJavaPluginManager == NULL)
    {
        DmtJavaPlugin_Debug("Fail to create DmtJavaPluginManager\n");
        return SYNCML_DM_FAIL;
    }

    DmtJavaPlugin_Debug("Calling DmtJavaPluginManager::ExecuteNode...\n");
    SYNCML_DM_RET_STATUS_T retcode = pJavaPluginManager->ExecuteNode(pArgs, pCorrelator, pTree, results);
    DmtJavaPlugin_Debug("DmtJavaPluginManager::ExecuteNode -> retcode = %d\n", retcode);

    return retcode;
}

//Support read write data plugin
extern "C"
SYNCML_DM_RET_STATUS_T DMT_PluginLib_Data_GetPluginTree(
    const char*                pPath,
    DMMap<DMString, DMString>& mapParameters,
    PDmtAPIPluginTree&         pPluginTree
)
{
    DmtJavaPlugin_Debug("Enter DMT_PluginLib_Data_GetPluginTree...\n");
    DmtJavaPlugin_Debug("Plugin path = %s\n", pPath);

    PDmtJavaPluginTree pJavaPluginTree = new DmtJavaPluginTree(pPath, mapParameters);
    if (pJavaPluginTree == NULL)
    {
        DmtJavaPlugin_Debug("Fail to create DmtJavaPluginTree\n");
        return SYNCML_DM_FAIL;
    }

    DmtJavaPlugin_Debug("Calling DmtJavaPluginTree::BuildPluginTree...\n");
    SYNCML_DM_RET_STATUS_T retcode = pJavaPluginTree->BuildPluginTree();
    DmtJavaPlugin_Debug("DmtJavaPluginManager::BuildPluginTree -> retcode = %d\n", retcode);

    pPluginTree = (retcode != SYNCML_DM_SUCCESS ? NULL : pJavaPluginTree);

    return retcode;
}

extern "C"
int DMT_PluginLib_GetAPIVersion(void)
{
    return DMT_PLUGIN_VERSION_1_1;
}
