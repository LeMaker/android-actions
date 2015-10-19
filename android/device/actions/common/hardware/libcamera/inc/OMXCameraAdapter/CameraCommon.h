#ifndef CAMERA_COMMON_H
#define CAMERA_COMMON_H
namespace android
{

#define Q16_OFFSET                  16

#define OMX_CMD_TIMEOUT             5000000  //3 sec.
#define OMX_CAPTURE_TIMEOUT         5000000  //5 sec.

#define FOCUS_THRESHOLD             5 //[s.]

#define MIN_JPEG_QUALITY            1
#define MAX_JPEG_QUALITY            100
#define EXP_BRACKET_RANGE           10

#define FOCUS_DIST_SIZE             100
#define FOCUS_DIST_BUFFER_SIZE      500

#define TOUCH_DATA_SIZE             200

#define FRAME_RATE_FULL_HD          27

#define FACE_DETECTION_BUFFER_SIZE  0x1000
#define MAX_NUM_FACES_SUPPORTED     35


#define SIZE_ALIGN_UP(s,order) (((s)+(1<<(order))-1)&(~((1<<(order)) - 1)))    
#define SIZE_ALIGN_DOWN(s,order) ((s)&(~((1<<(order)) - 1)))    

#define SIZE_ALIGN_UP_16(s)  SIZE_ALIGN_UP(s,4)
#define SIZE_ALIGN_DOWN_16(s)  SIZE_ALIGN_DOWN(s,4)
#define SIZE_ALIGN_UP_32(s)  SIZE_ALIGN_UP(s,5)
#define SIZE_ALIGN_DOWN_32(s)  SIZE_ALIGN_DOWN(s,5)


/** Defines the major version of the core */
#define SPECVERSIONMAJOR  1
/** Defines the minor version of the core */
#define SPECVERSIONMINOR  1
/** Defines the revision of the core */
#define SPECREVISION      2
/** Defines the step version of the core */
#define SPECSTEP          0

#define OMX_INIT_STRUCT(_s_, _name_)        \
    memset(&(_s_), 0x0, sizeof(_name_));    \
    (_s_).nSize = sizeof(_name_);           \
    (_s_).nVersion.s.nVersionMajor = SPECVERSIONMAJOR;    \
    (_s_).nVersion.s.nVersionMinor = SPECVERSIONMINOR;    \
    (_s_).nVersion.s.nRevision = SPECREVISION;        \
    (_s_).nVersion.s.nStep = SPECSTEP

#define OMX_INIT_STRUCT_PTR(_s_, _name_)   \
    memset((_s_), 0x0, sizeof(_name_));         \
    (_s_)->nSize = sizeof(_name_);              \
    (_s_)->nVersion.s.nVersionMajor = SPECVERSIONMAJOR;      \
    (_s_)->nVersion.s.nVersionMinor = SPECVERSIONMINOR;      \
    (_s_)->nVersion.s.nRevision = SPECREVISION;          \
    (_s_)->nVersion.s.nStep = SPECSTEP

#define GOTO_EXIT_IF(_CONDITION,_ERROR) {                                       \
    if ((_CONDITION)) {                                                         \
        eError = (_ERROR);                                                      \
        goto EXIT;                                                              \
    }                                                                           \
}

/*
#define OMX_Init        ActionOMX_Init
#define OMX_Deinit      ActionOMX_Deinit
#define OMX_ComponentNameEnum   ActionOMX_ComponentNameEnum
#define OMX_GetHandle   ActionOMX_GetHandle
#define OMX_FreeHandle  ActionOMX_FreeHandle
#define OMX_SetupTunnel ActionOMX_SetupTunnel
*/

};
#endif
