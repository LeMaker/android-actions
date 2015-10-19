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

/*=======================================================================

  Header Name: syncmldm_security.c

  General Description: This file contains definition of functions to 
                        build the credentials with Basic, MD5 and 
                        HMAC-MD5 and to generate the nonce string.
  NOTE: All the user name/serverid and password are NULL terminated 
        strings.
=======================================================================*/

#include "xpl_Time.h"
#include "dmStringUtil.h"
#include "dm_security.h"
#include "xpt-b64.h" /* To include base64Encode function */
#include "md5.h" /* To include smlMD5Init, smlMD5Update and smlMD5Final functions */

#ifdef PLATFORM_ANDROID
#include <sys/stat.h>   /* include open and read functions */
#include <fcntl.h>      /* include O_RDONLY definition */
#endif

/*=======================================================================
                              MACROS
=======================================================================*/

#define MAXIMUM_SAVE_BYTES  4 /* This will be used in base64Encode function*/
#define COLON_PLUS_NULL     2
#define SYNCML_DM_NEXT_BLOCK 1

extern void GetIMEI(char * str);

/*=======================================================================
                        LOCAL FUNCTION PROTOTYPES
=======================================================================*/

/************************************************************************
FUNCTION: get_pseudo_random

DESCRIPTION: Generates pseudo random bytes and stores them in the
             input buffer in reverse order
 
ARGUMENTS PASSED:   

UINT8 *buffer
      - Buffer which will contain output pseudo random bytes
length 
      - Number of psuedo random bytes to be generated


RETURN VALUE:
void

PRE-CONDITIONS:

   
POST-CONDITIONS:

IMPORTANT NOTES:
  None
************************************************************************/
static void get_pseudo_random(UINT8 *pbBuffer, UINT16 wLength)
{
#ifdef PLATFORM_ANDROID
  /* Use /dev/urandom for secure PRNG. */
  int fd;
  ssize_t count = 0;
  while ((fd = open("/dev/urandom", O_RDONLY)) == -1) {
    /* This open should always succeed; loop forever until it does. */
  }
  while (count < wLength) {
    int b = read(fd, &pbBuffer[count], wLength - count);
    if (b == -1) {
      /* Error: should never happen with /dev/urandom; try again. */
    } else {
      count += b;
    }
  }
  close(fd);
#else
  /* Location of this function is 
   * /vobs/core_browser/code/cryptolib/src/rsa_msg_enc.c
   */
   INT16 wCount = 0;

   XPL_CLK_CLOCK_T t = XPL_CLK_GetClock();  
   /* The current time is given as seed for generating
    * pseudo random numbers */
   srand ((UINT32)t);
   for (wCount = 0; wCount < wLength; wCount++)
   {
      pbBuffer[wCount] = (UINT8) rand ();
      if (pbBuffer[wCount] == 0)
      {
         wCount--;
      }
   }
#endif
}

/************************************************************************
FUNCTION: syncmldm_sec_build_basic_cred

DESCRIPTION: This function build the credentials for basic 
             authentication scheme:                   
             Let B64 = the base64 encoding function.
             Credential String = B64(username:password)
 
ARGUMENTS PASSED:   

SYNCMLDM_BASIC_SEC_INFO_T *ps_basic_sec_info
     - Reference to the structure containg user name and password.

RETURN VALUE:
SYNCMLDM_SEC_CREDENTIALS_T*
     - Reference to the structure containing credential string and
       its length.

PRE-CONDITIONS:

   
POST-CONDITIONS:
  Caller MUST free the memory allocated by the callee for the 
  return structure and data members of the structure.

IMPORTANT NOTES:
  None
************************************************************************/
SYNCMLDM_SEC_CREDENTIALS_T* syncmldm_sec_build_basic_cred(
                    const SYNCMLDM_BASIC_SEC_INFO_T *ps_basic_sec_info)
{
  UINT8 *pb_username_and_password = NULL;        
  UINT8  ab_savebytes[MAXIMUM_SAVE_BYTES];
  UINT32 dw_client_cred_length  = 0;
  UINT32 dw_offset              = 0;        
  UINT32 dw_length              = 0;
  UINT32 dw_estimated_basic_cred_length = 0;
  SYNCMLDM_SEC_CREDENTIALS_T *ps_security_credential = NULL;

  memset(ab_savebytes, '\0', MAXIMUM_SAVE_BYTES);
  
  /* To build basic credential string both user name and password
   * are mandatory if either use name or password is null then
   * this function return NULL.
   */
  if((ps_basic_sec_info == NULL) ||
     (ps_basic_sec_info->pb_user_name_or_server_id == NULL) || 
     (ps_basic_sec_info->pb_password == NULL))
  {            
    return (SYNCMLDM_SEC_CREDENTIALS_T*)NULL;
  }     
  
  /* We have to add COLON_PLUS_NULL because we have to add ':'
   * length and null character length  
   */
  dw_length = 
    ((DmStrlen((CPCHAR)ps_basic_sec_info->pb_user_name_or_server_id)
    + DmStrlen((CPCHAR)ps_basic_sec_info->pb_password)) 
    + COLON_PLUS_NULL);

  /* RFC 2045 say's (in First paragraph of Section 6.8) 
   * "The encoding and decoding algorithms are simple, but the
   * encoded data are consistently only about 33% larger than the
   * unencoded data.". That's why i am calculating 
   * dw_estimated_basic_cred_length by dw_length + (dw_length/2)
   */
  dw_estimated_basic_cred_length = dw_length + (dw_length/2);

  ps_security_credential = (SYNCMLDM_SEC_CREDENTIALS_T*)
        DmAllocMem((sizeof(UINT16) + (sizeof(UINT8) * dw_estimated_basic_cred_length)));
  if(ps_security_credential == NULL)
  {
    return ps_security_credential;
  }
      
  ps_security_credential->w_credential_string_length = 0;    

  pb_username_and_password = (UINT8*)DmAllocMem(dw_length);    
  if(pb_username_and_password == NULL)
  {    
    DmFreeMem(ps_security_credential);
    return (SYNCMLDM_SEC_CREDENTIALS_T*)NULL;      
  }
    
  memset(ps_security_credential->ab_credential_string, '\0', dw_estimated_basic_cred_length);    
  memset(pb_username_and_password, '\0',dw_length);
  
  dw_client_cred_length = 
    DmStrlen((CPCHAR)ps_basic_sec_info->pb_user_name_or_server_id);

  memcpy(pb_username_and_password, 
         ps_basic_sec_info->pb_user_name_or_server_id, 
         dw_client_cred_length);
  
  /* We have to add the ':' after UserName to build the
   * basic credential string
   */
  pb_username_and_password[dw_client_cred_length++] = ':';
  
  memcpy(&pb_username_and_password[dw_client_cred_length], 
         ps_basic_sec_info->pb_password, 
         DmStrlen((CPCHAR)ps_basic_sec_info->pb_password));

  dw_client_cred_length += DmStrlen((CPCHAR)ps_basic_sec_info->pb_password);    

  /* base64Encode function encodes the (UserName:Password) to 
   * base64 encoding and it return the lenght of the encoded
   * string
   */
  ps_security_credential->w_credential_string_length = 
         (UINT16)base64Encode(
                      ps_security_credential->ab_credential_string, 
                      dw_estimated_basic_cred_length, 
                      pb_username_and_password, 
                      (unsigned long*)&dw_client_cred_length, 
                      (unsigned long*)&dw_offset, SYNCML_DM_NEXT_BLOCK, 
                      ab_savebytes
                      );    
  
  DmFreeMem(pb_username_and_password);
  
  return ps_security_credential;
}

/************************************************************************
FUNCTION: build_md5_cred

DESCRIPTION: This function build the credentials for MD5 
             authentication scheme:                   
             Let H = the MD5 Hashing function.
             Let Digest = the output of the MD5 Hashing function.
             Let B64 = the base64 encoding function.
             Credential String = H(B64(H(username:password)):nonce)
 
ARGUMENTS PASSED:   

SYNCMLDM_MD5_SEC_INFO_T *ps_md5_sec_info
     - Reference to the structure containing user name, password
       and nonce.

RETURN VALUE:
SYNCMLDM_SEC_CREDENTIALS_T*
     - Reference to the structure containing credential string and
       it's length.

PRE-CONDITIONS:
   
POST-CONDITIONS:
  Caller MUST free the memory allocated by the callee for the 
  return parameter.

IMPORTANT NOTES:
  None
************************************************************************/
SYNCMLDM_SEC_CREDENTIALS_T* syncmldm_sec_build_md5_cred(
                    const SYNCMLDM_MD5_SEC_INFO_T *ps_md5_sec_info)
{  
  UINT8 ab_savebytes[MAXIMUM_SAVE_BYTES];    

  UINT32 dw_client_cred_length = 0;
  UINT32 dw_offset = 0;  

  MD5_CTX s_md5_ctx;
  UINT8   ab_hash[SYNCML_DM_HASHLEN]; /* Contains the hash data after 
                                       * smlMD5Final function is called 
                                       */

  SYNCMLDM_SEC_CREDENTIALS_T *ps_security_credential = NULL;

  memset(ab_hash, '\0', SYNCML_DM_HASHLEN);
  memset(ab_savebytes, '\0', MAXIMUM_SAVE_BYTES);
  
  if((ps_md5_sec_info == NULL) ||
     (ps_md5_sec_info->pb_user_name_or_server_id == NULL) || 
     (ps_md5_sec_info->pb_password == NULL) || 
     (ps_md5_sec_info->pb_nonce == NULL))
  {        
    return ps_security_credential;
  }

  ps_security_credential = (SYNCMLDM_SEC_CREDENTIALS_T*)
      DmAllocMem((sizeof(UINT16) + (sizeof(UINT8) + SYNCML_DM_BAS64_ENCODING_SIZE_IN_MD5)));
  if(ps_security_credential == NULL)
  {
    return ps_security_credential;
  }      
  
  memset(ps_security_credential->ab_credential_string, '\0', SYNCML_DM_BAS64_ENCODING_SIZE_IN_MD5);

  /****** Calculation of the part B64(H(username:password) **********/
  smlMD5Init(&s_md5_ctx);  
  smlMD5Update(&s_md5_ctx, ps_md5_sec_info->pb_user_name_or_server_id, 
        DmStrlen((CPCHAR)ps_md5_sec_info->pb_user_name_or_server_id));
  smlMD5Update(&s_md5_ctx, (UINT8*)":", 1);
  smlMD5Update(&s_md5_ctx, ps_md5_sec_info->pb_password, 
               DmStrlen((CPCHAR)ps_md5_sec_info->pb_password));
  smlMD5Final(ab_hash, &s_md5_ctx);
  dw_client_cred_length = SYNCML_DM_HASHLEN;
  dw_client_cred_length =  base64Encode(
                 ps_security_credential->ab_credential_string, 
                 SYNCML_DM_BAS64_ENCODING_SIZE_IN_MD5, 
                 ab_hash, 
                 (unsigned long*)&dw_client_cred_length, 
                 (unsigned long*)&dw_offset, SYNCML_DM_NEXT_BLOCK, 
                 ab_savebytes);
  /**** End of the Calculation of part B64(H(username:password) *****/

  /*** Calculation of the part H(B64(H(username:password)):nonce) ***/
  smlMD5Init(&s_md5_ctx);
  smlMD5Update(&s_md5_ctx, ps_security_credential->ab_credential_string, dw_client_cred_length);
  smlMD5Update(&s_md5_ctx, (UINT8*)":", 1);
  smlMD5Update(&s_md5_ctx, ps_md5_sec_info->pb_nonce, ps_md5_sec_info->w_nonce_length);
  smlMD5Final(ab_hash, &s_md5_ctx);
  dw_client_cred_length = SYNCML_DM_HASHLEN;
  
  if(ps_md5_sec_info->o_encode_base64 == TRUE)
  {
    ps_security_credential->w_credential_string_length = 
        (UINT16)base64Encode(ps_security_credential->ab_credential_string, 
                                              SYNCML_DM_BAS64_ENCODING_SIZE_IN_MD5, 
                                              ab_hash,
                                              (unsigned long*)&dw_client_cred_length, 
                                              (unsigned long*)&dw_offset, SYNCML_DM_NEXT_BLOCK, 
                                              ab_savebytes
                                              );
  }
  else
  {
    ps_security_credential->w_credential_string_length = 
                                 (UINT16)dw_client_cred_length;
    memcpy(ps_security_credential->ab_credential_string, 
           &ab_hash, 
           dw_client_cred_length);
  }
  /****** End of the Calculation of part 
          H(B64(H(username:password)):nonce) *****/
  return ps_security_credential;
}

/************************************************************************
FUNCTION: build_hmac_cred

DESCRIPTION: This function build the credentials for HMAC 
             authentication scheme:                   
             Let H = the MD5 Hashing function.
             Let Digest = the output of the MD5 Hashing function.
             Let B64 = the base64 encoding function.                   
             Credential String =
             H(B64(H(username:password)):nonce:B64(H(message body)))
 
ARGUMENTS PASSED:   
SYNCMLDM_HMAC_SEC_INFO_T *ps_hmac_sec_info
     - Reference to the structure containing user name, password,
       nonce and starting pointer to the SyncML Document.

RETURN VALUE:
SYNCMLDM_SEC_CREDENTIALS_T*
     - Reference to the structure containing credential string and
       it's length.

PRE-CONDITIONS:  
   
POST-CONDITIONS:
  Caller MUST free the memory allocated by the callee for the 
  return parameter.

IMPORTANT NOTES:
  None
************************************************************************/
SYNCMLDM_SEC_CREDENTIALS_T* syncmldm_sec_build_hmac_cred(
                     const SYNCMLDM_HMAC_SEC_INFO_T *ps_hmac_sec_info)
{
  UINT8  ab_savebytes[MAXIMUM_SAVE_BYTES];  
  
  UINT32 dw_client_cred_length  = 0;
  UINT32 dw_body_encoded_length = 0;
  UINT32 dw_hash_length         = SYNCML_DM_HASHLEN;
  UINT32 dw_offset = 0;  

  MD5_CTX s_md5_ctx;
  UINT8 ab_hash[SYNCML_DM_HASHLEN];

  SYNCMLDM_SEC_CREDENTIALS_T *ps_security_credential = NULL;
  
  memset(ab_savebytes, '\0', MAXIMUM_SAVE_BYTES);
  memset(ab_hash, '\0', SYNCML_DM_HASHLEN);

  /* If any one of the following parameter is NULL hash can't 
   * be build.
   */
  if((ps_hmac_sec_info == NULL) ||
     (ps_hmac_sec_info->pb_user_name_or_server_id == NULL) || 
     (ps_hmac_sec_info->pb_password == NULL) || 
     (ps_hmac_sec_info->pb_nonce == NULL) || 
     (ps_hmac_sec_info->pb_syncml_document == NULL) || 
     (ps_hmac_sec_info->dw_syncml_document_length == 0))
  {    
    return ps_security_credential;
  }

  ps_security_credential = (SYNCMLDM_SEC_CREDENTIALS_T*) DmAllocMem((sizeof(UINT8) + (sizeof(UINT8) + 
                                                                                                                   SYNCML_DM_MAX_CRED_BUFFER_SIZE)));
  if(ps_security_credential == NULL)
  {
    return ps_security_credential;
  }      
  
  memset(ps_security_credential->ab_credential_string, '\0', SYNCML_DM_MAX_CRED_BUFFER_SIZE);
  
  /***** Calculation of the part B64(H(username:password) ******/
  smlMD5Init(&s_md5_ctx);  
  smlMD5Update(&s_md5_ctx, 
		                    ps_hmac_sec_info->pb_user_name_or_server_id, 
                           DmStrlen((CPCHAR)ps_hmac_sec_info->pb_user_name_or_server_id));

  smlMD5Update(&s_md5_ctx, (UINT8*)":", 1);
  smlMD5Update(&s_md5_ctx, ps_hmac_sec_info->pb_password, 
               DmStrlen((CPCHAR)ps_hmac_sec_info->pb_password));
  smlMD5Final(ab_hash, &s_md5_ctx);
  dw_client_cred_length = SYNCML_DM_HASHLEN;
  dw_client_cred_length = 
          base64Encode(ps_security_credential->ab_credential_string, 
                       SYNCML_DM_MAX_CRED_BUFFER_SIZE, 
                       ab_hash, 
                       (unsigned long*)&dw_client_cred_length, 
                       (unsigned long*)&dw_offset, SYNCML_DM_NEXT_BLOCK, 
                       ab_savebytes);
  /*** End of the Calculation of part B64(H(username:password) ****/
  
  ps_security_credential->ab_credential_string[dw_client_cred_length++]  = ':';
  memcpy(&(ps_security_credential->ab_credential_string[dw_client_cred_length]), 
               ps_hmac_sec_info->pb_nonce, 
               ps_hmac_sec_info->w_nonce_length);

  dw_client_cred_length = dw_client_cred_length + 
                                  ps_hmac_sec_info->w_nonce_length;
  ps_security_credential->ab_credential_string[dw_client_cred_length++]
                                                                  = ':';

  /********** Calculation of B64(H(message body)) *****************/
  smlMD5Init(&s_md5_ctx);  
  smlMD5Update(&s_md5_ctx, ps_hmac_sec_info->pb_syncml_document, 
                        ps_hmac_sec_info->dw_syncml_document_length);
  smlMD5Final(ab_hash, &s_md5_ctx);
  dw_body_encoded_length =  base64Encode(&ps_security_credential->ab_credential_string[dw_client_cred_length], 
                                                                     SYNCML_DM_MAX_CRED_BUFFER_SIZE, 
                                                                     ab_hash, 
                                                                     (unsigned long*)&dw_hash_length, 
                                                                     (unsigned long*)&dw_offset, SYNCML_DM_NEXT_BLOCK, 
                                                                      ab_savebytes);  
  /******** End of the Calculation of B64(H(message body)) *******/
  
  dw_client_cred_length += dw_body_encoded_length;

  /****** Calculation of 
        H(B64(H(username:password)):nonce:B64(H(message body))) *****/
  smlMD5Init(&s_md5_ctx);
  smlMD5Update(&s_md5_ctx, 
            ps_security_credential->ab_credential_string, 
            dw_client_cred_length);  
  smlMD5Final(ab_hash, &s_md5_ctx);   

  /* Reset the length variable.*/
  dw_hash_length = SYNCML_DM_HASHLEN;
  
  if(ps_hmac_sec_info->o_encode_base64 == TRUE)
  {
    dw_client_cred_length = SYNCML_DM_HASHLEN;
    ps_security_credential->w_credential_string_length = 
     (UINT16)base64Encode(
                    ps_security_credential->ab_credential_string, 
                                  SYNCML_DM_MAX_CRED_BUFFER_SIZE, 
                                  ab_hash,
                                  (unsigned long*)&dw_client_cred_length, 
                                  (unsigned long*)&dw_offset, SYNCML_DM_NEXT_BLOCK, 
                                  ab_savebytes
                                 );        
  }
  else
  {
    ps_security_credential->w_credential_string_length = 
                                        (UINT16)dw_hash_length;
    memcpy(ps_security_credential->ab_credential_string, &ab_hash, 
                                                         dw_hash_length);
  }
  ps_security_credential->ab_credential_string
        [ps_security_credential->w_credential_string_length] = '\0';
  /****** End of the calculation of 
      H(B64(H(username:password)):nonce:B64(H(message body))) *****/  
  return ps_security_credential;
}

/************************************************************************
FUNCTION: syncmldm_sec_generate_nonce

DESCRIPTION: This function generate the nonce.:                   
             Let H = the MD5 Hashing function.             
             Let B64 = the base64 encoding function.                   
             Nonce String =
             b64(H(Random number+IMEI+User Name+Password+ServerId))
 
ARGUMENTS PASSED:   
SYNCMLDM_NONCE_GENERATE_PARAMETER_INFO_T *ps_nonce_gen_parameters
     - Reference to the structure containing user name, password,
       serverid.

RETURN VALUE:
SYNCMLDM_NONCE_STRING_INFO_T*
     - Reference to the structure containing nonce string and
       it's length.

PRE-CONDITIONS:  
   
POST-CONDITIONS:
  Caller MUST free the memory allocated by the callee for the 
  return parameter.

IMPORTANT NOTES:
  None
************************************************************************/
SYNCMLDM_NONCE_STRING_INFO_T* syncmldm_sec_generate_nonce(
      const SYNCMLDM_NONCE_GENERATE_PARAMETER_INFO_T *ps_nonce_gen_parameters,
      CPCHAR devID)
{
  UINT8  ab_hash[SYNCML_DM_HASHLEN];
  UINT8  ab_savebytes[MAXIMUM_SAVE_BYTES];  
  UINT8 *pb_nonce_string_parameters = NULL;
  UINT8 ab_pseudo_random[SYNCML_DM_PSEUDO_RANDOM_NUM_LENGTH];
  
  UINT32 dw_current_string_length = 0;  
  UINT32 dw_total_length = 0;
  UINT32 dw_hash_length  = SYNCML_DM_HASHLEN;  
  UINT32 dw_offset = 0;  
  UINT32 lengthDevID;

  MD5_CTX s_md5_ctx;      
  
  SYNCMLDM_NONCE_STRING_INFO_T *ps_nonce_string_info = NULL;

  
  if(ps_nonce_gen_parameters == NULL || !devID)
  {
    return NULL;
  }

  lengthDevID = DmStrlen(devID);
  memset(ab_hash, '\0', SYNCML_DM_HASHLEN);
  memset(ab_savebytes, '\0', MAXIMUM_SAVE_BYTES);

  get_pseudo_random(ab_pseudo_random, SYNCML_DM_PSEUDO_RANDOM_NUM_LENGTH);

  dw_total_length = 
          DmStrlen((CPCHAR)ps_nonce_gen_parameters->pb_user_name) +
          DmStrlen((CPCHAR)ps_nonce_gen_parameters->pb_password) +
          DmStrlen((CPCHAR)ps_nonce_gen_parameters->pb_server_id) +
          SYNCML_DM_PSEUDO_RANDOM_NUM_LENGTH + lengthDevID + 1;
  pb_nonce_string_parameters = (UINT8*)DmAllocMem((sizeof(char) * dw_total_length));
  if(pb_nonce_string_parameters == NULL)
  {
    return NULL;
  }

  ps_nonce_string_info = (SYNCMLDM_NONCE_STRING_INFO_T*)
       DmAllocMem((sizeof(UINT16) + (sizeof(UINT8) * SYNCML_DM_MAX_CRED_BUFFER_SIZE)));
  if(ps_nonce_string_info == NULL)
  {
    DmFreeMem(pb_nonce_string_parameters);
    return ps_nonce_string_info;
  }  

  memset(pb_nonce_string_parameters, '\0', dw_total_length);
  memset(ps_nonce_string_info->ab_nonce_string,'\0', SYNCML_DM_MAX_CRED_BUFFER_SIZE);

  dw_total_length = 0;
  
  /* Adding RandomNumber, IMEI, UserName, Password and ServerId 
   */
  dw_current_string_length = SYNCML_DM_PSEUDO_RANDOM_NUM_LENGTH;
  memcpy(pb_nonce_string_parameters, 
         ab_pseudo_random,
         dw_current_string_length);
  dw_total_length += dw_current_string_length;

  dw_current_string_length = lengthDevID;
  memcpy(&pb_nonce_string_parameters[dw_total_length],devID,dw_current_string_length);
  dw_total_length += dw_current_string_length;

  dw_current_string_length = DmStrlen((CPCHAR)ps_nonce_gen_parameters->pb_user_name);
  memcpy(&pb_nonce_string_parameters[dw_total_length], 
         ps_nonce_gen_parameters->pb_user_name,
         dw_current_string_length);
  dw_total_length += dw_current_string_length;
  
  dw_current_string_length = DmStrlen((CPCHAR)ps_nonce_gen_parameters->pb_password);
  memcpy(&pb_nonce_string_parameters[dw_total_length], 
         ps_nonce_gen_parameters->pb_password,
         dw_current_string_length);
  dw_total_length += dw_current_string_length;

  dw_current_string_length = DmStrlen((CPCHAR)ps_nonce_gen_parameters->pb_server_id);
  memcpy(&pb_nonce_string_parameters[dw_total_length], 
         ps_nonce_gen_parameters->pb_server_id,
         dw_current_string_length);
  dw_total_length += dw_current_string_length;
   
  /* Hashing the nonce string */
  smlMD5Init(&s_md5_ctx);  
  smlMD5Update(&s_md5_ctx, pb_nonce_string_parameters, dw_total_length);
  smlMD5Final(ab_hash, &s_md5_ctx);

  /* Encoding the generated nonce string into base64 */
  ps_nonce_string_info->w_nonce_string_length = 
    (UINT16)base64Encode(
        ps_nonce_string_info->ab_nonce_string, 
                 SYNCML_DM_MAX_CRED_BUFFER_SIZE, 
                 ab_hash, 
                 (unsigned long*)&dw_hash_length, 
                 (unsigned long*)&dw_offset, SYNCML_DM_NEXT_BLOCK, 
                 ab_savebytes
                 );  
  
  DmFreeMem(pb_nonce_string_parameters);
  
  ps_nonce_string_info->ab_nonce_string[ps_nonce_string_info->w_nonce_string_length] = '\0';
  return ps_nonce_string_info;
}

/************************************************************************
FUNCTION: build_digest_cred

DESCRIPTION: This function build the credentials for MD5 
             authentication scheme:                   
             Let H = the MD5 Hashing function.
             Let Digest = the output of the MD5 Hashing function.
             Let B64 = the base64 encoding function.
             Credential String = H(B64(H(username:password)):nonce)
 
ARGUMENTS PASSED:   

SYNCMLDM_MD5_SEC_INFO_T *ps_md5_sec_info
     - Reference to the structure containing user name, password
       and nonce.

RETURN VALUE:
SYNCMLDM_SEC_CREDENTIALS_T*
     - Reference to the structure containing credential string and
       it's length.

PRE-CONDITIONS:
   
POST-CONDITIONS:
  Caller MUST free the memory allocated by the callee for the 
  return parameter.

IMPORTANT NOTES:
  None
************************************************************************/
SYNCMLDM_SEC_CREDENTIALS_T* syncmldm_sec_build_digest_cred(
                    const SYNCMLDM_MD5_SEC_INFO_T *ps_md5_sec_info)
{  
  UINT8 ab_savebytes[MAXIMUM_SAVE_BYTES];    

  UINT32 dw_client_cred_length = 0;
  UINT32 dw_offset = 0;  

  MD5_CTX s_md5_ctx;
  UINT8   ab_hash[SYNCML_DM_HASHLEN]; /* Contains the hash data after 
                                       * smlMD5Final function is called 
                                       */

  SYNCMLDM_SEC_CREDENTIALS_T *ps_security_credential = NULL;

  memset(ab_hash, '\0', SYNCML_DM_HASHLEN);
  memset(ab_savebytes, '\0', MAXIMUM_SAVE_BYTES);
  
  if ((ps_md5_sec_info == NULL) ||
      (ps_md5_sec_info->pb_user_name_or_server_id == NULL) || 
      (ps_md5_sec_info->pb_password == NULL) )
  {        
     return ps_security_credential;
  }

  ps_security_credential = (SYNCMLDM_SEC_CREDENTIALS_T*)
      DmAllocMem((sizeof(UINT16) + (sizeof(UINT8) + SYNCML_DM_HASHLEN)));
  if(ps_security_credential == NULL)
  {
    return ps_security_credential;
  }      
  
  memset(ps_security_credential->ab_credential_string, '0', SYNCML_DM_HASHLEN);

  
  smlMD5Init(&s_md5_ctx);  
  smlMD5Update(&s_md5_ctx, ps_md5_sec_info->pb_user_name_or_server_id, 
        DmStrlen((CPCHAR)ps_md5_sec_info->pb_user_name_or_server_id));
  UINT8 *p_nonce = NULL;
  if ( NULL != ps_md5_sec_info->pb_nonce )
  {
     int nonceLen = DmStrlen((CPCHAR)ps_md5_sec_info->pb_nonce);
     p_nonce = DmAllocMem(nonceLen + 2);
     if(p_nonce == NULL)
     {
        DmFreeMem(ps_security_credential);
        return (SYNCMLDM_SEC_CREDENTIALS_T*)NULL;      
     }
     memcpy(p_nonce + 1, ps_md5_sec_info->pb_nonce, nonceLen);
     p_nonce[0]= ':';
     p_nonce[nonceLen]= ':';
     smlMD5Update(&s_md5_ctx, (UINT8*)p_nonce, nonceLen + 2);
  }
  else
  {
     smlMD5Update(&s_md5_ctx, (UINT8*)":", 1);
  }
  smlMD5Update(&s_md5_ctx, ps_md5_sec_info->pb_password, 
               DmStrlen((CPCHAR)ps_md5_sec_info->pb_password));
  smlMD5Final(ab_hash, &s_md5_ctx);

  memcpy(ps_security_credential->ab_credential_string, 
           &ab_hash, 
           SYNCML_DM_HASHLEN);

  if ( NULL != p_nonce )
  {
     DmFreeMem(p_nonce);
     p_nonce = NULL;
  }

  return ps_security_credential;
}

#ifdef __cplusplus
}
#endif
