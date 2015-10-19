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

#ifndef __DMTCOMMITPLUGIN_H__
#define __DMTCOMMITPLUGIN_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/**  
 \file dmtCommitPlugin.hpp
 \brief  The dmtCommitPlugin.hpp header file contains constants and datatypes 
           for the device managment Commit plugin.\n    
           <b>Warning:</b>  All functions, structures, and classes from this header file are for internal usage only!!!
*/

#include "dmtEventData.hpp"  
#include "dmtPlugin.hpp"

/** 
* Enumeration for Event types, used for backward compatibility
*/
enum { 
/** Event "None" */
    DMT_EVENT_NONE = -1,
/** Event "Add" */
    DMT_EVENT_ADD = 0, 
/** Event "Delete" */
    DMT_EVENT_DELETE = 1,
/** Event "Replace" */
    DMT_EVENT_REPLACE = 2,
/** Event "Indirect Update" */
    DMT_EVENT_INDIRECT_UPDATE = 3
};
/** Definition for DMT_EVENT_T as INT32*/
typedef INT32 DMT_EVENT_T;

/** Structure Subscription Data, contains info about tree changes, used for backward compatibility */
struct DM_SUBSCRIPTION_DATA_T
{
/**
* Default constructor
*/
  DM_SUBSCRIPTION_DATA_T(){m_eAction = DMT_EVENT_NONE;}
/**
* Constructor
* \param strKey [in] - child node name as a string
* \param eAction [in] - child node operation as a DMT_EVENT_T
*/
  DM_SUBSCRIPTION_DATA_T( const DMString& strKey, DMT_EVENT_T eAction ){m_strKey = strKey; m_eAction = eAction;}

 /** child node name */
  DMString  m_strKey;
 
 /** child node operation */
  DMT_EVENT_T m_eAction;
};


/** DM Subscription Vector, inheritance from  DMVector, used for backward compatibility 
* \par Category: General
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
class DMSubscriptionVector : public DMVector<DM_SUBSCRIPTION_DATA_T>
{
};


#endif //__DMTCOMMITPLUGIN_H__
