/**
  @file src/components/ffmpeg/library_entry_point.c
  
  The library entry point. It must have the same name for each
  library of the components loaded by the ST static component loader.
  This function fills the version, the component name and if existing also the roles
  and the specific names for each role. This base function is only an explanation.
  For each library it must be implemented, and it must fill data of any component
  in the library
  
  Copyright (C) 2007-2008 STMicroelectronics
  Copyright (C) 2007-2008 Nokia Corporation and/or its subsidiary(-ies).

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
  
  $Date: 2008-06-27 12:00:23 +0200 (Fri, 27 Jun 2008) $
  Revision $Rev: 554 $
  Author $Author: pankaj_sen $

*/


#include <omxcore.h>
#include <string.h>
#include "omx_actvideodec_component.h"

/** @brief The library entry point. It must have the same name for each
* library of the components loaded by the ST static component loader.
* 
* This function fills the version, the component name and if existing also the roles
* and the specific names for each role. This base function is only an explanation.
* For each library it must be implemented, and it must fill data of any component
* in the library
* 
* @param stComponents pointer to an array of components descriptors.If NULL, the 
* function will return only the number of components contained in the library
* 
* @return number of components contained in the library 
*/
int omx_component_library_Setup(ActComponentType *stComponents) 
{
  int i;
  OMX_ERRORTYPE err = OMX_ErrorNone;

  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for video component\n",__func__);

  if (stComponents == NULL) {
    DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for video component, return code: %d\n",__func__, 1);
    return 1; /* Return Number of Components - one for video source component */
  }

  stComponents[0].componentVersion.s.nVersionMajor = 1;
  stComponents[0].componentVersion.s.nVersionMinor = 1;
  stComponents[0].componentVersion.s.nRevision = 1;
  stComponents[0].componentVersion.s.nStep = 1;

  stComponents[0].omxVersion.s.nVersionMajor = 1;
  stComponents[0].omxVersion.s.nVersionMinor = 1;
  stComponents[0].omxVersion.s.nRevision = 0;
  stComponents[0].omxVersion.s.nStep = 0;

  /* component name */
  stComponents[0].name = calloc(1, OMX_MAX_STRINGNAME_SIZE);
  if (stComponents[0].name == NULL) {
    err = OMX_ErrorInsufficientResources;
    goto ERR_HANDLE;
  }
  strncpy(stComponents[0].name, "OMX.Action.Video.Decoder", OMX_MAX_STRINGNAME_SIZE);
  stComponents[0].name[OMX_MAX_STRINGNAME_SIZE-1] = '\0';

    stComponents[0].libname = calloc(1, OMX_MAX_STRINGNAME_SIZE);
	if (stComponents[0].libname == NULL) {
	 err = OMX_ErrorInsufficientResources;
	 goto ERR_HANDLE;
	}
	strncpy(stComponents[0].libname, "libOMX.Action.Video.Decoder.so", OMX_MAX_STRINGNAME_SIZE);
	stComponents[0].libname[OMX_MAX_STRINGNAME_SIZE-1] = '\0';


  /* specific names */
  stComponents[0].name_specific_length = 1;
  stComponents[0].name_specific = calloc(stComponents[0].name_specific_length, sizeof(char *));
  if (stComponents[0].name_specific == NULL) {
    err = OMX_ErrorInsufficientResources;
    goto ERR_HANDLE;
  }

  for (i=0;i<stComponents[0].name_specific_length;i++) {
    stComponents[0].name_specific[i] = calloc(1, OMX_MAX_STRINGNAME_SIZE);
    if (stComponents[0].name_specific[i] == NULL) {
      err = OMX_ErrorInsufficientResources;
      goto ERR_HANDLE;
    }
  }

  strncpy(stComponents[0].name_specific[0], "OMX.Action.Video.Decoder", OMX_MAX_STRINGNAME_SIZE);
  stComponents[0].name_specific[0][OMX_MAX_STRINGNAME_SIZE-1] = '\0';

  /* component roles */
  stComponents[0].role_specific = calloc(stComponents[0].name_specific_length, sizeof(char *));
  if (stComponents[0].role_specific == NULL) {
    err = OMX_ErrorInsufficientResources;
    goto ERR_HANDLE;
  }

  for (i=0;i<stComponents[0].name_specific_length;i++) {
    stComponents[0].role_specific[i] = calloc(1, OMX_MAX_STRINGNAME_SIZE);
    if (stComponents[0].role_specific[i] == NULL) {
      err = OMX_ErrorInsufficientResources;
      goto ERR_HANDLE;
    }
  }

  strncpy(stComponents[0].role_specific[0], "video_source", OMX_MAX_STRINGNAME_SIZE);
  stComponents[0].role_specific[0][OMX_MAX_STRINGNAME_SIZE-1] = '\0';
  
  /* component constructor */
  stComponents[0].constructor = OMX_ComponentInit;
  
  stComponents[0].entry_func_name = calloc(1, OMX_MAX_STRINGNAME_SIZE);
  if (stComponents[0].entry_func_name == NULL) {
    err = OMX_ErrorInsufficientResources;
    goto ERR_HANDLE;
  }  
  strncpy(stComponents[0].entry_func_name, "OMX_ComponentInit", OMX_MAX_STRINGNAME_SIZE);
  

  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for video component, return code: %d\n",__func__, 1);
  return 1; /* Return Number of Components - one for video source component */
  
  ERR_HANDLE:
  if (stComponents[0].name != NULL) {
    free(stComponents[0].name);
    stComponents[0].name = NULL;
  }

  if (stComponents[0].libname != NULL) {
      free(stComponents[0].libname);
      stComponents[0].libname = NULL;
    }

  if (stComponents[0].name_specific != NULL) {
    for (i=0;i<stComponents[0].name_specific_length;i++) {
      if (stComponents[0].name_specific[i] != NULL) {
        free(stComponents[0].name_specific[i]);
      }
    }
    free(stComponents[0].name_specific);
    stComponents[0].name_specific = NULL;
  }

  if (stComponents[0].role_specific != NULL) {
    for (i=0;i<stComponents[0].name_specific_length;i++) {
      if (stComponents[0].role_specific[i] != NULL) {
        free(stComponents[0].role_specific[i]);
      }
    }
    free(stComponents[0].role_specific);
    stComponents[0].role_specific = NULL;
  }


  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for camera component, return code: 0x%X\n",__func__, err);
  return err;
}
