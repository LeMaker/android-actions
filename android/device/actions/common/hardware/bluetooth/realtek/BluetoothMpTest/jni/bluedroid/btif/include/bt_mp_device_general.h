#ifndef _BT_MP_DEVICE_GENERAL_H
#define _BT_MP_DEVICE_GENERAL_H

#include "bt_mp_base.h"

int BTDevice_SetPayloadType(BT_DEVICE *pBtDevice,BT_PAYLOAD_TYPE PayloadType);
int BTDevice_SetTxChannel(BT_DEVICE *pBtDevice,unsigned char ChannelNumber);
int BTDevice_SetRxChannel(BT_DEVICE *pBtDevice,unsigned char ChannelNumber);
int BTDevice_SetPowerGain(BT_DEVICE *pBtDevice,unsigned char PowerGainValue);
int BTDevice_SetPowerDac(BT_DEVICE *pBtDevice,unsigned char DacValue);
int BTDevice_SetTestMode(BT_DEVICE *pBtDevice,BT_TEST_MODE TestMode);
int BTDevice_SetPowerGainIndex(BT_DEVICE *pBtDevice,int Index);



#endif 