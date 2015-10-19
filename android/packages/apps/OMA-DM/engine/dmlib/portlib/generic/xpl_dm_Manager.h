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
 *      The xpl_dm_manager.h header file contains constants and function prototypes
 *      for dm specific functions      
 */

#ifndef XPL_DMMANAGER_H
#define XPL_DMMANAGER_H

#include "dmtDefs.h"

enum {
  SYNCML_DM_SETTINGS_PATH,   
  SYNCML_DM_PLUGINS_PATH,
  SYNCML_DM_SECURITY_LEVEL,
  SYNCML_DM_ESCAPE_CHAR,
  SYNCML_DM_MEMORY_AGING_INTERVAL,
  SYNCML_DM_MEMORY_AGING_TIME,
  SYNCML_DM_POWER_FAIL_IJECTION,
  SYNCML_DM_DUMP_SESSION_PACKAGE_PATH,
  SYNCML_DM_SESSION_LIB,
  SYNCML_DM_VERSION,
  SYNCML_DM_DMACC_ROOT_PATH,
  SYNCML_DM_NODENAME_SERVERID,
  SYNCML_DM_FEATURE_ID_POC_PROVISION_VIA_OMADM,
  SYNCML_DM_SESSION_ID,
};
typedef INT8 SYNCML_DM_ENVIRONMENT_T;

#define SYNCML_REP_PROTOCOL_VERSION_1_1 "1.1"
#define SYNCML_REP_PROTOCOL_VERSION_1_2 "1.2"


// DM: file, which contains one of SYNCML_REP_PROTOCOL_VERSION_* constants
#define XPL_VERSION_FILENAME "version.txt"

#ifdef __cplusplus
extern "C" {
#endif

SYNCML_DM_RET_STATUS_T  XPL_DM_Init(void);
SYNCML_DM_RET_STATUS_T  XPL_DM_DeInit(void);
CPCHAR     XPL_DM_GetEnv(SYNCML_DM_ENVIRONMENT_T env_var);
BOOLEAN   XPL_DM_IsFeatureEnabled(CPCHAR pUri);        

#ifdef DM_STATIC_FILES
const UINT8 *    XPL_DM_GetPluginsConfig(UINT32 *pSize);
const UINT8 *    XPL_DM_GetFstab(UINT32 *pSize);
UINT8               XPL_DM_GetMDFCount();
const UINT8 *    XPL_DM_GetMDF(UINT8 index, UINT32 *pSize);
#endif

UINT32  XPL_DM_GetChunkSize();

#ifdef __cplusplus
}
#endif

#endif /* XPL_DMMANAGER_H */
