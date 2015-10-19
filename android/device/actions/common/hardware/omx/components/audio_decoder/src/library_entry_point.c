/**
  @file src/components/mad/library_entry_point.c
  
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
  
  $Date$
  Revision $Rev$
  Author $Author$

*/

//#include <st_static_component_loader.h>
#include <omx_maddec_component.h>

/** The library entry point. It must have the same name for each
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
int omx_component_library_Setup(ActComponentType *stComponents) {

  int i;
  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s \n",__func__);
  if (stComponents == NULL) {
    DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s \n",__func__);
    return 1; // Return Number of Component/s
  }
  stComponents[0].componentVersion.s.nVersionMajor = 1; 
  stComponents[0].componentVersion.s.nVersionMinor = 1; 
  stComponents[0].componentVersion.s.nRevision = 1;
  stComponents[0].componentVersion.s.nStep = 1;
  
  stComponents[0].omxVersion.s.nVersionMajor = 1;
  stComponents[0].omxVersion.s.nVersionMinor = 1;
  stComponents[0].omxVersion.s.nRevision = 0;
  stComponents[0].omxVersion.s.nStep = 0;

  stComponents[0].name = calloc(1,OMX_MAX_STRINGNAME_SIZE);
  if (stComponents[0].name == NULL) {
    return OMX_ErrorInsufficientResources;
  }
  strncpy(stComponents[0].name, "OMX.Action.Audio.Decoder", OMX_MAX_STRINGNAME_SIZE);
  
  stComponents[0].libname = calloc(1, OMX_MAX_STRINGNAME_SIZE);
  if (stComponents[0].libname == NULL) {
  	return OMX_ErrorInsufficientResources;
  }
  strncpy(stComponents[0].libname, "libOMX.Action.Audio.Decoder.so", OMX_MAX_STRINGNAME_SIZE);

  stComponents[0].name_specific_length = NUM_ACTIONS_AUDIO_CODECS; 
  stComponents[0].constructor = OMX_ComponentInit;  
  	stComponents[0].entry_func_name = calloc(1, OMX_MAX_STRINGNAME_SIZE);
  if (stComponents[0].entry_func_name == NULL) {
    return OMX_ErrorInsufficientResources;
  }
  strncpy(stComponents[0].entry_func_name, "OMX_ComponentInit", OMX_MAX_STRINGNAME_SIZE);
   
  
  stComponents[0].name_specific = calloc(stComponents[0].name_specific_length,sizeof(char *));  
  
  if(stComponents[0].name_specific == NULL)
  {
    return OMX_ErrorInsufficientResources;	
  }
  
  for(i = 0; i < stComponents[0].name_specific_length; i++)
  {
	stComponents[0].name_specific[i] = calloc(1,OMX_MAX_STRINGNAME_SIZE);
	if (stComponents[0].name_specific[i] == NULL) {
	  return OMX_ErrorInsufficientResources;
	}
  }
  
  stComponents[0].role_specific = calloc(stComponents[0].name_specific_length,sizeof(char *));  
  if(stComponents[0].role_specific == NULL)
  {
    return OMX_ErrorInsufficientResources;
  }
  
  for(i = 0; i < stComponents[0].name_specific_length; i++)
  {
	  stComponents[0].role_specific[i] = calloc(1,OMX_MAX_STRINGNAME_SIZE);
	  if (stComponents[0].role_specific[i] == NULL) {
	    return OMX_ErrorInsufficientResources;
	  }
  }

  strncpy(stComponents[0].name_specific[0], 	AUDIO_DEC_MP3_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[0], 	AUDIO_DEC_MP3_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[1],		AUDIO_DEC_AC3_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[1], 	AUDIO_DEC_AC3_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[2], 	AUDIO_DEC_AAC_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[2], 	AUDIO_DEC_AAC_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[3], 	AUDIO_DEC_AMR_NB_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[3], 	AUDIO_DEC_AMR_NB_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[4], 	AUDIO_DEC_OGG_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[4], 	AUDIO_DEC_OGG_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[5], 	AUDIO_DEC_WMASTD_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[5], 	AUDIO_DEC_WMASTD_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[6], 	AUDIO_DEC_WMALSL_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[6], 	AUDIO_DEC_WMALSL_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[7], 	AUDIO_DEC_WMAPRO_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[7], 	AUDIO_DEC_WMAPRO_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[8], 	AUDIO_DEC_APE_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[8], 	AUDIO_DEC_APE_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[9], 	AUDIO_DEC_FLAC_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[9], 	AUDIO_DEC_FLAC_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[10], 	AUDIO_DEC_RA_ROLE, 		OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[10], 	AUDIO_DEC_RA_ROLE, 		OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[11], 	AUDIO_DEC_PCM_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[11], 	AUDIO_DEC_PCM_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[12], 	AUDIO_DEC_ADPCM_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[12], 	AUDIO_DEC_ADPCM_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[13], 	AUDIO_DEC_DTS_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[13], 	AUDIO_DEC_DTS_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[14], 	AUDIO_DEC_ACELP_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[14], 	AUDIO_DEC_ACELP_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[15], 	AUDIO_DEC_MPC_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[15], 	AUDIO_DEC_MPC_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[16], 	AUDIO_DEC_AIFF_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[16], 	AUDIO_DEC_AIFF_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[17], 	AUDIO_DEC_AMR_WB_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[17], 	AUDIO_DEC_AMR_WB_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[18], 	AUDIO_DEC_MP1_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[18], 	AUDIO_DEC_MP1_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].name_specific[19], 	AUDIO_DEC_MP2_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  strncpy(stComponents[0].role_specific[19], 	AUDIO_DEC_MP2_ROLE, 	OMX_MAX_STRINGNAME_SIZE);
  
  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s \n",__func__);

  return 1;
}
