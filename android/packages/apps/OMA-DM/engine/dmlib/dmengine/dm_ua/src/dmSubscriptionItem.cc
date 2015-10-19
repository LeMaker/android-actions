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

    Source Name: dmSubscriptionItem.cc

    General Description: Implementation of the DMSubscriptionItem class

==================================================================================================*/

#include "dmSubscriptionItem.h"
#include "dm_uri_utils.h"
#include "dmdefs.h"
#include "dmtoken.h"
#include "xpl_Logger.h"
#include "xpl_dm_Manager.h"
#include "dm_tree_class.H"

DMSubscriptionItem::DMSubscriptionItem(CPCHAR szPath ) : DMConfigItem (szPath ) 
{
    m_eAction = SYNCML_DM_EVENT_NONE;
    m_nType = SYNCML_DM_EVENT_NODE;

}


SYNCML_DM_RET_STATUS_T 
DMSubscriptionItem::Set(const DmtEventSubscription & pItem)
{
    return DmtEventSubscription::Set(pItem.GetAction(),
                                   pItem.GetType(),
                                   pItem.GetTopic(),
                                   pItem.GetPrincipals(TRUE),
                                   pItem.GetPrincipals(FALSE));

}


SYNCML_DM_RET_STATUS_T 
DMSubscriptionItem::Set(CPCHAR szPath,
                          CPCHAR szConfig, 
                          const DMMap<INT32, DMString>& aDict)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    XPL_LOG_DM_TMN_Debug(("Parse subscription=%s for path =%s\n", szConfig,szPath));
    dm_stat = DMConfigItem::Set(szPath);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
    
    
    DMToken oParser(FALSE,szConfig,'&');
    CPCHAR pCommand = NULL;
  
    while ( (pCommand = oParser.nextSegment()) != NULL )
    {
      
        SYNCML_DM_EVENT_ACTION_T nAction = SYNCML_DM_EVENT_NONE;
        SYNCML_DM_EVENT_TYPE_T nType = SYNCML_DM_EVENT_NODE;

    
        if ( DmStrcmp(pCommand,"A") == 0 )
            nAction = SYNCML_DM_EVENT_ADD;
        else if ( DmStrcmp(pCommand,"D") ==0  )
            nAction = SYNCML_DM_EVENT_DELETE;
        else if ( DmStrcmp(pCommand,"I") == 0 )
            nAction = SYNCML_DM_EVENT_INDIRECT;
        else if ( DmStrcmp(pCommand,"R") == 0 )
            nAction = SYNCML_DM_EVENT_REPLACE | SYNCML_DM_EVENT_RENAME;
        else if (DmStrcmp(pCommand,"N") == 0 )
            nType = SYNCML_DM_EVENT_NODE;
        else if ( DmStrcmp(pCommand,"C") == 0 )
            nType = SYNCML_DM_EVENT_CUMULATIVE;
        else if ( DmStrcmp(pCommand,"F") == 0 )
            nType = SYNCML_DM_EVENT_DETAIL;
        else
        {
            dm_stat = ParseSegment(pCommand,aDict); 
            if ( dm_stat != SYNCML_DM_SUCCESS )
                return dm_stat;
            continue;
        }    

        m_eAction |= nAction;
        m_nType = nType;
    
  }

  return SYNCML_DM_SUCCESS;

}



SYNCML_DM_RET_STATUS_T
DMSubscriptionItem::ParseSegment(CPCHAR szSegment,
                                             const DMMap<INT32, DMString>& aDict) 
{
    DMParser oParser(szSegment,'=');
    CPCHAR pKeyWord = oParser.nextSegment();
    CPCHAR pValue = oParser.nextSegment();

    if ( pKeyWord == NULL || pValue == NULL )
        return SYNCML_DM_FAIL;
    
    if ( DmStrcmp(pKeyWord,"P") == 0 )
        ParsePrincipal(pValue,TRUE,aDict);
    else 
        if ( DmStrcmp(pKeyWord,"S") == 0 )  
            ParsePrincipal(pValue,FALSE,aDict);
        else
            if ( DmStrcmp(pKeyWord,"T") == 0 )  
                m_strTopic = pValue;

   return SYNCML_DM_SUCCESS;         
}


SYNCML_DM_RET_STATUS_T
DMSubscriptionItem::ParsePrincipal(CPCHAR szSegment,
                                              BOOLEAN bIsIgnore,
                                              const DMMap<INT32, DMString>& aDict ) 
{
    DMToken oParser(FALSE,szSegment,'+');
    CPCHAR pSegment = NULL;
    
    while ( (pSegment = oParser.nextSegment()) != NULL ) 
    {
        DMString strPrincipal;
        if ( aDict.lookup(DmAtoi(pSegment), strPrincipal) ) 
        {  
            if ( bIsIgnore ==  TRUE )        
                m_aIgnorePrincipals.push_back(DmtPrincipal(strPrincipal));
            else
                m_aNotifyPrincipals.push_back(DmtPrincipal(strPrincipal));
         }
        else 
        {
            XPL_LOG_DM_TMN_Error(("event file format error - unknown id %s\n \n", pSegment));
            return SYNCML_DM_FAIL;
        }
    }

    return SYNCML_DM_SUCCESS;
    
}




SYNCML_DM_RET_STATUS_T 
DMSubscriptionItem::Serialize( DMFileHandler& dmf,
                                        const DMMap<DMString, INT32>& aDict ) 
{
  
    SYNCML_DM_EVENT_ACTION_T nAction[4] = {SYNCML_DM_EVENT_ADD,
                                                            SYNCML_DM_EVENT_DELETE, 
                                                            SYNCML_DM_EVENT_REPLACE,
                                                            SYNCML_DM_EVENT_INDIRECT};
                               
    SYNCML_DM_EVENT_TYPE_T  nType[3] = {  SYNCML_DM_EVENT_NODE,
                                                                SYNCML_DM_EVENT_CUMULATIVE,
                                                                SYNCML_DM_EVENT_DETAIL};

  
    CPCHAR szEvent[4] = {"A", "D", "R", "I"};
    CPCHAR szEventDetail[3] = { "N", "C", "F"};
    UINT8 index;

    SYNCML_DM_RET_STATUS_T dm_stat =SYNCML_DM_SUCCESS;
    dm_stat = DMConfigItem::Serialize(dmf);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;

    DMString str = "";
    for (index = 0; index < DIM(nAction); index++ ) 
    {
        if ( IsEnabled(nAction[index],NULL) )
            AttachProperty( str, '&', szEvent[index] );
    }
    for (index = 0; index <  DIM(nType); index++ ) 
    {
        if ( m_nType == nType[index] )
        {
            AttachProperty( str, '&', szEventDetail[index] );
            break;
        }   
    }

    DMString strProperty;
    if ( m_strTopic != NULL )
    { 
        strProperty = "T=" + m_strTopic;
        AttachProperty(str, '&', strProperty);
    }      

    if ( m_aIgnorePrincipals.size() )
    { 
        DMConfigItem::CreateProperty( m_aIgnorePrincipals, "P=",aDict,strProperty);
        AttachProperty(str, '&', strProperty);
    }

    if ( m_aNotifyPrincipals.size() )
    {
        DMConfigItem::CreateProperty( m_aNotifyPrincipals, "S=",aDict,strProperty);
        AttachProperty(str, '&', strProperty);
    }

    if ( str.empty() )
        return SYNCML_DM_SUCCESS;
  
    if ( dmf.write(str.c_str(), str.length()) != SYNCML_DM_SUCCESS ||
         dmf.write( "\n", 1 )!= SYNCML_DM_SUCCESS )
        return SYNCML_DM_IO_FAILURE;

    return SYNCML_DM_SUCCESS;
}


void DMSubscriptionItem::UpdateDictionary( DMMap<DMString, INT32>& aDict )
{
  DMVector<DmtPrincipal> aPrincipals;
  INT32 iDummy = 0;
  INT32 index=0;
  
  for (index=0; index<m_aIgnorePrincipals.size(); index++)
  {
    if ( !aDict.lookup( m_aIgnorePrincipals[index].getName(), iDummy ) ){
      aDict.put( m_aIgnorePrincipals[index].getName(), aDict.size() + 1);
    }
  }  

  for (index=0; index<m_aNotifyPrincipals.size(); index++)
  {
    if ( !aDict.lookup(m_aNotifyPrincipals[index].getName(), iDummy ) ){
      aDict.put( m_aNotifyPrincipals[index].getName(), aDict.size() + 1);
    }
  }  
}
