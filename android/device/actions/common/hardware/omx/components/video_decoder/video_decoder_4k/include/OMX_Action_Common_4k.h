
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/* =============================================================================
*             Texas Instruments OMAP(TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found 
*  in the license agreement under which this software has been supplied.
* =========================================================================== */
/** OMX_TI_Common.h
  *  The LCML header file contains the definitions used by both the
  *  application and the component to access common items.
 */

#ifndef __OMX_ACTION_COMMON_H__
#define __OMX_ACTION_COMMON_H__

#include "OMX_Component.h"

/* OMX_TI_SEVERITYTYPE enumeration is used to indicate severity level of errors
          returned by TI OpenMax components.
    Critical      Requires reboot/reset DSP
    Severe       Have to unload components and free memory and try again
    Major        Can be handled without unloading the component
    Minor        Essentially informational
 */
typedef enum OMX_Action_SEVERITYTYPE {
    OMX_Action_ErrorCritical = 1,
    OMX_Action_ErrorSevere,
    OMX_Action_ErrorMajor,
    OMX_Action_ErrorMinor
} OMX_Action_SEVERITYTYPE;

typedef struct OMX_Action_FILLBUFFERTYPE{
	OMX_BUFFERHEADERTYPE* pBuffHead;
	OMX_U32 BufferNumOnPort;
}OMX_Action_FILLBUFFERTYPE;

#define MAX_PRIVATE_BUFFERS						6
#define NUM_OF_PORTS								2

#define VERSION_MAJOR								1
#define VERSION_MINOR        							0
#define VERSION_REVISION       						0
#define VERSION_STEP                     					0

typedef struct VIDEO_PROFILE_LEVEL
{
    	OMX_S32  nProfile;
    	OMX_S32  nLevel;
} VIDEO_PROFILE_LEVEL_TYPE;



typedef enum VID_CUSTOM_COLORFORMATTYPE
{
	OMX_COLOR_FormatYUV444Planar = (OMX_COLOR_FormatVendorStartUnused + 1),
	OMX_COLOR_FormatYUV224Planar
} VID_CUSTOM_COLORFORMATTYPE;
#define OMX_CONF_CHECK_CMD(_ptr1, _ptr2, _ptr3) \
{                                               \
	if(!_ptr1 || !_ptr2 || !_ptr3){             \
		eError = OMX_ErrorBadParameter;         \
		goto EXIT;                              \
	}                                           \
}

#define OMX_CONF_INIT_STRUCT(_s_, _name_)       \
	memset((_s_), 0x0, sizeof(_name_));         \
	(_s_)->nSize = sizeof(_name_);              \
	(_s_)->nVersion.s.nVersionMajor = VERSION_MAJOR;      \
	(_s_)->nVersion.s.nVersionMinor = VERSION_MINOR;      \
	(_s_)->nVersion.s.nRevision = VERSION_REVISION;       \
	(_s_)->nVersion.s.nStep = VERSION_STEP;   

#define OMX_CONF_SET_ERROR_BAIL(_eError, _eCode)\
{                                               \
	_eError = _eCode;                           \
    	goto EXIT;                                  \
}

#define OMX_CONF_CHK_VERSION(_s_, _name_, _e_)              \
	if((_s_)->nSize != sizeof(_name_)) _e_ = OMX_ErrorBadParameter; \
    	if(((_s_)->nVersion.s.nVersionMajor != VERSION_MAJOR)||         \
       	((_s_)->nVersion.s.nVersionMinor != VERSION_MINOR)||         \
       	((_s_)->nVersion.s.nRevision != VERSION_REVISION)||              \
       	((_s_)->nVersion.s.nStep != VERSION_STEP)) _e_ = OMX_ErrorVersionMismatch;\
    	if(_e_ != OMX_ErrorNone) goto EXIT;


#endif /*  end of  #ifndef __OMX_TI_COMMON_H__ */
/* File EOF */
