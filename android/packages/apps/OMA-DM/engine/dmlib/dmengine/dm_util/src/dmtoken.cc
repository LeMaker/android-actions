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

Source Name: dmToken.cc

General Description: This file contains implementation of utility classes DMToken, DMURI, DMParser

==================================================================================================*/

#include "dmtoken.h"
#include "dmdefs.h"
#include "xpl_Logger.h"
#include "syncml_dm_data_types.h" 
#include "dm_tree_class.H"

DMToken::DMToken()
{
    m_pDelimPos = NULL;
    m_pStr = NULL;
    m_pTokenPos = NULL;
    m_cDelim = SYNCML_DM_COMMA;
    m_bIsAlloc = TRUE;
}

DMToken::DMToken(char delimeter)
{
    m_pDelimPos = NULL;
    m_pStr = NULL;
    m_pTokenPos = NULL;
    m_cDelim = delimeter;
    m_bIsAlloc = TRUE;
}


DMToken::DMToken(BOOLEAN bIsAlloc, CPCHAR str, char delimeter)
{

    m_pDelimPos = NULL;
    m_pStr = NULL;
    m_bIsAlloc = bIsAlloc;
    m_cDelim = delimeter;
    assign(str);
}


DMToken::~DMToken()
{
    if ( m_bIsAlloc == TRUE )
    {    
        FreeAndSetNull(m_pStr);
    }    
    else
        reset();
}


CPCHAR DMToken::assign(CPCHAR szStr)
{
    if ( m_bIsAlloc == TRUE ) 
    {
        FreeAndSetNull(m_pStr);

        if ( szStr && szStr[0] )
        { // copy non empty string only
            m_pStr = (char *)DmAllocMem( DmStrlen(szStr)+1);
            if ( m_pStr != NULL) {
                DmStrcpy( m_pStr, szStr );
            }
            else {
                XPL_LOG_DM_TMN_Error(("DMToken::assign : unable allocate memory\n"));
            }
        }
    }
    else
    {
        reset();
        m_pStr = (char*)szStr;
    }        
    
    m_pDelimPos = m_pStr;
    m_pTokenPos = m_pStr;
    return m_pStr;
}

CPCHAR DMToken::nextSegment()
{

    if ( m_pDelimPos == NULL )
        return NULL;

    if ( m_pDelimPos != m_pStr )
    {
        *m_pDelimPos = m_cDelim;
        m_pTokenPos = m_pDelimPos+1;
    }    
    m_pDelimPos = DmStrchr(m_pTokenPos, m_cDelim);
    if(m_pDelimPos != NULL)
    {
       *m_pDelimPos = SYNCML_DM_NULL;
    }

    if ( m_pTokenPos[0] == SYNCML_DM_NULL )
        return NULL;
    
    return (m_pTokenPos);

}


UINT32 DMToken::getSegmentsCount()
{
    UINT32 count = 0;
    char * sTokenPos = m_pStr;

    if ( !sTokenPos )
        return 0;

    reset();

    m_pDelimPos = DmStrchr(m_pTokenPos,m_cDelim);
    while ( m_pDelimPos )
    {
        count++;
        m_pTokenPos = m_pDelimPos + 1;
        m_pDelimPos = DmStrchr(m_pTokenPos,m_cDelim);
    }

    if ( *m_pTokenPos == SYNCML_DM_NULL )
        return count;

    m_pDelimPos = m_pStr;
    m_pTokenPos = m_pStr;
    return count+1;
}

void DMToken::reset()
{
    if ( m_pDelimPos && m_pDelimPos != m_pStr )
        *m_pDelimPos = m_cDelim;
    m_pDelimPos = m_pStr;
    m_pTokenPos = m_pStr;
}

DMURI::DMURI() : DMToken(TRUE, NULL,SYNCML_DM_FORWARD_SLASH)
{
}

DMURI::DMURI(BOOLEAN bIsAlloc) : DMToken(bIsAlloc, NULL,SYNCML_DM_FORWARD_SLASH)
{
}

DMURI::DMURI(BOOLEAN bIsAlloc, CPCHAR szURI) : DMToken(bIsAlloc, szURI,SYNCML_DM_FORWARD_SLASH)
{
}


CPCHAR DMURI::getTailSegments() const
{
    if ( m_pDelimPos == NULL )
        return NULL;

    if ( m_pDelimPos == m_pStr && *m_pStr != SYNCML_DM_FORWARD_SLASH )
        return m_pStr;
    
    return m_pDelimPos+1;
}


CPCHAR DMURI::getLastSegment() 
{

    if ( m_pStr == NULL )
        return NULL;

    reset();    
    char * sPos = DmStrrchr(m_pStr,m_cDelim);
    if ( sPos == NULL )
        return NULL;
    else
        return sPos+1; 
}

CPCHAR DMURI::getParentURI()
{

     if ( m_pStr == NULL )
        return NULL;

     char * pDelimPosPrev = m_pDelimPos;
     m_pDelimPos = DmStrrchr(m_pStr, SYNCML_DM_FORWARD_SLASH);
     if ( !m_pDelimPos || m_pDelimPos == m_pStr )
     {
        m_pDelimPos = pDelimPosPrev;
        return NULL;
     }   

     *m_pDelimPos = SYNCML_DM_NULL;
     if ( pDelimPosPrev != m_pStr )
        *pDelimPosPrev = SYNCML_DM_FORWARD_SLASH;


     return m_pStr;
}

DMParser::DMParser(char delimeter)
{
    m_pDelimPos = NULL;
    m_pStr = NULL;
    m_cDelim = delimeter;
    
    m_nCurrentSegment = 0;
    m_nSegmentsCount = 0;

    m_aSegments = (DM_URI_SEGMENT_T*)DmAllocMem( dmTreeObj.GetMaxPathDepth() * sizeof(DM_URI_SEGMENT_T));

}

DMParser::DMParser(CPCHAR szURI, char delimeter) 
{
    m_pDelimPos = NULL;
    m_pStr = NULL;
    m_cDelim = delimeter;
   
   
    m_nCurrentSegment = 0;
    m_nSegmentsCount = 0;
    m_aSegments = (DM_URI_SEGMENT_T*)DmAllocMem( dmTreeObj.GetMaxPathDepth() * sizeof(DM_URI_SEGMENT_T));
    assign(szURI);
}


DMParser::~DMParser()
{
  reset();
  FreeAndSetNull(m_aSegments);
}

CPCHAR DMParser::nextSegment()
{

    if ( m_nCurrentSegment == m_nSegmentsCount )
        return NULL;

    char * pStr = m_aSegments[m_nCurrentSegment].m_pStr;
//    XPL_LOG_DM_TMN_Debug(("DMParser::nextSegment, pStr:%s, m_nCurrentSegment:%d, m_nLen:%d\n", pStr, m_nCurrentSegment, m_aSegments[m_nCurrentSegment].m_nLen));
    pStr[m_aSegments[m_nCurrentSegment].m_nLen] = SYNCML_DM_NULL;
//    XPL_LOG_DM_TMN_Debug(("DMParser::nextSegment, pStr:%s, m_nCurrentSegment:%d, m_nLen:%d\n", pStr, m_nCurrentSegment, m_aSegments[m_nCurrentSegment].m_nLen));

    m_nCurrentSegment++;

    return pStr;
}


CPCHAR DMParser::assign(CPCHAR szStr)
{
    DM_URI_SEGMENT_T segment;

    if ( !m_aSegments ) 
        return NULL;

    m_pStr = (char*)szStr;
    
    m_nCurrentSegment = 0;
    m_nSegmentsCount = 0;

    if ( !m_pStr )
        return m_pStr;
        

    m_pTokenPos = m_pStr;
    m_pDelimPos = DmStrchr(m_pTokenPos,m_cDelim);

    while ( m_pDelimPos )
    {
        segment.m_pStr = m_pTokenPos;
        segment.m_nLen = m_pDelimPos - m_pTokenPos;
        m_aSegments[m_nSegmentsCount++] = segment;
        m_pTokenPos = m_pDelimPos + 1;
        m_pDelimPos = DmStrchr(m_pTokenPos,m_cDelim);
    }

    if ( *m_pTokenPos == SYNCML_DM_NULL )
        return m_pStr;
        
    
    segment.m_pStr = m_pTokenPos;
    segment.m_nLen = DmStrlen(m_pTokenPos);
    m_aSegments[m_nSegmentsCount++] = segment;
    return m_pStr;

}

void DMParser::reset()
{
    if ( !m_aSegments ) 
        return;
    
    if ( m_nCurrentSegment > 0 )
    {
        for (int i=0; i<m_nSegmentsCount-1; i++)
            m_aSegments[i].m_pStr[m_aSegments[i].m_nLen] = m_cDelim;
    }
        
    m_nCurrentSegment = 0;
}

BOOLEAN DMParser::findSegment(CPCHAR szSegment)
{
    
     for (int i=0; i<m_nSegmentsCount; i++)
     {
        m_aSegments[i].m_pStr[m_aSegments[i].m_nLen] = SYNCML_DM_NULL;
        if ( DmStrcmp(m_aSegments[i].m_pStr,szSegment) == 0)
            return TRUE;
     }   

     return FALSE;   
}
