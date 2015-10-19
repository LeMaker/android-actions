#include "xpl_dm_Notifications.h"
#include "dmt.hpp"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


void  XPL_DM_NotifyTreeUpdate(CPCHAR szTopic, 
                              CPCHAR szPath,
                              SYNCML_DM_EVENT_TYPE_T nType,
                              UINT8 * pData,
                              UINT32 size)
{
  DmtEventMap  aEventMap;
  SYNCML_DM_RET_STATUS_T dm_stat;
  dm_stat = DmtTreeFactory::ParseUpdateEvent(pData,size,aEventMap);
  if ( dm_stat != SYNCML_DM_SUCCESS )
    return;
  for ( DmtEventMap::POS nPos = 0; nPos < aEventMap.end(); nPos++ )
  {
        const DMString & pParent = aEventMap.get_key(nPos);  
        printf( "OnTreeSaved, parent uri %s\n", pParent.c_str() );
  }
}

void  XPL_DM_NotifySessionProgress(BOOLEAN bIsStarted)
{
   if (bIsStarted)
        printf("Server Session started\n");
   else
        printf("Server Session stopped\n"); 

}



#ifdef __cplusplus
}
#endif

