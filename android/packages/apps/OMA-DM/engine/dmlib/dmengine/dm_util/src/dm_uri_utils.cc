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

    Source Name: dm_uri_utils.cc

    General Description: File contains implementation for helper functions for uri processing

==================================================================================================*/

#include "dm_uri_utils.h"
#include "xpl_dm_Manager.h"
#include "dmdefs.h"
#include "dmtoken.h"
#include "dm_tree_class.H" 

CEnv::CEnv()
{
  m_szWFS = m_szMainRFS = "";
}

CPCHAR CEnv::GetWFSFullPath(CPCHAR name, DMString & path)
{
   return GetFullPath(m_szWFS,name,path);
}

CPCHAR CEnv::GetMainRFSFullPath(CPCHAR name, DMString & path)
{
   return GetFullPath(m_szMainRFS,name,path);
}

CPCHAR CEnv::GetFullPath(CPCHAR fs_path, CPCHAR name, DMString & path)
{
   path  = fs_path;
   path += "/";
   path += name;
   return path.GetBuffer();
}

CPCHAR CEnv::GetRFSFullPath(int index, CPCHAR name, DMString & path)
{
   return GetFullPath(GetRFS(index),name,path);
}


BOOLEAN CEnv::Init()
{
  const char* szEnv = XPL_DM_GetEnv(SYNCML_DM_SETTINGS_PATH);

  if ( !szEnv )
    return FALSE;

  DMString strItem, strReminder(szEnv);

  while( DmStringParserGetItem( strItem, strReminder, ':' ) )
    m_aFSes.push_back( strItem );

  if ( m_aFSes.size() == 0 )
    return FALSE;
  
  m_szWFS = m_aFSes[0].c_str();

  if ( m_aFSes.size() > 1 )
    m_szMainRFS = m_aFSes[1].c_str();
  else
    m_szMainRFS = m_szWFS;

  return TRUE;
}

void CEnv::Done()
{
  m_szWFS = m_szMainRFS = "";
  m_aFSes.clear();
}



bool DmStringParserGetItem( DMString& strItem, DMString& strReminder, char cDelim )
{
  if ( strReminder[0] == 0 )
    return false;

  const char* s = DmStrchr( strReminder, cDelim );
  int nPos = s ? s - strReminder : -1; //strReminder.find( cDelim );
  
  if ( nPos < 0 ){
    strItem = strReminder;
    strReminder = "";
  }
  else {
    strItem.assign( strReminder, nPos );
    strReminder = DMString(s+1);
  }
  return true;
}


void DmParseURI(CPCHAR szPath, DMString& strURI, DMString& strKey )
{
  const char* szSlashPos = DmStrrchr( szPath, SYNCML_DM_FORWARD_SLASH );

  if ( !szSlashPos )
    return;
  
  strURI.assign( szPath, szSlashPos-szPath);

  szSlashPos++; // skip slash
  const char* szQuestionPos = DmStrchr( szSlashPos, '?' );
  
  if ( szQuestionPos ) {
    strKey.assign( szSlashPos, szQuestionPos-szSlashPos );
  }
  else
    strKey = szSlashPos;
}

int DmStrToStringVector(const char * pStr, int nLen, DMStringVector& oVector, char cDelim )
{
   // load/unload plugins
   DMString strChild, strList; // = (const char*)pStr;
   if (nLen >0)
   {
      strChild= DMString((const char*)pStr, nLen);      
      strList= DMString((const char*)pStr, nLen);      
   } else
   {
      strChild= (const char*)pStr;
      strList = (const char*)pStr;
   }
   
   int i=0;
   while ( DmStringParserGetItem( strChild, strList, cDelim ) ) 
   {
      oVector.push_back(strChild);
      i++;
   }
   return i;
}

char* GetURISegment(char **ppbURI) 
{
  return GetStringSegment(ppbURI, SYNCML_DM_FORWARD_SLASH); 
}

char* GetStringSegment(char **ppbURI, char cDelim) 
{
  char *pbURI = *ppbURI;
  char *pURIStartLocation = pbURI;

  if(pbURI != NULL)
  {
    pbURI = DmStrchr(pbURI, cDelim);
    if(pbURI != NULL)
    {
      *pbURI++ = SYNCML_DM_NULL;
    }
  }
  *ppbURI = pbURI;
  return (pURIStartLocation);
}

BOOLEAN  DmIsParentURI( CPCHAR szParentURI, CPCHAR szChildURI )
{
  DMParser sURIParserP(szParentURI);
  DMParser sURIParserC(szChildURI);
  CPCHAR sSegmentP = sURIParserP.nextSegment();
  CPCHAR sSegmentC = sURIParserC.nextSegment();    

  UINT16 countP = sURIParserP.getSegmentsCount();
  UINT16 countC = sURIParserC.getSegmentsCount();

  if ( countP == 0 )
    return TRUE; 

  if ( countP > countC )
    return FALSE; 

  while ( sSegmentP && sSegmentC )
  {
    if ( *sSegmentP != '*' && *sSegmentC != '*' && DmStrcmp(sSegmentC, sSegmentP) != 0 )
    {
      return FALSE;
    }
    
    sSegmentP = sURIParserP.nextSegment();
    sSegmentC = sURIParserC.nextSegment();    
  }
  return TRUE;
}


SYNCML_DM_RET_STATUS_T 
DmIsDMAccNodePath(CPCHAR szPath)
{
    //CPCHAR  dm_ver = XPL_DM_GetEnv(SYNCML_DM_VERSION);
    SYNCML_DM_RET_STATUS_T  result = SYNCML_DM_COMMAND_INVALID_URI;
  
    const char *found = NULL;
    
    if( dmTreeObj.IsVersion_12() )
    {
        found = DmStrstr(szPath, DM_DMACC_1_1_URI );
    }
    else 
    {
        found = DmStrstr(szPath, DM_DMACC_1_2_URI );
    }

    result = found && ( found == szPath )
                ? SYNCML_DM_FEATURE_NOT_SUPPORTED
                : SYNCML_DM_SUCCESS;

    return result;
}

BOOLEAN 
DmIsEnabledURI(CPCHAR szPath)
{

    SYNCML_DM_RET_STATUS_T  dm_stat;

    dm_stat = DmIsDMAccNodePath(szPath);
    if ( dm_stat ==  SYNCML_DM_FEATURE_NOT_SUPPORTED )
        return FALSE;

    return XPL_DM_IsFeatureEnabled(szPath);
}
