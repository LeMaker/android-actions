#ifndef _BT_MP_DEVICE_RTL8723A_H
#define _BT_MP_DEVICE_RTL8723A_H
#include "bt_mp_base.h"
int BTDevice_SetPayloadType_RTL8723A(BT_DEVICE *pBtDevice,BT_PAYLOAD_TYPE PayloadType);
int BTDevice_SetTxChannel_RTL8723A(BT_DEVICE *pBtDevice,unsigned char ChannelNumber);
int BTDevice_SetRxChannel_RTL8723A(BT_DEVICE *pBtDevice,unsigned char ChannelNumber);
int BTDevice_SetPowerGain_RTL8723A(BT_DEVICE *pBtDevice,unsigned char PowerGainValue);
int BTDevice_SetPowerDac_RTL8723A(BT_DEVICE *pBtDevice,unsigned char DacValue);



#endif 