/**
  @file test/components/audio/omxaudiodectest.c

  Test application that uses three OpenMAX components, a file reader, an audio decoder 
  and an ALSA sink. The application receives an compressed audio stream on input port
  from a file, decodes it and sends it to the ALSA sink, or to a file or standard output.
  The audio formats supported are:
  MP3 (FFmpeg)

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

#include <dlfcn.h>
#include "omxaudiodectest.h"
#include "libparser.h"

#define MP3_TYPE_SEL    1
#define VORBIS_TYPE_SEL 2
#define AAC_TYPE_SEL    3 
#define G726_TYPE_SEL   4 
#define AMR_TYPE_SEL    5 
#define COMPONENT_NAME_BASE "OMX.Action.Audio.Decoder"
#define BASE_ROLE "audio_decoder.mp3"
#define COMPONENT_NAME_BASE_LEN 20
#define SINK_NAME "OMX.st.alsa.alsasink"
#define FILE_READER "OMX.st.audio_filereader"
#define AUDIO_EFFECT "OMX.st.volume.component"
#define extradata_size 1024

/*
xieyihao 2012-6-26 9:52:38
macros to indentify whether use or not use the Actions specificated demuxer/parser
if working with third party parser/demuxer, pls uncomment this macros
*/
#define WITH_ACTIONS_PARSER


appPrivateType* appPriv;

OMX_BUFFERHEADERTYPE *outBufferFileRead[2];
OMX_BUFFERHEADERTYPE *inBufferAudioDec[2],*outBufferAudioDec[2];
OMX_BUFFERHEADERTYPE *inBufferVolume[2],*outBufferVolume[2];
OMX_BUFFERHEADERTYPE *inBufferSink[2];
int buffer_in_size = BUFFER_IN_SIZE; 
int buffer_out_size = BUFFER_OUT_SIZE;
static OMX_BOOL bEOS=OMX_FALSE;

OMX_CALLBACKTYPE audiodeccallbacks = { 
  .EventHandler    = audiodecEventHandler,
  .EmptyBufferDone = audiodecEmptyBufferDone,
  .FillBufferDone  = audiodecFillBufferDone
};



OMX_ERRORTYPE (*OMX_Init)();
OMX_ERRORTYPE (*OMX_Deinit)();
OMX_ERRORTYPE (*OMX_GetHandle)(OMX_HANDLETYPE *, OMX_STRING, OMX_PTR, OMX_CALLBACKTYPE *);
OMX_ERRORTYPE (*OMX_FreeHandle)(OMX_HANDLETYPE *);
OMX_ERRORTYPE (*OMX_ComponentNameEnum)(OMX_STRING, OMX_U32,	OMX_U32);
OMX_ERRORTYPE (*OMX_GetRolesOfComponent) (OMX_STRING , OMX_U32*, OMX_U8 **);
OMX_ERRORTYPE (*OMX_GetComponentsOfRole)( OMX_STRING role, OMX_U32 *,OMX_U8  **);

FILE *fd ,*outfile;
char *input_file, *output_file;
int selectedType = 0;
int seek_time, seek_start_time, seek_flag = OMX_FALSE, frame_cnt = 0;

static void setHeader(OMX_PTR header, OMX_U32 size) {
  OMX_VERSIONTYPE* ver = (OMX_VERSIONTYPE*)(header + sizeof(OMX_U32));
  *((OMX_U32*)header) = size;

  ver->s.nVersionMajor = VERSIONMAJOR;
  ver->s.nVersionMinor = VERSIONMINOR;
  ver->s.nRevision = VERSIONREVISION;
  ver->s.nStep = VERSIONSTEP;
}


void display_help() {
  printf("\n");
  printf("Usage: omxaudiodectest [-o outfile] [-stmdgh] filename\n");
  printf("\n");
  printf("       -o outfile: If this option is specified, the decoded stream is written to outfile\n");
  printf("                   This option can't be used with '-t' \n");
  printf("       -s single_ogg: Use the single role Ogg Vorbis decoder instead of the default one. Can't be used with -m or .mp3 file\n");
  printf("       -t: The audio decoder is tunneled with the ALSA sink\n");
  printf("       -m: For MP3 decoding use the mad library. Can't be used with -s or .ogg file\n");
  printf("       -d: If no output is specified, and no playback is specified,\n");
  printf("           this flag activated the print of the stream directly on stdout\n");
  printf("       -f: Use filereader with mad\n");
  printf("       -g: Gain of the audio sink[0...100]\n");
  printf("       -h: Displays this help\n");
  printf("\n");
  exit(1);
}

OMX_ERRORTYPE test_OMX_ComponentNameEnum() {
  char * name;
  int index;

  OMX_ERRORTYPE err = OMX_ErrorNone;

  DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s\n",__func__);
  name = malloc(OMX_MAX_STRINGNAME_SIZE);
  index = 0;
  while(1) {
    err = OMX_ComponentNameEnum (name, OMX_MAX_STRINGNAME_SIZE, index);
    if ((name != NULL) && (err == OMX_ErrorNone)) {
      DEBUG(DEFAULT_MESSAGES, "component %i is %s\n",index, name);
    } else break;
    if (err != OMX_ErrorNone) break;
      index++;
  }
  free(name);
  name = NULL;
  DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s result %i\n",__func__, err);
  return err;
}

OMX_ERRORTYPE test_OMX_RoleEnum(OMX_STRING component_name) {
  OMX_U32 no_of_roles;
  OMX_U8 **string_of_roles;
  OMX_ERRORTYPE err = OMX_ErrorNone;
  int index;

  DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s\n",__func__);
  DEBUG(DEB_LEV_SIMPLE_SEQ, "Getting roles of %s. Passing Null first...\n", component_name);
  err = OMX_GetRolesOfComponent(component_name, &no_of_roles, NULL);
  if (err != OMX_ErrorNone) {
    DEBUG(DEB_LEV_ERR, "Not able to retrieve the number of roles of the given component\n");
    DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s result %i\n",__func__, err);
    return err;
  }
  DEBUG(DEFAULT_MESSAGES, "The number of roles for the component %s is: %i\n", component_name, (int)no_of_roles);

  if(no_of_roles == 0) {
    DEBUG(DEB_LEV_ERR, "The Number or roles is 0.\nThe component selected is not correct for the purpose of this test.\nExiting...\n");    
    err = OMX_ErrorInvalidComponentName;
  }  else {
    string_of_roles = (OMX_U8**)malloc(no_of_roles * sizeof(OMX_STRING));
    for (index = 0; index<no_of_roles; index++) {
      *(string_of_roles + index) = (OMX_U8 *)malloc(no_of_roles*OMX_MAX_STRINGNAME_SIZE);
    }
    DEBUG(DEB_LEV_SIMPLE_SEQ, "...then buffers\n");

    err = OMX_GetRolesOfComponent(component_name, &no_of_roles, string_of_roles);
    if (err != OMX_ErrorNone) {
      DEBUG(DEB_LEV_ERR, "Not able to retrieve the roles of the given component\n");
    } else if(string_of_roles != NULL) {
      for (index = 0; index < no_of_roles; index++) {
        DEBUG(DEFAULT_MESSAGES, "The role %i for the component:  %s \n", (index + 1), *(string_of_roles+index));
      }
    } else {
      DEBUG(DEB_LEV_ERR, "role string is NULL!!! Exiting...\n");
      err = OMX_ErrorInvalidComponentName;
    }
    for (index = 0; index<no_of_roles; index++) {
      free(*(string_of_roles + index));
    }
    free(string_of_roles);
  }
  DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s result %i\n",__func__, err);
  return err;
}

OMX_ERRORTYPE test_OMX_ComponentEnumByRole(OMX_STRING role_name) {
  OMX_U32 no_of_comp_per_role;
  OMX_U8 **string_of_comp_per_role;
  OMX_ERRORTYPE err;
  int index;

  DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s\n",__func__);
  
  DEBUG(DEFAULT_MESSAGES, "Getting number of components per role for %s\n", role_name);

  err = OMX_GetComponentsOfRole(role_name, &no_of_comp_per_role, NULL);
  if (err != OMX_ErrorNone) {
    DEBUG(DEB_LEV_ERR, "Not able to retrieve the number of components of a given role\n");
    DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s result %i\n",__func__, err);
    return err;
  }
  DEBUG(DEFAULT_MESSAGES, "Number of components per role for %s is %i\n", role_name, (int)no_of_comp_per_role);

  string_of_comp_per_role = (OMX_U8**)malloc(no_of_comp_per_role * sizeof(OMX_STRING));
  /*
  xieyihao 2012-6-20 9:29:43
  the string memory is static allocated by the compiler in the OMX_CORE,
  so noneed to be malloced & freed outside
  fixme: what is the definition of the Function 
  			 OMX_GetComponentsOfRole in the OMX spcification?
  */
  /*
  for (index = 0; index<no_of_comp_per_role; index++) {
    string_of_comp_per_role[index] = malloc(OMX_MAX_STRINGNAME_SIZE);
  }
  */

  err = OMX_GetComponentsOfRole(role_name, &no_of_comp_per_role, string_of_comp_per_role);
  if (err != OMX_ErrorNone) {
    DEBUG(DEB_LEV_ERR, "Not able to retrieve the components of a given role\n");
    DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s result %i\n",__func__, err);
    for (index = 0; index<no_of_comp_per_role; index++) {
      if(string_of_comp_per_role[index]) {
        free(string_of_comp_per_role[index]);
        string_of_comp_per_role[index] = NULL;
      }
    }

    if(string_of_comp_per_role)  {
      free(string_of_comp_per_role);
      string_of_comp_per_role = NULL;
    }
    return err;
  }

  DEBUG(DEFAULT_MESSAGES, " The components are:\n");
  for (index = 0; index < no_of_comp_per_role; index++) {
    DEBUG(DEFAULT_MESSAGES, "%s\n", string_of_comp_per_role[index]);
  }
  /*
  xieyihao 2012-6-20 9:29:43
  the string memory is static allocated by the compiler in the OMX_CORE,
  so noneed to be malloced & freed outside
  */
  /*
  for (index = 0; index<no_of_comp_per_role; index++) {
    if(string_of_comp_per_role[index]) {
      free(string_of_comp_per_role[index]);
      string_of_comp_per_role[index] = NULL;
      DEBUG(DEFAULT_MESSAGES, "%s\n", string_of_comp_per_role[index]);
    }
  }
  */
	DEBUG(DEFAULT_MESSAGES, " %s %d\n", __FILE__, __LINE__);
  if(string_of_comp_per_role)  {
    free(string_of_comp_per_role);
    string_of_comp_per_role = NULL;
  }
  DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s result OMX_ErrorNone\n",__func__);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE test_OpenClose(OMX_STRING component_name) {
  OMX_ERRORTYPE err = OMX_ErrorNone;

  DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s\n",__func__);
  err = OMX_GetHandle(&appPriv->audiodechandle, component_name, NULL /*appPriv */, &audiodeccallbacks);
  if(err != OMX_ErrorNone) {
    DEBUG(DEB_LEV_ERR, "No component found\n");
  } else {
    err = OMX_FreeHandle(appPriv->audiodechandle);
    if(err != OMX_ErrorNone) {
      DEBUG(DEB_LEV_ERR, "In %s err %08x in Free Handle\n",__func__,err);
    }
  }
  DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s result %i\n",__func__, err);
  return err;
}


int flagIsMadRequested;

int flagSingleOGGSelected;

void* pComHandle = NULL;
static int GetCoreLibAndHandle(void)
{

    int ret = 0;

    void* pCoreModule = NULL;
    const char* pErr = dlerror();

    pCoreModule = dlopen("libOMX_Core.so", RTLD_LAZY | RTLD_GLOBAL);

    if( pCoreModule == NULL )
    {
        DEBUG(DEFAULT_MESSAGES,"dlopenfailed because %s\n",dlerror());
        ret = -1;
        goto UNLOCK_MUTEX;
    }

    OMX_Init = dlsym(pCoreModule, "ActionOMX_Init");
    pErr = dlerror();
    if( (pErr != NULL) || (OMX_Init == NULL) )
    {
        ret = -1;
        goto CLEAN_UP;
    }

    OMX_Deinit = dlsym(pCoreModule, "ActionOMX_Deinit");
    pErr = dlerror();
    if( (pErr != NULL) || (OMX_Deinit == NULL) )
    {
        ret = -1;
        goto CLEAN_UP;
    }

    OMX_GetHandle = dlsym(pCoreModule, "ActionOMX_GetHandle");
    pErr = dlerror();
    if( (pErr != NULL) || (OMX_GetHandle == NULL) )
    {
        ret = -1;
        goto CLEAN_UP;
    }

    OMX_FreeHandle = dlsym(pCoreModule, "ActionOMX_FreeHandle");
    pErr = dlerror();
    if( (pErr != NULL) || (OMX_FreeHandle == NULL) )
    {
        ret = -1;
        goto CLEAN_UP;
    }
    
    OMX_ComponentNameEnum = dlsym(pCoreModule, "ActionOMX_ComponentNameEnum");
    pErr = dlerror();
    if( (pErr != NULL) || (OMX_ComponentNameEnum == NULL) )
    {
        ret = -1;
        goto CLEAN_UP;
    }
    
    OMX_GetRolesOfComponent = dlsym(pCoreModule, "ActionOMX_GetRolesOfComponent");
    pErr = dlerror();
    if( (pErr != NULL) || (OMX_ComponentNameEnum == NULL) )
    {
        ret = -1;
        goto CLEAN_UP;
    }
    
    OMX_GetComponentsOfRole = dlsym(pCoreModule, "ActionOMX_GetComponentsOfRole");
    pErr = dlerror();
    if( (pErr != NULL) || (OMX_ComponentNameEnum == NULL) )
    {
        ret = -1;
        goto CLEAN_UP;
    }


CLEAN_UP:
    if(pCoreModule && ret == -1)
    {
        dlclose(pCoreModule);
        pCoreModule = NULL;
    }

UNLOCK_MUTEX:

    pComHandle = pCoreModule;


    return ret;

}
void Set_AudioPrms(OMX_HANDLETYPE pComponent, int samplerate)
{
	OMX_AUDIO_PARAM_PCMMODETYPE AudioPcmMode;
  /** settings of output port audio format - pcm */
  setHeader(&AudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
  AudioPcmMode.nPortIndex = 1;
  AudioPcmMode.nChannels = 2;
  AudioPcmMode.eNumData = OMX_NumericalDataSigned;
  AudioPcmMode.eEndian = OMX_EndianLittle;
  AudioPcmMode.bInterleaved = OMX_TRUE;
  AudioPcmMode.nBitPerSample = 16;
  AudioPcmMode.nSamplingRate = samplerate;
  AudioPcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
  AudioPcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
  AudioPcmMode.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
  OMX_SetParameter(pComponent, OMX_IndexParamAudioPcm, &AudioPcmMode);
}
int seek_fr_time, seek_to_time, seek_start_time, seek_now = 0,first_reach_seekpoint = 1;
int flagSeekSelected = 0;
int flagPauseSelected = 0;
int pause_point_time, pause_sleep_time, now_pause = 0;
int main(int argc, char** argv) {

  char *temp = NULL;
  OMX_ERRORTYPE err;
  OMX_INDEXTYPE eIndexParamFilename;
  OMX_STRING full_component_name;
  int index;
  int data_read;
  int gain=-1;
  OMX_AUDIO_CONFIG_VOLUMETYPE sVolume;
#ifdef WITH_ACTIONS_PARSER
	music_info_t audio_info;
	int read_bytes;
#endif
	input_file = argv[1];
	output_file = argv[2];
	
	if(flagSeekSelected)
	{		
		seek_fr_time = atoi(argv[3]);
		seek_to_time = atoi(argv[4]);	
	}
	
	if(flagPauseSelected)
	{
		pause_point_time = atoi(argv[3]);
		pause_sleep_time = atoi(argv[4]);	
	}
  if(0){
    display_help();
  } else {
    

    flagIsMadRequested = 0;

    flagSingleOGGSelected = 0;

    DEBUG(DEFAULT_MESSAGES, "Options selected:\n");
    DEBUG(DEFAULT_MESSAGES, "Decode file %s", input_file);
    DEBUG(DEFAULT_MESSAGES, " to ");
    if (0) {
    } else {
      if (1) {
        DEBUG(DEFAULT_MESSAGES, " %s\n", output_file);
      }
    }
  }
  

  if (1) {
    outfile = fopen(output_file,"wb");
    if(outfile == NULL) {
      DEBUG(DEB_LEV_ERR, "Error at opening the output file");
      exit(1);
    } 
  }

  if(1) {
#ifdef WITH_ACTIONS_PARSER
		int ret = parser_open(input_file, "AMR", &audio_info);
		if(ret == -1)
		{
			DEBUG(DEB_LEV_ERR, "Error in opening input file %s\n", input_file);
      exit(1);
		}
		buffer_in_size = audio_info.max_chunksize;
#else
    fd = fopen(input_file, "rb");
    if(fd == NULL) {
      DEBUG(DEB_LEV_ERR, "Error in opening input file %s\n", input_file);
      exit(1);
    }
#endif
  }
	
  /** initializing appPriv structure */
  appPriv = malloc(sizeof(appPrivateType));

  appPriv->decoderEventSem = malloc(sizeof(tsem_t));
  appPriv->eofSem = malloc(sizeof(tsem_t));
	appPriv->seekSem = malloc(sizeof(tsem_t));
	appPriv->pauseSem = malloc(sizeof(tsem_t));

  tsem_init(appPriv->decoderEventSem, 0);
  tsem_init(appPriv->eofSem, 0);
  tsem_init(appPriv->seekSem, 0);
	tsem_init(appPriv->pauseSem, 0);
  if(GetCoreLibAndHandle() == -1)
  {
  	DEBUG(DEB_LEV_ERR, "Error in dlsym functions\n");		
  }
	DEBUG(DEFAULT_MESSAGES, " %s %d\n", __FILE__, __LINE__);
  /** initialising openmax */
  err = OMX_Init();
  if (err != OMX_ErrorNone) {
    DEBUG(DEB_LEV_ERR, "The OpenMAX core can not be initialized. Exiting...\n");
    exit(1);
  } 
	DEBUG(DEFAULT_MESSAGES, " %s %d\n", __FILE__, __LINE__);
  DEBUG(DEFAULT_MESSAGES, "------------------------------------\n");
  test_OMX_ComponentNameEnum();
  DEBUG(DEFAULT_MESSAGES, "------------------------------------\n");
  test_OMX_RoleEnum(COMPONENT_NAME_BASE);
  DEBUG(DEFAULT_MESSAGES, "------------------------------------\n");
  test_OMX_ComponentEnumByRole(BASE_ROLE);
  DEBUG(DEFAULT_MESSAGES, "------------------------------------\n");
  test_OpenClose(COMPONENT_NAME_BASE);
  DEBUG(DEFAULT_MESSAGES, "------------------------------------\n");

  full_component_name = (OMX_STRING) malloc(OMX_MAX_STRINGNAME_SIZE);
  strcpy(full_component_name, "OMX.Action.Audio.Decoder");

  /** getting the handle of audio decoder */
  DEBUG(DEB_LEV_SIMPLE_SEQ, "Getting Audio %s Decoder Handle\n",full_component_name);
  err = OMX_GetHandle(&appPriv->audiodechandle, full_component_name, NULL /*appPriv */, &audiodeccallbacks);
  if(err != OMX_ErrorNone) {
    DEBUG(DEB_LEV_ERR, "Audio Decoder Component Not Found\n");
    exit(1);
  } 
  DEBUG(DEFAULT_MESSAGES, "Component %s opened\n", full_component_name);

	
#ifdef WITH_ACTIONS_PARSER
	Set_AudioPrms(appPriv->audiodechandle, audio_info.sample_rate);
#else
	/*
	the codec lib need the sample rate params
	set prms
	*/
	Set_AudioPrms(appPriv->audiodechandle, 32000);
#endif

	/*
			xieyihao 2012-7-12 12:36:28
			fixme:
			in the omx spec, some fields of the OMX_PARAM_PORTDEFINITIONTYPE is read only
			but according to the Android omx client design flow, All of them should be writable
			how to solve the problem? would this cause any compatibility problem?
			
	*/
#ifdef WITH_ACTIONS_PARSER
	{
		OMX_PARAM_PORTDEFINITIONTYPE PortDef;
		setHeader(&PortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
		PortDef.nPortIndex = 0;
		OMX_GetParameter(appPriv->audiodechandle, OMX_IndexParamPortDefinition, &PortDef);
		DEBUG(DEFAULT_MESSAGES, "default minimu size : %d\n", PortDef.nBufferSize);
		if(PortDef.nBufferSize < audio_info.max_chunksize)
		{
			PortDef.nBufferSize = audio_info.max_chunksize;
			DEBUG(DEFAULT_MESSAGES, "default minimu size less than requirement, change to : %d\n", PortDef.nBufferSize);
			OMX_SetParameter(appPriv->audiodechandle, OMX_IndexParamPortDefinition, &PortDef);
			OMX_GetParameter(appPriv->audiodechandle, OMX_IndexParamPortDefinition, &PortDef);	
			DEBUG(DEFAULT_MESSAGES, "after set portdefinition, buffersize: %d\n", PortDef.nBufferSize);
		}
		buffer_in_size = PortDef.nBufferSize ;
	}
#endif
	{
		OMX_PARAM_COMPONENTROLETYPE role;
		setHeader(&role, sizeof(OMX_PARAM_COMPONENTROLETYPE));	
		strcpy(role.cRole, "audio_decoder.amr");
		OMX_SetParameter(appPriv->audiodechandle, OMX_IndexParamStandardComponentRole, &role);
	}
	
		
  /*Send State Change Idle command to Audio Decoder*/
  if (1) 
  {
    err = OMX_SendCommand(appPriv->audiodechandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

    /** the output buffers of file reader component will be used 
    *  in the audio decoder component as input buffers 
    */ 
    if(0) {

    }
    else {
      err = OMX_AllocateBuffer(appPriv->audiodechandle, &inBufferAudioDec[0], 0, NULL, buffer_in_size);
      if(err != OMX_ErrorNone) {
        DEBUG(DEB_LEV_ERR, "Unable to allocate buffer\n");
        exit(1);
      }
      err = OMX_AllocateBuffer(appPriv->audiodechandle, &inBufferAudioDec[1], 0, NULL, buffer_in_size);
      if(err != OMX_ErrorNone) {
        DEBUG(DEB_LEV_ERR, "Unable to allocate buffer\n");
        exit(1);
      }
    }

    err = OMX_AllocateBuffer(appPriv->audiodechandle, &outBufferAudioDec[0], 1, NULL, buffer_out_size);
    if(err != OMX_ErrorNone) {
      DEBUG(DEB_LEV_ERR, "Unable to allocate buffer in audio dec\n");
      exit(1);
    }
    err = OMX_AllocateBuffer(appPriv->audiodechandle, &outBufferAudioDec[1], 1, NULL, buffer_out_size);
    if(err != OMX_ErrorNone) {
      DEBUG(DEB_LEV_ERR, "Unable to allocate buffer in audio dec\n");
      exit(1);
    }
    /*Wait for decoder state change to idle*/
    tsem_down(appPriv->decoderEventSem);
  }



  err = OMX_SendCommand(appPriv->audiodechandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
  if(err != OMX_ErrorNone) {
    DEBUG(DEB_LEV_ERR,"audio decoder state executing failed\n");
    exit(1);
  }
  /*Wait for decoder state change to executing*/
  tsem_down(appPriv->decoderEventSem);

  DEBUG(DEB_LEV_SIMPLE_SEQ,"All Component state changed to Executing\n");

  if(1) {
#ifdef WITH_ACTIONS_PARSER
		memcpy(inBufferAudioDec[0]->pBuffer, &(audio_info.buf), sizeof(void*));
		inBufferAudioDec[0]->nFilledLen = sizeof(void*);
    inBufferAudioDec[0]->nOffset = 0;
    inBufferAudioDec[0]->nFlags = OMX_BUFFERFLAG_CODECCONFIG;
    data_read = buffer_in_size;
    parser_chunk(inBufferAudioDec[1]->pBuffer, &data_read);
    inBufferAudioDec[1]->nFilledLen = data_read;
    inBufferAudioDec[1]->nOffset = 0;
#else
    data_read = fread(inBufferAudioDec[0]->pBuffer, 1, buffer_in_size, fd);
    inBufferAudioDec[0]->nFilledLen = data_read;
    inBufferAudioDec[0]->nOffset = 0;

    data_read = fread(inBufferAudioDec[1]->pBuffer, 1, buffer_in_size, fd);
    inBufferAudioDec[1]->nFilledLen = data_read;
    inBufferAudioDec[1]->nOffset = 0;
#endif
    DEBUG(DEB_LEV_PARAMS, "Empty first  buffer %x\n", (int)inBufferAudioDec[0]);
    err = OMX_EmptyThisBuffer(appPriv->audiodechandle, inBufferAudioDec[0]);
    DEBUG(DEB_LEV_PARAMS, "Empty second buffer %x\n", (int)inBufferAudioDec[1]);
    err = OMX_EmptyThisBuffer(appPriv->audiodechandle, inBufferAudioDec[1]);

  }
  /* Call FillThisBuffer now, to ensure that first two input buffers has already been sent to the component*/
  if (1) {
    err = OMX_FillThisBuffer(appPriv->audiodechandle, outBufferAudioDec[0]);
    if(err != OMX_ErrorNone) {
      DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Audio Dec\n", __func__,err);
      exit(1);
    }
    DEBUG(DEB_LEV_PARAMS, "Fill decoder second buffer %x\n", (int)outBufferAudioDec[1]);
    err = OMX_FillThisBuffer(appPriv->audiodechandle, outBufferAudioDec[1]);
    if(err != OMX_ErrorNone) {
      DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Audio Dec\n", __func__,err);
      exit(1);
    }
  }
	
	if(flagPauseSelected)
	{
		//tsem_down(appPriv->pauseSem);
		DEBUG(DEB_LEV_FULL_SEQ,"in %s,wating for pause time\n",__func__);
		sleep(pause_sleep_time);
		DEBUG(DEB_LEV_FULL_SEQ,"in %s,going to sleep while pause\n",__func__);
		err = OMX_SendCommand(appPriv->audiodechandle, OMX_CommandStateSet, OMX_StatePause, NULL);
		tsem_down(appPriv->decoderEventSem);
		sleep(pause_sleep_time);
		DEBUG(DEB_LEV_FULL_SEQ,"in %s,wakeup to continue\n",__func__);
		err = OMX_SendCommand(appPriv->audiodechandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
		tsem_down(appPriv->decoderEventSem);
	}
	
	if(flagSeekSelected)
	{
		DEBUG(DEB_LEV_FULL_SEQ,"in %s,waiting seek signal \n",__func__);
		tsem_down(appPriv->seekSem);
		DEBUG(DEB_LEV_FULL_SEQ,"in %s,flusing component ports\n",__func__);
		seek_now = 1;
		err = OMX_SendCommand(appPriv->audiodechandle, OMX_CommandFlush, OMX_ALL, NULL);
		DEBUG(DEB_LEV_FULL_SEQ,"in %s,waiting flushcomplete signal \n",__func__);
		/*because the existance of both input and output port, we need wait 2 signals*/
		tsem_down(appPriv->decoderEventSem);
		tsem_down(appPriv->decoderEventSem);
		seek_now = 0;
		first_reach_seekpoint = 0;
		DEBUG(DEB_LEV_FULL_SEQ,"in %s,read from the seek point \n",__func__);
		parser_seek(seek_to_time, SEEK_SET, &seek_start_time);
		DEBUG(DEB_LEV_FULL_SEQ,"in %s,seek to time %d \n",__func__,seek_start_time);
		data_read = buffer_in_size;
    parser_chunk(inBufferAudioDec[0]->pBuffer, &data_read);
    inBufferAudioDec[0]->nFilledLen = data_read;
    inBufferAudioDec[0]->nOffset = 0;
    inBufferAudioDec[0]->nFlags |= OMX_BUFFERFLAG_STARTTIME;
    inBufferAudioDec[0]->nTimeStamp = (OMX_S64)seek_start_time * 1000;
    data_read = buffer_in_size;
    parser_chunk(inBufferAudioDec[1]->pBuffer, &data_read);
    inBufferAudioDec[1]->nFilledLen = data_read;
    inBufferAudioDec[1]->nOffset = 0;
    
    DEBUG(DEB_LEV_FULL_SEQ, "Empty seekpoint  buffer1 %x\n", (int)inBufferAudioDec[0]);
    err = OMX_EmptyThisBuffer(appPriv->audiodechandle, inBufferAudioDec[0]);
    DEBUG(DEB_LEV_FULL_SEQ, "Empty seekpoint  buffer2 %x\n", (int)inBufferAudioDec[1]);
    err = OMX_EmptyThisBuffer(appPriv->audiodechandle, inBufferAudioDec[1]);
    
    err = OMX_FillThisBuffer(appPriv->audiodechandle, outBufferAudioDec[0]);
    if(err != OMX_ErrorNone) {
      DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Audio Dec\n", __func__,err);
      exit(1);
    }
    DEBUG(DEB_LEV_PARAMS, "Fill decoder second buffer %x\n", (int)outBufferAudioDec[1]);
    err = OMX_FillThisBuffer(appPriv->audiodechandle, outBufferAudioDec[1]);
    if(err != OMX_ErrorNone) {
      DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Audio Dec\n", __func__,err);
      exit(1);
    }
 
			
	}
	
	
	
	
  DEBUG(DEFAULT_MESSAGES,"Waiting for  EOS = %d\n",appPriv->eofSem->semval);

  tsem_down(appPriv->eofSem);

  DEBUG(DEFAULT_MESSAGES,"Received EOS \n");
  /*Send Idle Command to all components*/
  DEBUG(DEFAULT_MESSAGES, "The execution of the decoding process is terminated\n");

  err = OMX_SendCommand(appPriv->audiodechandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

  tsem_down(appPriv->decoderEventSem);

  DEBUG(DEFAULT_MESSAGES, "All component Transitioned to Idle\n");
  /*Send Loaded Command to all components*/

  err = OMX_SendCommand(appPriv->audiodechandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);

  DEBUG(DEFAULT_MESSAGES, "Audio dec to loaded\n");

  /*Free buffers is components are not tunnelled*/
  if (1) {
    err = OMX_FreeBuffer(appPriv->audiodechandle, 0, inBufferAudioDec[0]);
    err = OMX_FreeBuffer(appPriv->audiodechandle, 0, inBufferAudioDec[1]);
  }
  DEBUG(DEB_LEV_PARAMS, "Free Audio dec output ports\n");
  if (1) {
    err = OMX_FreeBuffer(appPriv->audiodechandle, 1, outBufferAudioDec[0]);
    err = OMX_FreeBuffer(appPriv->audiodechandle, 1, outBufferAudioDec[1]);
  } 

  tsem_down(appPriv->decoderEventSem);

  DEBUG(DEFAULT_MESSAGES, "All components released\n");

  /** freeing all handles and deinit omx */
  OMX_FreeHandle(appPriv->audiodechandle);
  DEBUG(DEB_LEV_SIMPLE_SEQ, "audiodec dec freed\n");

  OMX_Deinit();

  DEBUG(DEB_LEV_SIMPLE_SEQ, "All components freed. Closing...\n");


  free(appPriv->decoderEventSem);
  appPriv->decoderEventSem = NULL;

  free(appPriv->eofSem);
  appPriv->eofSem = NULL;
  
  free(appPriv->seekSem);
  appPriv->seekSem = NULL;
  
  free(appPriv->pauseSem);
  appPriv->pauseSem = NULL;
  free(appPriv);
  
  
  appPriv = NULL;
  if (1) {
    fclose(outfile);
  }

  if(1) {
#ifdef WITH_ACTIONS_PARSER
		parser_dispose();
#else
    if(fclose(fd) != 0) {
      DEBUG(DEB_LEV_ERR,"Error in closing input file stream\n");
      exit(1);
    }
    else {
      DEBUG(DEB_LEV_SIMPLE_SEQ,"Succees in closing input file stream\n");
    }
#endif
  }

  free(full_component_name);

  free(temp);
  dlclose(pComHandle);

  return 0;
}  

/* Callbacks implementation */


OMX_ERRORTYPE audiodecEventHandler(
  OMX_OUT OMX_HANDLETYPE hComponent,
  OMX_OUT OMX_PTR pAppData,
  OMX_OUT OMX_EVENTTYPE eEvent,
  OMX_OUT OMX_U32 Data1,
  OMX_OUT OMX_U32 Data2,
  OMX_OUT OMX_PTR pEventData)
{
  OMX_ERRORTYPE err;
  OMX_PARAM_PORTDEFINITIONTYPE param;
  OMX_AUDIO_PARAM_PCMMODETYPE pcmParam;
  
  DEBUG(DEB_LEV_SIMPLE_SEQ, "Hi there, I am in the %s callback\n", __func__);
  if(eEvent == OMX_EventCmdComplete) {
    if (Data1 == OMX_CommandStateSet) {
      DEBUG(DEB_LEV_SIMPLE_SEQ/*SIMPLE_SEQ*/, "Audio Decoder State changed in ");
      switch ((int)Data2) {
      case OMX_StateInvalid:
        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
        break;
      case OMX_StateLoaded:
        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
        break;
      case OMX_StateIdle:
        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
        break;
      case OMX_StateExecuting:
        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
        break;
      case OMX_StatePause:
        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
        break;
      case OMX_StateWaitForResources:
        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
        break;
      }
      tsem_up(appPriv->decoderEventSem);
    } else if (Data1 == OMX_CommandPortEnable){
      DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable  Event\n",__func__);
      tsem_up(appPriv->decoderEventSem);
    } else if (Data1 == OMX_CommandPortDisable){
      DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Disable Event\n",__func__);
      tsem_up(appPriv->decoderEventSem);
    } else if (Data1 == OMX_CommandFlush)
    {
    	DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port flush Event\n",__func__);
      tsem_up(appPriv->decoderEventSem);	
    } 
  } else if(eEvent == OMX_EventPortSettingsChanged) {
    DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Settings Changed Event\n", __func__);
    if (Data2 == 1) {
      param.nPortIndex = 1;
      setHeader(&param, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
      err = OMX_GetParameter(appPriv->audiodechandle,OMX_IndexParamPortDefinition, &param);
      /*Get Port parameters*/
      pcmParam.nPortIndex=1;
      setHeader(&pcmParam, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
      err = OMX_GetParameter(appPriv->audiodechandle, OMX_IndexParamAudioPcm, &pcmParam);

     
     
    } else if (Data2 == 0) {
      /*Get Port parameters*/
      param.nPortIndex = 0;
      setHeader(&param, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
      err = OMX_GetParameter(appPriv->audiodechandle,OMX_IndexParamPortDefinition, &param);
    }
  } else if(eEvent == OMX_EventBufferFlag) {
    DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
    if((int)Data2 == OMX_BUFFERFLAG_EOS) {
    	DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
      tsem_up(appPriv->eofSem);
    }
  } else {
    DEBUG(DEB_LEV_SIMPLE_SEQ, "Param1 is %i\n", (int)Data1);
    DEBUG(DEB_LEV_SIMPLE_SEQ, "Param2 is %i\n", (int)Data2);
  }
  
  return OMX_ErrorNone;
}

OMX_ERRORTYPE audiodecEmptyBufferDone(
  OMX_OUT OMX_HANDLETYPE hComponent,
  OMX_OUT OMX_PTR pAppData,
  OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
  OMX_ERRORTYPE err;
  int data_read;
  static int iBufferDropped=0;
  DEBUG(DEB_LEV_FULL_SEQ, "Hi there, I am in the %s callback.\n", __func__);

  if(flagSeekSelected && seek_now) {
  } else {

#ifdef WITH_ACTIONS_PARSER
		data_read = buffer_in_size;
		parser_chunk(pBuffer->pBuffer, &data_read); 
#else
    data_read = fread(pBuffer->pBuffer, 1, buffer_in_size, fd);
#endif
    pBuffer->nFilledLen = data_read;
    pBuffer->nOffset = 0;
    if (data_read <= 0) {
      DEBUG(DEB_LEV_SIMPLE_SEQ, "In the %s no more input data available\n", __func__);
      iBufferDropped++;
      
      if(iBufferDropped>=2) {
      	DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
        tsem_up(appPriv->eofSem);
        return OMX_ErrorNone;
      }
      
      DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
      pBuffer->nFilledLen=0;
      pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
      bEOS=OMX_TRUE;
      DEBUG(DEB_LEV_FULL_SEQ, "In %s empty the last buffer\n", __func__);
      err = OMX_EmptyThisBuffer(hComponent, pBuffer);
      return OMX_ErrorNone;
    }
    pBuffer->nFilledLen = data_read;
    if(!bEOS) {
      DEBUG(DEB_LEV_FULL_SEQ, "Empty buffer %x\n", (int)pBuffer);
      err = OMX_EmptyThisBuffer(hComponent, pBuffer);
    } else {
      DEBUG(DEB_LEV_FULL_SEQ, "In %s Dropping Empty This buffer to Audio Dec\n", __func__);
    }
  }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE audiodecFillBufferDone(
  OMX_OUT OMX_HANDLETYPE hComponent,
  OMX_OUT OMX_PTR pAppData,
  OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
  OMX_ERRORTYPE err;
  int i;
  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s \n",__func__);
  /* Output data to ALSA sink */
  if(pBuffer != NULL){
    if (pBuffer->nFilledLen == 0) {
      DEBUG(DEB_LEV_ERR, "Ouch! In %s: had 0 data size in output buffer...\n", __func__);
      return OMX_ErrorNone;
    }
    if (1) {
      if(pBuffer->nFilledLen > 0) {
        fwrite(pBuffer->pBuffer, 1, pBuffer->nFilledLen, outfile);
      }
      pBuffer->nFilledLen = 0;
      DEBUG(DEB_LEV_FULL_SEQ,"time: %d\n",pBuffer->nTimeStamp/1000);
      if(flagSeekSelected && pBuffer->nTimeStamp / 1000 > seek_fr_time && first_reach_seekpoint)
      {
      	if(first_reach_seekpoint == 1)
      	{
      		DEBUG(DEB_LEV_FULL_SEQ,"in %s, need seek\n",__func__);
      		tsem_up(appPriv->seekSem);
      	}
      	first_reach_seekpoint++;	
      }
      else
      {
	      err = OMX_FillThisBuffer(hComponent, pBuffer);
	      if(err != OMX_ErrorNone) {
	        DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__,err);
	      }
			}
    }
  } else {
    DEBUG(DEB_LEV_ERR, "Ouch! In %s: had NULL buffer to output...\n", __func__);
  }
  return OMX_ErrorNone;
}

