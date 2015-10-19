#ifndef __LOG_H__
#define __LOG_H__

#define __ANDROID_LOG_
#ifdef __ANDROID_LOG_

#undef LOG_TAG
#define LOG_TAG "OMX_CAM"

#ifdef ANDROID
#include "utils/Log.h"
#ifndef printf
#define printf ALOGD
#endif
#endif


#else
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#endif



#define V4L2DBUG_LEVEL 3
#define V4L2DBUG_VERB 4
#define V4L2DBUG_ERR 1
#define V4L2DBUG_PARAM 2

#define OMXDBUG_LEVEL 3
#define OMXDBUG_VERB 4
#define OMXDBUG_ERR 1
#define OMXDBUG_PARAM 2


//#define OMXCAM_INCR_PRINT
#ifndef OMXCAM_INCR_PRINT

#if V4L2DBUG_LEVEL > 0
#define V4L2DBUG(n, fmt, args...)\
	do {\
		if ((n&V4L2DBUG_LEVEL)==V4L2DBUG_ERR){\
			ALOGE("V4l2-" fmt, ##args);\
		}\
		else if((n&V4L2DBUG_LEVEL)==V4L2DBUG_PARAM){\
			ALOGD("V4l2-" fmt, ##args);\
		}\
		else{\
			ALOGV("V4l2-" fmt, ##args);\
		}\
	} while (0)
#else
#define V4L2DBUG(n, fmt, args...) {}
#endif

#if OMXDBUG_LEVEL > 0
#define OMXDBUG(n, fmt, args...)\
	do {\
		if ((n&OMXDBUG_LEVEL)==OMXDBUG_ERR){\
			ALOGE("OMX-" fmt, ##args);\
		}\
		else if((n&OMXDBUG_LEVEL)==OMXDBUG_PARAM){\
			ALOGD("OMX-" fmt, ##args);\
		}\
		else{\
			ALOGV("OMX-" fmt, ##args);\
		}\
	} while (0)
#else
#define OMXDBUG(n, fmt, args...) {}
#endif

#else // #ifndef OMXCAM_INCR_PRINT

#if V4L2DBUG_LEVEL > 0
#define V4L2DBUG(n, fmt, args...)\
	do {\
		if ((n&V4L2DBUG_LEVEL)==V4L2DBUG_ERR){\
			ALOGE("V4l2-%s,%d: " fmt, __func__, __LINE__, ##args);\
		}\
		else if((n&V4L2DBUG_LEVEL)==V4L2DBUG_PARAM){\
			ALOGD("V4l2-%s,%d: " fmt, __func__, __LINE__, ##args);\
		}\
		else{\
			ALOGD("V4l2-%s,%d: " fmt, __func__, __LINE__, ##args);\
		}\
	} while (0)
#else
#define V4L2DBUG(n, fmt, args...) {}
#endif

#if OMXDBUG_LEVEL > 0
#define OMXDBUG(n, fmt, args...)\
	do {\
		if ((n&OMXDBUG_LEVEL)==OMXDBUG_ERR){\
			ALOGE("OMX-%s,%d: " fmt, __func__, __LINE__, ##args);\
		}\
		else if((n&OMXDBUG_LEVEL)==OMXDBUG_PARAM){\
			ALOGD("OMX-%s,%d: " fmt, __func__, __LINE__, ##args);\
		}\
		else{\
			ALOGD("OMX-%s,%d: " fmt, __func__, __LINE__, ##args);\
		}\
	} while (0)
#else
#define OMXDBUG(n, fmt, args...) {}
#endif


#endif //#ifndef OMXCAM_INCR_PRINT


#endif
