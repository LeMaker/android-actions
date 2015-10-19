#ifndef __OMX_CAMERA_SOURCE_CAPABILITIES__
#define __OMX_CAMERA_SOURCE_CAPABILITIES__

#include <unistd.h>

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Image.h"
#include "OMX_IVCommon.h"
#include "ACT_OMX_Index.h"
#include "ACT_OMX_IVCommon.h"

#include "omx_base_component.h"

OMX_ERRORTYPE getCamCapabilities(OMX_ACT_CAPTYPE *caps, OMX_U32 id);

#endif
