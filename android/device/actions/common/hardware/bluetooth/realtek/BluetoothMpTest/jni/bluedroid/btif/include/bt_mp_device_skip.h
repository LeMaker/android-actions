#ifndef _BT_MP_DEVICE_SKIP_H
#define _BT_MP_DEVICE_SKIP_H
#include "bt_mp_base.h"


int BTDevice_SetPacketType_NOSUPPORT(BT_DEVICE *pBtDevice,BT_PKT_TYPE PktType);
int BTDevice_SetWhiteningCoeffEnable_NOSUPPORT(BT_DEVICE *pBtDevice,unsigned char WhiteningCoeffEnable);
int BTDevice_SetPayloadType_NOSUPPORT(BT_DEVICE *pBtDevice,BT_PAYLOAD_TYPE PayloadType);
int BTDevice_SetTxChannel_NOSUPPORT(BT_DEVICE *pBtDevice,unsigned char ChannelNumber);
int BTDevice_SetRxChannel_NOSUPPORT(BT_DEVICE *pBtDevice,unsigned char ChannelNumber);
int BTDevice_SetPowerGain_NOSUPPORT(BT_DEVICE *pBtDevice,unsigned char PowerGainValue);
int BTDevice_SetPowerDac_NOSUPPORT(BT_DEVICE *pBtDevice,unsigned char DacValue);
int BTDevice_SetTestMode_NOSUPPORT(BT_DEVICE *pBtDevice,BT_TEST_MODE TestMode);
int BTDevice_SetFWPowerTrackEnable_NOSUPPORT(BT_DEVICE *pBtDevice,unsigned char FWPowerTrackEnable);
int BTDevice_SetHitTarget_NOSUPPORT(BT_DEVICE *pBtDevice,ULONG64 HitTarget);
int BTDevice_SetFWPowerTrackEnable_NOSUPPORT(BT_DEVICE *pBtDevice,unsigned char FWPowerTrackEnable);
int BTDevice_SetHoppingMode_NOSUPPORT(BT_DEVICE *pBtDevice,BT_PKT_TYPE PKTTYPE);
int BTDevice_SetHCIReset_NOSUPPORT(BT_DEVICE *pBtDevice,int Delay_mSec);
int BTDevice_SetPowerGainIndex_NOSUPPORT(BT_DEVICE *pBtDevice,int Index);
int BTDevice_GetBTClockTime_NOSUPPORT(BT_DEVICE *pBtDevice,unsigned long btClockTime);



#endif 