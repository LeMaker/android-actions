#include "dmt.hpp"

#ifndef NO_CAF
#ifndef DM_NO_LOCKING
#include "datainputstream.h"


DmtEvent::DmtEvent( DataInputStream* pInputStream )
{
  if ( !pInputStream )
    return;

  char* pStr; pInputStream->readUTF(pStr);
  m_strURI = pStr;


  free( pStr );

  int nKeyNum;
  for ( pInputStream->readInt(nKeyNum); nKeyNum > 0; nKeyNum-- ) {
    pInputStream->readUTF(pStr);
    int nValue;  pInputStream->readInt(nValue);

    m_mapKeys.put( pStr, nValue );

    free( pStr );
  }

  pInputStream->readLongLong( m_llCommitID );
}

#endif
#endif
