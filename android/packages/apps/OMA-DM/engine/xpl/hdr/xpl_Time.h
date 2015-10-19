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

#ifndef XPL_TIME_H
#define XPL_TIME_H

#ifdef __cplusplus
extern "C" {
#endif
/************** HEADER FILE INCLUDES *****************************************/
#include "xpl_Port.h"

/************** CONSTANTS ****************************************************/
#define XPL_CLK_INVALID_CLOCK  (0xffffffff)
#define XPL_CLK_HANDLE_INVALID (-1)


#define XPL_DM_TIMER_CATEGORY  0


/************** STRUCTURES, ENUMS, AND TYPEDEFS ******************************/
typedef UINT32 XPL_CLK_CLOCK_T;
typedef UINT64 XPL_CLK_LONG_CLOCK_T;
typedef INT32 XPL_TIMER_HANDLE_T;

typedef struct
{
    INT32 tm_sec;   /* valid range for seconds is from 0 to 61.*/
    INT32 tm_min;   /* minute of the hour. Valid range is from 0 to 59. */
    INT32 tm_hour;  /* hour of the day. Valid range is from 0 to 23. */

    INT32 tm_mday; /* day of the month.  Valid range is from 1 to 31. */
    INT32 tm_mon;  /* month of the year. Valid range is from 0 to 11. */
    INT32 tm_year; /* the year since 1900 */
    INT32 tm_wday; /* days since Sunday. Valid range is 0 to 6. */
    INT32 tm_yday; /* days since January 1.  Valid range is 0 to 365 */
    INT32 tm_isdst;/* flag for alternate daylight savings time.
                      1 indicates DST active, 0 indicates DST inactive */
															 
} XPL_CLK_TM_T;


enum
{
     XPL_CLK_RET_SUCCESS  = 0,     /* operation successfully completed */
     XPL_CLK_RET_FAIL = 1,         /* operation failed */
     XPL_CLK_RET_BADARGUMENT = 2   /* bad argument */
};
typedef UINT8  XPL_CLK_RET_STATUS_T;

typedef void (*XPL_CLK_TIMER_CBACK)(void);

/*=================================================================================
                                     FUNCTION PROTOTYPES
===================================================================================*/

/* Returns the value of time in seconds since the Epoch */
XPL_CLK_CLOCK_T XPL_CLK_GetClock();

/* Returns the value of time in microseconds since Epoch */
XPL_CLK_LONG_CLOCK_T XPL_CLK_GetClockMs();

/* Returns a broken-down time expressed as
* Coordinated Universal Time (UTC). The broken-down time is stored in the 
* structure referred to by parsed_clock. */ 
XPL_CLK_RET_STATUS_T XPL_CLK_GetTime(XPL_CLK_TM_T *parsed_time);

/*Converts the calendar time pointed to by clock into a broken-down time expressed as
* Coordinated Universal Time (UTC). The broken-down time is stored in the 
  structure referred to by parsed_clock. */ 
XPL_CLK_RET_STATUS_T XPL_CLK_UnpackTime(XPL_CLK_CLOCK_T clock, XPL_CLK_TM_T *parsed_clock);

/* Converts the broken-down time, expressed as local time, in the structure pointed
   to by parsed_clock, into a time since the Epoch value with the same encoding as that
   of the values returned by XPL_CLK_Time(). */
XPL_CLK_CLOCK_T XPL_CLK_PackTime(XPL_CLK_TM_T *parsed_clock);

/* Returs timezone */
INT32 XPL_CLK_GetTimeZone(void);

/* Starts timer */
XPL_TIMER_HANDLE_T XPL_CLK_StartTimer(XPL_PORT_T port, UINT32 interval, XPL_CLK_TIMER_CBACK reply_timer);

/* Stops timer */
XPL_CLK_RET_STATUS_T XPL_CLK_StopTimer(XPL_TIMER_HANDLE_T handle);


#ifdef __cplusplus
}
#endif

#endif /* XPL_TIME_H */
