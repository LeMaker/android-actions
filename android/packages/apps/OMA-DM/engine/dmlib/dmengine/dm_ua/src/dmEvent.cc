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

    Source Name: dmEvent.cc

    General Description: Implementation of the DMEventPath and DMEventData classes

==================================================================================================*/

#include "dmEvent.h"

DMEventPath::DMEventPath(CPCHAR szPath)
{
     m_strPath = szPath;
}


SYNCML_DM_RET_STATUS_T DMEventPath::Serialize(DMBufferWriter & oBuffer)
{
    return oBuffer.WriteString(m_strPath);  
}

UINT32 DMEventPath::GetSize()
{
    return m_strPath.length()+1; 

}

SYNCML_DM_RET_STATUS_T 
DMEventPath::Deserialize(DMBufferReader & oBuffer)
{
    m_strPath = oBuffer.ReadString();
    return SYNCML_DM_SUCCESS;
}



DMEventData::DMEventData(BOOLEAN bIsLeaf,
                      SYNCML_DM_EVENT_ACTION_T eAction,
                      BOOLEAN bIsEnabledByParent,
                      CPCHAR strName,
                      CPCHAR strNewName)
{
    m_bIsLeaf = bIsLeaf;
    m_eAction = eAction;
    m_strName = strName;
    m_strNewName = strNewName;
    m_bIsEnabledByParent = bIsEnabledByParent;

}  

void 
DMEventData::AddAction(SYNCML_DM_EVENT_ACTION_T eAction)  
{ 
    m_eAction |= eAction;
}    


void 
DMEventData::RemoveAction(SYNCML_DM_EVENT_ACTION_T eAction)  
{ 
    m_eAction &= ~eAction;
}    



void 
DMEventData::SetLeaf(BOOLEAN bIsLeaf) 
{ 
    m_bIsLeaf = bIsLeaf; 
}


void
DMEventData::SetEnabledByParent(BOOLEAN bIsEnabledByParent)
{
   m_bIsEnabledByParent = bIsEnabledByParent;
}

void 
DMEventData::SetAction(SYNCML_DM_EVENT_ACTION_T eAction) 
{ 
    m_eAction = eAction; 
}



SYNCML_DM_RET_STATUS_T 
DMEventData::SetName(CPCHAR strName) 
{ 
      m_strName = strName;
      return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T 
DMEventData::SetNewName(CPCHAR strNewName) 
{ 
      m_strNewName = strNewName;
      return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T
DMEventData::Serialize(DMBufferWriter & oBuffer)
{
   
    oBuffer.WriteUINT8(m_eAction);
    oBuffer.WriteUINT8(m_bIsLeaf);

    oBuffer.WriteString(m_strName);

    if ( (m_eAction & SYNCML_DM_EVENT_RENAME) == SYNCML_DM_EVENT_RENAME )
    {
        oBuffer.WriteString(m_strNewName);
    }
    return SYNCML_DM_SUCCESS;

}

UINT32 DMEventData::GetSize()
{
    UINT32 size = sizeof(UINT8)*2;
    size +=  m_strName.length() + 1;
    if ( (m_eAction & SYNCML_DM_EVENT_RENAME) ==  SYNCML_DM_EVENT_RENAME)
    {
        size +=  m_strNewName.length() + 1;
    }    
    return size;

}

SYNCML_DM_RET_STATUS_T 
DMEventData::Deserialize(DMBufferReader & oBuffer)
{
    m_eAction = oBuffer.ReadUINT8();
    m_bIsLeaf = (BOOLEAN)oBuffer.ReadUINT8();
    m_strName = oBuffer.ReadString();
    if ( (m_eAction & SYNCML_DM_EVENT_RENAME) ==  SYNCML_DM_EVENT_RENAME)
    {
        m_strNewName = oBuffer.ReadString();
    }
    return SYNCML_DM_SUCCESS;
}
