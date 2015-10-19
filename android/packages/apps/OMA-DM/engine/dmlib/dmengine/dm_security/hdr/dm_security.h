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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SYNCMLDM_SECURITY_H
#define _SYNCMLDM_SECURITY_H

/*========================================================================

  Header Name: syncmldm_security.h

  General Description: This file contains prototypes, data type 
                       definitions and constants required to assess  
                       Security API's.
========================================================================*/

#include "xpl_Types.h"
#include "syncml_dm_data_types.h"

/*=======================================================================
                         CONSTANTS 
========================================================================*/
#define SYNCML_DM_PSEUDO_RANDOM_NUM_LENGTH 20 /* maximum array size of the 
                                               * pseudo ramdom number array
                                               */
#define SYNCML_DM_MAX_CRED_BUFFER_SIZE     200
#define SYNCML_DM_HASHLEN                  16 
#define SYNCML_DM_BAS64_ENCODING_SIZE_IN_MD5 \
                             (SYNCML_DM_HASHLEN + (SYNCML_DM_HASHLEN/2))

/*========================================================================
                       STRUCTURES AND OTHER TYPEDEFS
========================================================================*/
/* NOTE: User Name and Password are NULL terminated strings
 */
typedef struct 
{
  UINT8 *pb_user_name_or_server_id;
  UINT8 *pb_password;
} SYNCMLDM_BASIC_SEC_INFO_T;

typedef struct 
{
  UINT8 *pb_user_name;
  UINT8 *pb_password;
  UINT8 *pb_server_id;
} SYNCMLDM_NONCE_GENERATE_PARAMETER_INFO_T;

typedef struct 
{
  UINT16 w_nonce_string_length;
  UINT8  ab_nonce_string[1];  
} SYNCMLDM_NONCE_STRING_INFO_T;

typedef struct 
{  
  UINT16 w_credential_string_length;
  UINT8  ab_credential_string[1];
} SYNCMLDM_SEC_CREDENTIALS_T;

typedef struct 
{
  UINT8  *pb_user_name_or_server_id;
  UINT8  *pb_password;
  UINT8  *pb_nonce;
  BOOLEAN o_encode_base64;
  UINT16 w_nonce_length;  
} SYNCMLDM_MD5_SEC_INFO_T;

typedef struct 
{
  UINT8  *pb_user_name_or_server_id;
  UINT8  *pb_password;
  UINT8  *pb_nonce;
  UINT8  *pb_syncml_document;
  BOOLEAN o_encode_base64;
  UINT16 w_nonce_length;    
  UINT32 dw_syncml_document_length;  
} SYNCMLDM_HMAC_SEC_INFO_T;

SYNCMLDM_SEC_CREDENTIALS_T* syncmldm_sec_build_basic_cred(
                     const SYNCMLDM_BASIC_SEC_INFO_T *ps_basic_sec_info);

SYNCMLDM_SEC_CREDENTIALS_T* syncmldm_sec_build_md5_cred(
                         const SYNCMLDM_MD5_SEC_INFO_T *ps_md5_sec_info);

SYNCMLDM_SEC_CREDENTIALS_T* syncmldm_sec_build_digest_cred(
                         const SYNCMLDM_MD5_SEC_INFO_T *ps_md5_sec_info);

SYNCMLDM_SEC_CREDENTIALS_T* syncmldm_sec_build_hmac_cred(
                       const SYNCMLDM_HMAC_SEC_INFO_T *ps_hmac_sec_info);

SYNCMLDM_NONCE_STRING_INFO_T* syncmldm_sec_generate_nonce(
    const SYNCMLDM_NONCE_GENERATE_PARAMETER_INFO_T *ps_nonce_gen_parameters,
    CPCHAR devID);

#endif

#ifdef __cplusplus
}
#endif
