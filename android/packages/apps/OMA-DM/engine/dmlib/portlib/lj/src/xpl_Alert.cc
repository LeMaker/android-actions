#include "xpl_dm_ServerAlert.h"
#include <DM_DisplayAlert.h>
#include <DM_ConfirmAlert.h>
#include <DM_SingleChoiceAlert.h>
#include <DM_MultipleChoiceAlert.h>
#include <DM_TextInputAlert.h>



SYNCML_DM_RET_STATUS_T XPL_DM_ShowConfirmAlert(INT32 maxDisplayTime, CPCHAR msg, XPL_DM_ALERT_RES_T defaultResponse, XPL_DM_ALERT_RES_T * responseCode)
{
   return DmShowConfirmAlert(maxDisplayTime, msg, (DM_ALERT_RES_T)defaultResponse, (DM_ALERT_RES_T *)responseCode);
}


SYNCML_DM_RET_STATUS_T XPL_DM_ShowDisplayAlert(INT32 minDisplayTime, CPCHAR msg)
{
    return DmShowDisplayAlert(minDisplayTime, msg);
}


SYNCML_DM_RET_STATUS_T XPL_DM_ShowTextInputAlert(INT32 maxDisplayTime, 
                                            CPCHAR msg, 
                                            CPCHAR defaultResponse,
                                            INT32 maxLength, 
                                            XPL_DM_ALERT_INPUT_T inputType, 
                                            XPL_DM_ALERT_ECHO_T echoType,
                                            XPL_DM_ALERT_TEXTINPUT_RES_T * userResponse )
{
   return DmShowTextInputAlert(maxDisplayTime, 
                               msg, 
                               defaultResponse,
                               maxLength, 
                               (DM_ALERT_INPUT_T)inputType, 
                               (DM_ALERT_ECHO_T)echoType,
                               (DM_ALERT_TEXTINPUT_RESPONSE_T *)userResponse );
}


SYNCML_DM_RET_STATUS_T  XPL_DM_ShowSingleChoiceAlert(INT32 maxDisplayTime, 
                                                CPCHAR msg,
                                                DMStringVector & choices,
                                                INT32 defaultResponse,
                                                XPL_DM_ALERT_SCHOICE_RES_T * userResponse ) 
{
   return  DmShowSingleChoiceAlert(maxDisplayTime, 
                                   msg,
                                   choices,
                                   defaultResponse,
                                   (DM_ALERT_SINGLECHOICE_RESPONSE_T *)userResponse);
}


SYNCML_DM_RET_STATUS_T XPL_DM_ShowMultipleChoiceAlert(INT32 maxDisplayTime,
                                                       CPCHAR msg, 
                                                       DMStringVector & choices, 
                                    		       DMStringVector & defaultResponses,
                                    		       XPL_DM_ALERT_MCHOICE_RES_T * userResponse) 
{

   return DmShowMultipleChoiceAlert(maxDisplayTime,
                                    msg, 
                                    choices, 
                                    defaultResponses,
                                    (DM_ALERT_MULTIPLECHOICE_RESPONSE_T *)userResponse);
}

