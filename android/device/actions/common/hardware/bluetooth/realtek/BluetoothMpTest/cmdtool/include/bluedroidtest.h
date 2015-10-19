#ifndef __FOUNDATION_H
#define __FOUNDATION_H



#define STR_BT_MP_ENABLE    "enable"
#define STR_BT_MP_DISABLE   "disable"
#define STR_BT_MP_DUT_MODE   "dut_mode_configure"


#define STR_BT_MP_EXEC   "bt_mp_Exec"

#define STR_BT_MP_GET_PARA   "bt_mp_GetPara"

#define STR_BT_MP_HCI_RESET   "bt_mp_HciReset"
#define STR_BT_MP_CONTTXSTART    "bt_mp_ConTxStart"
#define STR_BT_MP_CONTTXSTOP    "bt_mp_ConTxStop"
#define STR_BT_MP_PKTTXSTART     "bt_mp_PktTxStart"
#define STR_BT_MP_PKTTXSTOP     "bt_mp_PktTxStop"
#define STR_BT_MP_PKTRXSTART      "bt_mp_PktRxStart"
#define STR_BT_MP_PKTRXSTOP     "bt_mp_PktRxStop"

#define STR_BT_MP_SET_GAIN_TABLE   "bt_mp_SetGainTable"
#define STR_BT_MP_SET_DAC_TABLE     "bt_mp_SetDacTable"


#define STR_BT_MP_REPORTTX   "bt_mp_ReportTx"
#define STR_BT_MP_REPORTRX   "bt_mp_ReportRx"

#define STR_BT_MP_REG_RF   "bt_mp_RegRf"
#define STR_BT_MP_REG_MD   "bt_mp_RegMd"

#define STR_BT_MP_TX_PARA_DELIM         ","
#define STR_BT_MP_RX_RESULT_DELIM     ","

#define STR_BT_MP_SET_PARA1     "bt_mp_SetPara1"
#define STR_BT_MP_SET_PARA2      "bt_mp_SetPara2"
#define STR_BT_MP_SET_HIT     "bt_mp_SetHit"

#define STR_BT_MP_HCI_CMD   "hci_cmd"


typedef enum
{
    BT_FUNCTION_SUCCESS = 0,
    ERROR_BT_DISABLE = 0x80,
    ERROR_BT_INVALID_PARA_COUNT,


    ERROR_BT_IF_RETURN_STATUS_NUM
}ADB_IF_FUNCTION_RETURN_STATUS;


void
bt_mp_LogMsg(const char *fmt_str, ...);



void bdt_log(const char *fmt_str, ...);
















#endif
