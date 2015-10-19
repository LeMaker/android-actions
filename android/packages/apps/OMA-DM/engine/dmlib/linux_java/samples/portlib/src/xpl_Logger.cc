#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

#include "xpl_Logger.h"

#ifdef __cplusplus  
extern "C" {
#endif

static int logIndex;
static bool logOpened;

static const char *const logName[] = {
    "DM_API",    
    "DM_TMN",  
    "DM_PLG",  
    "DM_SESS",    
    "DM_CONN",    
    "DM_XPL",
    "DM_CP"
};

static void _log(int level, CPCHAR format, va_list ap);



void XPL_LOG_Startup()
{
}

void XPL_LOG_Shutdown()
{
}

void XPL_LOG_Debug(XPL_LOGS_PORT_T logPort,CPCHAR format, ...) 
{
    va_list ap;
    va_start(ap, format);
     _log(LOG_DEBUG, format, ap);
    va_end(ap);    
}

void XPL_LOG_Warn(XPL_LOGS_PORT_T logPort,CPCHAR format, ...) 
{
    va_list ap;
    va_start(ap, format);
    _log(LOG_WARNING, format, ap);
    va_end(ap);
}

void XPL_LOG_Error(XPL_LOGS_PORT_T logPort,CPCHAR format, ...) 
{
    va_list ap;
    va_start(ap, format);
    _log(LOG_ERR, format, ap);
    va_end(ap);
}


void XPL_LOG_Add(XPL_PORT_T logPort)
{
  logIndex = logPort;
}

void _log(int level, CPCHAR format, va_list ap) {
    vprintf(format, ap);
}

#ifdef __cplusplus  
}
#endif
