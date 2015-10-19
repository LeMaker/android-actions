/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 *  DESCRIPTION:
 *      The xpl_dm_manager.cc file contains implementation 
 *      for dm specific functions      
 */

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdlib.h>
#include "dmprofile.h"
#include "xpl_dm_Manager.h"
#include "dm_uri_utils.h"
#include "xpl_Logger.h"
#include "dmtTreeImpl.hpp"


#ifdef FEAT_DM_VERSION_FLEX
#include "SETUP_GetFlex.h"
#endif
/********************************************************************************************************************/

static DMString     dm_version;
static CPCHAR       dm_poc_enabled = NULL;

SYNCML_DM_RET_STATUS_T XPL_DM_Init()
{
  if( 0 == dm_version.length() )
  { 
#ifdef FEAT_DM_VERSION_FLEX
    CPCHAR ver = getenv("dm_setting_version");
    if ( ver == NULL ) 
    {
      BOOLEAN bIsVersion12;
      getFlexBit_setupflex(DL_DB_FEATURE_ID_DM_PROTOCOL_VERSION_12_AVAILABLE,&bIsVersion12);
      if ( bIsVersion12 )
      {
         dm_version = SYNCML_REP_PROTOCOL_VERSION_1_2;
      }
      else
      {
         dm_version = "1.1.2";
      }
    }
    else
    {
      dm_version = ver;
    }
#else
    CPCHAR ver = getenv("dm_setting_version");
    if ( ver == NULL ) 
    {
       dm_version = SYNCML_REP_PROTOCOL_VERSION_1_2;
    }
    else
    {
       dm_version = ver;
    }
#endif
  }		
#ifdef FEAT_DM_VERSION_FLEX
  if ( dm_poc_enabled == NULL )
  {
    BOOLEAN bIsEnabled = TRUE;
    getFlexBit_setupflex(DL_DB_FEATURE_ID_POC_PROVISION_VIA_OMADM__AVAILABLE,&bIsEnabled);
    dm_poc_enabled = (bIsEnabled)?"1":"0";
  }
#endif
  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T XPL_DM_DeInit()
{
  dm_version = NULL;
  return SYNCML_DM_SUCCESS;
}


BOOLEAN   XPL_DM_IsFeatureEnabled(CPCHAR pUri)
{
	return TRUE;
}


static const char * session_lib = "libdmssession.so";

CPCHAR XPL_DM_GetEnv(SYNCML_DM_ENVIRONMENT_T env_var)
{

    switch ( env_var )
    {
        case SYNCML_DM_SETTINGS_PATH :   
            return getenv("dm_setting_root");
            
        case SYNCML_DM_PLUGINS_PATH :
            return getenv("dm_setting_plugin");
            
        case SYNCML_DM_SECURITY_LEVEL :
            return getenv("DM_SRV_SEC_LEVEL");
            
        case SYNCML_DM_ESCAPE_CHAR :
            return getenv("DM_ESCAPE_CHAR");
            
        case SYNCML_DM_MEMORY_AGING_INTERVAL :
            return getenv("DM_AGING_CHECK_INTERVAL");
            
        case SYNCML_DM_MEMORY_AGING_TIME :
            return getenv("DM_AGING_TIME");
            
        case SYNCML_DM_POWER_FAIL_IJECTION :
            return getenv("power_fail");
            
        case SYNCML_DM_DUMP_SESSION_PACKAGE_PATH :
            return getenv("DUMP_SYNCML_PATH");

        case SYNCML_DM_SESSION_LIB :  
            return session_lib;

       case SYNCML_DM_VERSION:
            return dm_version.GetBuffer();	
       
       case SYNCML_DM_DMACC_ROOT_PATH:                  
            return ( dm_version == SYNCML_REP_PROTOCOL_VERSION_1_2 )
                      ? DM_DMACC_1_2_URI
                      : DM_DMACC_1_1_URI;

       case SYNCML_DM_NODENAME_SERVERID:
            return ( dm_version == SYNCML_REP_PROTOCOL_VERSION_1_2 )
                      ? DM_SERVERID_1_2
                      : DM_SERVERID_1_1;

       case SYNCML_DM_SESSION_ID:
            return "./DevDetail/Ext/Conf/PMF/Agents/syncmldm/Sessionid";

       case SYNCML_DM_FEATURE_ID_POC_PROVISION_VIA_OMADM:
            return dm_poc_enabled;
            
    }  

    return NULL;

}

#define DM_CHUNK_BUFFER_LJ 8192

/*==================================================================================================
  FUNCTION        : XPL_DM_GetChunkSize

  DESCRIPTION     : Get chunk buffer size for Linux-Java platform
  ARGUMENT PASSED :

  OUTPUT PARAMETER:
  RETURN VALUE    :
  IMPORTANT NOTES :
  ==================================================================================================*/
UINT32  XPL_DM_GetChunkSize()
{
  return DM_CHUNK_BUFFER_LJ;
}
