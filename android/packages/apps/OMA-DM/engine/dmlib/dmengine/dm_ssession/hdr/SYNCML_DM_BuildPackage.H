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

#ifndef SYNCML_DM_BUILDPACKAGE_H
#define SYNCML_DM_BUILDPACKAGE_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: SYNCML_DM_BuildPackage.H

    General Description: This contains declaration of SYNCML_DM_BuildPackage calss.

==================================================================================================*/

#include "syncml_dm_data_types.h"
#include "dm_tree_typedef.h"
#include "dmtFirmAlert.hpp"
#include "dmSessionDefs.h"
#include "dmClientSrvCreds.h"

/* Include header files required in any SyncML session */
extern "C" {
#include "sml.h"
#include "smldef.h"
#include "smldevinfdtd.h"
#include "smlmetinfdtd.h"
#include "smldmtnddtd.h"
#include "mgrutil.h"
#include "smlerr.h"
}

enum {
    SYNCML_DM_FACTORY_BOOTSTRAP_UNKNOWN = 0,
    SYNCML_DM_FACTORY_BOOTSTRAP_FCS11384,
    SYNCML_DM_FACTORY_BOOTSTRAP_FCS14345
};
typedef INT8 SYNCML_DM_FACTORY_BOOTSTRAP_T;

static char DMACC_FACTORY_BOOTSTRAP_CLIENTPW_FCS11384[] = {0xeb, 0xe8, 0xef, 0xee, 0xec, 0};
static char DMACC_FACTORY_BOOTSTRAP_SERVERPW_FCS11384[] = {0xfc, 0xe9, 0xe2, 0xe4, 0xe0, 0};
static char DMACC_FACTORY_BOOTSTRAP_USERNAME_FCS11384[] = {0xe0, 0xe5, 0xe7, 0xea, 0xeb, 0};

// Openwave
static char DMACC_FACTORY_BOOTSTRAP_CLIENTPW_FCS14345[] = {0xeb, 0xe8, 0xef, 0xee, 0xec, 0xec, 0};
static char DMACC_FACTORY_BOOTSTRAP_SERVERPW_FCS14345[] = {0xfc, 0xe9, 0xe2, 0xe4, 0xe0, 0xe0, 0};
static char DMACC_FACTORY_BOOTSTRAP_USERNAME_FCS14345[] = {0xe0, 0xe5, 0xe7, 0xea, 0xeb, 0xeb, 0};


class DMProcessScriptSession;

class SYNCML_DM_BuildPackage
{   
  public:
        /* Class constructor */
    SYNCML_DM_BuildPackage();
      
    /* Class destructor */
    virtual ~SYNCML_DM_BuildPackage();

    /* Operators to allocate and delete memory for operation */
    void *operator new(size_t);
    void operator delete (void *);

    void Cleanup();
    void Init(DMProcessScriptSession *pSession);

           /* This function will be called by HandleEndMessage to build up the SyncML package header.*/
    SYNCML_DM_RET_STATUS_T BuildStartSyncHdr(SmlSyncHdrPtr_t p_Content, BOOLEAN isPackageOne);
    SYNCML_DM_RET_STATUS_T BuildFinishSyncHdr(SYNCML_DM_CHAL_TYPE_T serverChalType);

    SYNCML_DM_RET_STATUS_T BuildStatus(SmlStatusPtr_t p_status );

    SYNCML_DM_RET_STATUS_T BuildResultsCommand( SmlResultsPtr_t p_results );
        /* This function will be called by other functions to build up status for responding DM 
         * package. */
    SmlStatusPtr_t AllocateStatus(
            UINT8          *p_CmdRefData,        /* Receiving DM package command Id */
            UINT8          *p_CmdName,
            UINT8          *p_SourceRefData,
            UINT8          *p_TargetRefData,
            DM_CHALLENGE_T *pClientChal,
            SYNCML_DM_RET_STATUS_T  status_Code,
            const DMStringVector*   responses );


    /* This function will be called by BuildPackageOne and HandleStartMessage to add the last
     * element to the DM package. */
    SYNCML_DM_RET_STATUS_T EndSyncmlDoc(Boolean_t  final);


    SYNCML_DM_RET_STATUS_T AllocateResult(SmlResultsPtr_t & p_results,
                                               CPCHAR p_target_uri,
                                               CPCHAR p_CmdIdRef,
                                               DMGetData *p_get_ret_data,
                                               BOOLEAN  is_ThisGetStructResult,
                                               BOOLEAN  isFirstGetStruct,
                                               BOOLEAN  isThisGetPropResult,
                                               CPCHAR szMsgID,
                                               SmlPcdataPtr_t p_data=NULL );

    SYNCML_DM_RET_STATUS_T AllocateTndsResult(CPCHAR pStrTargetUri,
                                          DMGetData *p_get_ret_data,
                                          const SYNCML_DM_GET_ON_LIST_RET_DATA_T& oGetStructData,
                                          SmlPcdataPtr_t p_data );

    BOOLEAN ListTndsProp( CPCHAR uri, CPCHAR prop, BOOLEAN &listProp);

    /* BuildPackageOne and the HandleStatus will call this function to build up the ALERT 1226
     * command */
    SYNCML_DM_RET_STATUS_T BuildAlert1226Command();
    //SYNCML_DM_RET_STATUS_T BuildAlert1226Command (DMFirmAlertVector * pFirmAlerts);
    SYNCML_DM_SESSION_DIRECTION_T getDirection() const;

    /* BuildPackageOne and the HandleStatus will call this function to build up the ALERT
     * command with management session type information. */
    SYNCML_DM_RET_STATUS_T BuildAlertCommand(UINT16 alertCode, CPCHAR pSource, CPCHAR pTarget);
      
    /* BuildPackageOne and the HandleStatus will call this function to build up the REPLACE
     * command with DenInfo data from the DM tree. */
    SYNCML_DM_RET_STATUS_T BuildReplaceCommand();
        
    /* This function will be called by SessionStart to build up DM package one with device 
     * information. */
    SYNCML_DM_RET_STATUS_T BuildPackageOne(CPCHAR pServerID, DmtSessionProp * pSessionProp);

    /* This function will be called by BuildPackage functions to build up PC data structure.*/
    void BuildPcData(SmlPcdataPtr_t  p_PcData,
                        SmlPcdataType_t content_Type,
                        SmlPcdataExtension_t extension,
                        UINT32 dataSize,
                        UINT8 *p_Content);

    /* This function will be called by BuildPackage functions to build up the meta data 
     * structure */
    void BuildMetaInfo(SmlPcdataPtr_t       p_PcData,
                         UINT8 *p_Format,     UINT8 *p_Type,
                         UINT8 *p_Mark,       UINT8 *p_Size,  
                         UINT8 *p_NextNonce,  UINT8 *p_Version,
                         UINT8 *p_Maxmsgsize, UINT8 *p_Maxobjsize);

    SYNCML_DM_RET_STATUS_T BuildMetaSizeInfo(SmlPcdataPtr_t       p_PcData, int datasize);

    /* To access the MaxMessageSize data member */
    UINT32 GetMaxMessageSize() const;
        
    /* To set the MaxMessageSize data member */
    void SetMaxMessageSize(UINT32 passedin_MaxMsgSize);

    /* To access the MaxObjectSize data member */
    UINT32 GetMaxObjectSize() const;
        
    /* To set the MaxObjectSize data member */
    void SetMaxObjectSize(UINT32 passedin_MaxObjSize);

    inline const char * GetMsgRef() {return pMessageIdOfServer;}

    inline void SetMsgRef(CPCHAR pMsgRef) {pMessageIdOfServer = pMsgRef;}

    /* Free the memory of the p_GetStructData */
    void  FreeGetStructData(SYNCML_DM_GET_ON_LIST_RET_DATA_T& p_GetStructData);

    static SYNCML_DM_RET_STATUS_T GetClientAuthValues( const DMString& device_id,
                                                     const DMString& server_id,
                                                     DMString&       client_name,
                                                     DMString&       client_password );

    static SYNCML_DM_RET_STATUS_T GetServerAuthValues( const DMString& device_id,
                                                     const DMString& server_id,
                                                     DMString&       server_password );

    static SYNCML_DM_RET_STATUS_T GetDeviceID( DMString& device_id );

    SYNCML_DM_RET_STATUS_T DecodBase64Data(SmlItemPtr_t &p_add_item, DMAddData & oAddData, BOOLEAN decodeB64);
#ifdef LOB_SUPPORT
    BOOLEAN IsProcessingLargeObject() const{ return m_bProcessingLargeObject; }
    SYNCML_DM_RET_STATUS_T GenerateAlertForLOB(DMCommandType currentComm);

     DMCommandType GetLargeObjectCmdType() const { return largeObjectCmd;}
    SYNCML_DM_RET_STATUS_T LargeObjectRecvFirstChunk(DMAddData & oAddData,
                                                      DMCommandType cmdType,
                                                      SmlItemPtr_t p_data_item);
    SYNCML_DM_RET_STATUS_T LargeObjectRecvNextChunk(DMAddData & oAddData,
                                                      DMCommandType cmdType,
                                                      SmlItemPtr_t p_data_item);
    BOOLEAN LargeObjectSendNextChunk(SYNCML_DM_RET_STATUS_T &dm_stat, BOOLEAN &isLastChunk);
    SYNCML_DM_RET_STATUS_T LargeObjectSendFirstChunk(SYNCML_DM_RESULT_VALUE &pResult);
    void LargeObjectClear();
#endif
    /* create factory boot strap user name */
    static SYNCML_DM_RET_STATUS_T CreateFactoryBootStrapUserName( SYNCML_DM_FACTORY_BOOTSTRAP_T bootStrapCode,
                                                               const DMString&               server_id,
                                                               const DMString&               devIMEI,
                                                               DMString&                     user_name );

    
    /* create factory boot strap password for client or server */
     static SYNCML_DM_RET_STATUS_T CreateFactoryBootStrapServerPW( SYNCML_DM_FACTORY_BOOTSTRAP_T bootStrapCode,
                                                               const DMString&       server_id,
                                                               const DMString&       abIMEI,
                                                               DMString&             server_password );
    
    /* create factory boot strap password for client or server */
     static SYNCML_DM_RET_STATUS_T CreateFactoryBootStrapClientPW( SYNCML_DM_FACTORY_BOOTSTRAP_T bootStrapCode,
                                                               const DMString&       server_id,
                                                               const DMString&       abIMEI,
                                                               DMString&             client_password );
     
     static SYNCML_DM_RET_STATUS_T BuildFactoryBootstrapCredData(DMString& pp_credential_data,
                                                            CPCHAR p_CredData,
                                                            CPCHAR p_CredData1);

     /* get factory bootstap code with user name */
     static SYNCML_DM_FACTORY_BOOTSTRAP_T GetBootStrapCodeUserName( const DMString& user_name );

     /* get factory bootstap code with client password */
     static SYNCML_DM_FACTORY_BOOTSTRAP_T GetBootStrapCodeClientPW(const DMString& password);

     /* get factory bootstap code with server password */
     static SYNCML_DM_FACTORY_BOOTSTRAP_T GetBootStrapCodeServerPW(const DMString& password );

     static CPCHAR GetIMEINumber(CPCHAR devID);
     BOOLEAN IsSessionId();  
 private:
        /* This function will be called by BuildPackage functions to allocate memory for PC data 
           structue and call BuildPcData to build up the structure.*/
    void BuildPcDataWAllocMem (SmlPcdataPtr_t  *pp_PcData,
                                   UINT32 dataSize,
                                   UINT8 *p_Content);

 
    /* Get the remaining Security Info from the DM Tree.*/
    SYNCML_DM_RET_STATUS_T GetRemainingSecInfo(CPCHAR pParentName,
                                              DMClientServerCreds *pClientServerCreds,
                                              CPCHAR devID);

    
     BOOLEAN IsLargerThanMaxObjSize( int dataSize, BOOLEAN isESN, BOOLEAN isToServer);
#ifdef LOB_SUPPORT
     DmtDataChunk * AllocateChunkBuffer();
     SYNCML_DM_RET_STATUS_T SetChunkData(DMAddData & oAddData, DMCommandType cmdType, BOOLEAN isLastChunk);
     SYNCML_DM_RET_STATUS_T SetChunkDataToEngine(DMAddData & oAddData, BOOLEAN isAllSet);
     SYNCML_DM_RET_STATUS_T GetLargeObjectTargetURI(SmlItemPtr_t p_get_item);
     SYNCML_DM_RET_STATUS_T GetChunkData(DMBuffer & oSendData, int bufSize);
     SYNCML_DM_RET_STATUS_T GetESNData2TempFile(int & dataSize);
     SYNCML_DM_RET_STATUS_T CreateESNTempFile();
     SYNCML_DM_RET_STATUS_T OpenESNTempFile();
     SYNCML_DM_RET_STATUS_T GetChunkDataFromEngine();
     SYNCML_DM_RET_STATUS_T AllocateLargeObjectResult(SmlResultsPtr_t & p_results);
     SYNCML_DM_RET_STATUS_T BuildLargeObjectDataLast(SmlResultsPtr_t & p_results, int size, BOOLEAN isFirstChunk);
     SYNCML_DM_RET_STATUS_T BuildLargeObjectDataNext(SmlResultsPtr_t & p_results, int dataBufferSize);
     SYNCML_DM_RET_STATUS_T LargeObjectSendFirstChunkNoSpace(SYNCML_DM_RESULT_VALUE &pResult ,int size , int dataBufferSize );
     SYNCML_DM_RET_STATUS_T  LargeObjectSendGetFreeSpace(  SmlResultsPtr_t p_results,
                                                            SmlItemPtr_t p_results_item ,
                                                            int &size , 
                                                            int  &originalSize , 
                                                            int  &dataBufferSize, 
                                                            MemSize_t &freeSpace  );
     SYNCML_DM_RET_STATUS_T  LargeObjectRecvDecodeData(DMAddData & oAddData, DMCommandType cmdType, SmlItemPtr_t p_data_item);

      SYNCML_DM_RET_STATUS_T LargeObjectRecvLastChunk(DMAddData & oAddData,
                                                      DMCommandType cmdType,
                                                      SmlItemPtr_t p_data_item);

     SYNCML_DM_RET_STATUS_T   IsRecveivingLastChunk(DMAddData & oAddData,
                                                      BOOLEAN bMoreData,
                                                      int  &size,
                                                      BOOLEAN &lsLastChunk);

    SYNCML_DM_RET_STATUS_T  PrepareESNDataBuffer(UINT8 **pEncData, UINT32 & encLen , UINT8 **bufp);
    SYNCML_DM_RET_STATUS_T SetLastChunkData(DMAddData & oAddData,DMCommandType cmdType);
    SYNCML_DM_RET_STATUS_T  SetLastChunkDataFromFile(DMAddData & oAddData,
                                                        DMBuffer  &lastData,
                                                        UINT8**szBuf,
                                                        int chunksize,
                                                        UINT8  *decDataBuf,
                                                        UINT32 decMaxBufLen,
                                                        UINT32 &lastChunkSize);
    SYNCML_DM_RET_STATUS_T  SetLastChunkRemainData(DMAddData & oAddData,
                                                        DMBuffer  &lastData,
                                                        UINT8* szBuf,
                                                        int chunksize,
                                                        UINT8  *decDataBuf,
                                                        UINT32 decMaxBufLen,
                                                        UINT32 lastChunkSize);
    SYNCML_DM_RET_STATUS_T PreProcessFirstMoreData(DMAddData & oAddData,
                                                  DMCommandType cmdType,
                                                  SmlItemPtr_t p_data_item,
                                                  int &size );

#endif
 
    SmlSyncHdrPtr_t pSyncHdr;        /* Pointer to the SyncHdr memory.*/

    DMString    pMessageIdOfServer;  /* Server message id reference */
    UINT32      MaxMessageSize;     /* Maximum message size can be handled */
    UINT32       MaxObjectSize;        /* Maximum object size can be handled */

    UINT16      commandId;  /* Sending package Command id reference */
    UINT16      messageId;  /* Message id reference, static across the BuildObjs in a
                                         * Session. */
    InstanceID_t sendInstanceId;    /* Reference for Send DM doc SyncML instance */    
    DMProcessScriptSession * pDmMgmtSessionObj; /* Pointer to the MgmtSessionObj */
    SYNCML_DM_SESSION_DIRECTION_T sessionDirection;  /* Remember the SessionDirection */
    DMGetData m_oAccData;
    DMFirmAlertVector firmAlertVec; /* used to store firmAlertVector, rebuild 1226 in case authentication failure*/
#ifdef LOB_SUPPORT
    BOOLEAN m_bProcessingLargeObject;
    BOOLEAN  m_bESN;
    BOOLEAN m_bBinary;
    DmtDataChunk *m_pChunkData;
    UINT32 m_ChunkOffset; 

    /* Large obect delivery */
    int largeObjectCmdSize;
    int largeObjectSize;
    int largeObjectBufferUsedSize;
    UINT32 largeObjectChunkSize;
    int largeObjectChunkOffset;

    DMBuffer  largeObjectBuffer;
    DMString  largeObjectCmdRef;
    DMString  largeObjectMsgRef;
    DMBuffer  largeObjectTURI;
    DMBuffer  largeObjectSURI;
    DMBuffer  largeObjectType;
    SYNCML_DM_FORMAT_T  largeObjectFormat;
    DMCommandType largeObjectCmd;
    DMFileHandler *largeObjFileHandle;
    DMString largeObjFileName;
#endif
};
  
#endif /* SYNCML_DM_BUILDPACKAGE_H */
