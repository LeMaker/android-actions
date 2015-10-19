
#ifndef __CAMERA_ATRACE_TAG__
#define __CAMERA_ATRACE_TAG__

//#define CAMERA_ATRACE

#ifdef CAMERA_ATRACE
#include <utils/Trace.h>

#ifndef CAMERA_ATRACE_TAG 
#define CAMERA_ATRACE_TAG ATRACE_TAG_GRAPHICS
#endif

#define CAMERA_SCOPEDTRACE(name)  \
    ScopedTrace _t(CAMERA_ATRACE_TAG, name)

#else

#define CAMERA_SCOPEDTRACE(name)  

#endif


#endif



