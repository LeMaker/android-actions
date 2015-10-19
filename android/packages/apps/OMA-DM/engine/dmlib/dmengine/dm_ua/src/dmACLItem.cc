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

    Source Name: dmACLItem.cc

    General Description: Implementation of the dmACLItem class

==================================================================================================*/

#include "dm_uri_utils.h"
#include "dm_tree_class.H"
#include "xpl_Logger.h"
#include "dmACLItem.h"

DMAclItem::DMAclItem(CPCHAR szPath, CPCHAR szAcl ) : DmtAcl(szAcl), DMConfigItem(szPath) 
{
}

SYNCML_DM_RET_STATUS_T 
DMAclItem::ParsePermission(CPCHAR szKeyWord, SYNCML_DM_ACL_PERMISSIONS_T * pPermission)
{
     if ( pPermission == NULL )
        return SYNCML_DM_FAIL;

     if ( DmStrcmp(szKeyWord,"A") == 0 )
         *pPermission = ADD;
     else if ( DmStrcmp(szKeyWord,"D") ==0 )
         *pPermission = DELETE;
     else if ( DmStrcmp(szKeyWord,"E") == 0 )
         *pPermission = EXEC;
     else if ( DmStrcmp(szKeyWord,"G") == 0 )
          *pPermission = GET;
     else if ( DmStrcmp(szKeyWord,"R") == 0 )
          *pPermission = REPLACE;
     else 
     {
        XPL_LOG_DM_TMN_Error(("ACL file format error - unknown command %s\n \n", szKeyWord));
         return SYNCML_DM_FAIL;
     }
     return SYNCML_DM_SUCCESS; 

}


SYNCML_DM_RET_STATUS_T
DMAclItem::ParseSegment(CPCHAR szSegment,
                                             const DMMap<INT32, DMString>& aDict) 
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    DMParser oParser(szSegment,'=');
    CPCHAR pKeyWord = oParser.nextSegment();
    CPCHAR pValue = oParser.nextSegment();

    if ( pKeyWord == NULL || pValue == NULL )
        return SYNCML_DM_FAIL;

    SYNCML_DM_ACL_PERMISSIONS_T nPermission = 0;

    dm_stat = ParsePermission(pKeyWord,&nPermission);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
   
    return ParsePrincipal(pValue,nPermission,aDict);       
    
 
}

SYNCML_DM_RET_STATUS_T
DMAclItem::ParsePrincipal(CPCHAR szSegment,
                                            SYNCML_DM_ACL_PERMISSIONS_T nPermission,
                                            const DMMap<INT32, DMString>& aDict ) 
{
    DMToken oParser(FALSE,szSegment,'+');
    CPCHAR  pSegment = NULL;
    
    while ( (pSegment = oParser.nextSegment()) != NULL ) 
    {
        DMString strPrincipal;
        if ( aDict.lookup(DmAtoi(pSegment), strPrincipal) ) 
        {
             AddPermission( DmtPrincipal( strPrincipal ), nPermission );
        }
        else 
        {
             XPL_LOG_DM_TMN_Error(("ACL file format error - unknown id %s\n \n", pSegment));
             return SYNCML_DM_FAIL;
        }   
   }
   return SYNCML_DM_SUCCESS; 
    
}



SYNCML_DM_RET_STATUS_T 
DMAclItem::Set(CPCHAR szPath,
                       CPCHAR szConfig, 
                       const DMMap<INT32, DMString>& aDict )
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    dm_stat = DMConfigItem::Set(szPath);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
    
    DMToken oParser(FALSE,szConfig,'&');
    CPCHAR pCommand = NULL;
  
    while ( (pCommand = oParser.nextSegment()) != NULL )
    {
        dm_stat = ParseSegment(pCommand,aDict);
        if ( dm_stat != SYNCML_DM_SUCCESS )
            return dm_stat;
    }    

    return SYNCML_DM_SUCCESS;
  
}

void DMAclItem::UpdateDictionary( DMMap<DMString, INT32>& aDict )
{
  DMVector<DmtPrincipal> aPrincipals;
  INT32 iDummy = 0;
  
  GetPrincipals( aPrincipals );

  for ( INT32 i = 0; i < aPrincipals.size(); i++ ){
    if ( !aDict.lookup( aPrincipals[i].getName(), iDummy ) ){
      aDict.put( aPrincipals[i].getName(), aDict.size() + 1);
    }
  }
}

SYNCML_DM_RET_STATUS_T 
DMAclItem::Serialize( DMFileHandler& dmf,
                                  const DMMap<DMString, INT32>& aDict )
{
    
    DMVector<DmtPrincipal> aPrincipals;
    GetPrincipals( aPrincipals );

    DMStringVector strCmdPerm;
    SYNCML_DM_ACL_PERMISSIONS_T nPermissions[5] = {ADD, DELETE, GET, EXEC, REPLACE};
    CPCHAR szCmds[5] = {"A=", "D=", "G=", "E=", "R="};

    DMString str = "";
    DMString strProperty = "";

    SYNCML_DM_RET_STATUS_T dm_stat =SYNCML_DM_SUCCESS;
    dm_stat = DMConfigItem::Serialize(dmf);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;

    for ( UINT32 nPerm = 0; nPerm < DIM(nPermissions); nPerm++ ) 
    {
         DMVector<DmtPrincipal> aSelectedPrincipals;
         for ( INT32 i = 0; i < aPrincipals.size(); i++ )
         {
              if ( IsPermitted(aPrincipals[i], nPermissions[nPerm]) )
                   aSelectedPrincipals.push_back(aPrincipals[i]);
         }
         if ( aSelectedPrincipals.size() )
         {
             CreateProperty(aSelectedPrincipals, szCmds[nPerm], aDict, strProperty);
             AttachProperty(str, '&', strProperty);
         }   
    }

    if ( str.empty() )
        return SYNCML_DM_SUCCESS;
  
    if (dmf.write(str.c_str(), str.length()) != SYNCML_DM_SUCCESS ||
            dmf.write("\n", 1) != SYNCML_DM_SUCCESS) {
        return SYNCML_DM_IO_FAILURE;
    }

    return SYNCML_DM_SUCCESS;
}
