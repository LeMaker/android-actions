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

/*==================================================================================================

    Source Name: dmNotification.cc

    General Description: Implementation of the notification agent interface

==================================================================================================*/

#include "dmt.hpp"
#include "dmStringUtil.h"
#include "dmNotification.h"
#include "dmSessionFactory.h"
#include "xpl_Logger.h"

/*==============================================================================================
Function:    dmProcessNotification

Description: This method is called to parse out the individual bits from the received Package0
             and store their values into the defined structure.
             This method will also allocate memory as needed for the string values.

NOTE: To simplify the logic of this method, it has knowledge of the various bit-field sizes. So
      if the Spec ever changes the bit-field sizes or order of Package0, this method must be
      updated.  Using #defines for the various bit-field sizes would greatly increase the
      complexity of this method and make it more difficult to maintain.
             
==============================================================================================*/
SYNCML_DM_RET_STATUS_T
DmProcessNotification(const UINT8 *pk0Data,
                          UINT32 pk0Size,
                          DmtNotification & notification)
{
    UINT8   *pCurrData;
    UINT8   *pTrigger;
    UINT8   triggerLen;
    UINT16  tempData16;
    UINT8   serverIdLen;
    UINT32  usedSize;
    UINT8   md5Digest[DM_MD5_DIGEST_LENGTH];
    DMBuffer vendorData;
    char    server_data[24];

    SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;

    XPL_LOG_DM_SESS_Debug(("DMProcessNotification Enter\n"));

    // The Package0 MUST have at least 192 bits(24 Bytes) for this function to avoid bus errors.
    if (pk0Size < 24)
    {
        return (SYNCML_DM_FAIL);
    }
    
  // Start the current data pointer.
    pCurrData = (UINT8*)pk0Data;
    
    // The MD5 (HMAC) digest is 128 bits(16 Bytes).
    memcpy(md5Digest, pCurrData, DM_MD5_DIGEST_LENGTH);
    pCurrData = pCurrData + DM_MD5_DIGEST_LENGTH;
    
    // Save off the rest of the Pk0 since this is the Trigger portion used for the Digest.
    pTrigger = pCurrData;
    triggerLen = pk0Size - DM_MD5_DIGEST_LENGTH;

    // Grab the next 16 bits being careful to avoid endian problems.
    tempData16 = (pCurrData[0] << 8) | (pCurrData[1]);
    pCurrData = pCurrData + 2;
    
    // The DM Version is the top 10 bits.
    // DM: initialized but never used warning by ARM compiler
    // dmVersion = ((tempData16 & 0xFFC0) >> 6);
    
    // The UI Mode is the next 2 bits.
    notification.setUIMode((UINT8)((tempData16 & 0x0030) >> 4));
    
    // The Initiator is the next 1 bit.
    notification.setInitiator((UINT8)((tempData16 & 0x0008) >> 3));
    

    // The Future Reserved is 27 bits. 3 of those bits are in tempData16. So skip 24 bits.
    pCurrData = pCurrData + 3;
    
    // The SessionID is 16 bits being careful to avoid endian problems.
    tempData16 = (pCurrData[0] << 8) | (pCurrData[1]);
    pCurrData = pCurrData + 2;
    notification.setSessionID(tempData16);

    // The ServerID Length is 8 bits.
    serverIdLen = *pCurrData;
    pCurrData = pCurrData + 1;
    

    if (serverIdLen > 0 && serverIdLen < sizeof(server_data))
    {
        // The ServerID string is <length>*8 bits.
        memcpy(server_data, pCurrData, serverIdLen);
        server_data[serverIdLen] = '\0';
        notification.setServerID(server_data);
        pCurrData = pCurrData + serverIdLen;
    }
    else
        return SYNCML_DM_FAIL;
    
    
     // Calculate the amount of data we've read so far.
    usedSize = pCurrData - pk0Data;
    
    // The Vendor Specific portion is n bits. Check if there is anything left in the buffer. 
    if (pk0Size > usedSize)
    {
        // There's still information left. Copy it into the pVendorData.
        vendorData.assign(pCurrData, pk0Size - usedSize);    
        if ( vendorData.getBuffer() == NULL )
            return SYNCML_DM_DEVICE_FULL;
    }

    SYNCML_DM_AuthContext_T AuthContext;

    AuthContext._AuthFlag = TRUE;
    AuthContext._pServerId    = server_data;
    AuthContext._md5Digest    = md5Digest;
    AuthContext._pTrigger     = pTrigger;
    AuthContext._triggerLen   = triggerLen;

#ifndef DM_NOTIFICATION_AGENT_NO_AUTH
    retStatus = DmAuthenticateServer( AuthContext );
#endif

    notification.setAuthFlag( AuthContext._AuthFlag );
    XPL_LOG_DM_SESS_Debug(("DMProcessNotification Exit\n"));
    return retStatus;
}
