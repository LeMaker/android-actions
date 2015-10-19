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

#ifndef DMCONFIGITEM_H
#define DMCONFIGITEM_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*=================================================================================================

Header Name: dmConfigItem.h

General Description: This file contains declaration of the DMConfigItem class

==================================================================================================*/

#include "dmvector.h"
#include "dmstring.h"
#include "jem_defs.hpp"
#include "dmtPrincipal.hpp"
#include "SyncML_DM_FileHandle.H"

/**
 * DMConfigItem is the base class for configuration records such as ACL or Event subscriptions.
 */
class DMConfigItem : public JemBaseObject {
public:

 /**
 * Default constructor 
 */
  DMConfigItem() {};

  /**
  * Constructor, which sets path of configuration item 
  * \param szPath [in] - configuration path (URI of a node)
  */
  DMConfigItem(CPCHAR szPath); 

 /**
  * Saves configuration record back to a file 
  * \param dmf [in] - file handler object
  * \param aDict [in] - dictionary of principals
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
 virtual SYNCML_DM_RET_STATUS_T Serialize(DMFileHandler& dmf,
                                         const DMMap<DMString, INT32>& aDict ) = 0 ;  

 /**
  * Parses encoded configuration data and assign values of object members 
  * \param szPath [in] - configuration path (URI of a node)
  * \param szConfig[in] - encoded configuration record
  * \param aDict [in] - dictionary of principals
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  virtual SYNCML_DM_RET_STATUS_T Set(CPCHAR szPath,
                                CPCHAR szConfig, 
                                const DMMap<INT32, DMString>& aDict) = 0;


 /**
  * Adds principals stored in the configuration item into dictionary before serialization of config file 
  * \param aDict [out] - dictionary of principals
  */
  virtual void UpdateDictionary(DMMap<DMString, INT32>& aDict ) = 0;


 /**
  * Attaches property into encoded configuration record before serialization of config item  
  * \param strConfig [out] - encoded configuration data
  * \param cDelim[in] - delimeter to use for particular property
  * \param szProperty [in] - property to attach
  */
  static void AttachProperty(DMString & strConfig, 
                          char cDelim, 
                          CPCHAR szProperty );

 /**
  * Creates "key=value" config property  (example "G=1+2")
  * \param aPrincipals [in] - vector of principals to add into value
  * \param szKey[in] - key  ( example "G=" ) 
  * \param aDict [in] - dictionary of principals
  * \param szProperty [out] - created config property
  */
  static void CreateProperty(const DMVector<DmtPrincipal> aPrincipals, 
                            CPCHAR szKey, 
                            const DMMap<DMString, INT32>& aDict,
                            DMString & strProperty);
 
/**
  * Returns configuration path
  */
  const DMString & GetPath() const { return m_strPath; }


protected:
  /* Configuration path*/
  DMString m_strPath;

/**
  * Saves configuration path back to a file 
  * \param dmf [in] - file handler object
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  virtual SYNCML_DM_RET_STATUS_T Serialize( DMFileHandler& dmf ); 

/**
  * Sets value of configuration path 
  * \param szPath [in] - configuration path (URI of a node)
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  virtual SYNCML_DM_RET_STATUS_T Set(CPCHAR szPath);
  
};

typedef JemSmartPtr<DMConfigItem> PDMConfigItem;

#endif 
