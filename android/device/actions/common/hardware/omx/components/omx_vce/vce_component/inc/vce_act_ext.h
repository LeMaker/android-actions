#ifndef __VCE_ACT_EXT_H__
#define __VCE_ACT_EXT_H__

#if 1
/*ÔÚomxcore.h¶¨Òå*/
/** Defines the major version of the core */
#ifndef SPECVERSIONMAJOR
#define SPECVERSIONMAJOR  1
#endif

/** Defines the minor version of the core */
#ifndef SPECVERSIONMINOR
#define SPECVERSIONMINOR  1
#endif

/** Defines the revision of the core */
#ifndef SPECREVISION
#define SPECREVISION      0
#endif

/** Defines the step version of the core */
#ifndef SPECSTEP
#define SPECSTEP          0
#endif

#endif

#define OMX_PARAMETER_INIT_STRUCT(_s_, _name_,_nPortIndex_)       \
	memset((_s_), 0x0, sizeof(_name_));         \
	(_s_)->nSize = sizeof(_name_);              \
	(_s_)->nVersion.s.nVersionMajor = SPECVERSIONMAJOR;      \
	(_s_)->nVersion.s.nVersionMinor = SPECVERSIONMINOR;      \
	(_s_)->nVersion.s.nRevision = SPECREVISION;       \
	(_s_)->nVersion.s.nStep = SPECSTEP;     \
    (_s_)->nPortIndex = _nPortIndex_;

#endif
