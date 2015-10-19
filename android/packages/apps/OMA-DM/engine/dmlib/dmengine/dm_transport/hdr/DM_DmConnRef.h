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

#ifndef DM_DMCONNREF_H
#define DM_DMCONNREF_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

//--------------------------------------------------------------------------------------------------
//
//   Header Name: DM_DmConnRef.h
//
//   General Description: DmConnRef implementation class header file. 
//
//--------------------------------------------------------------------------------------------------

#include "dmstring.h"
#include "dmvector.h"
#include "dmt.hpp"
#include "DM_DmtUtil.h"

/**
 * The dm connection reference class. 
 */
class DmConnRef {
public:
    /**
     * The definition for the node path - ./SyncML/Con.
     */
    static const char * const SYNCML_CON;
    /**
     * The definition for the node name - NAP.
     */
    static const char * const NODE_NAME_NAP;
    /**
     * The definition for the node name - PX.
     */
    static const char * const NODE_NAME_PX;
    /**
     * The definition for the node name - Bearer.
     */
    static const char * const NODE_NAME_BEARER;
    /**
     * The definition for the node name - AddrType.
     */
    static const char * const NODE_NAME_ADDRTYPE;
    /**
     * The definition for the node name - Addr.
     */
    static const char * const NODE_NAME_ADDR;
    /**
     * The definition for the node name - Auth.
     */
    static const char * const NODE_NAME_AUTH;
    /**
     * The definition for the node name - Id.
     */
    static const char * const NODE_NAME_ID;
    /**
     * The definition for the node name - Secret.
     */
    static const char * const NODE_NAME_SECRET;
    /**
     * The definition for the node name - PortNbr.
     */
    static const char * const NODE_NAME_PORTNBR;

    /**
     * The data structure for ./SyncML/Con/w/NAP
     */
    class DmSyncmlNAP * mpNAP;

    /**
     * The data structure for ./SyncML/Con/w/PX
     */
    class DmSyncmlPX * mpPX;

    /**
     * The default deconstructor.
     */
    ~DmConnRef();

    /**
     * Create a instance for DmConnRef.
     * @param ConRef the value in the node ./SyncML/DMAcc/x/ConRef
     * @return a pointer to the DmConnRef instance. Return NULL for error.
     * @note please release the instance by invoker, after finishing to use 
     *       the instance.
     */
    static DmConnRef * Create(const char * ConRef);

    /**
     * Create a network profile.
     * @param profile the network profile name. If users pass a profile name, 
     * then this function will create a real profile in the phone. If users pass
     * a temp profile with the full path, such as /tmp/dm_tmp_profile, this function
     * will create a temp profile, and users can not see it in the UI. After finishing
     * to use the tmp profile, invoker need call DeleteNetworkProfile(const char * profile)
     * to remove the tmp profile.
     * @return true, if successful. Otherwise, return false, if any errors.
     * @note After finishing to use the tmp profile, invoker need call DeleteNetworkProfile(const char * profile)
     * to remove the tmp profile.
     */
    bool CreateNetworkProfile(INT8 * profile);

    /**
     * Delete a network profile.
     * @param profile the network profile name.
     * @return true, if successful. Otherwise, return false, if any errors.
     * @see CreateNetworkProfile(const char * profile);
     */
    bool DeleteNetworkProfile(INT8 * profile);

    /**
     * Get the proxy IP address and port, which type is IPV4.
     * @param proxy [out] the proxy IP address.
     * @param port [out] the proxy port.
     * @return true, if successful. Otherwise, return false, if any errors.
     */
    bool GetIPV4Proxy(DMString &proxy, UINT16 &port);

    /**
     * Get the proxy user and password.
     * @param user [out] the proxy user.
     * @param pwd [out] the proxy pwd.
     * @return true, if successful. Otherwise, return false, if any errors.
     */
    bool GetIPV4ProxyAuth(DMString &user, DMString &pwd);

private:
    /**
     * The protected default constructor.
     */
    DmConnRef() {
        mpNAP = NULL;
        mpPX = NULL;
    };

    /**
     * Get the DmSyncmlNAP instance.
     * @param pTree pTree the pointer to DmtTree instance.
     * @return a pointer to the DmSyncmlNAP instance. Return NULL for error.
     */
    static DmSyncmlNAP * GetNAP(PDmtTree pTree);

    /**
     * Get the DmSyncmlPX instance.
     * @param pTree pTree the pointer to DmtTree instance.
     * @return a pointer to the DmSyncmlPX instance. Return NULL for error.
     */
    static DmSyncmlPX * GetPX(PDmtTree pTree);

    /**
     * Get the DMMap<DMString, DmSyncmlAuth>. 
     * The key will be the node name of ./SyncML/Con/w/PX/Auth/z
     *         or the node name of ./SyncML/Con/w/NAP/Auth/y
     * The value will be the instance of DmSyncmlAuth.
     * @param pTree pTree the pointer to DmtTree instance.
     * @param nodeName the node name.
     * @param auths [out] the DMMap<DMString, DmSyncmlAuth>. 
     * @return true, if successful, otherwise return false.
     */
    static bool GetAuths(PDmtTree pTree, const char * nodeName,
        DMMap<DMString, class DmSyncmlAuth> &auths);
};

/**
 * This class will describe the structure for ./SyncML/Con/w/NAP/Auth and 
 * ./SyncML/Con/w/PX/Auth
 * Used as structure.
 */
class DmSyncmlAuth {
public:
    /**
     * The definition for ./SyncML/Con/w/NAP/Auth/y in SyncML dm spec.
     * NAP_AUTH_PAP is for PAP - "Password Authentication Protocol"
     * NAP_AUTH_CHAP is for CHAP - "Challenge Handshake Authentication Protocol"
     */
    static const char * const NAP_AUTH_PAP;
    static const char * const NAP_AUTH_CHAP;

    /**
     * The definition for ./SyncML/Con/w/PX/Auth/z in SyncML dm spec.
     */
    static const char * const PX_AUTH_HTTP_BASIC;
    static const char * const PX_AUTH_HTTP_DIGEST;
    static const char * const PX_AUTH_WTLS_SS;

    /**
     * ./SyncML/Con/w/NAP/Auth/y/Id
     * ./SyncML/Con/w/PX/Auth/z/Id
     */
    DMString mId;
    /**
     * ./SyncML/Con/w/NAP/Auth/y/Secret
     * ./SyncML/Con/w/PX/Auth/z/Secret
     */
    DMString mSecret;

    /**
     * The default constructor.
     */
    DmSyncmlAuth() {
    };

    /**
     * The copy constructor.
     * @param id ./SyncML/Con/w/NAP/Auth/y/Id or ./SyncML/Con/w/PX/Auth/z/Id.
     * @param secret ./SyncML/Con/w/NAP/Auth/y/Secret
     *        or ./SyncML/Con/w/PX/Auth/z/Secret.
     */
    DmSyncmlAuth(DmSyncmlAuth &auth) {
        mId = auth.mId;
        mSecret = auth.mSecret;
    };

    /**
     * The constructor.
     * @param id ./SyncML/Con/w/NAP/Auth/y/Id or ./SyncML/Con/w/PX/Auth/z/Id.
     * @param secret ./SyncML/Con/w/NAP/Auth/y/Secret
     *        or ./SyncML/Con/w/PX/Auth/z/Secret.
     */
    DmSyncmlAuth(DMString id, DMString secret) {
        mId = id;
        mSecret = secret;
    };
    
    inline DmSyncmlAuth & operator= (const DmSyncmlAuth &auth) {
        mId = auth.mId;
        mSecret = auth.mSecret;
        return *this;
    }
};

/**
 * This class will describe the structure for ./SyncML/Con/w/NAP
 * Used as structure.
 */
class DmSyncmlNAP {
public:
    /**
     * The definition for ./SyncML/Con/w/NAP/Bearer in SyncML dm spec.
     */
    enum NAP_BEARER {
        NAP_BEARER_OBEX = 1,
        NAP_BEARER_GSM_USSD = 2,
        NAP_BEARER_GSM_SMS = 3,
        NAP_BEARER_ANSI_136_GUTS = 4,
        NAP_BEARER_IS_95_CDMA_SMS = 5,
        NAP_BEARER_IS_95_CDMA_CSD = 6,
        NAP_BEARER_IS_95_CDMA_PACKET = 7,
        NAP_BEARER_ANSI_136_CSD = 8,
        NAP_BEARER_ANSI_136_GPRS = 9,
        NAP_BEARER_GSM_CSD = 10,
        NAP_BEARER_GSM_GPRS = 11,
        NAP_BEARER_AMPS_CDPD = 12,
        NAP_BEARER_PDC_CSD = 13,
        NAP_BEARER_PDC_PACKET = 14,
        NAP_BEARER_IDEN_SMS = 15,
        NAP_BEARER_IDEN_CSD = 16,
        NAP_BEARER_IDEN_PACKET = 17,
        NAP_BEARER_FLEX_REFLEX = 18,
        NAP_BEARER_PHS_SMS = 19,
        NAP_BEARER_PHS_CSD = 20,
        NAP_BEARER_TETRA_SDS = 21,
        NAP_BEARER_TETRA_PACKET = 22,
        NAP_BEARER_MOBITEX_MPAK = 23,
        NAP_BEARER_ANSI_136_GHOST = 24
    };
    
    /**
     * The definition for ./SyncML/Con/w/NAP/AddrType in SyncML dm spec.
     */
    enum NAP_ADDRTYPE {
        NAP_ADDRTYPE_IPV4 = 1,
        NAP_ADDRTYPE_IPV6 = 2,
        NAP_ADDRTYPE_E164 = 3,
        NAP_ADDRTYPE_ALPHA = 4,
        NAP_ADDRTYPE_APN = 5
    } ;

    /**
     * ./SyncML/Con/w/NAP/Bearer
     */
    NAP_BEARER mBearer;

    /**
     * ./SyncML/Con/w/NAP/AddrType
     */
    NAP_ADDRTYPE mAddrType;

    /**
     * ./SyncML/Con/w/NAP/Addr
     */
    DMString mAddr;

    /**
     * ./SyncML/Con/w/NAP/Auth
     */
    DMMap<DMString, DmSyncmlAuth> mAuth;

    /**
     * The default constructor.
     */
    DmSyncmlNAP() {
        mBearer = NAP_BEARER_GSM_GPRS;
        mAddrType = NAP_ADDRTYPE_APN;
    };

    /**
     * The constructor.
     * @param bearer ./SyncML/Con/w/NAP/Bearer
     * @param addrType ./SyncML/Con/w/NAP/AddrType
     * @param addr ./SyncML/Con/w/NAP/Addr
     * @param auth ./SyncML/Con/w/NAP/Auth
     */
    DmSyncmlNAP(NAP_BEARER bearer, NAP_ADDRTYPE addrType, DMString addr,
            DMMap<DMString, DmSyncmlAuth> auth) {
        mBearer = bearer;
        mAddrType = addrType;
        mAddr = addr;
        mAuth = auth;
    };
};

/**
 * This class will describe the structure for ./SyncML/Con/w/PX
 * Used as structure.
 */
class DmSyncmlPX {
public:
    /**
     * The definition for ./SyncML/Con/w/PX/AddrType in SyncML dm spec.
     */
    enum PX_ADDRTYPE {
        PX_ADDRTYPE_IPV4 = 1,
        PX_ADDRTYPE_IPV6 = 2,
        PX_ADDRTYPE_E164 = 3,
        PX_ADDRTYPE_ALPHA = 4
    };

    /**
     * ./SyncML/Con/w/PX/Bearer
     */
    UINT16 mPortNbr;

    /**
     * ./SyncML/Con/w/PX/AddrType
     */
    PX_ADDRTYPE mAddrType;

    /**
     * ./SyncML/Con/w/PX/Addr
     */
    DMString mAddr;

    /**
     * ./SyncML/Con/w/PX/Auth
     */
    DMMap<DMString, DmSyncmlAuth> mAuth;

    /**
     * The default constructor.
     */
    DmSyncmlPX() {
        mAddrType = PX_ADDRTYPE_IPV4;
        mPortNbr = 80;
    };

    /**
     * The constructor.
     * @param portNbr ./SyncML/Con/w/PX/PortNbr
     * @param addrType ./SyncML/Con/w/PX/AddrType
     * @param addr ./SyncML/Con/w/PX/Addr
     * @param auth ./SyncML/Con/w/PX/Auth
     */
    DmSyncmlPX(UINT16 portNbr, PX_ADDRTYPE addrType, DMString addr,
            DMMap<DMString, DmSyncmlAuth> auth) {
        mPortNbr = portNbr;
        mAddrType = addrType;
        mAddr = addr;
        mAuth = auth;
    };
};

#endif //DM_DMCONNREF_H
