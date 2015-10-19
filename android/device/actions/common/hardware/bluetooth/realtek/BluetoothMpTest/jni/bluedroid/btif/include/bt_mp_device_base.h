#ifndef __BT_MP_DEVICE_BASE_H
#define __BT_MP_DEVICE_BASE_H


#include "bt_mp_base.h"
#include <stdio.h>

int BTBASE_HitTargetAccessCodeGen(BT_DEVICE *pBtDevice,ULONG64 HitTarget,unsigned long *pAccessCode);
int BTBASE_GetPayLoadTypeValidFlag(BT_DEVICE *pBtDevice,BT_TEST_MODE TestMode,BT_PKT_TYPE PKT_TYPE,unsigned int *ValidFlag);
//int BTBASE_MEMCPY(void *pDes,void *pSrc,int SIZE);

//Device BASE Function
int BTDevice_SetMutiRxEnable(BT_DEVICE *pBtDevice,int IsMultiPktRx);
int BTDevice_SetTxGainTable(BT_DEVICE *pBtDevice,unsigned char  *pTable);
int BTDevice_SetTxDACTable(BT_DEVICE *pBtDevice,unsigned char  *pTable);
int BTDevice_SetWhiteningCoeffEnable(BT_DEVICE *pBtDevice,unsigned char WhiteningCoeffEnable);
int BTDevice_SetTxChannel(BT_DEVICE *pBtDevice,unsigned char ChannelNumber);
int BTDevice_SetPacketType(BT_DEVICE *pBtDevice,BT_PKT_TYPE PktType);
int BTDevice_SetFWPowerTrackEnable(BT_DEVICE *pBtDevice,unsigned char FWPowerTrackEnable);
int BTDevice_SetHitTarget(BT_DEVICE *pBtDevice,ULONG64 HitTarget);
int BTDevice_SetHciReset(BT_DEVICE *pBtDevice,int Delay_mSec);
int BTDevice_GetBTClockTime(BT_DEVICE *pBtDevice,unsigned long *btClockTime);
int BTDevice_SetHoppingMode(BT_DEVICE *pBtDevice,BT_PKT_TYPE pktType);
int BTDevice_SetResetMDCount(BT_DEVICE *pBtDevice);
int BTDevice_SetLETxChannel(BT_DEVICE *pBtDevice,unsigned char ChannelNumber);
int BTDevice_SetPesudoOuterSetup(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam);
int BTDevice_SetPackHeader(BT_DEVICE *pBtDevice,unsigned int packHeader);

int BTDevice_CalculatedTxBits(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport,int pktTx_conTx,unsigned long  *txbits,unsigned long *txpkt_cnt);

int
BTDevice_RecvAnyHciEvent(
	BT_DEVICE *pBtDevice,
	unsigned char  *pEvent
	);
int
BTDevice_SendHciCommandWithEvent(
	BT_DEVICE *pBtDevice,
	unsigned int  OpCode,
	unsigned char PayLoadLength,
	unsigned char *pPayLoad,
	unsigned char  EventType,
	unsigned char  *pEvent,
	unsigned char  *pEventLen
	);        
int BTDevice_GetPayloadLenTable(BT_DEVICE *pBtDevice,unsigned char  *pTable,int length);
int BTDevice_SetContinueTxBegin(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);
int BTDevice_SetContinueTxStop(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);
int BTDevice_SetContinueTxUpdate(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);
int BTDevice_SetPktTxBegin(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);
int BTDevice_SetPktTxUpdate(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);
int BTDevice_SetPktTxStop(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);
int BTDevice_SetPktRxBegin(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);
int BTDevice_SetPktRxUpdate(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);
int BTDevice_SetPktRxStop(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);

#endif