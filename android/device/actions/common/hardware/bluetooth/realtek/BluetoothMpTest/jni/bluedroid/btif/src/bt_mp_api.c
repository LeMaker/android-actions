#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <termios.h>
#include <time.h>

#include "bt_mp_transport.h"
#include "bt_mp_api.h"
#include "bt_mp_base.h"
#include "bt_mp_build.h"
#include "bt_user_func.h"
#include "bt_mp_device_general.h"
#include "bt_mp_device_base.h"
#include "bt_mp_device_skip.h"
#include "bt_mp_device_rtl8723a.h"
#include "bt_mp_module_base.h"
#include "foundation.h"

#include "hardware/bluetoothmp.h"



#define LOG_TAG "BTIF_MP_API"
#include "btif_api.h"

#include "gki.h"
#include "btu.h"


#include "hcidefs.h"
#include "hcimsgs.h"
#include "btif_common.h"

#include "bluetooth_mp_opcode.h"






#define DEFAULT_HIT_ADDRESS 0x0000009e8b33

#define DEFAULT_CH_NUM                      10
#define DEFAULT_PKT_TYPE                    BT_PKT_3DH5
#define DEFAULT_PAYLOAD_TYPE            BT_PAYLOAD_TYPE_PRBS9
#define DEFAULT_PKT_COUNT                   0
#define DEFAULT_TX_GAIN_VALUE               0xA9
#define DEFAULT_WHITE_COEFF_ENABLE    0

#define DEFAULT_TX_GAIN_INDEX            0xFF
#define DEFAULT_TSET_MODE                   BT_PSEUDO_MODE
#define DEFAULT_TX_DAC                          0x13
#define DEFAULT_PKTHEADER                   0x1234
#define DEFAULT_MULTI_RX_ENABLE             0

typedef  struct _EVENT_STRING{

     char  EventData[3];//,XX

}EVENT_STRING;

char* BT_SendHciCmd(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer)
{
    int rtn = 0;
    const char  *delim = STR_BT_MP_TX_PARA_DELIM;
    char    * token = NULL;
    unsigned short opCode = 0;
    unsigned char  paraLen = 0;
    unsigned char paraMeters[255];
    unsigned char maxParaCount = 255;

    const unsigned char nTotalParaCount = 2;//at least 2 parameters: opcode, parameterlen, parameters
    unsigned char rxParaCount = 0;


    unsigned char  EventType = 0x0E;
    unsigned char  pEvent[255] = {0};
    EVENT_STRING pEventString[255];
    unsigned char EventLen = 0;
    unsigned char i = 0;
    
    bt_mp_LogMsg("++%s: %s\n", STR_BT_MP_HCI_CMD, p);


    token = strtok(p, delim);
    if(token != NULL)
    {
        opCode=  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    token = strtok(NULL, delim);
    if(token != NULL)
    {
        paraLen=  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    
    unsigned char iIndex = 0;
    while(maxParaCount--)
    {
        //end of parameter   
        token = strtok(NULL, delim);
        if(token != NULL)
        {
            paraMeters[iIndex++]=  strtol(token, NULL ,16);
            rxParaCount++;
        }
        else
        {
            goto EXIT;
        }
    }
    


EXIT:
    bt_mp_LogMsg("%s: rxParaCount = %d", STR_BT_MP_HCI_CMD, rxParaCount);
    
    if(rxParaCount != paraLen + 2)
    {
        sprintf(pNotifyBuffer, "%s%s%x\n", STR_BT_MP_HCI_CMD, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_INVALID_PARA_COUNT);
    }
    else
    {
        bt_mp_LogMsg("OpCode:0x%04x, 0x%02x",
                                   opCode,
                                   paraLen                                   
                                   );   


        rtn = pBtModule->SendHciCommandWithEvent(pBtModule, opCode, paraLen, paraMeters,  EventType,  pEvent, &EventLen);

        bt_mp_LogMsg("%s%s%x\n", STR_BT_MP_HCI_CMD, STR_BT_MP_RX_RESULT_DELIM, rtn);


        sprintf(pNotifyBuffer, "%s", STR_BT_MP_HCI_CMD);

        for(i = 0; i < EventLen; i++)
        {
            sprintf(pEventString[i].EventData, "%s%x", STR_BT_MP_RX_RESULT_DELIM, pEvent[i]);
            strcat(pNotifyBuffer, pEventString[i].EventData);
        }
        strcat(pNotifyBuffer, "\n");

        
    }

    bt_mp_LogMsg("--%s", STR_BT_MP_HCI_CMD);
    return pNotifyBuffer;
}



char* BT_GetPara(BT_MODULE  *pBtModule, char* pNotifyBuffer)
{
    bt_mp_LogMsg("++%s", STR_BT_MP_GET_PARA);
    
    bt_mp_LogMsg("%s%s%x%s%x%s%x%s%lx%s%x%s%x%s%x%s%x%s%x%s%x%s%llx\n",
                                STR_BT_MP_GET_PARA,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mChannelNumber,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mPacketType,
                                STR_BT_MP_RX_RESULT_DELIM,                                
                                pBtModule->pBtParam->mPayloadType,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mTxPacketCount,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mTxGainValue,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mWhiteningCoeffEnable,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mTxGainIndex,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mTestMode,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mTxDAC,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mPacketHeader,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mHitTarget
                                );   
 
    sprintf(pNotifyBuffer, "%s%s%x%s%x%s%x%s%lx%s%x%s%x%s%x%s%x%s%x%s%x%s%llx\n",
                                STR_BT_MP_GET_PARA,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mChannelNumber,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mPacketType,
                                STR_BT_MP_RX_RESULT_DELIM,                                
                                pBtModule->pBtParam->mPayloadType,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mTxPacketCount,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mTxGainValue,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mWhiteningCoeffEnable,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mTxGainIndex,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mTestMode,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mTxDAC,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mPacketHeader,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pBtModule->pBtParam->mHitTarget                                
                                ); 
    
    

    
    bt_mp_LogMsg("--%s", STR_BT_MP_GET_PARA);    
    return pNotifyBuffer;
}


char* BT_SetGainTable(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer)
{
    const char  *delim = STR_BT_MP_TX_PARA_DELIM;
    char    * token = NULL;
    const unsigned char BT_PARA1_COUNT = 7;
    unsigned char rxParaCount = 0;
    
    
    bt_mp_LogMsg("++%s: %s", STR_BT_MP_SET_GAIN_TABLE, p);

    token = strtok(p, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->TXGainTable[0]=  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->TXGainTable[1]=  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->TXGainTable[2]=  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->TXGainTable[3]=  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->TXGainTable[4]=  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->TXGainTable[5]=  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->TXGainTable[6]=  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    //end of parameter   
    token = strtok(NULL, delim);
    if(token != NULL)
    {
        bt_mp_LogMsg("token = %s", token);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

EXIT:
    bt_mp_LogMsg("%s: rxParaCount = %d", STR_BT_MP_SET_GAIN_TABLE, rxParaCount);
    
    if(rxParaCount != BT_PARA1_COUNT)
    {
        sprintf(pNotifyBuffer, "%s%s%x\n", STR_BT_MP_SET_GAIN_TABLE, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_INVALID_PARA_COUNT);
    }
    else
    {
        bt_mp_LogMsg("TXGainTable:0x%02x, 0x%02x,0x%02x, 0x%02x,0x%02x, 0x%02x,0x%02x",
                                   pBtModule->pBtParam->TXGainTable[0],
                                   pBtModule->pBtParam->TXGainTable[1],
                                   pBtModule->pBtParam->TXGainTable[2],
                                   pBtModule->pBtParam->TXGainTable[3],
                                   pBtModule->pBtParam->TXGainTable[4],
                                   pBtModule->pBtParam->TXGainTable[5],
                                   pBtModule->pBtParam->TXGainTable[6]
                                   );   

        bt_mp_LogMsg("%s%s%x\n", STR_BT_MP_SET_GAIN_TABLE, STR_BT_MP_RX_RESULT_DELIM, BT_FUNCTION_SUCCESS);
        sprintf(pNotifyBuffer, "%s%s%x\n", STR_BT_MP_SET_GAIN_TABLE, STR_BT_MP_RX_RESULT_DELIM, BT_FUNCTION_SUCCESS);
    }


    bt_mp_LogMsg("--%s", STR_BT_MP_SET_GAIN_TABLE);
    return pNotifyBuffer;
}



char* BT_SetDacTable(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer)
{
    const char  *delim = STR_BT_MP_TX_PARA_DELIM;
    char    * token = NULL;
    const unsigned char BT_PARA1_COUNT = 5;
    unsigned char rxParaCount = 0;
    
    
    bt_mp_LogMsg("++%s: %s", STR_BT_MP_SET_DAC_TABLE, p);

    token = strtok(p, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->TXDACTable[0]=  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->TXDACTable[1]=  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->TXDACTable[2]=  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->TXDACTable[3]=  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->TXDACTable[4]=  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    //end of parameter   
    token = strtok(NULL, delim);
    if(token != NULL)
    {
        bt_mp_LogMsg("token = %s", token);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

EXIT:
    bt_mp_LogMsg("%s: rxParaCount = %d", STR_BT_MP_SET_DAC_TABLE, rxParaCount);
    
    if(rxParaCount != BT_PARA1_COUNT)
    {
        sprintf(pNotifyBuffer, "%s%s%x\n", STR_BT_MP_SET_DAC_TABLE, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_INVALID_PARA_COUNT);
    }
    else
    {
        bt_mp_LogMsg("TXDACTable:0x%02x, 0x%02x,0x%02x, 0x%02x,0x%02x",
                                   pBtModule->pBtParam->TXDACTable[0],
                                   pBtModule->pBtParam->TXDACTable[1],
                                   pBtModule->pBtParam->TXDACTable[2],
                                   pBtModule->pBtParam->TXDACTable[3],
                                   pBtModule->pBtParam->TXDACTable[4]
                                   );   
        bt_mp_LogMsg("%s%s%x\n", STR_BT_MP_SET_DAC_TABLE, STR_BT_MP_RX_RESULT_DELIM, BT_FUNCTION_SUCCESS);
        sprintf(pNotifyBuffer, "%s%s%x\n", STR_BT_MP_SET_DAC_TABLE, STR_BT_MP_RX_RESULT_DELIM, BT_FUNCTION_SUCCESS);
    }

    bt_mp_LogMsg("--%s", STR_BT_MP_SET_DAC_TABLE);
    return pNotifyBuffer;    
}


char* BT_SetPara1(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer)
{
    const char  *delim = STR_BT_MP_TX_PARA_DELIM;
    char    * token = NULL;
    const unsigned char BT_PARA1_COUNT = 5;
    unsigned char rxParaCount = 0;
    
    
    bt_mp_LogMsg("++%s: %s\n", STR_BT_MP_SET_PARA1, p);

    //    unsigned char mChannelNumber;
    token = strtok(p, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->mChannelNumber =  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    //BT_PKT_TYPE   mPacketType;
    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->mPacketType =  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    //BT_PAYLOAD_TYPE mPayloadType;
    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->mPayloadType =  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    //unsigned long mTxPacketCount;
    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->mTxPacketCount =  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    //unsigned char mTxGainValue;
    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->mTxGainValue =  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    //end of parameter   
    token = strtok(NULL, delim);
    if(token != NULL)
    {
        bt_mp_LogMsg("token = %s", token);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

EXIT:
    bt_mp_LogMsg("%s: rxParaCount = %d\n", STR_BT_MP_SET_PARA1, rxParaCount);
    
    if(rxParaCount != BT_PARA1_COUNT)
    {
        sprintf(pNotifyBuffer, "%s%s%x\n", STR_BT_MP_SET_PARA1, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_INVALID_PARA_COUNT);
    }
    else
    {
        bt_mp_LogMsg("mChannelNumber:0x%x, mPacketType:0x%x, mPayloadType:0x%x, mTxPacketCount:0x%x, mTxGainValue:0x%x, mWhiteningCoeffEnable:0x%x",
                                   pBtModule->pBtParam->mChannelNumber,
                                   pBtModule->pBtParam->mPacketType,
                                   pBtModule->pBtParam->mPayloadType,
                                   pBtModule->pBtParam->mTxPacketCount,
                                   pBtModule->pBtParam->mTxGainValue,
                                   pBtModule->pBtParam->mWhiteningCoeffEnable
                                   );   

        bt_mp_LogMsg("%s%s%x\n", STR_BT_MP_SET_PARA1, STR_BT_MP_RX_RESULT_DELIM, BT_FUNCTION_SUCCESS);
        sprintf(pNotifyBuffer, "%s%s%x\n", STR_BT_MP_SET_PARA1, STR_BT_MP_RX_RESULT_DELIM, BT_FUNCTION_SUCCESS);
    }

    bt_mp_LogMsg("--%s\n", STR_BT_MP_SET_PARA1);
    return pNotifyBuffer;    
}



char* BT_SetPara2(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer)
{
    const char  *delim = ",";
    char    * token = NULL;
    const unsigned char BT_PARA1_COUNT = 5;
    unsigned char rxParaCount = 0;
    
    bt_mp_LogMsg("++%s: %s\n", STR_BT_MP_SET_PARA2, p);

    //unsigned char mTxGainIndex;
    token = strtok(p, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->mTxGainIndex = strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    //BT_TEST_MODE mTestMode;
    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->mTestMode = strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    //unsigned char mTxDAC;
    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->mTxDAC =  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

     //unsigned char mWhiteningCoeffEnable;
    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->mWhiteningCoeffEnable =  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    //unsigned int  mPacketHeader;
    token = strtok(NULL, delim);
    if(token != NULL)
    {
        pBtModule->pBtParam->mPacketHeader =  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

     //end of parameter   
    token = strtok(NULL, delim);
    if(token != NULL)
    {
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

EXIT:
    bt_mp_LogMsg("%s: rxParaCount = %d", STR_BT_MP_SET_PARA2, rxParaCount);
    
    if(rxParaCount != BT_PARA1_COUNT)
    {
        sprintf(pNotifyBuffer, "%s%s%x\n", STR_BT_MP_SET_PARA2, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_INVALID_PARA_COUNT);
    }
    else
    {
        bt_mp_LogMsg("mTxGainIndex:0x%x, mTestMode:0x%x, mTxDAC:0x%x, mWhiteningCoeffEnable:0x%x, mPacketHeader:0x%x",
                                   pBtModule->pBtParam->mTxGainIndex,
                                   pBtModule->pBtParam->mTestMode,
                                   pBtModule->pBtParam->mTxDAC,
                                    pBtModule->pBtParam->mWhiteningCoeffEnable,
                                   pBtModule->pBtParam->mPacketHeader
                                   );   

        bt_mp_LogMsg("%s%s%x\n", STR_BT_MP_SET_PARA2, STR_BT_MP_RX_RESULT_DELIM, BT_FUNCTION_SUCCESS);
        sprintf(pNotifyBuffer, "%s%s%x\n", STR_BT_MP_SET_PARA2, STR_BT_MP_RX_RESULT_DELIM, BT_FUNCTION_SUCCESS);
    }

    bt_mp_LogMsg("--%s\n", STR_BT_MP_SET_PARA2);
    return pNotifyBuffer;    
    
}







char* BT_SetHit(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer)
{
    bt_mp_LogMsg("++%s: %s\n", STR_BT_MP_SET_HIT, p);
    pBtModule->pBtParam->mHitTarget = strtoull(p, NULL, 16);
    
    bt_mp_LogMsg("%s:0x%llx\n", STR_BT_MP_SET_HIT, pBtModule->pBtParam->mHitTarget);
    
    bt_mp_LogMsg("%s%s%x\n", STR_BT_MP_SET_HIT, STR_BT_MP_RX_RESULT_DELIM, BT_FUNCTION_SUCCESS);    
    sprintf(pNotifyBuffer, "%s%s%x\n", STR_BT_MP_SET_HIT, STR_BT_MP_RX_RESULT_DELIM, BT_FUNCTION_SUCCESS);    

    bt_mp_LogMsg("--%s\n", STR_BT_MP_SET_HIT);
    return pNotifyBuffer;    
}




char* BT_Exec(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer)
{
    int ParameterIndex = 0;
    BT_PARAMETER        *pBtParam =NULL;
    int rtn = BT_FUNCTION_SUCCESS;

    bt_mp_LogMsg("++%s%s\n", STR_BT_MP_EXEC, p);

    ParameterIndex = strtol(p, NULL ,16);
    bt_mp_LogMsg("%s:ParameterIndex = 0x%x\n", STR_BT_MP_EXEC, ParameterIndex);
    
    if(NOTTHING< ParameterIndex&&
        ParameterIndex < NUMBEROFBT_ACTIONCONTROL_TAG)
    {

        pBtParam=pBtModule->pBtParam;
        pBtParam->ParameterIndex=ParameterIndex;
        rtn = pBtModule->ActionControlExcute(pBtModule);

        sprintf(pNotifyBuffer, "%s%s%x%s%x\n", 
                STR_BT_MP_EXEC, 
                STR_BT_MP_RX_RESULT_DELIM,
                ParameterIndex,
                STR_BT_MP_RX_RESULT_DELIM,
                rtn                
                );
         bt_mp_LogMsg("%s%s%x%s%x\n", 
                STR_BT_MP_EXEC, 
                STR_BT_MP_RX_RESULT_DELIM,
                ParameterIndex,
                STR_BT_MP_RX_RESULT_DELIM,
                rtn                
                );
    }
    else
    {
        sprintf(pNotifyBuffer, "%s%s%s%s%x\n", 
                STR_BT_MP_EXEC, 
                STR_BT_MP_RX_RESULT_DELIM,
                p,
                STR_BT_MP_RX_RESULT_DELIM,
                FUNCTION_PARAMETER_ERROR
                );
        bt_mp_LogMsg("%s%s%s%s%x\n", 
                STR_BT_MP_EXEC, 
                STR_BT_MP_RX_RESULT_DELIM,
                p,
                STR_BT_MP_RX_RESULT_DELIM,
                FUNCTION_PARAMETER_ERROR
                );        
    }

    bt_mp_LogMsg("--%s\n", STR_BT_MP_EXEC);
    
    return pNotifyBuffer;    
}


char* BT_ReportTx(BT_MODULE  *pBtModule, char* pNotifyBuffer)
{
    BT_PARAMETER        *pBtParam =NULL;
    BT_DEVICE_REPORT    *pModuleBtReport=NULL;
    int rtn = BT_FUNCTION_SUCCESS;    

    bt_mp_LogMsg("++%s\n", STR_BT_MP_REPORTTX);
    
    pBtParam=pBtModule->pBtParam;
    pModuleBtReport=pBtModule->pModuleBtReport;
    pBtParam->ParameterIndex=PACKET_TX_UPDATE;
    rtn=pBtModule->ActionControlExcute(pBtModule);
    
    if (rtn != BT_FUNCTION_SUCCESS)
    {
            sprintf(pNotifyBuffer, "%s%s%x\n", STR_BT_MP_REPORTTX, STR_BT_MP_RX_RESULT_DELIM, rtn);
            bt_mp_LogMsg("%s%s%x\n", STR_BT_MP_REPORTTX, STR_BT_MP_RX_RESULT_DELIM, rtn);
            goto exit;
    }
    else
    {
        bt_mp_LogMsg("%s%s%lx%s%lx\n",
                                STR_BT_MP_REPORTTX,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pModuleBtReport->TotalTXBits,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pModuleBtReport->TotalTxCounts);
        
        sprintf(pNotifyBuffer, "%s%s%lx%s%lx\n",
                                STR_BT_MP_REPORTTX,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pModuleBtReport->TotalTXBits,
                                STR_BT_MP_RX_RESULT_DELIM,
                                pModuleBtReport->TotalTxCounts);            
    }
    
exit:
    
    bt_mp_LogMsg("--%s\n", STR_BT_MP_REPORTTX);
    return pNotifyBuffer;
}

char* BT_ReportRx(BT_MODULE  *pBtModule, char* pNotifyBuffer)
{
    BT_PARAMETER        *pBtParam =NULL;
    BT_DEVICE_REPORT    *pModuleBtReport=NULL;
    int rtn=BT_FUNCTION_SUCCESS;    

    bt_mp_LogMsg("++%s\n", STR_BT_MP_REPORTRX);

    pBtParam=pBtModule->pBtParam;
    pModuleBtReport=pBtModule->pModuleBtReport;

    pBtParam->ParameterIndex=PACKET_RX_UPDATE;
    rtn=pBtModule->ActionControlExcute(pBtModule) ;
    if (rtn != BT_FUNCTION_SUCCESS)
    {

        bt_mp_LogMsg("%s%s%x\n", STR_BT_MP_PKTRXSTART, STR_BT_MP_RX_RESULT_DELIM, rtn);        
        sprintf(pNotifyBuffer, "%s%s%x\n", STR_BT_MP_PKTRXSTART, STR_BT_MP_RX_RESULT_DELIM, rtn);
        goto exit;
    }
    else
    {

        bt_mp_LogMsg("%s%s%x%s%lx%s%lx%s%lx\n",
                        STR_BT_MP_REPORTRX,
                        STR_BT_MP_RX_RESULT_DELIM,
                        pModuleBtReport->IsRxRssi,
                        STR_BT_MP_RX_RESULT_DELIM,
                        pModuleBtReport->TotalRXBits,
                         STR_BT_MP_RX_RESULT_DELIM,
                        pModuleBtReport->TotalRxCounts,
                         STR_BT_MP_RX_RESULT_DELIM,
                        pModuleBtReport->TotalRxErrorBits
                        );   
     
        sprintf(pNotifyBuffer, "%s%s%x%s%lx%s%lx%s%lx\n",
                        STR_BT_MP_REPORTRX,
                        STR_BT_MP_RX_RESULT_DELIM,
                        pModuleBtReport->IsRxRssi,
                        STR_BT_MP_RX_RESULT_DELIM,
                        pModuleBtReport->TotalRXBits,
                         STR_BT_MP_RX_RESULT_DELIM,
                        pModuleBtReport->TotalRxCounts,
                         STR_BT_MP_RX_RESULT_DELIM,
                        pModuleBtReport->TotalRxErrorBits
                    );   
    }

exit:

    bt_mp_LogMsg("--%s\n", STR_BT_MP_REPORTRX);
    
    return pNotifyBuffer;    
}


/**

register	    read/Write	    address	msb	        lsb	data	return(Status)	value
bt_regmd	(write)1	            1	                1	        1	2	1	¡¡
	(read)0	1	1	1	0	1	2
bt_regrf	(write)1	1	1	1	2	1	¡¡
	(read)0	1	1	1	0	1	2


*/

char* BT_RegRf(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer)
{
    int rtn = BT_FUNCTION_SUCCESS;
    const char  *delim = STR_BT_MP_TX_PARA_DELIM;
    char    * token = NULL;
    unsigned char BT_REGRF_PARA_COUNT = 4;//4// 4 -read, 5- write
    unsigned char rxParaCount = 0;
    unsigned char opReadWrite = 0;
    unsigned char address = 0;    
    unsigned char msb = 0;    
    unsigned char lsb = 0;    
    unsigned short dataReadWrite = 0; //for opReadWrite = 1(write)        
    
    
    bt_mp_LogMsg("++%s: %s\n", STR_BT_MP_REG_RF, p);

    //unsigned char opReadWrite;
    token = strtok(p, delim);
    if(token != NULL)
    {
        opReadWrite =  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    //unsigned char address
    token = strtok(NULL, delim);
    if(token != NULL)
    {
        address =  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    //unsigned char msb;
    token = strtok(NULL, delim);
    if(token != NULL)
    {
        msb =  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    //unsigned char lsb;
    token = strtok(NULL, delim);
    if(token != NULL)
    {
        lsb =  strtol(token, NULL ,16);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

    if(opReadWrite == 1)//write
    {
        BT_REGRF_PARA_COUNT = 5;
        //unsigned char dataToWrite;
        token = strtok(NULL, delim);
        if(token != NULL)
        {
            dataReadWrite =  strtol(token, NULL ,16);
            rxParaCount++;
        }
        else
        {
            goto EXIT;
        }

    }
    else
    {
        BT_REGRF_PARA_COUNT = 4;
    }

    //end of parameter   
    token = strtok(NULL, delim);
    if(token != NULL)
    {
        bt_mp_LogMsg("token = %s", token);
        rxParaCount++;
    }
    else
    {
        goto EXIT;
    }

EXIT:
    bt_mp_LogMsg("%s: rxParaCount = %d\n", STR_BT_MP_REG_RF, rxParaCount);
    
    if(rxParaCount != BT_REGRF_PARA_COUNT)
    {
        sprintf(pNotifyBuffer, "%s%s%x\n", STR_BT_MP_REG_RF, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_INVALID_PARA_COUNT);
    }
    else
    {



        bt_mp_LogMsg("opReadWrite:0x%x, address:0x%x, msb:0x%x, lsb:0x%x, dataToWrite:0x%x",
                                   opReadWrite,
                                   address,
                                   msb,
                                   lsb,
                                   dataReadWrite
                                   );   

        bt_mp_LogMsg("%s%s%x\n", STR_BT_MP_REG_RF, STR_BT_MP_RX_RESULT_DELIM, BT_FUNCTION_SUCCESS);
        sprintf(pNotifyBuffer, "%s%s%x\n", STR_BT_MP_REG_RF, STR_BT_MP_RX_RESULT_DELIM, BT_FUNCTION_SUCCESS);

        if(opReadWrite == 0)
        {
            rtn = pBtModule->GetRfRegMaskBits(pBtModule, address, msb, lsb, &dataReadWrite);
            bt_mp_LogMsg("%s%s%x%s%x\n", STR_BT_MP_REG_RF, STR_BT_MP_RX_RESULT_DELIM, rtn, STR_BT_MP_RX_RESULT_DELIM, dataReadWrite);
            sprintf(pNotifyBuffer, "%s%s%x%s%x\n", STR_BT_MP_REG_RF, STR_BT_MP_RX_RESULT_DELIM, rtn, STR_BT_MP_RX_RESULT_DELIM, dataReadWrite);            
        }
        else
        {
            rtn = pBtModule->SetRfRegMaskBits(pBtModule, address, msb, lsb, dataReadWrite);
            bt_mp_LogMsg("%s%s%x\n", STR_BT_MP_REG_RF, STR_BT_MP_RX_RESULT_DELIM, rtn);
            sprintf(pNotifyBuffer, "%s%s%x\n", STR_BT_MP_REG_RF, STR_BT_MP_RX_RESULT_DELIM, rtn);
        }
                
    }

    bt_mp_LogMsg("--%s\n", STR_BT_MP_REG_RF);
    return pNotifyBuffer;    
}

char* BT_RegMd(BT_MODULE  *pBtModule, char *p, char* pNotifyBuffer)
{
    return pNotifyBuffer;

}

void BT_GetBDAddr(BT_MODULE  *pBtModule)
{
    unsigned char   pEvt[256];
    unsigned char   EvtLen = 0;
    unsigned char   para[256];
    
    if (pBtModule->SendHciCommandWithEvent(pBtModule,0x1009,0,para,0x0e,pEvt, &EvtLen) != BT_FUNCTION_SUCCESS)
    {
        printf("Get BD Addree Fail!!..\n");
    }
    printf("BD_ADDR=[0x%.2x%.2x%.2x%.2x%.2x%.2x]\n",pEvt[11],pEvt[10],pEvt[9],pEvt[8],pEvt[7],pEvt[6]);
    return ;
}

void bt_mp_module_init(BASE_INTERFACE_MODULE *pBaseInterfaceModuleMemory, BT_MODULE *pBtModuleMemory)
{
    BASE_INTERFACE_MODULE   *pBaseInterface = NULL;
    BT_MODULE               *pBtModule = NULL;

    unsigned char           pTxGainTable[7]={0x49,0x4d,0x69,0x89,0x8d,0xa9,0xa9};  //RTL8761 Table
    unsigned char           pTxDACTable[5]={0x10,0x11,0x12,0x13,0x14};

    BT_PARAMETER        *pBtParam =NULL;
    BT_DEVICE           *pBtDevice=NULL;
    BT_DEVICE_REPORT    *pModuleBtReport=NULL;



    BuildTransportInterface(
            &pBaseInterface,
            pBaseInterfaceModuleMemory,
            1,
            115200,
            NULL,//open
            bt_transport_SendHciCmd,
            bt_transport_RecvHciEvt,
            NULL,//close
            UserDefinedWaitMs
            );

    //Build Module
    bt_mp_LogMsg("Build Module....!!\n");
    BuildBluetoothModule(
            pBaseInterface,
            &pBtModule,
            pBtModuleMemory,
            NULL,
            pTxGainTable,
            pTxDACTable
            );

    pBtModuleMemory->pBtParam->mHitTarget = 0x0000009e8b33;

    pBtModuleMemory->pBtParam->mChannelNumber = DEFAULT_CH_NUM;
    pBtModuleMemory->pBtParam->mPacketType = DEFAULT_PKT_TYPE;
    pBtModuleMemory->pBtParam->mPayloadType= DEFAULT_PAYLOAD_TYPE;
    pBtModuleMemory->pBtParam->mTxPacketCount = DEFAULT_PKT_COUNT;
    pBtModuleMemory->pBtParam->mTxGainValue= DEFAULT_TX_GAIN_VALUE;
    pBtModuleMemory->pBtParam->mWhiteningCoeffEnable = DEFAULT_WHITE_COEFF_ENABLE;
    pBtModuleMemory->pBtParam->mTxGainIndex= DEFAULT_TX_GAIN_INDEX;
    pBtModuleMemory->pBtParam->mTestMode= DEFAULT_TSET_MODE;
    pBtModuleMemory->pBtParam->mTxDAC= DEFAULT_TX_DAC;
    pBtModuleMemory->pBtParam->mPacketHeader= DEFAULT_PKTHEADER;
    pBtModuleMemory->pBtParam->mMutiRxEnable= DEFAULT_MULTI_RX_ENABLE;
   
    return ;
}
