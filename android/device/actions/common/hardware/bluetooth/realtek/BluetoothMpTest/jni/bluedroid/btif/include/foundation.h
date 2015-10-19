#ifndef __FOUNDATION_H
#define __FOUNDATION_H
#include <pthread.h>
// Constants
#define INVALID_POINTER_VALUE		0
#define NO_USE						0

#define LEN_1_BYTE					1
#define LEN_2_BYTE					2
#define LEN_3_BYTE					3
#define LEN_4_BYTE					4
#define LEN_5_BYTE					5
#define LEN_6_BYTE					6
#define LEN_11_BYTE					11
#define LEN_13_BYTE					13
#define LEN_14_BYTE					14
#define LEN_16_BYTE					16
#define LEN_2048_BYTE				2048


#define LEN_1_BIT					1

#define BYTE_MASK					0xff
#define BYTE_SHIFT					8
#define HEX_DIGIT_MASK				0xf
#define BYTE_BIT_NUM				8
#define LONG_BIT_NUM				32

#define BIT_0_MASK					0x1
#define BIT_1_MASK					0x2
#define BIT_2_MASK					0x4
#define BIT_3_MASK					0x8
#define BIT_4_MASK					0x10
#define BIT_5_MASK					0x20
#define BIT_6_MASK					0x40
#define BIT_7_MASK					0x80
#define BIT_8_MASK					0x100

#define BIT_7_SHIFT					7
#define BIT_8_SHIFT					8


//  Define BT HCI Max Pkt size
#define     HCI_CMD_LEN_MAX	0x102
#define     HCI_EVT_LEN_MAX	0x101
#define     HCI_ACL_LEN_MAX	0x424
#define     HCI_SCO_LEN_MAX	0x102
#define     HCI_BUF_LEN_MAX  HCI_ACL_LEN_MAX

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif


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

/*
enum FUNCTION_RETURN_STATUS
{
	FUNCTION_SUCCESS = 0,
	FUNCTION_ERROR,
};

*/


enum INTERFACE_TYPE
{
	TYPE_USB = 0,
	TYPE_UART,
};

/// Base interface module alias
typedef struct BASE_INTERFACE_MODULE_TAG BASE_INTERFACE_MODULE;



typedef int
(*BASE_FP_OPEN)(
	BASE_INTERFACE_MODULE *pBaseInterface
	);



typedef int
(*BASE_FP_SEND)(
	BASE_INTERFACE_MODULE *pBaseInterface,
	unsigned char *pWritingBuf,
	unsigned char Len
	);



typedef int
(*BASE_FP_RECV )(
	BASE_INTERFACE_MODULE *pBaseInterface,
	unsigned char *pReadingBuf,
	unsigned char *pRetLen
	);



typedef int
(*BASE_FP_CLOSE)(
	BASE_INTERFACE_MODULE *pBaseInterface
	);



typedef void
(*BASE_FP_WAIT_MS)(
	BASE_INTERFACE_MODULE *pBaseInterface,
	unsigned long WaitTimeMs
	);





typedef void
(*BASE_FP_SET_USER_DEFINED_DATA_POINTER)(
	BASE_INTERFACE_MODULE *pBaseInterface,
	int UserDefinedData
	);



typedef void
(*BASE_FP_GET_USER_DEFINED_DATA_POINTER)(
	BASE_INTERFACE_MODULE *pBaseInterface,
	int *pUserDefinedData
	);


#define MP_TRANSPORT_EVENT_RX_HCIEVT              0x0001
#define MP_TRANSPORT_EVENT_RX_ACL                    0x0002
#define MP_TRANSPORT_EVENT_RX_EXIT                  0x8000


/// Base interface module structure
struct BASE_INTERFACE_MODULE_TAG
{
	BASE_FP_OPEN Open;
	BASE_FP_SEND Send;
	BASE_FP_RECV Recv;
	BASE_FP_CLOSE Close;
	BASE_FP_WAIT_MS WaitMs;
	
	BASE_FP_SET_USER_DEFINED_DATA_POINTER   SetUserDefinedDataPointer;
	BASE_FP_GET_USER_DEFINED_DATA_POINTER   GetUserDefinedDataPointer;

	unsigned char InterfaceType;

	// User defined data
	int UserDefinedData;

	//for usb , uart
	unsigned char PortNo;		

	//for uart
	unsigned long Baudrate;

       unsigned short rx_ready_events;
       pthread_mutex_t mutex;
       pthread_cond_t  cond;
       unsigned char evtBuffer[255];
       unsigned char evtLen;
       
};





void
BuildTransportInterface(
	BASE_INTERFACE_MODULE **ppBaseInterface,
	BASE_INTERFACE_MODULE *pBaseInterfaceModuleMemory,
	unsigned char	PortNo,
	unsigned long Baudrate,
	BASE_FP_OPEN Open,
	BASE_FP_SEND Send,
	BASE_FP_RECV Recv,
	BASE_FP_CLOSE Close,
	BASE_FP_WAIT_MS WaitMs
	);




// User data pointer of base interface structure setting and getting functions
void
base_interface_SetUserDefinedDataPointer(
	BASE_INTERFACE_MODULE *pBaseInterface,
	int UserDefinedData
	);

void
base_interface_GetUserDefinedDataPointer(
	BASE_INTERFACE_MODULE *pBaseInterface,
	int *pUserDefinedData
	);





// Math functions

// Binary and signed integer converter
unsigned long
SignedIntToBin(
	long Value,
	unsigned char BitNum
	);

long
BinToSignedInt(
	unsigned long Binary,
	unsigned char BitNum
	);



// Arithmetic
unsigned long
DivideWithCeiling(
	unsigned long Dividend,
	unsigned long Divisor
	);

void
bt_mp_LogMsg(const char *fmt_str, ...);



void bdt_log(const char *fmt_str, ...);
















#endif
