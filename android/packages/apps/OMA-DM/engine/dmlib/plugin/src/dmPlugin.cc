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

    Source Name: dmPlugin.cc

    General Description: Implementation of the DMPlugin class

==================================================================================================*/

#include "dmt.hpp"
#include "dmdefs.h"    
#include "xpl_Logger.h"
#include "dm_tree_class.H"
#include "dmPluginManager.h"

DMPlugin::DMPlugin(SYNCML_DM_PLUGIN_TYPE_T type,
                   BOOLEAN  bOverlayPlugin,
                   const DMString & strPath, 
                   DMStringMap & mapParameters)
{
   
    m_type=type;
    m_bOverlayPlugin = bOverlayPlugin;
    m_strPath=strPath;
    m_mapParameters=mapParameters;

    pfGetTree = NULL;
    pfExecute2 = NULL;
    pfCheckConstraint = NULL;
    pfOnCommit = NULL;

    m_hLibData = NULL;
    m_hLibExec = NULL;
    m_hLibConstraint = NULL;
    m_hLibCommit = NULL;

    m_pTree=NULL;

    UpdateLastAccessedTime();
}


DMPlugin::~DMPlugin()
{
  
    UnloadSymbols();
}

void
DMPlugin::UnloadSymbols()
{
    if (m_pTree!=NULL) 
    {
      XPL_LOG_DM_PLG_Debug(("Release m_pTree: %s\n", m_strPath.c_str()));
      m_pTree->Release();
    }

    pfGetTree = NULL;
    pfExecute2 = NULL;
    pfCheckConstraint = NULL;
    pfOnCommit = NULL;
    m_pTree=NULL;
 
    m_hLibData = NULL;
    m_hLibExec = NULL;
    m_hLibConstraint = NULL;
    m_hLibCommit = NULL;
}

BOOLEAN 
DMPlugin::IsSymbolsLoaded()
{
    return (m_pTree!=NULL) || (m_hLibData!=NULL) || 
      (m_hLibExec) || (m_hLibConstraint) || (m_hLibCommit);
}


BOOLEAN 
DMPlugin::IsLibReleased(XPL_DL_HANDLE_T handler)
{
    if ( handler ==  m_hLibData || handler == m_hLibExec ||
         handler ==  m_hLibConstraint || handler == m_hLibCommit )
       return TRUE;
    return FALSE;
}


void 
DMPlugin::LoadSymbol(CPCHAR szType, 
                     CPCHAR szName, 
                     XPL_DL_HANDLE_T * phLibHandle, 
                     void ** ppFunc)
{

    if (*phLibHandle == 0)
    {
        DMString strKey = szType;
        DMString value;
        if (m_mapParameters.lookup(strKey, value))
        {
            DMPluginManager & oPluginManager = dmTreeObj.GetPluginManager();
            *phLibHandle = oPluginManager.LoadLib(value.c_str());   
        }
        XPL_LOG_DM_PLG_Debug(("lookedup %s=%s\n", strKey.c_str(), value.c_str()));
   }

   if (*phLibHandle != 0)
   {
      *ppFunc = XPL_DL_GetFunction(*phLibHandle, szName);
   } 
   XPL_LOG_DM_PLG_Debug(("loadSymbol lib=0x%x, func=0x%x\n", *phLibHandle, *ppFunc));
}



SYNCML_DM_RET_STATUS_T 
DMPlugin::GetTree(CPCHAR szPath, 
                  PDmtAPIPluginTree & pTree)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    UpdateLastAccessedTime();

    if ( m_pTree!=NULL )
    {
      pTree=m_pTree;
      return dm_stat;
    }

    if ( pfGetTree==NULL)
      LoadSymbol( m_bOverlayPlugin ? PLUGIN_DATA_OVERLAY_NAME : PLUGIN_DATA_NAME,
        "DMT_PluginLib_Data_GetPluginTree",&m_hLibData,(void **) &pfGetTree);

    if (pfGetTree != NULL)
        dm_stat = pfGetTree(szPath,m_mapParameters,pTree);

    m_pTree = pTree;

    XPL_LOG_DM_PLG_Debug(("DMPlugin::GetTree(%s)=0x%x\n", szPath, (void *)pTree));
    return dm_stat;
}

SYNCML_DM_RET_STATUS_T 
DMPlugin::Execute(CPCHAR szPath, 
                  CPCHAR szArgs, 
                  CPCHAR szCorrelator, 
                  PDmtTree pTree, 
                  DMString & results)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    UpdateLastAccessedTime();
 
    if ( pfExecute2==NULL)
        LoadSymbol(PLUGIN_EXECUTE_NAME,"DMT_PluginLib_Execute2",&m_hLibExec,(void **)&pfExecute2);

    if (pfExecute2 != NULL)
    {
        dm_stat = pfExecute2(szPath,m_mapParameters,szArgs,szCorrelator,pTree,results);
        return dm_stat;
    } 

    return SYNCML_DM_FAIL;
}

SYNCML_DM_RET_STATUS_T 
DMPlugin::CheckConstraint(CPCHAR szPath, 
                          PDmtTree pTree) 
{
    UpdateLastAccessedTime();

    if ( pfCheckConstraint==NULL)
        LoadSymbol(PLUGIN_CONSTRAINT_NAME,"DMT_PluginLib_CheckConstraint",&m_hLibConstraint,(void **)&pfCheckConstraint);

    if ( pfCheckConstraint != NULL )
        return pfCheckConstraint(szPath,m_mapParameters,pTree);
    else
        return SYNCML_DM_FAIL;
}


void 
DMPlugin::OnCommit(const DmtEventMap &aUpdatedNodes, 
                   PDmtTree pTree)
{

    UpdateLastAccessedTime();

    if ( pfOnCommit==NULL)
        LoadSymbol(PLUGIN_COMMIT_NAME,"DMT_PluginLib_OnCommit",&m_hLibCommit,(void **)&pfOnCommit);

    if ( pfOnCommit != NULL )
        pfOnCommit(aUpdatedNodes,m_mapParameters,pTree);
}

void DMPlugin::UpdateLastAccessedTime()
{
    m_lastAccessedTime = XPL_CLK_GetClock();
}

XPL_CLK_CLOCK_T DMPlugin::GetLastAccessedTime()
{
    return m_lastAccessedTime;
}
