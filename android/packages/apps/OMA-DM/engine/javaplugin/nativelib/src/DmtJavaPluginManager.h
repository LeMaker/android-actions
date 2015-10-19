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

#ifndef __DMTJAVAPLUGINMANAGER_H__
#define __DMTJAVAPLUGINMANAGER_H__

#include "DmtJavaItem.h"
#include "DmtJavaPluginTree.h"

#include "jem_defs.hpp"

#include "dmt.hpp"

/* NOTE: These constants have to be equal to corresponding    */
/* constants in com.android.omadm.plugin.DmtData class. */
enum {
    JAVA_SYNCML_DM_DATAFORMAT_NULL   = 0,
    JAVA_SYNCML_DM_DATAFORMAT_STRING = 1,
    JAVA_SYNCML_DM_DATAFORMAT_INT    = 2,
    JAVA_SYNCML_DM_DATAFORMAT_BOOL   = 3,
    JAVA_SYNCML_DM_DATAFORMAT_BIN    = 4,
    JAVA_SYNCML_DM_DATAFORMAT_NODE   = 5,
    JAVA_SYNCML_DM_DATAFORMAT_B64    = 6,
    JAVA_SYNCML_DM_DATAFORMAT_XML    = 7,
    JAVA_SYNCML_DM_DATAFORMAT_DATE   = 8,
    JAVA_SYNCML_DM_DATAFORMAT_TIME   = 10,
    JAVA_SYNCML_DM_DATAFORMAT_FLOAT  = 11
};

class DmtJavaPluginManager : public JemBaseObject
{
public:
    DmtJavaPluginManager(const char* pPath, DMStringMap& mapParameters);
    ~DmtJavaPluginManager();

    SYNCML_DM_RET_STATUS_T GetNodeValue(
        const char* pPath,
        DmtData&    data);

    SYNCML_DM_RET_STATUS_T SetNodeValue(
        const char*    pPath,
        const DmtData& data);

    SYNCML_DM_RET_STATUS_T CreateInteriorNode(
        const char* pPath);

    SYNCML_DM_RET_STATUS_T CreateLeafNode(
        const char*    pPath,
        const DmtData& data);

    SYNCML_DM_RET_STATUS_T RenameNode(
        const char* pPath,
        const char* pNewNodeName);

    SYNCML_DM_RET_STATUS_T DeleteNode(
        const char* pPath);

    SYNCML_DM_RET_STATUS_T Commit();

    SYNCML_DM_RET_STATUS_T ExecuteNode(
        const char* pArgs,
        const char* pCorrelator,
        PDmtTree    pTree,
        DMString&   result);

    SYNCML_DM_RET_STATUS_T BuildPluginTree(
        PDmtJavaPluginTree pTree);

private:
    bool InitJNIEnv();
    bool InitJavaPluginManager();
    bool InitJavaPlugin(const char* pPath, DMStringMap& mapParameters);

    void ReleasePluginManager();

    SYNCML_DM_RET_STATUS_T GetDmtNodeValue(
        jint&          type,
        char*&         pValue,
        const DmtData& data);

    SYNCML_DM_RET_STATUS_T SetDmtNodeValue(
        const char* pType,
        const char* pValue,
        DmtData&    data);

    SYNCML_DM_RET_STATUS_T SetServerID(
        PDmtTree pTree);

    JNIEnv*    mEnv;
    DmtJObject mJavaPluginManager;
    bool       mIsInitialized;
};

typedef JemSmartPtr<DmtJavaPluginManager> PDmtJavaPluginManager;

#endif //__DMTJAVAPLUGINMANAGER_H___
