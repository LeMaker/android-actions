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

#ifndef XPL_LOGGER_H
#define XPL_LOGGER_H

/************** HEADER FILE INCLUDES *****************************************/

#include "xpl_Port.h"

#ifdef __cplusplus
extern "C" {
#endif

/************** CONSTANTS ****************************************************/

#define MAX_LOG_STRING_SIZE   255

#define XPL_LOG_NOLOGS        0
#define XPL_LOG_DEBUG         1
#define XPL_LOG_WARN          2
#define XPL_LOG_ERROR         3
#define XPL_LOG_ALL           4   

// DM: for each XPL_LOG_Startup call there must be a corresponding XPL_LOG_Shutdown call
void XPL_LOG_Startup();
void XPL_LOG_Shutdown();

#if defined(PLATFORM_X86) || defined(EZX_PRODUCT_SCMA11REF)

#if defined(PLATFORM_ANDROID)
  #define LOG_TAG "libdmengine"
  #include <utils/Log.h>

  #ifndef LOGD
  #define LOGD(args...) ALOGD(args)
  #endif

  #ifndef LOGE
  #define LOGE(args...) ALOGE(args)
  #endif

  #ifndef LOGI
  #define LOGI(args...) ALOGI(args)
  #endif

  #ifndef LOGV
  #define LOGV(args...) ALOGV(args)
  #endif

  #ifndef LOGW
  #define LOGW(args...) ALOGW(args)
  #endif

  #define XPL_LOG_Error(logPortId, format, args...) \
    LOGE(format, ##args )

  #define XPL_LOG_Warn(logPortId,  format, args...) \
    LOGW(format, ##args )
    
  #define XPL_LOG_Debug(logPortId, format, args...) \
    LOGD(format, ##args )
    
  #define XPL_LOG_Enter(logPortId, format, args...) \
    LOGD(format, ##args )

  #define XPL_LOG_Leave(logPortId, format, args...) \
    LOGD(format, ##args )

  #define XPL_LOG_Startup()
  #define XPL_LOG_Shutdown()

  // define XPL_LOG_LEVEL in makefile; don't define it here.
//  #define XPL_LOG_LEVEL XPL_LOG_ALL  

  // DM: use DM log if building standalone DM engine on Redhat linux machine
#elif ( XPL_LOG_LEVEL == XPL_LOG_DEBUG ) || ( XPL_LOG_LEVEL == XPL_LOG_WARN ) || ( XPL_LOG_LEVEL == XPL_LOG_ERROR ) || ( XPL_LOG_LEVEL == XPL_LOG_ALL )
    void XPL_LOG_Error(XPL_LOGS_PORT_T logPort, CPCHAR format, ...);
    void XPL_LOG_Warn(XPL_LOGS_PORT_T  logPort, CPCHAR format, ...);
    void XPL_LOG_Debug(XPL_LOGS_PORT_T logPort, CPCHAR format, ...);
    void XPL_LOG_Enter(XPL_LOGS_PORT_T logPort, CPCHAR format, ...);
    void XPL_LOG_Leave(XPL_LOGS_PORT_T logPort, CPCHAR format, ...);
#endif

#else 
  // DM: use LJ system log if this is a LJ phone build
  #include <libaplog.h>

    struct aplog_port_struct* XPL_GetPortPtr( XPL_LOGS_PORT_T logPort );

  #define XPL_LOG_Error(logPortId, format, args...) \
    aplog_port_err_fileline( XPL_GetPortPtr( logPortId ), format, ##args )

  #define XPL_LOG_Warn(logPortId,  format, args...) \
    aplog_port_warning_fileline( XPL_GetPortPtr( logPortId ), format, ##args )
    
  #define XPL_LOG_Debug(logPortId, format, args...) \
    aplog_port_debug_fileline( XPL_GetPortPtr( logPortId ), format, ##args )
    
  #define XPL_LOG_Enter(logPortId, format, args...) \
    aplog_port_func_enter_print( XPL_GetPortPtr( logPortId ), format, ##args )

  #define XPL_LOG_Leave(logPortId, format, args...) \
    aplog_port_func_exit_print( XPL_GetPortPtr( logPortId ), format, ##args )

  #define XPL_LOG_LEVEL XPL_LOG_ALL      
#endif

#if ( XPL_LOG_LEVEL == XPL_LOG_DEBUG ) || ( XPL_LOG_LEVEL == XPL_LOG_WARN ) || ( XPL_LOG_LEVEL == XPL_LOG_ERROR ) || ( XPL_LOG_LEVEL == XPL_LOG_ALL )
  #define XPL_LOG_DM_API_Error2( args... )   XPL_LOG_Error(XPL_LOG_PORT_DM_API,   args)
  #define XPL_LOG_DM_PLG_Error2( args... )   XPL_LOG_Error(XPL_LOG_PORT_DM_PLG,   args)
  #define XPL_LOG_DM_TMN_Error2( args... )   XPL_LOG_Error(XPL_LOG_PORT_DM_TMN,   args)
  #define XPL_LOG_DM_SESS_Error2( args... )  XPL_LOG_Error(XPL_LOG_PORT_DM_SESS,  args)
  #define XPL_LOG_DM_CONN_Error2( args... )  XPL_LOG_Error(XPL_LOG_PORT_DM_CONN,  args)
  #define XPL_LOG_DM_XPL_Error2( args... )   XPL_LOG_Error(XPL_LOG_PORT_DM_XPL,   args)
  #define XPL_LOG_DM_CP_Error2( args... )    XPL_LOG_Error(XPL_LOG_PORT_DM_CP,    args)

  #define XPL_LOG_DM_API_Error( args... )    XPL_LOG_DM_API_Error2 args
  #define XPL_LOG_DM_PLG_Error( args... )    XPL_LOG_DM_PLG_Error2 args
  #define XPL_LOG_DM_TMN_Error( args... )    XPL_LOG_DM_TMN_Error2 args
  #define XPL_LOG_DM_SESS_Error( args... )   XPL_LOG_DM_SESS_Error2 args
  #define XPL_LOG_DM_CONN_Error( args... )   XPL_LOG_DM_CONN_Error2 args
  #define XPL_LOG_DM_XPL_Error( args... )    XPL_LOG_DM_XPL_Error2 args 
  #define XPL_LOG_DM_CP_Error( args... )     XPL_LOG_DM_CP_Error2 args
#else
  #define XPL_LOG_DM_API_Error( args... ) 
  #define XPL_LOG_DM_PLG_Error( args... )
  #define XPL_LOG_DM_TMN_Error( args... ) 
  #define XPL_LOG_DM_SESS_Error( args... ) 
  #define XPL_LOG_DM_CONN_Error( args... ) 
  #define XPL_LOG_DM_XPL_Error( args... ) 
  #define XPL_LOG_DM_CP_Error( args... ) 
#endif

#if ( XPL_LOG_LEVEL == XPL_LOG_DEBUG ) || ( XPL_LOG_LEVEL == XPL_LOG_WARN ) || ( XPL_LOG_LEVEL == XPL_LOG_ALL )
  #define XPL_LOG_DM_API_Warn2( args... )  XPL_LOG_Warn(XPL_LOG_PORT_DM_API,   args)
  #define XPL_LOG_DM_PLG_Warn2( args... )  XPL_LOG_Warn(XPL_LOG_PORT_DM_PLG,   args)
  #define XPL_LOG_DM_TMN_Warn2( args... )  XPL_LOG_Warn(XPL_LOG_PORT_DM_TMN,   args)
  #define XPL_LOG_DM_SESS_Warn2( args... ) XPL_LOG_Warn(XPL_LOG_PORT_DM_SESS,  args)
  #define XPL_LOG_DM_CONN_Warn2( args... ) XPL_LOG_Warn(XPL_LOG_PORT_DM_CONN,  args)
  #define XPL_LOG_DM_XPL_Warn2( args... )  XPL_LOG_Warn(XPL_LOG_PORT_DM_XPL,   args)
  #define XPL_LOG_DM_CP_Warn2( args... )   XPL_LOG_Warn(XPL_LOG_PORT_DM_CP,    args)

  #define XPL_LOG_DM_API_Warn( args... )   XPL_LOG_DM_API_Warn2 args
  #define XPL_LOG_DM_PLG_Warn( args... )   XPL_LOG_DM_PLG_Warn2 args
  #define XPL_LOG_DM_TMN_Warn( args... )   XPL_LOG_DM_TMN_Warn2 args
  #define XPL_LOG_DM_SESS_Warn( args... )  XPL_LOG_DM_SESS_Warn2 args
  #define XPL_LOG_DM_CONN_Warn( args... )  XPL_LOG_DM_CONN_Warn2 args
  #define XPL_LOG_DM_XPL_Warn( args... )   XPL_LOG_DM_XPL_Warn2 args
  #define XPL_LOG_DM_CP_Warn( args... )    XPL_LOG_DM_CP_Warn2 args
#else
  #define XPL_LOG_DM_API_Warn( args... ) 
  #define XPL_LOG_DM_PLG_Warn( args... ) 
  #define XPL_LOG_DM_TMN_Warn( args... ) 
  #define XPL_LOG_DM_SESS_Warn( args... ) 
  #define XPL_LOG_DM_CONN_Warn( args... ) 
  #define XPL_LOG_DM_XPL_Warn( args... ) 
  #define XPL_LOG_DM_CP_Warn( args... )
#endif

#if ( XPL_LOG_LEVEL == XPL_LOG_DEBUG ) || ( XPL_LOG_LEVEL == XPL_LOG_ALL )
  #define XPL_LOG_DM_API_Debug2( args... )   XPL_LOG_Debug(XPL_LOG_PORT_DM_API,  args)
  #define XPL_LOG_DM_PLG_Debug2( args... )   XPL_LOG_Debug(XPL_LOG_PORT_DM_PLG,  args)
  #define XPL_LOG_DM_TMN_Debug2( args... )   XPL_LOG_Debug(XPL_LOG_PORT_DM_TMN,  args)
  #define XPL_LOG_DM_SESS_Debug2( args... )  XPL_LOG_Debug(XPL_LOG_PORT_DM_SESS, args)
  #define XPL_LOG_DM_CONN_Debug2( args... )  XPL_LOG_Debug(XPL_LOG_PORT_DM_CONN, args)
  #define XPL_LOG_DM_XPL_Debug2( args... )   XPL_LOG_Debug(XPL_LOG_PORT_DM_XPL,  args)
  #define XPL_LOG_DM_CP_Debug2( args... )    XPL_LOG_Debug(XPL_LOG_PORT_DM_CP,   args)

  #define XPL_LOG_DM_API_Enter2( args... )   XPL_LOG_Enter(XPL_LOG_PORT_DM_API,  args)
  #define XPL_LOG_DM_PLG_Enter2( args... )   XPL_LOG_Enter(XPL_LOG_PORT_DM_PLG,  args)
  #define XPL_LOG_DM_TMN_Enter2( args... )   XPL_LOG_Enter(XPL_LOG_PORT_DM_TMN,  args)
  #define XPL_LOG_DM_SESS_Enter2( args... )  XPL_LOG_Enter(XPL_LOG_PORT_DM_SESS, args)
  #define XPL_LOG_DM_CONN_Enter2( args... )  XPL_LOG_Enter(XPL_LOG_PORT_DM_CONN, args)
  #define XPL_LOG_DM_XPL_Enter2( args... )   XPL_LOG_Enter(XPL_LOG_PORT_DM_XPL,  args)
  #define XPL_LOG_DM_CP_Enter2( args... )    XPL_LOG_Enter(XPL_LOG_PORT_DM_CP,   args)

  #define XPL_LOG_DM_API_Leave2( args... )   XPL_LOG_Leave(XPL_LOG_PORT_DM_API,  args)
  #define XPL_LOG_DM_PLG_Leave2( args... )   XPL_LOG_Leave(XPL_LOG_PORT_DM_PLG,  args)
  #define XPL_LOG_DM_TMN_Leave2( args... )   XPL_LOG_Leave(XPL_LOG_PORT_DM_TMN,  args)
  #define XPL_LOG_DM_SESS_Leave2( args... )  XPL_LOG_Leave(XPL_LOG_PORT_DM_SESS, args)
  #define XPL_LOG_DM_CONN_Leave2( args... )  XPL_LOG_Leave(XPL_LOG_PORT_DM_CONN, args)
  #define XPL_LOG_DM_XPL_Leave2( args... )   XPL_LOG_Leave(XPL_LOG_PORT_DM_XPL,  args)
  #define XPL_LOG_DM_CP_Leave2( args... )    XPL_LOG_Leave(XPL_LOG_PORT_DM_CP,   args)

  #define XPL_LOG_DM_API_Debug( args... )   XPL_LOG_DM_API_Debug2 args
  #define XPL_LOG_DM_PLG_Debug( args... )   XPL_LOG_DM_PLG_Debug2 args
  #define XPL_LOG_DM_TMN_Debug( args... )   XPL_LOG_DM_TMN_Debug2 args 
  #define XPL_LOG_DM_SESS_Debug( args... )  XPL_LOG_DM_SESS_Debug2 args
  #define XPL_LOG_DM_CONN_Debug( args... )  XPL_LOG_DM_CONN_Debug2 args
  #define XPL_LOG_DM_XPL_Debug( args... )   XPL_LOG_DM_XPL_Debug2 args 
  #define XPL_LOG_DM_CP_Debug( args... )    XPL_LOG_DM_CP_Debug2 args  

  #define XPL_LOG_DM_API_Enter( args... )   XPL_LOG_DM_API_Enter2 args 
  #define XPL_LOG_DM_PLG_Enter( args... )   XPL_LOG_DM_PLG_Enter2 args 
  #define XPL_LOG_DM_TMN_Enter( args... )   XPL_LOG_DM_TMN_Enter2 args 
  #define XPL_LOG_DM_SESS_Enter( args... )  XPL_LOG_DM_SESS_Enter2 args
  #define XPL_LOG_DM_CONN_Enter( args... )  XPL_LOG_DM_CONN_Enter2 args
  #define XPL_LOG_DM_XPL_Enter( args... )   XPL_LOG_DM_XPL_Enter2 args 
  #define XPL_LOG_DM_CP_Enter( args... )    XPL_LOG_DM_CP_Enter2 args  

  #define XPL_LOG_DM_API_Leave( args... )   XPL_LOG_DM_API_Leave2 args 
  #define XPL_LOG_DM_PLG_Leave( args... )   XPL_LOG_DM_PLG_Leave2 args 
  #define XPL_LOG_DM_TMN_Leave( args... )   XPL_LOG_DM_TMN_Leave2 args 
  #define XPL_LOG_DM_SESS_Leave( args... )  XPL_LOG_DM_SESS_Leave2 args
  #define XPL_LOG_DM_CONN_Leave( args... )  XPL_LOG_DM_CONN_Leave2 args
  #define XPL_LOG_DM_XPL_Leave( args... )   XPL_LOG_DM_XPL_Leave2 args 
  #define XPL_LOG_DM_CP_Leave( args... )    XPL_LOG_DM_CP_Leave2 args  
#else 
  #define XPL_LOG_DM_API_Debug( args... ) 
  #define XPL_LOG_DM_PLG_Debug( args... ) 
  #define XPL_LOG_DM_TMN_Debug( args... )
  #define XPL_LOG_DM_SESS_Debug( args... ) 
  #define XPL_LOG_DM_CONN_Debug( args... ) 
  #define XPL_LOG_DM_XPL_Debug( args... ) 
  #define XPL_LOG_DM_CP_Debug( args... ) 

  #define XPL_LOG_DM_API_Enter( args... ) 
  #define XPL_LOG_DM_PLG_Enter( args... ) 
  #define XPL_LOG_DM_TMN_Enter( args... )
  #define XPL_LOG_DM_SESS_Enter( args... ) 
  #define XPL_LOG_DM_CONN_Enter( args... ) 
  #define XPL_LOG_DM_XPL_Enter( args... ) 
  #define XPL_LOG_DM_CP_Enter( args... ) 

  #define XPL_LOG_DM_API_Leave( args... ) 
  #define XPL_LOG_DM_PLG_Leave( args... ) 
  #define XPL_LOG_DM_TMN_Leave( args... )
  #define XPL_LOG_DM_SESS_Leave( args... ) 
  #define XPL_LOG_DM_CONN_Leave( args... ) 
  #define XPL_LOG_DM_XPL_Leave( args... ) 
  #define XPL_LOG_DM_CP_Leave( args... ) 
#endif

#ifdef __cplusplus
}
#endif

#endif /* XPL_LOGGER_H */
