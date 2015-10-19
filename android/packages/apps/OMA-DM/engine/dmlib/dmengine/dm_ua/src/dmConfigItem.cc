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

    Source Name: dmConfigItem.cc

    General Description: Implementation of the DMConfigItem class

==================================================================================================*/

#include "dmConfigItem.h"
#include "dm_uri_utils.h"
#include "dmdefs.h"
#include "dmtoken.h"

DMConfigItem::DMConfigItem(CPCHAR szPath)
{
    m_strPath = szPath;
}

void DMConfigItem::AttachProperty( DMString & strConfig, char cDelim, CPCHAR szProperty )
{
  if ( !strConfig[0] == 0 )
  {
    char s[2] = {cDelim,0};
    strConfig += s;
  }
    
  strConfig += szProperty;
}


SYNCML_DM_RET_STATUS_T DMConfigItem::Serialize( DMFileHandler& dmf)
{
     if ( dmf.write( "[", 1 ) != SYNCML_DM_SUCCESS ||
          dmf.write( m_strPath.c_str(), m_strPath.length())!= SYNCML_DM_SUCCESS ||
          dmf.write( "]\n", 2 )  != SYNCML_DM_SUCCESS )
         return SYNCML_DM_IO_FAILURE;
     return SYNCML_DM_SUCCESS;

}

void DMConfigItem::CreateProperty( const DMVector<DmtPrincipal> aPrincipals, 
                                             CPCHAR szKey, 
                                             const DMMap<DMString, INT32>& aDict,
                                             DMString & strProperty )
{

    if ( aPrincipals.size() == 0 )
        return; 

    DMString strPrincipals = "";
    for (INT32 index=0; index<aPrincipals.size(); index++)
    {
        const DmtPrincipal& oPrincipal = aPrincipals[index];
        INT32 nID = aDict.get(oPrincipal.getName());
        char szNumBuffer[20] = ""; 
        sprintf( szNumBuffer, "%d", nID );

        AttachProperty( strPrincipals, '+', szNumBuffer );
    }

    strProperty = szKey;
    strProperty += strPrincipals;

}


SYNCML_DM_RET_STATUS_T 
DMConfigItem::Set(CPCHAR szPath)
{
    if ( szPath == NULL )
        return SYNCML_DM_FAIL;

    m_strPath = szPath;
    return SYNCML_DM_SUCCESS;

}
