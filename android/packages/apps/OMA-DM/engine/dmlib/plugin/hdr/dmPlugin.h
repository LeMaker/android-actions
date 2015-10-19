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

#ifndef DMPLUGIN_H
#define DMPLUGIN_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

Header Name: dmPlugin.h

General Description: This file contains the declaration of DMPlugin.

==================================================================================================*/

#include "dmt.hpp"
#include "xpl_dm_Manager.h"
#include "dmtPlugin.hpp" 
#include "dmtEventData.hpp"
#include "xpl_Lib.h"
#include "xpl_Time.h"
#include "dmEventSubscription.h"

#define PLUGIN_DATA_NAME             "_data"
#define PLUGIN_DATA_OVERLAY_NAME   "_dataExt"
#define PLUGIN_EXECUTE_NAME        "_exec"
#define PLUGIN_CONSTRAINT_NAME     "_const"
#define PLUGIN_COMMIT_NAME     "_commit"

enum
{
  SYNCML_DM_UNKNOWN_PLUGIN      = 0,  
  SYNCML_DM_DATA_PLUGIN         = 1,  //Can be ORed
  SYNCML_DM_EXECUTE_PLUGIN      = 2,
  SYNCML_DM_CONSTRAINT_PLUGIN   = 4,
  SYNCML_DM_COMMIT_PLUGIN = 8
}; 
typedef UINT8 SYNCML_DM_PLUGIN_TYPE_T;

#define  MAX_PLUGINTYPES 4


/**
 * Represents a basic class for all plug-ins managed by Plugin Manager.
 */
class DMPlugin: public DMEventSubscription, public JemBaseObject
{
  public:
   /**
  * Constructor that initializes plug-in object  
  * \param type [in] - type of a plug-in
  * \param bOverlayPlugin [in] - specifies if data plug-in as an overlay one
  * \param path [in] - plug-in root path (mounting point)
  * \param aParameters [in] - plug-in parameters from config file 
  */ 
   DMPlugin(SYNCML_DM_PLUGIN_TYPE_T type,
            BOOLEAN  bOverlayPlugin,
            const DMString & path, 
            DMStringMap & aParameters);

  /**
  * Destructor  
  */ 
  virtual ~DMPlugin();

  /**
  * Retrieves a plug-in type  
  * \return Return Type (SYNCML_DM_PLUGIN_TYPE_T) 
  */  
  SYNCML_DM_PLUGIN_TYPE_T GetPluginType() const  { return m_type; }

  /**
  * Verifies if plugin is an overlay plug-in 
  * \return TRUE if it is an overlay plug-in 
  */ 
  BOOLEAN  IsOveralyPlugin() const  {  return m_bOverlayPlugin; }
    
  /**
  * Retrieves a plug-in root path  
  * \return Return Type (DMString &) 
  */
  const DMString & GetPath() const  {  return m_strPath; }

  /**
  * Retrieves plug-in parameters  
  * \return Return Type (DMStringMap &) 
  */
  const DMStringMap & GetParameters() const  {  return m_mapParameters; }

  /**
  * Retrieves a plug-in tree for data plug-in  
  * \param szPath [in] - root path of plug-in tree
  * \param pTree [out] - plug-in tree created in the plug-in
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T GetTree(CPCHAR szPath, 
                                 PDmtAPIPluginTree & pTree);

   /**
  * Verifies if plug-in tree is empty  
  * \return TRUE if is empty 
  */ 
  BOOLEAN IsTreeEmpty()  {  return ((m_pTree==NULL) ?TRUE:FALSE); }

  /**
  * Performs "Execute" command on a plug-in node  
  * \param szPath [in] - node path
  * \param szArgs [in] - exec plug-in arguments 
  * \param szCorrelator [in] - correlator  
  * \param pTree [in] - current DM session tree
  * \param results [out] - result of execute plug-in
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
   SYNCML_DM_RET_STATUS_T Execute(CPCHAR szPath, 
                                  CPCHAR szArgs, 
                                  CPCHAR szCorrelator, 
                                  PDmtTree pTree, 
                                  DMString& results);
        

  /**
  * Checks constraints on sub tree  
  * \param szPath [in] - root path of a sub tree
  * \param pTree [in] - current DM session tree
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T CheckConstraint(CPCHAR szPath,  
                                         PDmtTree pTree);

   /**
  * Notifies about changes in the DM tree  
  * \param szPath [in] - root path of a sub tree
  * \param pTree [in] - current DM session tree
  */
  void OnCommit(const DmtEventMap &aUpdatedNodes, 
                PDmtTree pTree);

 
  /**
  * Releases data plug-in tree and resets values of an object  
  */
  void UnloadSymbols();

  /**
  * Verifies if any shared libraries are loaded
  * \return TRUE if shared libs are loaded   
    */
  BOOLEAN IsSymbolsLoaded();

   /**
  * Verifies if handler on specified shared library is realeased  
  * \param handler [in] - shared library handler
  * \return TRUE if released 
  */
  BOOLEAN IsLibReleased(XPL_DL_HANDLE_T handler);

  /**
  * Retrieves plug-in last accessed time  
  * \return Return Type (XPL_CLK_CLOCK_T) 
  */
  XPL_CLK_CLOCK_T GetLastAccessedTime();

private:
  /* Plug-in root path */  
  DMString m_strPath;      
  /* Parameters from config file */
  DMStringMap m_mapParameters;  

  /* Dynamic library handler for data plug-in */
  XPL_DL_HANDLE_T m_hLibData;
  /* Dynamic library handler for exec plug-in */
  XPL_DL_HANDLE_T m_hLibExec;
  /* Dynamic library handler for constraint plug-in */
  XPL_DL_HANDLE_T m_hLibConstraint;
  /* Dynamic library handler for commit plug-in */
  XPL_DL_HANDLE_T m_hLibCommit;

  /* Plug-in tree */
  PDmtAPIPluginTree m_pTree;

  /* Pointer on exported function that retrieves plug-in tree  */ 
  SYNCML_DM_RET_STATUS_T (* pfGetTree) (CPCHAR szPath, 
                                        DMStringMap & mapParameters,    
                                        PDmtAPIPluginTree & pPluginTree);

  /* Pointer on exported function that performs "Execute" command on a plug-in node  */
 SYNCML_DM_RET_STATUS_T (* pfExecute2) (CPCHAR szPath, 
                                        DMStringMap & mapParameters,
                                        CPCHAR szArgs, 
                                        CPCHAR szCorrelator,
                                        PDmtTree pTree, 
                                        DMString & results);

 /* Pointer on exported function that checks constraints  */
 SYNCML_DM_RET_STATUS_T (* pfCheckConstraint) (CPCHAR szPath, 
                                               DMStringMap & mapParameters,
                                               PDmtTree pTree);

  /* Pointer on exported function that inform about DMT updates  */
 void (*pfOnCommit) (const DmtEventMap &aUpdatedNodes,
                     DMStringMap& mapParameters,
                     PDmtTree pTree);

 /* Last accessed time */
 XPL_CLK_CLOCK_T m_lastAccessedTime; 
 /* Plug-in type */
 SYNCML_DM_PLUGIN_TYPE_T m_type;  
 /* Specifies if plug-in is an overlay */
 BOOLEAN    m_bOverlayPlugin;


 /**
 * Loads dynamic library and retrieves pointer on plug-in fuction  
 * \param szType [in] - suffix for the type of plug-in
 * \param szName [in] - name of exported function
 * \param phLibHandle [out] - pointer on dynamic library handler
 * \param ppFunc [out] - pointer on function exported from dynamic library
 */
 void LoadSymbol(CPCHAR szType,
                 CPCHAR szName,
                 XPL_DL_HANDLE_T * phLibHandle,
                 void ** ppFunc);

 /**
  * Updates the last accessed time
  */
 void UpdateLastAccessedTime(); 
};

typedef JemSmartPtr<DMPlugin> PDMPlugin;

#endif //End of file
