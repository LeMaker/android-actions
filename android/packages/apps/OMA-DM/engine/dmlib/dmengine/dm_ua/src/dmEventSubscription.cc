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

    Source Name: dmEventSubscription.cc

    General Description: Implementation of the DMEventSubscription class

==================================================================================================*/

#include "dmEventSubscription.h"
#include "dm_uri_utils.h"
#include "dmdefs.h"
#include "dmtoken.h"
#include "xpl_Logger.h"
#include "xpl_dm_Manager.h"
#include "dm_tree_class.H"

SYNCML_DM_RET_STATUS_T
DMEventSubscription::ParseSegment(CPCHAR szSegment) 
{
    DMParser oParser(szSegment,'=');
    CPCHAR pKeyWord = oParser.nextSegment();
    CPCHAR pValue = oParser.nextSegment();

    if ( pKeyWord == NULL || pValue == NULL )
        return SYNCML_DM_FAIL;
    
    if ( DmStrcmp(pKeyWord,"Ignore") == 0 )
        ParsePrincipal(pValue,TRUE);
    else 
        if ( DmStrcmp(pKeyWord,"Notify") == 0 )  
            ParsePrincipal(pValue,FALSE);

   return SYNCML_DM_SUCCESS;         
}


SYNCML_DM_RET_STATUS_T
DMEventSubscription::ParsePrincipal(CPCHAR szSegment,
                                              BOOLEAN bIsIgnore)
{
    DMToken oParser(FALSE,szSegment,'+');
    CPCHAR pSegment = NULL;
    
    while ( (pSegment = oParser.nextSegment()) != NULL ) 
    {
        DMString strPrincipal = pSegment;
        if ( bIsIgnore ==  TRUE )        
            m_aIgnorePrincipals.push_back(DmtPrincipal(strPrincipal));
        else
            m_aNotifyPrincipals.push_back(DmtPrincipal(strPrincipal));
    }

    return SYNCML_DM_SUCCESS;
    
}



SYNCML_DM_RET_STATUS_T 
DMEventSubscription::Set(CPCHAR szConfig)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    
    DMToken oParser(FALSE,szConfig,'&');
    CPCHAR pCommand = NULL;
  
    while ( (pCommand = oParser.nextSegment()) != NULL )
    {
      
        SYNCML_DM_EVENT_ACTION_T nAction = SYNCML_DM_EVENT_NONE;
        SYNCML_DM_EVENT_TYPE_T nType = SYNCML_DM_EVENT_NODE;

    
        if ( DmStrcmp(pCommand,"Add") == 0 )
            nAction = SYNCML_DM_EVENT_ADD;
        else if ( DmStrcmp(pCommand,"Delete") ==0  )
            nAction = SYNCML_DM_EVENT_DELETE;
        else if ( DmStrcmp(pCommand,"Indirect") == 0 )
            nAction = SYNCML_DM_EVENT_INDIRECT;
        else if (( DmStrcmp(pCommand,"Replace") == 0 || DmStrcmp(pCommand,"Rename") == 0 ))
            nAction = SYNCML_DM_EVENT_REPLACE | SYNCML_DM_EVENT_RENAME;
        else if (DmStrcmp(pCommand,"Node") == 0 )
            nType = SYNCML_DM_EVENT_NODE;
        else if ( DmStrcmp(pCommand,"Cumulative") == 0 )
            nType = SYNCML_DM_EVENT_CUMULATIVE;
        else if ( DmStrcmp(pCommand,"Detail") == 0 )
            nType = SYNCML_DM_EVENT_DETAIL;
        else
        {
            dm_stat = ParseSegment(pCommand); 
            if ( dm_stat != SYNCML_DM_SUCCESS )
                return dm_stat;
            continue;
        }    

        m_eAction |= nAction;
        m_nType = nType;
    
  }

  return SYNCML_DM_SUCCESS;

}

BOOLEAN 
DMEventSubscription::VerifyPrincipal(DMTree * pTree, 
                                        const DMVector<DmtPrincipal> & aPrincipals)
{

    for (INT32 index=0; index < aPrincipals.size(); index++)
    {
        const DmtPrincipal & oPrincipal = aPrincipals[index];
        if ( oPrincipal.getName() == "OTA" )
        {
            CPCHAR  szDMAccRootPath = ::XPL_DM_GetEnv( SYNCML_DM_DMACC_ROOT_PATH );
            CPCHAR  szServerIdNodeName = ::XPL_DM_GetEnv( SYNCML_DM_NODENAME_SERVERID );
            DMString dmProfleNodeName;
            BOOLEAN bIsFound;

            bIsFound = pTree->GetParentOfKeyValue (pTree->GetServerId(),
                                                   szServerIdNodeName,
                                                   szDMAccRootPath,
                                                   dmProfleNodeName);
            if ( bIsFound )
                return TRUE;
        }  
        else
             if ( oPrincipal.getName() == pTree->GetServerId() )
                return TRUE;
    }   
    return FALSE;

}	






BOOLEAN 
DMEventSubscription::IsEnabled(SYNCML_DM_EVENT_ACTION_T nAction, 
                                  DMTree* pTree)
{
    BOOLEAN bIsEnabled = ((m_eAction & nAction) == nAction);
    if ( bIsEnabled == TRUE )
    {
        if ( pTree == NULL )
            return TRUE;

        
        if ( m_aIgnorePrincipals.size() )
        {
            if  ( VerifyPrincipal(pTree,m_aIgnorePrincipals) == TRUE )
                 return FALSE;
       }    

        if ( m_aNotifyPrincipals.size() )
        {
           if ( VerifyPrincipal(pTree,m_aNotifyPrincipals) == FALSE )
                return FALSE;
        } 

        return TRUE;
        
  }
    
  return FALSE;
    
}
