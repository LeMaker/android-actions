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

    Source Name: dmConstraints.cc

    General Description: Implementation of the DMConstraints class

==================================================================================================*/

#include <limits.h>
#include "dm_uri_utils.h"
#include "dmStringUtil.h"
#include "dmConstraints.h" 

DMConstraints::DMConstraints()
{
    Init();
}


DMConstraints::~DMConstraints()
{
}

void DMConstraints::Init()
{
    m_nMax = INT_MAX;
    m_nMin = INT_MIN;
    m_nMaxLen = SHRT_MAX;
    m_nMinLen = 0;
    m_nnMaxLen = 255;
    m_nMaxRecurrance = 0xffff;
    m_nMaxMultiNodes = 0;

    m_psValues = NULL;
    m_nNodeType = SYNCML_DM_FORMAT_NULL;
    m_psRegexp = NULL; 
    m_psAutoNodes = NULL; 
    m_psRecurAfterSegment = NULL; 
    m_psForeignKey = NULL; 
    m_psChild = NULL;    
    m_psDepend = NULL; 
  
    m_psnValues = NULL;
    m_psnRegexp = NULL;
}



SYNCML_DM_RET_STATUS_T 
DMConstraints::ReadDefaultValue(DMMetaDataBuffer * pBuffer, 
                                             INT32 nNodeType)
{
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;

    switch( nNodeType )
    {
          case SYNCML_DM_FORMAT_BOOL:
               m_nDef_Value = pBuffer->ReadUINT8();
               break;
               
           case SYNCML_DM_FORMAT_CHR:
               m_psDef_Value = pBuffer->ReadString();
               break;
               
           case SYNCML_DM_FORMAT_INT:
               m_nDef_Value = pBuffer->ReadUINT32();
               break;
               
           case SYNCML_DM_FORMAT_FLOAT:
               m_fDef_Value = pBuffer->ReadFLOAT();
               break;
               
           case SYNCML_DM_FORMAT_DATE:
               m_psDef_Value = pBuffer->ReadString();
               break;
               
           case SYNCML_DM_FORMAT_TIME:
               m_psDef_Value = pBuffer->ReadString();
               break;
    }
    return ret_status;

}


SYNCML_DM_RET_STATUS_T 
DMConstraints::Read(DMMetaDataBuffer * pBuffer, 
                        UINT8 count, 
                        INT32 nNodeType)
{
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;
    UINT8 type; 

    Init();
    for (int i=0; i<count; i++)
    {
        type = pBuffer->ReadUINT8();
        switch ( type )
        {
            case SYNCML_DM_DDF_MIN:
                m_nMin = pBuffer->ReadUINT32();
                break;

            case SYNCML_DM_DDF_MAX:
                m_nMax = pBuffer->ReadUINT32();
                break;
                
            case SYNCML_DM_DDF_VALUES:
                m_psValues = pBuffer->ReadString();
                break;
                
            case SYNCML_DM_DDF_DEFAULTVALUE:
                ret_status = ReadDefaultValue(pBuffer,nNodeType);
                m_nNodeType = nNodeType;
                break;

            case SYNCML_DM_DDF_MAXLEN:
                m_nMaxLen = pBuffer->ReadUINT16();
                break;
          
            case SYNCML_DM_DDF_MINLEN:
                m_nMinLen = pBuffer->ReadUINT16();
                break;
          
            case SYNCML_DM_DDF_REGEXP:
                m_psRegexp = pBuffer->ReadString();
                break;
              
            case SYNCML_DM_DDF_NMAXLEN:
                m_nnMaxLen = pBuffer->ReadUINT16();
                break;

            case SYNCML_DM_DDF_NVALUES:
                m_psnValues = pBuffer->ReadString();
                break;

            case SYNCML_DM_DDF_NREGEXP:
                m_psnRegexp = pBuffer->ReadString();
                break;

            case SYNCML_DM_DDF_AUTONODE:
                m_psAutoNodes = pBuffer->ReadString();
                break;

            case SYNCML_DM_DDF_RECUR_AFTER_SEGMENT:
                m_psRecurAfterSegment = pBuffer->ReadString();
                break;

            case SYNCML_DM_DDF_MAX_RECURRANCE:
                m_nMaxRecurrance = pBuffer->ReadUINT16();
                break;

            case SYNCML_DM_DDF_FK:
                m_psForeignKey = pBuffer->ReadString();
                break;

            case SYNCML_DM_DDF_CHILD:
                m_psChild = pBuffer->ReadString();
                break;

            case SYNCML_DM_DDF_DEPEND:
                m_psDepend = pBuffer->ReadString();
                break;

            case SYNCML_DM_DDF_MAX_MULTINODES:
                m_nMaxMultiNodes = pBuffer->ReadUINT16();
                break;

        }

        
   }
    return ret_status;

}

void DMConstraints::GetDefaultString( DMString& strValue )const
{
  char s[MAX_FLOAT_STRING_LENGTH] = "";

  switch ( m_nNodeType )
  {
    case SYNCML_DM_FORMAT_INT:
      DmSprintf(s, "%d", m_nDef_Value );
      break;

    case SYNCML_DM_FORMAT_BOOL:
      strValue = m_nDef_Value ? "true" : "false";
      return;
    
    case SYNCML_DM_FORMAT_CHR:
      strValue = m_psDef_Value;
      return;

    case SYNCML_DM_FORMAT_FLOAT:
      DmSprintf(s, "%+e", m_fDef_Value );
      break;

    case SYNCML_DM_FORMAT_DATE:
      strValue = m_psDef_Value;
      return;

    case SYNCML_DM_FORMAT_TIME:
      strValue = m_psDef_Value;
      return;

  }
  
  strValue = s;
}
