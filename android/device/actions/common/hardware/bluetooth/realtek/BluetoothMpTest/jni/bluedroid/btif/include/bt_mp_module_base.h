#ifndef _BT_MP_MODULE_BASE_H
#define _BT_MP_MODULE_BASE_H

#include "bt_mp_base.h"
 
int BTModule_ActionControlExcute(
				BT_MODULE *pBtModule
	                        );

int BTModule_ActionReport(
				BT_MODULE *pBtModule,
				int ActiceItem,
                BT_DEVICE_REPORT *pReport
	            );

int BTModule_UpDataParameter(
                                BT_MODULE *pBtModule,
                                BT_PARAMETER 	*pParam
                                );


int BTModule_DownloadPatchCode(
				BT_MODULE *pBtModule,
				unsigned char *pPatchcode,
				int patchLength,
				int Mode);

int
BTModule_SendHciCommandWithEvent(
	BT_MODULE *pBtModule,
	unsigned int  OpCode,
	unsigned char PayLoadLength,
	unsigned char *pPayLoad,
	unsigned char  EventType,
	unsigned char  *pEvent,
	unsigned char  *pEventLen
	);
int
BTModule_RecvAnyHciEvent(
	BT_MODULE *pBtModule,
	unsigned char  *pEvent
	);
int
BTModule_GetMDRegMaskBits(
	BT_MODULE *pBtModule,
	unsigned char Addr,
	unsigned char Msb,
	unsigned char Lsb,
	unsigned long *pUserValue
	); 
int
BTModule_SetMDRegMaskBits(
	BT_MODULE *pBtModule,
	unsigned char Addr,
	unsigned char Msb,
	unsigned char Lsb,
	const unsigned long UserValue
	);
int
BTModule_GetRFRegMaskBits(
	BT_MODULE *pBtModule,
	unsigned char Addr,
	unsigned char Msb,
	unsigned char Lsb,
	unsigned long *pUserValue
	);
int
BTModule_SetRFRegMaskBits(
	BT_MODULE *pBtModule,
	unsigned char Addr,
	unsigned char Msb,
	unsigned char Lsb,
	const unsigned long UserValue
	);
#endif

