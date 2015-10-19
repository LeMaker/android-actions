#ifndef _BT_MP_API_H
#define _BT_MP_API_H

#include "bt_mp_base.h"


char* BT_SendHciCmd(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer);

char* BT_GetPara(BT_MODULE  *pBtModule, char* pNotifyBuffer);

char* BT_SetPara1(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer);
char* BT_SetPara2(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer);
char* BT_SetHit(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer);

char* BT_SetDacTable(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer);
char* BT_SetGainTable(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer);

char* BT_Exec(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer);

char* BT_ReportTx(BT_MODULE  *pBtModule, char* pNotifyBuffer);
char* BT_ReportRx(BT_MODULE  *pBtModule, char* pNotifyBuffer);

char* BT_RegRf(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer);
char* BT_RegMd(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer);


void BT_GetBDAddr(BT_MODULE  *pBtModule);
void bt_mp_module_init(BASE_INTERFACE_MODULE *pBaseInterfaceModuleMemory, BT_MODULE
        *pBtModuleMemory);




#endif
