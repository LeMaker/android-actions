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

#ifndef DMURIUTILITY_H
#define DMURIUTILITY_H

/*==================================================================================================

Header Name: dm_uri_utils.h

General Description: This file contains declaration declaration of helper functions  for uri processing

==================================================================================================*/

#include <stdlib.h>
#include "xpl_Types.h"        
#include "syncml_dm_data_types.h"
#include "dmvector.h"

#define SYNCML_DM_NULL           ('\0')
#define SYNCML_DM_AMPERSAND      ('&')
#define SYNCML_DM_STAR           ('*')
#define SYNCML_DM_PLUS           ('+')
#define SYNCML_DM_EQUAL_TO       ('=')
#define SYNCML_DM_FORWARD_SLASH  ('/')
#define SYNCML_DM_DOT            ('.')
#define SYNCML_DM_PERCENTAGE     ('%')
#define SYNCML_DM_QUESTION_MARK  ('?')
#define SYNCML_DM_SPACE          (' ')
#define SYNCML_DM_COMMA          (',')


#ifdef __cplusplus  
extern "C" {
#endif

void DmParseURI(CPCHAR szPath, DMString& strURI, DMString& strKey );

bool DmStringParserGetItem( DMString& strItem, DMString& strReminder, char cDelim );

int DmStrToStringVector(const char * pStr, int nLen, DMStringVector& oVector, char cDelim );

char* GetURISegment(char **ppbURI); 

char* GetStringSegment(char **ppbURI, char cDelim); 

/**
  * Verifies if path is the DMAcc path
  * \param szPath [in/out] - node path
  * \return TRUE if path is the DMAcc path
  */
SYNCML_DM_RET_STATUS_T DmIsDMAccNodePath(CPCHAR szPath);

/**
  * Verifies if path is enabled
  * \param szPath [in/out] - node path
  * \return TRUE if enabled 
  */
BOOLEAN DmIsEnabledURI(CPCHAR szPath);

/**
  * Verifies if szParentURI is direct or indirect parent (including node itself) of node szChildURI
  * Both URIs can contain multi-node characters ('*')
  * \param szParentURI [in] - parent path
  * \param szChildURI [in] - child path
  * \return TRUE if path is a parent 
  */
BOOLEAN  DmIsParentURI(CPCHAR szParentURI, CPCHAR szChildURI);


#ifdef __cplusplus
}
#endif

class CEnv
{
public:
  CEnv();

  BOOLEAN Init();
  void Done();

  CPCHAR GetWFS() const { return m_szWFS;}
  CPCHAR GetMainRFS() const { return m_szMainRFS; }
  int GetRFSCount() const { return m_aFSes.size() > 1 ? m_aFSes.size() - 1 : 0;}
  CPCHAR GetRFS( int index ) const { return (index +1) >= m_aFSes.size() ? NULL : m_aFSes[index +1].c_str(); }
  CPCHAR GetWFSFullPath(CPCHAR name, DMString & path);
  CPCHAR GetMainRFSFullPath(CPCHAR name, DMString & path);
  CPCHAR GetRFSFullPath(int index, CPCHAR name, DMString & path);
  
private:
  DMStringVector  m_aFSes; // first element is WFS, second is MainRFS, all other - auxulary RFS
  CPCHAR m_szWFS;
  CPCHAR m_szMainRFS;

  CPCHAR GetFullPath(CPCHAR fs_path, CPCHAR name, DMString & path);
};

#endif
