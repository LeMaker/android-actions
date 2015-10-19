#include <stdlib.h>
#include "xpl_Time.h"
#include "dmtSessionProp.hpp"


void DmtSessionProp::generateSessionID()
{
  if ( m_nDirection == SYNCML_DM_CLIENT_INITIATED_SESSION )
  {
      XPL_CLK_CLOCK_T t = XPL_CLK_GetClock();  
      
      /* The current time is given as seed for generating
       * pseudo random numbers */
      srand((UINT32)t);
      m_nSessionID = rand ();  // engine generated session ID
      if( !m_nSessionID )
          m_nSessionID = 1;
  }
}
