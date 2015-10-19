#include "dmStringUtil.h"
#include "dmbuffer.h"
#include "dmdefs.h"
#include "xpl_Logger.h"

DMBuffer::DMBuffer()
{
    m_pBuf = NULL;
    m_nCapacity = 0;
    m_nSize = 0;
}

DMBuffer::~DMBuffer()
{
    FreeAndSetNull(m_pBuf);
    m_nCapacity = 0;
    m_nSize = 0;
}


DMBuffer& DMBuffer::operator=(const DMBuffer & buffer )
{
  assign(buffer.getBuffer(),buffer.getSize());
  return *this;
}

UINT8 * DMBuffer::allocate(UINT32 nCapacity)
{
    if ( !nCapacity )
      return m_pBuf;

    m_nSize = 0;
    if ( m_nCapacity < nCapacity + 1 )
    {
      FreeAndSetNull(m_pBuf);
      m_pBuf = (UINT8*)DmAllocMem(nCapacity+1);
      if ( m_pBuf )
      {
        m_nCapacity = nCapacity+1;
        memset(m_pBuf,0,m_nCapacity);
      }
      else
      {
        m_nCapacity = 0;
        XPL_LOG_DM_TMN_Error(("DMBuffer::Allocate : Unable to allocate memory\n"));   
      }  
    }
    else
      if ( m_pBuf )
        memset(m_pBuf,0,m_nCapacity);
    return m_pBuf;
}

void DMBuffer::reset()
{
    m_pBuf = NULL;
    m_nCapacity = 0;
    m_nSize = 0;
}
void  DMBuffer::free()
{
    FreeAndSetNull(m_pBuf);
    m_nCapacity = 0;
    m_nSize = 0;
}

void DMBuffer::clear()
{
    if ( m_pBuf )
    {
       memset(m_pBuf,0,m_nCapacity);
       m_nSize = 0;
    }   
}

UINT8 * DMBuffer::assign(CPCHAR szStr)
{
   return assign((UINT8*)szStr,DmStrlen(szStr));
}
void  DMBuffer::append(const UINT8 * pBuffer, INT32 size)
{
    if ( m_pBuf && (m_nSize + size <= m_nCapacity))
    {
	  memcpy((UINT8*)&m_pBuf[m_nSize],pBuffer,size); 
	  m_nSize += size;
    }
}

UINT8 * DMBuffer::assign(const UINT8 * pBuffer, INT32 size)
{
    allocate(size);
    if ( m_pBuf )
    {
      memcpy(m_pBuf,pBuffer,size); 
      m_pBuf[size] = (UINT8)'\0';
      m_nSize = size;
    }  
    else
    {
      m_nSize = 0;
    }
    return m_pBuf;
}

void DMBuffer::setSize(UINT32 size)
{
    if ( size == 0 )
        clear();
    else
    {
        if ( size <= m_nCapacity-1 )
        {
          m_nSize = size;
        }
        else
        {
          m_nSize = m_nCapacity - 1;
        }
        m_pBuf[m_nSize] = (UINT8)'\0';
    }    
}


void DMBuffer::copyTo(UINT8 ** pBuffer) const
{
   if ( pBuffer == NULL )
     return;
   
   *pBuffer = NULL;
    
   *pBuffer = (UINT8*)DmAllocMem(m_nSize+1);
   if ( *pBuffer == NULL )
   {
      XPL_LOG_DM_TMN_Error(("DMBuffer::copyTo : Unable to allocate memory\n"));   
      return; 
   }
   if ( m_nSize )
       memcpy(*pBuffer,m_pBuf,m_nSize+1);
   else
       (*pBuffer)[0] = 0;
}


void DMBuffer::copyTo(DMString &sStr) const 
{
   
   if ( m_nSize == 0 )
     return;

   sStr.assign((char*)m_pBuf,m_nSize);
}
void DMBuffer::copyTo(INT32 offset,INT32 size, DMBuffer& pBuffer) const
{
	if(offset > m_nSize)
		return;

	pBuffer.assign((char*)&m_pBuf[offset],size);
}
void DMBuffer::copyTo(char * pStr) const
{
   if ( pStr == NULL )
     return;
   
   if ( m_nSize == 0 )
     return;
    
   memcpy(pStr,m_pBuf,m_nSize+1);
}

BOOLEAN DMBuffer::compare(CPCHAR pStr) const 
{
   if ( !m_nSize || m_nSize != DmStrlen(pStr) )
     return FALSE;
   return ( DmStrncmp(pStr,(CPCHAR)m_pBuf,m_nSize) == 0 ? TRUE : FALSE );
  
}


BOOLEAN DMBuffer::compare(CPCHAR pStr, UINT32 size) const 
{

   if ( !m_nSize || m_nSize < size )
     return FALSE;

   return ( DmStrncmp(pStr,(CPCHAR)m_pBuf,size) == 0 ? TRUE : FALSE );
  
}



void DMBuffer::attach(UINT8 * pBuffer, INT32 nCapacity)
{
    FreeAndSetNull(m_pBuf);
    m_pBuf = pBuffer;
    m_nCapacity = nCapacity;
    m_nSize = nCapacity-1;
    m_pBuf[m_nSize] = (UINT8)'\0';
}    

UINT8 * DMBuffer::detach()
{
    UINT8 *pBuffer = m_pBuf;
    m_pBuf = NULL;
    m_nCapacity = 0;
    m_nSize = 0;
    return pBuffer;
}

