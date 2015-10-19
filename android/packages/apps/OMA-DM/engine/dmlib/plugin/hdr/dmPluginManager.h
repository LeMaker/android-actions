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

#ifndef DMPLUGINMANAGER_H
#define DMPLUGINMANAGER_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

Header Name: dmPluginManager.h

General Description: This file contains the declaration of DMPluginManager.

==================================================================================================*/

#include "syncml_dm_data_types.h"   
#include "dmt.hpp"
#include "dmtPlugin.hpp"  
#include "dmPlugin.h"  
#include "dm_tree_typedef.h"
#include "xpl_Lib.h"

typedef DMVector<PDMPlugin> DMPluginVector;

class CEnv;
class DMTree;

/**
 * Plugin Manager represents a manager to handle plug-ins registration
 */
class DMPluginManager 
{
public:

  /**
  * Default constructor
  */
   DMPluginManager();

   /**
   * Destructor
   */
   ~DMPluginManager();


  /**
  * Initializes plug-in manager  
  * \param env [in] - pointer on env object
  * \param pTree [in] - pointer on DM tree object
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T Init( CEnv* env, DMTree* tree );


  /**
  * Deinitializes plug-in manager  
  */ 
  void DeInit();


  /**
  * Loads plug-in shared library 
  * \param libName [in] - name of shared library
  * \return Return Type (XPL_DL_HANDLE_T) 
  * - handler on shared library if  operation is completed successfully,
  * - XPL_DL_HANDLE_INVALID otherwise 
  */ 
  XPL_DL_HANDLE_T LoadLib(CPCHAR libName); 


  /**
  * Finds suitable plug-in by specified path (exact match)
  * \param type [in] - plug-in type
  * \param szPath [in] - plug-in path
  * \return Return Type (PDMPlugin) 
  * - smart pointer on plug-in object if operation is completed successfully,
  * - NULL otherwise 
  */
  PDMPlugin FindPlugin(SYNCML_DM_PLUGIN_TYPE_T type, CPCHAR szPath);


  /**
  * Finds suitable commit plug-in by specified path (exact match)
  * \param path [in] - plug-in path
  * \return Return Type (PDMPlugin) 
  * - smart pointer on plug-in object if operation is completed successfully,
  * - NULL otherwise 
  */ 
  PDMPlugin FindCommitPlugin(CPCHAR szPath);

    
  /**
  * Retrieves all plug-in of specified path
  * \param type [in] - plug-in path
  * \return Return Type (DMPluginVector *) 
  * - pointer on plug-in vector if operation is completed successfully,
  * - NULL otherwise 
  */
  DMPluginVector * GetPlugins(SYNCML_DM_PLUGIN_TYPE_T type);


  /**
  * Verifies if plug-in path is not blocked 
  * \param type [in] - plug-in type
  * \param szPath [in] - plug-in path
  * \return TRUE if path is enabled 
  */
  BOOLEAN IsMountPointEnabled(CPCHAR szPath) const; 


   /**
  * Mounts plug-in nodes  
  * \param szRootPath [in] - root path of plug-ins to mount
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T UpdatePluginNodes(CPCHAR szRootPath);

  
     
   /**
  * Retrieves list of plug-ins registered on sub tree
  * \param szPath [in] - root path of sub tree
  * \param type [in] - plug-in type
  * \param aPlugins [out] - list of requested plug-ins 
  */
   void GetPlugins(CPCHAR szPath,
                   SYNCML_DM_PLUGIN_TYPE_T type,
                   DMPluginVector& aPlugins );

#ifndef DM_STATIC_PLUGINS    
   /**
  * Loads plug-in configuration file  
  * \param fileName [in] - name of a file
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
   SYNCML_DM_RET_STATUS_T LoadPluginFile(CPCHAR fileName);

#endif
  /**
  * Checks plug-in shared objects aging timeout  
  * \param nAgingTime [in] - aging timeout
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
   SYNCML_DM_RET_STATUS_T CheckPluginAging(INT32 nAgingTime);

private:
   /* Pointer on DM tree */
   DMTree   *m_pTree;

#ifndef DM_STATIC_PLUGINS
   /* Collection of plug-in config files */
   DMMap<DMString, DMPluginVector> m_mapFilePluginVectors;  
#endif

   /* Data plug-ins vector */
   DMPluginVector m_dataPlugins;
   /* Constraint plug-ins vector */
   DMPluginVector m_constPlugins;
   /* Executable plug-ins vector */
   DMPluginVector m_execPlugins;
   /* Commit plug-ins vector */
   DMPluginVector m_commPlugins;
   /* Blocked plug-ins vector */
   DMStringVector m_oBlockedPlugins;

   /* Map of loaded shared libraries */
   DMMap<DMString,XPL_DL_HANDLE_T> m_pluginLibs;

   /* Index array */ 
   static const SYNCML_DM_PLUGIN_TYPE_T m_nIndex[MAX_PLUGINTYPES];


   /**
  * Checks plug-in type from config file  
  * \param aMap [in/out] - map of plug-in parameters
  * \param filePath [in] - path to a config file
  * \param pluginName [in] - mask to verify type of a plug-in
  * \param type [in] - plug-in type to verify against
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */    
   SYNCML_DM_PLUGIN_TYPE_T CheckPlugin(DMStringMap & aMap,
                                       CPCHAR filePath,
                                       CPCHAR pluginName,
                                       SYNCML_DM_PLUGIN_TYPE_T type);

   /**
  * Creates plug-in object and adds it into internal storage according to config record  
  * \param fileName [in] - name of a config file
  * \param strPath [in] - plug-in path
  * \param aMap [in] - map of plug-in parameters from a config file
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */    
   SYNCML_DM_RET_STATUS_T AddPlugin(CPCHAR fileName, 
                                    DMString &  strPath, 
                                    DMStringMap & aMap);

  /**
  * Creates DM plug-in proxy node and mounts it to DM tree  
  * \param pPlugin [in] - smart pointer on data plug-in
  * \param oNodeProps [in] - property of a plug-in proxy node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */    
  SYNCML_DM_RET_STATUS_T MountNode(PDMPlugin & pPlugin,
                                   DMAddNodeProp & oNodeProps);


  /**
  * Matches node path and plun-in path  (due to multinode support)
  * \param szNodePath [in] - DM node path
  * \param szPluginPath [in] -  plug-in path
  * \return TRUE if paths are matched 
  */    
  static BOOLEAN PathMatch(CPCHAR szNodePath,  
                           CPCHAR szPluginPath);

  /**
  * Parses line from a config file 
  * \param fileName [in] - name of a config file
  * \param szLine [in] - line to parse
  * \param strSectionName [in] - name of currect block (plug-in path)
  * \param aMap [in] - map of plug-in parameters from a config file
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */    
  SYNCML_DM_RET_STATUS_T ParseLine(CPCHAR szFileName, 
                                   CPCHAR szLine, 
                                   DMString &strSectionName, 
                                   DMStringMap &aMap);


#ifndef DM_STATIC_PLUGINS
  /**
  * Searches all config file in the specified folder 
  * \param szPath [in] - name of directory to search
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */    
  SYNCML_DM_RET_STATUS_T LoadDirectory(CPCHAR szPath);
#endif

#ifdef DM_STATIC_FILES

  /**
  * Loads static config file 
  * \param pBuffer [in] - static buffer
  * \param size [in] - size of a buffer
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */    
  SYNCML_DM_RET_STATUS_T LoadPluginFile(UINT8 *pBuffer, 
                                        UINT32 size);
#endif
};

#endif 
