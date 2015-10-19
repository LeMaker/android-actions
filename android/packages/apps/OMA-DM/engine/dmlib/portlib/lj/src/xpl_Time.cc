#include <string.h>
#include <time.h>
#include <sys/time.h> 
#include "xpl_Time.h"

#ifdef __cplusplus  
extern "C" {
#endif

XPL_CLK_CLOCK_T XPL_CLK_GetClock()
{
    return (XPL_CLK_CLOCK_T)time(NULL);
}

XPL_CLK_LONG_CLOCK_T XPL_CLK_GetClockMs()
{
    struct timeval tv1;
    gettimeofday( &tv1, NULL );
    return (XPL_CLK_LONG_CLOCK_T)(tv1.tv_usec + (tv1.tv_sec * 1000000 ));
}


XPL_CLK_RET_STATUS_T XPL_CLK_GetTime(XPL_CLK_TM_T *parsed_time)
{
    return XPL_CLK_RET_SUCCESS;
}

XPL_CLK_RET_STATUS_T XPL_CLK_UnpackTime(XPL_CLK_CLOCK_T clock, XPL_CLK_TM_T *parsed_clock)
{
    struct tm t;
    time_t tt = clock;
    if ( gmtime_r(&tt, &t) != NULL )
    {
        parsed_clock->tm_sec = t.tm_sec; 
        parsed_clock->tm_min = t.tm_min;
        parsed_clock->tm_hour = t.tm_hour;
        parsed_clock->tm_mday = t.tm_mday;
        parsed_clock->tm_mon = t.tm_mon;
        parsed_clock->tm_year = t.tm_year;
        parsed_clock->tm_wday = t.tm_wday;
        parsed_clock->tm_yday = t.tm_yday;
        parsed_clock->tm_isdst = t.tm_isdst;
        return XPL_CLK_RET_SUCCESS;   
    }
    return XPL_CLK_RET_FAIL;  
    
}       

XPL_CLK_CLOCK_T XPL_CLK_PackTime(XPL_CLK_TM_T *parsed_clock)
{

    struct tm t;
    memset(&t, 0, sizeof(struct tm));
    t.tm_sec = parsed_clock->tm_sec; 
       t.tm_min = parsed_clock->tm_min;
    t.tm_hour = parsed_clock->tm_hour;
    t.tm_mday = parsed_clock->tm_mday;
    t.tm_mon = parsed_clock->tm_mon;
    t.tm_year = parsed_clock->tm_year;
    t.tm_wday = parsed_clock->tm_wday;
    t.tm_yday = parsed_clock->tm_yday;
    t.tm_isdst = parsed_clock->tm_isdst;
 
    return (XPL_CLK_CLOCK_T)mktime(&t);
}

INT32 XPL_CLK_GetTimeZone(void)
{
    tzset();
    return (INT32)timezone;    
}


/* Starts timer */
XPL_TIMER_HANDLE_T XPL_CLK_StartTimer(XPL_PORT_T port, UINT32 interval, XPL_CLK_TIMER_CBACK reply_timer)
{
    return XPL_CLK_HANDLE_INVALID; 
}

/* Stops timer */
XPL_CLK_RET_STATUS_T XPL_CLK_StopTimer(XPL_TIMER_HANDLE_T handle)
{
    return XPL_CLK_RET_SUCCESS; 
}

#ifdef __cplusplus  
}
#endif

