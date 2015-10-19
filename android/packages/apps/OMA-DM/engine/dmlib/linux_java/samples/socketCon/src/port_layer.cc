#include "dmConnection.h"
#include "xpl_dm_ServerAlert.h"
#include "xpl_dm_Notifications.h"

// empty function impls from dmConnection_impl.cc 

/*
DmBrwConnector * DmBrwCreateConnector()
{
  printf("Connector creator called- return null\n");
    return NULL;
}

SYNCML_DM_RET_STATUS_T DmBrwDestroyConnector(DmBrwConnector * browser_handler)
{
    return SYNCML_DM_SUCCESS;

}
*/

// empty function impls from dmUIAlert.cc

SYNCML_DM_RET_STATUS_T XPL_DM_ShowConfirmAlert(INT32 maxDisplayTime, CPCHAR msg, XPL_DM_ALERT_RES_T defaultResponse, XPL_DM_ALERT_RES_T * responseCode)
{
  static int i = 0;
  i++;
  *responseCode = (i%2) ? XPL_DM_ALERT_RES_YES : XPL_DM_ALERT_RES_NO;

  return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T XPL_DM_ShowDisplayAlert(INT32 minDisplayTime, CPCHAR msg)
{
  return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T XPL_DM_ShowTextInputAlert(INT32 maxDisplayTime, 
                                            CPCHAR msg, 
                                            CPCHAR defaultResponse,
                                            INT32 maxLength, 
                                            XPL_DM_ALERT_INPUT_T inputType, 
                                            XPL_DM_ALERT_ECHO_T echoType,
                                            XPL_DM_ALERT_TEXTINPUT_RES_T * userResponse )
{
  return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T  XPL_DM_ShowSingleChoiceAlert(INT32 maxDisplayTime, 
                                                CPCHAR msg,
                                                DMStringVector & choices,
                                                INT32 defaultResponse,
                                                XPL_DM_ALERT_SCHOICE_RES_T * userResponse ) 
{
  return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T XPL_DM_ShowMultipleChoiceAlert(INT32 maxDisplayTime,
                                                                  CPCHAR msg, 
                                    				   DMStringVector & choices, 
                                    				   DMStringVector & defaultResponses,
                                    				   XPL_DM_ALERT_MCHOICE_RES_T * userResponse) 
{
  return SYNCML_DM_SUCCESS;
}


