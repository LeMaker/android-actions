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

#ifndef DMCONFIGMANAGER_H
#define DMCONFIGMANAGER_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

Header Name: dmConfigManager.h

General Description: Declaration of DMConfigManager, the base class for ACL and Event managers.

==================================================================================================*/

#include "dmACLItem.h"

/**
 * DMConfigManager is the base class for ACL and Event subscription records management.
 */
class DMConfigManager {
public:

  /**
  * Default constructor
  */
  DMConfigManager();

  /**
  * Default destructor
  */
  virtual ~DMConfigManager();

  /**
  * Initializes a manager  
  * \param env [in] - pointer on env object
  * \param pTree[in] - pointer on DM tree object
  * \param fileType [in] - specifies type of config file object will handle
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T Init(CEnv* env, 
                              DMTree* pTree , 
                              SYNCML_DM_FILE_TYPE_T fileType);

  /**
  * Deinitializes manager 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T DeInit();

  /**
  * Loads configuartion records into memory
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T Deserialize();

  /**
  * Saves configurations records 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T Serialize();

  /**
  * Rollbacks all changes in the config records
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T Revert();

  /**
  * Deletes configuration record
  * \param env [in] - configuration path to be deleted
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T Delete(CPCHAR szPath);
  
  
protected:
  /* pointer on env object */
  CEnv* m_pEnv;
  /* pointer on DMTree object */
  DMTree* m_pTree;
  /* file name */
  DMString m_strFileName;
  /* flag to specify if config records are modified */
  BOOLEAN m_bChanged;
  /* flag to specify if config records are loaded */
  BOOLEAN m_bLoaded;
  /* time stamp of last deserialization */
  XPL_CLK_CLOCK_T m_nLastLoadTimeStamp;
  /* type of config file (acl or event subscriptions */
  SYNCML_DM_FILE_TYPE_T m_efileType;
  /* storage for confif records */
  DMVector<PDMConfigItem> m_aConfig;

  /**
  * Allocates new configuration item
  * \return pointer on new config item 
  */ 
  virtual DMConfigItem * AllocateConfigItem() = 0;

  /**
  * Returns name of configuration file
  * \param strFileName [out] - file name
  */ 
  virtual void GetFileName(DMString & strFileName) = 0;   

  /**
  * Ensures that config file is locked
  */ 
  void CheckLocking();  

   /**
  * Finds config item by path
  * \param szPath [in] - configuration path to be found
  * \param oConfigItem [out] - smart pointer on found config item
  * \return index of config item in the internal storage if found, -1 otherwise
  */ 
  INT32 Find(CPCHAR szPath, 
             PDMConfigItem & oConfigItem) const;

  /**
  * Finds config item by path
  * \param szPath [in] - configuration path to be found
  * \return index of config item in the internal storage if found, -1 otherwise
  */ 
  INT32 Find(CPCHAR szPath) const;

  /**
  * Gets config item by path
  * \param szPath [in] - configuration path 
  * \param oConfigItem [out] - smart pointer on found config item
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T Get(CPCHAR szPath, 
                             PDMConfigItem & pItem) const;


private:

   /**
  * Cleans internal memory
  */ 
  void ClearMemory();

  /**
  * Adds new config item into storage
  * \param szPath [in] - configuration path 
  * \param szConfig [in] - encoded config record
  * \param aDict [in] - dictionary of principals
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T Add(const DMString & szPath,
                             CPCHAR szConfig,
                             const DMMap<INT32, DMString>& aDict);

  /**
  * Extracts path from a config file line (line is encoded as "[path]")
  * \param line [in] - config file line 
  * \param strPath [in] - extracted path
  * \return TRUE if peration is completed successfully
  */ 
  static BOOLEAN GetPath(char* line, 
                               DMString& strPath );

  /**
  * Reads new line from a config file
  * \param dmf [in] - file handler object
  * \param line [out] - config file line 
  * \return TRUE if peration is completed successfully
  */ 
  static BOOLEAN GetNextLine(DMFileHandler& dmf, 
                             char *line);

   /**
  * Performs recovery if needed
  */ 
  void CheckRecovery() const;

  /**
  * Adds principals stored in the config items into dictionary before serialization of config file 
  * \param aDict [out] - dictionary of principals
  */  
  void UpdateDictionary(DMMap<DMString, INT32>& aDict);

  /**
  * Serializes dictionary into a config file 
  * \param dmf [in] - file handler object
  * \param aDict [out] - dictionary of principals
  */  
  static SYNCML_DM_RET_STATUS_T SerializeDictionary(DMFileHandler& dmf,
                                                                   const DMMap<DMString, INT32>& aDict);


  /**
  * Deserializes dictionary from a config file 
  * \param dmf [in] - file handler object
  * \param aDict [out] - dictionary of principals
  */  
  static SYNCML_DM_RET_STATUS_T DeserializeDictionary(DMFileHandler& dmf,
                                                                 DMMap<INT32, DMString>& aDict);
  
};

#endif 
