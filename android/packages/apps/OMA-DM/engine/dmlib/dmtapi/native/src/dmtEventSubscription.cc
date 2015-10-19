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

    Source Name: dmtEvent.cc

    General Description: Implementation of DmtEvent class.

==================================================================================================*/

#include "dmtEvent.hpp" 

SYNCML_DM_RET_STATUS_T 
DmtEventSubscription::Set(SYNCML_DM_EVENT_ACTION_T eAction,
                       SYNCML_DM_EVENT_TYPE_T nType)
{
    m_eAction = eAction;
    m_nType = nType;
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T 
DmtEventSubscription::Set(SYNCML_DM_EVENT_ACTION_T eAction,
                        SYNCML_DM_EVENT_TYPE_T nType,
                        CPCHAR szTopic,
                        const DMVector<DmtPrincipal> & aIgnorePrincipals,
                        const DMVector<DmtPrincipal> & aNotifyPrincipals)
{
    SYNCML_DM_RET_STATUS_T dm_stat;

    Set(eAction,nType);
    dm_stat = SetTopic(szTopic);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
    m_aIgnorePrincipals = aIgnorePrincipals;
    m_aNotifyPrincipals = aNotifyPrincipals;
    return SYNCML_DM_SUCCESS;

}

SYNCML_DM_RET_STATUS_T 
DmtEventSubscription::SetTopic(CPCHAR szTopic)
{
    m_strTopic = szTopic;
    return SYNCML_DM_SUCCESS;
}
  
SYNCML_DM_RET_STATUS_T 
DmtEventSubscription::AddPrincipal(const DmtPrincipal & oPrincipal, BOOLEAN bIsIgnore)
{
    if ( bIsIgnore )
        m_aIgnorePrincipals.push_back(oPrincipal);
    else
        m_aNotifyPrincipals.push_back(oPrincipal);
    return SYNCML_DM_SUCCESS;
}

  
const DMVector<DmtPrincipal> &
DmtEventSubscription::GetPrincipals(BOOLEAN bIsIgnore) const
{
    if ( bIsIgnore )
        return m_aIgnorePrincipals;
    else 
        return m_aNotifyPrincipals;
}
