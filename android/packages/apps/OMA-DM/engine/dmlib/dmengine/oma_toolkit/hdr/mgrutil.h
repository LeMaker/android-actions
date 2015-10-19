/*************************************************************************/
/* module:          SyncML API for freeing SyncML C structures           */
/*                                                                       */   
/* file:            mgrutil.h                                            */
/* target system:   all                                                  */
/* target OS:       all                                                  */   
/*                                                                       */   
/* Description:                                                          */   
/* Definitions for internal use within the SyncML implementation         */
/*************************************************************************/



/*
 * Copyright Notice
 * Copyright (c) Ericsson, IBM, Lotus, Matsushita Communication 
 * Industrial Co., Ltd., Motorola, Nokia, Openwave Systems, Inc., 
 * Palm, Inc., Psion, Starfish Software, Symbian, Ltd. (2001).
 * All Rights Reserved.
 * Implementation of all or part of any Specification may require 
 * licenses under third party intellectual property rights, 
 * including without limitation, patent rights (such a third party 
 * may or may not be a Supporter). The Sponsors of the Specification 
 * are not responsible and shall not be held responsible in any 
 * manner for identifying or failing to identify any or all such 
 * third party intellectual property rights.
 * 
 * THIS DOCUMENT AND THE INFORMATION CONTAINED HEREIN ARE PROVIDED 
 * ON AN "AS IS" BASIS WITHOUT WARRANTY OF ANY KIND AND ERICSSON, IBM, 
 * LOTUS, MATSUSHITA COMMUNICATION INDUSTRIAL CO. LTD, MOTOROLA, 
 * NOKIA, PALM INC., PSION, STARFISH SOFTWARE AND ALL OTHER SYNCML 
 * SPONSORS DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING 
 * BUT NOT LIMITED TO ANY WARRANTY THAT THE USE OF THE INFORMATION 
 * HEREIN WILL NOT INFRINGE ANY RIGHTS OR ANY IMPLIED WARRANTIES OF 
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
 * SHALL ERICSSON, IBM, LOTUS, MATSUSHITA COMMUNICATION INDUSTRIAL CO., 
 * LTD, MOTOROLA, NOKIA, PALM INC., PSION, STARFISH SOFTWARE OR ANY 
 * OTHER SYNCML SPONSOR BE LIABLE TO ANY PARTY FOR ANY LOSS OF 
 * PROFITS, LOSS OF BUSINESS, LOSS OF USE OF DATA, INTERRUPTION OF 
 * BUSINESS, OR FOR DIRECT, INDIRECT, SPECIAL OR EXEMPLARY, INCIDENTAL, 
 * PUNITIVE OR CONSEQUENTIAL DAMAGES OF ANY KIND IN CONNECTION WITH 
 * THIS DOCUMENT OR THE INFORMATION CONTAINED HEREIN, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH LOSS OR DAMAGE.
 * 
 * The above notice and this paragraph must be included on all copies 
 * of this document that are made.
 * 
 */



#ifndef _MGR_UTIL_H
  #define _MGR_UTIL_H


/* Prototypes of exported SyncML API functions */
SML_API Ret_t smlFreeProtoElement(VoidPtr_t pProtoElement);
SML_API void smlFreePcdata(SmlPcdataPtr_t pPcdata);
SML_API void smlFreePcdataList(SmlPcdataListPtr_t list);

SML_API void smlFreeSyncHdr(SmlSyncHdrPtr_t pSyncHdr);
SML_API void smlFreeSync(SmlSyncPtr_t pSync);
SML_API void smlFreeGeneric(SmlGenericCmdPtr_t pGenericCmd);
SML_API void smlFreeAlert(SmlAlertPtr_t pAlert);
SML_API void smlFreeAtomic(SmlAtomicPtr_t pAtomic);
#if (defined EXEC_SEND || defined EXEC_RECEIVE)
  SML_API void smlFreeExec(SmlExecPtr_t pExec);
#endif
SML_API void smlFreeGetPut(SmlPutPtr_t pGetPut);
SML_API void smlFreeMap(SmlMapPtr_t pMap);
SML_API void smlFreeResults(SmlResultsPtr_t pResults);
#if (defined SEARCH_SEND || defined SEARCH_RECEIVE)
  SML_API void smlFreeSearch(SmlSearchPtr_t pSearch);
#endif
SML_API void smlFreeStatus(SmlStatusPtr_t pStatus);
SML_API void smlFreeCredPtr(SmlCredPtr_t pCred);
SML_API void smlFreeChalPtr(SmlChalPtr_t pChal);
SML_API void smlFreeSourceTargetPtr(SmlSourcePtr_t pSourceTarget);
SML_API void smlFreeSourceList(SmlSourceListPtr_t pSourceList);
SML_API void smlFreeSourceRefList(SmlSourceRefListPtr_t pSourceRefList);
SML_API void smlFreeTargetRefList(SmlTargetRefListPtr_t pTargetRefList);
SML_API void smlFreeItemPtr(SmlItemPtr_t pItem);
SML_API void smlFreeItemList(SmlItemListPtr_t pItemList);
SML_API void smlFreeMapItemPtr(SmlMapItemPtr_t pMapItem);
SML_API void smlFreeMapItemList(SmlMapItemListPtr_t pMapItemList);

#ifdef __USE_METINF__
SML_API void smlFreeMetinfAnchor(SmlMetInfAnchorPtr_t data);
SML_API void smlFreeMetinfMem(SmlMetInfMemPtr_t data);
SML_API void smlFreeMetinfMetinf(SmlMetInfMetInfPtr_t data);
#endif
#ifdef __USE_DEVINF__
SML_API void smlFreeDevInfDatastore(SmlDevInfDatastorePtr_t data);
SML_API void smlFreeDevInfDatastoreList(SmlDevInfDatastoreListPtr_t data);
SML_API void smlFreeDevInfXmitList(SmlDevInfXmitListPtr_t data);
SML_API void smlFreeDevInfXmit(SmlDevInfXmitPtr_t data);
SML_API void smlFreeDevInfDSMem(SmlDevInfDSMemPtr_t data);
SML_API void smlFreeDevInfSynccap(SmlDevInfSyncCapPtr_t data);
SML_API void smlFreeDevInfExt(SmlDevInfExtPtr_t data);
SML_API void smlFreeDevInfExtList(SmlDevInfExtListPtr_t data);
SML_API void smlFreeDevInfCTData(SmlDevInfCTDataPtr_t data);
SML_API void smlFreeDevInfCTDataList(SmlDevInfCTDataListPtr_t data);
SML_API void smlFreeDevInfCTDataProp(SmlDevInfCTDataPropPtr_t data);
SML_API void smlFreeDevInfCTDataPropList(SmlDevInfCTDataPropListPtr_t data);
SML_API void smlFreeDevInfCTCap(SmlDevInfCTCapPtr_t data);
SML_API void smlFreeDevInfCtcapList(SmlDevInfCtcapListPtr_t data);
SML_API void smlFreeDevInfDevInf(SmlDevInfDevInfPtr_t data);
#endif

#ifdef __USE_DMTND__
SML_API void smlFreeDmTnd(SmlDmTndPtr_t data);
#endif

#ifndef __SML_LITE__  /* these API calls are NOT included in the Toolkit lite version */
SML_API String_t smlPcdata2String( SmlPcdataPtr_t pcdata );
SML_API SmlPcdataPtr_t smlString2Pcdata( String_t str );
SML_API SmlPcdataPtr_t smlPcdataDup(SmlPcdataPtr_t pcdata); 
SML_API MemSize_t smlGetFreeBuffer(InstanceID_t id);
#endif

#ifdef __USE_ALLOCFUNCS__
SML_API SmlPcdataPtr_t smlAllocPcdata();
SML_API SmlPcdataListPtr_t smlAllocPcdataList();
SML_API SmlChalPtr_t smlAllocChal();
SML_API SmlCredPtr_t smlAllocCred();
SML_API SmlSourcePtr_t smlAllocSource();
SML_API SmlTargetPtr_t smlAllocTarget();
SML_API SmlSourceListPtr_t smlAllocSourceList();
SML_API SmlSyncHdrPtr_t smlAllocSyncHdr();
SML_API SmlItemPtr_t smlAllocItem();
SML_API SmlItemListPtr_t smlAllocItemList();
SML_API SmlGenericCmdPtr_t smlAllocGeneric();
SML_API SmlAddPtr_t smlAllocAdd();
SML_API SmlCopyPtr_t smlAllocCopy();
SML_API SmlReplacePtr_t smlAllocReplace();
SML_API SmlDeletePtr_t smlAllocDelete();
SML_API SmlAlertPtr_t smlAllocAlert();
SML_API SmlAtomicPtr_t smlAllocAtomic();
SML_API SmlSequencePtr_t smlAllocSequence();
SML_API SmlSyncPtr_t smlAllocSync();
SML_API SmlExecPtr_t smlAllocExec();
SML_API SmlGetPtr_t smlAllocGet();
SML_API SmlPutPtr_t smlAllocPut();
SML_API SmlMapItemPtr_t smlAllocMapItem();
SML_API SmlMapItemListPtr_t smlAllocMapItemList();
SML_API SmlMapPtr_t smlAllocMap();
SML_API SmlResultsPtr_t smlAllocResults();
SML_API SmlSearchPtr_t smlAllocSearch();
SML_API SmlTargetRefListPtr_t smlAllocTargetRefList();
SML_API SmlSourceRefListPtr_t smlAllocSourceRefList();
SML_API SmlStatusPtr_t smlAllocStatus();
SML_API SmlUnknownProtoElementPtr_t smlAllocUnknownProtoElement();
#ifdef __USE_METINF__
SML_API SmlMetInfMetInfPtr_t smlAllocMetInfMetInf();
SML_API SmlMetInfAnchorPtr_t smlAllocMetInfAnchor();
SML_API SmlMetInfMemPtr_t smlAllocMetInfMem();
#endif // MetInf

#ifdef __USE_DEVINF__
SML_API SmlDevInfExtPtr_t smlAllocDevInfExt();
SML_API SmlDevInfExtListPtr_t smlAllocDevInfExtList();
SML_API SmlDevInfSyncCapPtr_t smlAllocDevInfSyncCap();
SML_API SmlDevInfCTDataPtr_t smlAllocDevInfCTData();
SML_API SmlDevInfCTDataListPtr_t smlAllocDevInfCTDataList();
SML_API SmlDevInfCTDataPropPtr_t smlAllocDevInfCTDataProp();
SML_API SmlDevInfCTDataPropListPtr_t smlAllocDevInfCTDataPropList();
SML_API SmlDevInfCTCapPtr_t smlAllocDevInfCTCap();
SML_API SmlDevInfCtcapListPtr_t smlAllocDevInfCtcapList();
SML_API SmlDevInfDSMemPtr_t smlAllocDevInfDSMem();
SML_API SmlDevInfXmitPtr_t smlAllocDevInfXmit();
SML_API SmlDevInfXmitListPtr_t smlAllocDevInfXmitList();
SML_API SmlDevInfDatastorePtr_t smlAllocDevInfDatastore();
SML_API SmlDevInfDatastoreListPtr_t smlAllocDevInfDatastoreList();
SML_API SmlDevInfDevInfPtr_t smlAllocDevInfDevInf();
#endif // DevInf

#ifdef __USE_DMTND__
SML_API SmlDmTndPtr_t smlAllocDmTnd();
SML_API SmlDmTndNodeListPtr_t smlAllocDmTndNodeList();
SML_API SmlDmTndNodePtr_t smlAllocDmTndNode();
SML_API SmlDmTndRTPropsPtr_t smlAllocDmTndRTProps();
SML_API SmlDmTndFormatPtr_t smlAllocDmTndFormat();
SML_API SmlDmTndTypePtr_t smlAllocDmTndType();
#endif // DmTnd
#endif // AllocFuncs
#endif // MgrUtil.h
