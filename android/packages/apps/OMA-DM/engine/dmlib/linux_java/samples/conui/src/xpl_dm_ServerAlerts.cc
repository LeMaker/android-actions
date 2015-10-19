#include "xpl_dm_ServerAlert.h"


SYNCML_DM_RET_STATUS_T XPL_DM_ShowConfirmAlert(INT32 maxDisplayTime, CPCHAR msg, XPL_DM_ALERT_RES_T defaultResponse, XPL_DM_ALERT_RES_T * responseCode)
{
  
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

