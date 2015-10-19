#include "dmdefs.h"
#include "xpl_Logger.h"
#include "dmstring.h"
#include "xpl_StringUtil.h"
#include <ctype.h>

#define DM_STR_ALIGN_BY   16

static inline int GetAdjustedLen( int nLen )
{
  int nRes = (nLen + DM_STR_ALIGN_BY - 1) / DM_STR_ALIGN_BY * DM_STR_ALIGN_BY;
  return nRes;
}

DMString::DMString(CPCHAR szStr)
{
  m_pStr = NULL;
  (*this) = szStr;
}

DMString::DMString(CPCHAR szStr, INT32 nLen)
{
  m_pStr = NULL;
  assign( szStr, nLen );
}

DMString::DMString(const DMString& str)
{
  m_pStr = NULL;
  (*this) = str;
}


DMString& DMString::operator=( CPCHAR szStr )
{
  int nTrgLen = szStr ? DmStrlen(szStr) : 0;
  assign(szStr, nTrgLen);
  return *this;
}

DMString::~DMString()
{
  DmFreeMem(m_pStr);
}
   
  
BOOLEAN DMString::operator==(CPCHAR szStr) const
{
  if ( m_pStr == szStr )
    return TRUE;

  if ( !szStr )
    return FALSE;

  return DmStrcmp( c_str(), szStr ) == 0;
}

DMString& DMString::operator += (CPCHAR szStr)
{
  INT32 nLen2 = szStr ? DmStrlen( szStr ) : 0;

  if ( nLen2 == 0 )
    return *this; // x + null sill x

  INT32 nLen1 = DmStrlen( *this );
  INT32 nCurBuffer = m_pStr ? GetAdjustedLen(nLen1+1) : 0;

  if ( nCurBuffer > (nLen1 + nLen2) )
  {
    DmStrcat(m_pStr, szStr);
  }
  else
  {
    char* pNewStr = (char*)DmAllocMem( GetAdjustedLen(nLen1 + nLen2 + 1));

    if ( pNewStr ) {
      DmStrcpy( pNewStr, c_str() );
      DmStrcat( pNewStr, szStr );
    }
    FreeAndSetNull(m_pStr);
    m_pStr = pNewStr;
  }
  return *this;
}

// append a single character
DMString& DMString::operator += (char c)
{
  INT32 nLen1 = DmStrlen( *this );
  INT32 nCurBuffer = m_pStr ? GetAdjustedLen(nLen1+1) : 0;

  if (nCurBuffer > (nLen1 + 1))
  {
    m_pStr[nLen1] = c;
    m_pStr[nLen1 + 1] = '\0';
  }
  else
  {
    char* pNewStr = (char*)DmAllocMem( GetAdjustedLen(nLen1 + 1 + 1));

    if (pNewStr) {
      DmStrcpy(pNewStr, c_str());
      pNewStr[nLen1] = c;
      pNewStr[nLen1 + 1] = '\0';
    }
    FreeAndSetNull(m_pStr);
    m_pStr = pNewStr;
  }
  return *this;
}

void DMString::SetAt( INT32 nPos, char c )
{
  if ( nPos < 0 || nPos >= (INT32)DmStrlen(c_str()) ) {
    XPL_LOG_DM_API_Error(("Invalid pos in DMString::SetAt function: pos %d in string %s\n",nPos,c_str()));
    return;
  }

  m_pStr[ nPos ] = c;
}

void DMString::replaceAll(char oldCh, char newCh) {

  char * pch = NULL;

  if (m_pStr != NULL) {
    pch = DmStrchr(m_pStr, oldCh);
    
    while (pch!=NULL) {
      SetAt(pch - m_pStr, newCh);
      pch=DmStrchr(pch+1, oldCh);
    }
  }
}
BOOLEAN DMString::Encode()
{
  const static char s_cHexDigits[] = "0123456789ABCDEF";
  if ( !m_pStr )
    return TRUE; // nothing to encode

  char* pNewBuf = (char*)DmAllocMem( GetAdjustedLen(DmStrlen(m_pStr) * 3 + 1) );

  if ( !pNewBuf )
    return FALSE; // out of memory

  CPCHAR szSrc = m_pStr;
  char* szTrg = pNewBuf;

  // keep intact reserved and unreserved characters, encode all others
  //    reserved    = ";" | "/" | "?" | ":" | "@" | "&" | "=" | "+" | "$" | ","
  //    unreserved  = alphanum | mark
  //    mark        = "-" | "_" | "." | "!" | "~" | "*" | "'" | "(" | ")"
  
  while ( *szSrc ) {
    if ( (*szSrc >= '&' && *szSrc <= ';') ||
      (*szSrc >= '?' && *szSrc <= 'Z') ||
      (*szSrc >= 'a' && *szSrc <= 'z')  ||
      (*szSrc == '!' ) ||
      (*szSrc == '=' ) ||
      (*szSrc == '$' ) ||
      (*szSrc == '_' ) ||
      (*szSrc == '~' ) 
      )
      *szTrg++ = *szSrc++;
    else {
//      *szTrg++ = '\\'; // Using bask slash + x instead of % 
//      *szTrg++ = 'x'; 
// Fllow RFC 2396
      *szTrg++ = '%';

      *szTrg++ = s_cHexDigits[ ((*szSrc) >> 4) & 0xF ];
      *szTrg++ = s_cHexDigits[ (*szSrc) & 0xF ];
      szSrc++;
    }
  }
  
  *szTrg = 0;

  FreeAndSetNull(m_pStr);
  m_pStr = pNewBuf;
  return TRUE;
}

inline INT32 HexDigit2N( char c ) {
  if ( c >= '0' && c <= '9' )
    return c-'0';

  if ( c >= 'A' && c <= 'F' )
    return c - 'A' + 10;
  
  if ( c >= 'a' && c <= 'f' )
    return c - 'a' + 10;

  return -1;
}

BOOLEAN DMString::Decode()
{
  CPCHAR szSrc = m_pStr;
  char* szTrg = m_pStr;

  while ( *szSrc ) {
     if ( *szSrc  != '%'  &&   *szSrc  != '\\' ) 
      *szTrg++ = *szSrc++;
    else {
      if( *szSrc  == '\\' ) 
      szSrc++; // skip back splash 
      szSrc++; // skip % or x  
     INT32 n1 = HexDigit2N( *szSrc++ );

      if ( n1 < 0 )
        return FALSE;
      
      INT32 n2 = HexDigit2N( *szSrc++ );

      if ( n2 < 0 )
        return FALSE;

      *szTrg++ = (unsigned char)((n1 << 4) + n2);
    }
  }
  *szTrg++ = 0;
  
  return TRUE;
}

BOOLEAN DMString::assign( CPCHAR pStr, INT32 nLen )
{
  if (!pStr || !pStr[0] || nLen <= 0) {
    FreeAndSetNull(m_pStr);
    return TRUE;
  }

  INT32 nCurBuffer = m_pStr ? GetAdjustedLen(length()+1) : 0;

  if ( nCurBuffer <= nLen ) {
    FreeAndSetNull(m_pStr);
    m_pStr = (char *)DmAllocMem( GetAdjustedLen(nLen+1));
  }

  if ( !m_pStr ) 
    return FALSE;
  
  DmStrncpy( m_pStr, pStr, nLen );
  m_pStr[nLen] = 0;
  return TRUE;
}

void DMString::trim()
{
  if (length() > 0 )
  {
     char *pStr = m_pStr;
     while (*pStr == ' ') ++pStr ;
     INT32 len = DmStrlen(pStr);
     if (len > 0)
     {
         while (*(pStr + len - 1) ==' ') --len ;
         *((char *)(memmove(m_pStr, pStr, len)) + len) = 0;
     }
  }
}

INT32 DMString::length() const
{
  return m_pStr ? DmStrlen(m_pStr) : 0;
}

void DMString::clear()
{
  FreeAndSetNull(m_pStr);
}

CPCHAR DMString::detach()
{
   CPCHAR pStr = GetBuffer();
   m_pStr = NULL;
   return pStr;
}

void DMString::attach(CPCHAR pStr)
{
    DmFreeMem(m_pStr);
    m_pStr = (char*)pStr;
}

char* DMString::AllocateBuffer( int nLen )
{
  INT32 nCurBuffer = m_pStr ? GetAdjustedLen(length()+1) : 0;

  if ( nCurBuffer >= nLen )
    return m_pStr;

    DmFreeMem(m_pStr);
    m_pStr = (char *)DmAllocMem( GetAdjustedLen(nLen));

    if ( m_pStr )
      m_pStr[0]=0;
    
    return m_pStr;
}
//------------------------------------------------------------------------
// FUNCTION        : RemoveSufix
//
// DESCRIPTION     : This function remove the suffix of one string and assigns to a new string
//
// RETURN VALUE    : 
// POST-CONDITIONS :
// IMPORTANT NOTES :
//------------------------------------------------------------------------
void DMString::RemoveSufix(CPCHAR sourceStr, int seperator)
{
  if(sourceStr != NULL)
  {	
	 INT32 len=0;
	 CPCHAR ptr = DmStrrchr(sourceStr, seperator);
	  if (ptr != NULL)
		 len=(INT32)(ptr-sourceStr);
	  assign(sourceStr, len);
  }	 
}

INT32
DMString::CompareNoCase( const DMString& str2 ) const
{
  INT32       val1 = 0;
  INT32       val2 = 0;
  const char* p1 = m_pStr;
  const char* p2 = str2.m_pStr;
  
  if( p1 && p2 )
  {  
    do
    {
      val1 = xplTolower(*(p1++));
      val2 = xplTolower(*(p2++));
    } 
    while( val1 && val2 && ( val1 == val2 ) );
  }
  else
  {
    val1 = p1 ? 1 : 0;
    val2 = p2 ? 1 : 0;
  }
  
  return (val2 - val1);
}


