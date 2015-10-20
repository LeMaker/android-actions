/**
  src/omxcore.h

  OpenMAX Integration Layer Core. This library implements the OpenMAX core
  responsible for environment setup, components tunneling and communication.

  Copyright (C) 2007-2009 STMicroelectronics
  Copyright (C) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

*/

#ifndef __ST_OMXCORE_H__
#define __ST_OMXCORE_H__

#include <OMX_Component.h>
#include <OMX_Types.h>
#include <pthread.h>

#ifdef _BOSA_RM_
#include "utils.h"
#include "component_loader.h"
#endif

#include "omx_comp_debug_levels.h"
#ifdef ANDROID_COMPILATION
#include <oscl_base_macros.h>
#else
#define OSCL_IMPORT_REF
#define OSCL_EXPORT_REF
#endif

#ifdef _BOSA_RM_
//forward decl
struct BOSA_COMPONENTLOADER;

OMX_ERRORTYPE BOSA_AddComponentLoader(struct BOSA_COMPONENTLOADER *pLoader);
#endif

typedef struct {
  OMX_VERSIONTYPE componentVersion; /**< the version of the component in the OpenMAX standard format */
  OMX_VERSIONTYPE omxVersion; /**< the version of the component in the OpenMAX standard format */
  char* name; /**< String that represents the name of the component, ruled by the standard */
  char* libname; /**< String that represents the name of the component, ruled by the standard */
  unsigned int name_specific_length;/**< this field contains the number of roles of the component */
  char** name_specific; /**< Strings those represent the names of the specific format components */
  char** role_specific; /**< Strings those represent the names of the specific format components */
  char* entry_func_name;/**< Strings those represent the names of the entry func name of constructor in component */
  OMX_ERRORTYPE (*constructor)(OMX_COMPONENTTYPE*,OMX_STRING cComponentName); /**< constructor function pointer for each Linux ST OpenMAX component */
} ActComponentType;

/** Defines the major version of the core */
#define SPECVERSIONMAJOR  1
/** Defines the minor version of the core */
#define SPECVERSIONMINOR  1
/** Defines the revision of the core */
#define SPECREVISION      0
/** Defines the step version of the core */
#define SPECSTEP          0

#define OMX_CONF_INIT_STRUCT(_s_, _name_)       \
	memset((_s_), 0x0, sizeof(_name_));         \
	(_s_)->nSize = sizeof(_name_);              \
	(_s_)->nVersion.s.nVersionMajor = SPECVERSIONMAJOR;      \
	(_s_)->nVersion.s.nVersionMinor = SPECVERSIONMINOR;      \
	(_s_)->nVersion.s.nRevision = SPECREVISION;       \
	(_s_)->nVersion.s.nStep = SPECSTEP;

#endif
