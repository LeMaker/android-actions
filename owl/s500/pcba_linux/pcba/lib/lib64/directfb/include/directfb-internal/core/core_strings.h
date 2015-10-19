#ifndef __CORE__CORE_STRINGS_H__
#define __CORE__CORE_STRINGS_H__
#include <core/surface.h>


struct DFBCoreSurfaceTypeFlagsName {
     CoreSurfaceTypeFlags flag;
     const char *name;
};

#define DirectFBCoreSurfaceTypeFlagsNames(Identifier) struct DFBCoreSurfaceTypeFlagsName Identifier[] = { \
     { CSTF_LAYER, "LAYER" }, \
     { CSTF_WINDOW, "WINDOW" }, \
     { CSTF_CURSOR, "CURSOR" }, \
     { CSTF_FONT, "FONT" }, \
     { CSTF_SHARED, "SHARED" }, \
     { CSTF_INTERNAL, "INTERNAL" }, \
     { CSTF_EXTERNAL, "EXTERNAL" }, \
     { CSTF_PREALLOCATED, "PREALLOCATED" }, \
     { CSTF_NONE, "NONE" } \
};

#endif
