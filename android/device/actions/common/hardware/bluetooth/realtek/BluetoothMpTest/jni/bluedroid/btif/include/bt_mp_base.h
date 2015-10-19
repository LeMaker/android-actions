#ifndef __BT_MP_BASE_H
#define __BT_MP_BASE_H

#include <stdio.h>
#include <string.h>
//#include <math.h>
//#include <Windows.h>
//#include <iostream.h>

#include "foundation.h"

#define USE_CHAR_STR

//#define BUILDER_C

//-----------------------------------------------------------------------------------------------------------------
//	Base  define
//-----------------------------------------------------------------------------------------------------------------
#ifdef BUILDER_C

typedef 
enum _bool{ false, true }bool;


#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


typedef   unsigned long long ULONG64;
///////////////////////////////////////////////////////////////////////////////////////////////////
//Debug 
///////////////////////////////////////////////////////////////////////////////////////////////////
#define DBG_ON
// Debug Window
#define DBG_CONSOLE

//Debug LEVEL
//#define DBG_REG_SETTING	

///////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef DBG_ON


		#ifdef DBG_CONSOLE
			#define DBGPRINTF printf
		#else
        void DBGPRINTF(char *prompt, ...)
        {
			char buf[2048];
			va_list argptr;
			//int cnt;

			va_start(argptr, prompt);
			vsprintf(buf, prompt, argptr);
			OutputDebugString(buf);
			va_end(argptr);
			return ;
        }
		#endif
#else
	void DBGPRINTF(char *prompt, ...)
	{

		return ;
	}
#endif
//-----------------------------------------------------------------------------------------------------------------
//	Realtek define
//-----------------------------------------------------------------------------------------------------------------
#define MAX_HCI_COMANND_BUF_SIZ 256
#define MAX_HCI_EVENT_BUF_SIZ 	256
#define MAX_TXGAIN_TABLE_SIZE   7
#define MAX_TXDAC_TABLE_SIZE	5
#define SEC_CLOCK_NUMBER        3200  // 1 clock =312.5u sec  1sec = 3200 


#ifndef _FUNCIION_RETURN_
#define _FUNCIION_RETURN_

typedef enum
{
	BT_FUNCTION_SUCCESS = 0,
	FUNCTION_ERROR,

	FUNCTION_HCISEND_ERROR,
	FUNCTION_HCISEND_STAUTS_ERROR,
	FUNCTION_PARAMETER_ERROR,
	FUNCTION_PARAMETER_INVALID_CHANNEL,
	FUNCTION_NO_SUPPORT,
	FUNCTION_TRX_STATUS_ERROR,
	FUNCTION_RX_RUNNING,
	FUNCTION_TX_RUNNING,
	FUNCTION_RX_MAXCOUNT,

	FUNCTION_TX_FINISH,
	FUNCTION_RX_FINISH,

       FUNCTION_CONTX_USER_ABORT,
       FUNCTION_TX_USER_ABORT,
       FUNCTION_RX_USER_ABORT,

	////////////////////////
	NumOf_FUNCTION_RETURN_STATUS
}FUNCTION_RETURN_STATUS;

typedef enum
{
    ERROR_BT_DISABLE = 0x80,
    ERROR_BT_INVALID_PARA_COUNT,


    ERROR_BT_IF_RETURN_STATUS_NUM
}ADB_IF_FUNCTION_RETURN_STATUS;


#endif
typedef enum {
	NOTTHING=0,

	//Module Select Process
	MODULE_INIT=1,

        //set table
	SETTXGAINTABLE=2,
        SETTXDACTABLE=3,
	//Device Setting
	HCI_RESET,
	SET_TXCHANNEL,
	SET_RXCHANNEL,
	SET_LETXCHANNEL,
	SET_POWERGAININDEX,
	SET_POWERGAIN,
	SET_POWERDAC,
	SET_PAYLOADTYPE,
	SET_WHITENINGCOFFENABLE,
	SET_PACKETTYPE,
	SET_HITTARGET,
	SET_TESTMODE,
	SET_MUTIRXENABLE,
	//Module Control
	PACKET_TX_START,
    PACKET_TX_UPDATE,
	PACKET_TX_STOP,

    PACKET_RX_START,
    PACKET_RX_UPDATE,
	PACKET_RX_STOP,

	CONTINUE_TX_START,
    CONTINUE_TX_UPDATE,
	CONTINUE_TX_STOP,

	CONTINUE_TX_LE_START,
    CONTINUE_TX_LE_UPDATE,
	CONTINUE_TX_LE_STOP,
	
	
	//Global Certification

	HOPPING_DWELL_TIME,

	//Roport
    REPORT_CLEAR,




	//////////////////////
	NUMBEROFBT_ACTIONCONTROL_TAG

}BT_ACTIONCONTROL_TAG;



typedef enum
{
	BT_PAYLOAD_TYPE_ALL0 = 0,
	BT_PAYLOAD_TYPE_ALL1 = 1,
	BT_PAYLOAD_TYPE_0101 = 2,
	BT_PAYLOAD_TYPE_1010 = 3,
	BT_PAYLOAD_TYPE_0x0_0xF = 4,
	BT_PAYLOAD_TYPE_0000_1111 = 5,
	BT_PAYLOAD_TYPE_1111_0000 = 6,
	BT_PAYLOAD_TYPE_PRBS9 = 7,
	//////////////////////////////////
	BT_PAYLOAD_TYPE_NUM =8	
}BT_PAYLOAD_TYPE;



typedef enum
{
	BT_PKT_1DH1=0,
	BT_PKT_1DH3,
	BT_PKT_1DH5,
	BT_PKT_2DH1,
	BT_PKT_2DH3,
	BT_PKT_2DH5,
	BT_PKT_3DH1,
	BT_PKT_3DH3,
	BT_PKT_3DH5,
	BT_PKT_LE,
	//////////////////////////////////////////////////
	BT_PKT_TYPE_NULL,
	BT_PKT_TYPE_RTL8723A,
	BT_PKT_TYPE_NUM
}BT_PKT_TYPE;

typedef enum
{
	        BT_DUT_MODE = 0,
	        BT_PSEUDO_MODE =1,
                /////////////////
            NUMBEROFBT_TEST_MODE
}BT_TEST_MODE;

extern unsigned char Arrary_PayloadType_str[BT_PAYLOAD_TYPE_NUM][30];

typedef struct BT_PARAMETER_TAG   BT_PARAMETER;
typedef struct BT_DEVICE_REPORT_TAG BT_DEVICE_REPORT;

typedef struct BT_CHIPINFO_TAG   BT_CHIPINFO;
struct  BT_PARAMETER_TAG
{
    int ParameterIndex;

	BT_TEST_MODE mTestMode;
	unsigned char mChannelNumber;
    BT_PKT_TYPE   mPacketType;
	unsigned char mTxGainIndex;
	unsigned char mTxGainValue;
	unsigned long mTxPacketCount;
    BT_PAYLOAD_TYPE mPayloadType;
	unsigned int  mPacketHeader;
	unsigned char mWhiteningCoeffEnable;
	unsigned char mTxDAC;
	ULONG64 mHitTarget;
	unsigned int  mMutiRxEnable;
    unsigned char TXGainTable[MAX_TXGAIN_TABLE_SIZE];
	unsigned char TXDACTable[MAX_TXDAC_TABLE_SIZE];
};
struct BT_DEVICE_REPORT_TAG { //
	

	unsigned long	TotalTXBits;
	unsigned long	TXUpdateBits;
	unsigned long	TotalTxCounts;
	unsigned long	TXPktUpdateCnts;


	unsigned long	TotalRXBits;
	unsigned long	RXUpdateBits;
	unsigned long	RXPktUpdateCnts;
	unsigned long	TotalRxCounts;
	unsigned long	TotalRxErrorBits;
	int				IsRxRssi;
	float			ber;	

	BT_CHIPINFO	*pBTInfo;

};

enum RTK_BT_CHIP_ID_GROUP_{
    RTK_BT_CHIP_ID_UNKNOWCHIP=0xFF,
    RTK_BT_CHIP_ID_RTL8723A=0,
    RTK_BT_CHIP_ID_RTL8723B=1,
    RTK_BT_CHIP_ID_RTL8821A=2,
    RTK_BT_CHIP_ID_RTL8761A=3,
    /////////////////////////    /
    NumOfRTKCHID
};
struct BT_CHIPINFO_TAG 
{ 
		unsigned int HCI_Version;
		unsigned int HCI_SubVersion;
		unsigned int LMP_Version;
		unsigned int LMP_SubVersion;

		unsigned int ChipType;
		unsigned int Version;
		int Is_After_PatchCode;

};
//-----------------------------------------------------------------------------------------------------------------
//		Device
//-----------------------------------------------------------------------------------------------------------------
//Device Level::member Funcion
//-----------------------------------------------------------------------------------------------------------------
typedef struct BT_DEVICE_TAG   BT_DEVICE;

//-->Table
typedef int
(*BT_FP_SET_TXGAINTABLE)(
	BT_DEVICE *pBtDevice,
	unsigned char  *pTable
	);
typedef int
(*BT_FP_SET_TXDACTABLE)(
	BT_DEVICE *pBtDevice,
	unsigned char  *pTable
	);
typedef int
(*BT_FP_GET_PAYLOADLENTABLE)(
	BT_DEVICE *pBtDevice,
	unsigned char  *pTable,
	int length
	);	
//------------------------------------------------------------------------------------------------------------------
//-->HCI Command & Event
typedef int
(*BT_FP_SEND_HCICOMMANDWITHEVENT)(
	BT_DEVICE *pBtDevice,
	unsigned int  OpCode,
	unsigned char PayLoadLength,
	unsigned char *pPayLoad,
	unsigned char  EventType,
	unsigned char  *pEvent,
       unsigned char  *pEventLen
	);
typedef int
(*BT_FP_RECV_ANYEVENT)(
	BT_DEVICE *pBtDevice,
	unsigned char  *pEvent
	);

typedef int
(*BT_FB_SEND_HCI_CMD)(
	BT_DEVICE *pBt,
	unsigned char PktType,
	unsigned char *pWritingBuf,
	unsigned long Len
	);

typedef int
(*BT_FB_RECV_HCI_EVENT)(
	BT_DEVICE *pBt,
	unsigned char PktType,
	unsigned char *pReadingBuf,
	unsigned long *pLen
	);

//------------------------------------------------------------------------------------------------------------------	
//-->Vendor HCI Command Control
typedef int
(*BT_FP_SET_HOPPINGMODE)(
	BT_DEVICE *pBtDevice,
	BT_PKT_TYPE pktType
	);
typedef int
(*BT_FP_SET_FWPOWERTRACKENABLE)(
	BT_DEVICE *pBtDevice,
	unsigned char FWPowerTrackEnable
	);
typedef int
(*BT_FP_SET_HCIRESET)(
	BT_DEVICE *pBtDevice,
	int Delay_mSec
	);
//
typedef int
(*BT_FP_GET_BT_CLOCK_TIME)(
	BT_DEVICE *pBtDevice,
	unsigned long *btClockTime
	);

//----------------------------------------------------------------
//-->Register Control 
typedef int 
(*BT_FP_SET_PESUDOOUTERSETUP)(BT_DEVICE *pBtDevice,
   BT_PARAMETER *pParam
);

typedef int
(*BT_FP_SET_SETPACKHEADER)(
	BT_DEVICE *pBtDevice,
	unsigned int packHeader
	);
typedef int
(*BT_FP_SET_SETRESETMDCOUNT)(
	BT_DEVICE *pBtDevice
	);
typedef int
(*BT_FP_SET_POWERGAININDEX)(
	BT_DEVICE *pBtDevice,
	int Index
	);
	
typedef int
(*BT_FP_SET_MUTIRXENABLE)(
	BT_DEVICE *pBtDevice,
	int IsMultiPktRx
	);	
typedef int
(*BT_FP_SET_TESTMODE)(
	BT_DEVICE *pBtDevice,
	BT_TEST_MODE TestMode
	);
typedef int
(*BT_FP_SET_HITTARGET)(
	BT_DEVICE *pBtDevice,
	ULONG64 HitTarget
	);
	

typedef int
(*BT_FP_SET_PACKETTYPE)(
	BT_DEVICE *pBtDevice,
	BT_PKT_TYPE PktType
	);
	
typedef int
(*BT_FP_SET_WHITENINGCOFFENABLE)(
	BT_DEVICE *pBtDevice,
	unsigned char WhiteningCoeffEnable
	);
typedef int
(*BT_FP_SET_PAYLOADTYPE)(
	BT_DEVICE *pBtDevice,
	BT_PAYLOAD_TYPE PayloadType
	);
typedef int
(*BT_FP_SET_TXCHANNEL)(
	BT_DEVICE *pBtDevice,
	unsigned char ChannelNumber
	);
typedef int
(*BT_FP_SET_RXCHANNEL)(
	BT_DEVICE *pBtDevice,
	unsigned char ChannelNumber
	);	
typedef int
(*BT_FP_SET_POWERGAIN)(
	BT_DEVICE *pBtDevice,
	unsigned char PowerGainValue
	);
typedef int
(*BT_FP_SET_POWERDAC)(
	BT_DEVICE *pBtDevice,
	unsigned char DacValue
	);		

//------------------------------------------------------------------------------------------------------------------
//-->Register Read/Write 
typedef int
  (*BT_FP_SET_MD_REG_MASK_BITS)(
	BT_DEVICE *pBtDevice,
	unsigned char Addr,
	unsigned char Msb,
	unsigned char Lsb,
	const unsigned long UserValue
	);

typedef int
(*BT_FP_GET_MD_REG_MASK_BITS)(
	BT_DEVICE *pBtDevice,
	unsigned char RegStartAddr,
	unsigned char Msb,
	unsigned char Lsb,
	unsigned long *pReadingValue
	);


typedef int
(*BT_FP_SET_RF_REG_MASK_BITS)(
	BT_DEVICE *pBtDevice,
	unsigned char RegStartAddr,
	unsigned char Msb,
	unsigned char Lsb,
	const unsigned long WritingValue
	);

typedef int
(*BT_FP_GET_RF_REG_MASK_BITS)(
	BT_DEVICE *pBtDevice,
	unsigned char RegStartAddr,
	unsigned char Msb,
	unsigned char Lsb,
	unsigned long *pReadingValue
	);
	
//------------------------------------------------------------------------------------------------------------------
//-->BASE
typedef int
(*BT_BASE_FP_HITTARGETACCRESSCODEGEN)(
				BT_DEVICE *pBtDevice,
				ULONG64 HitTarget,
				unsigned long *pAccessCode
                                );
typedef int
(*BT_BASE_FP_GETPAYLOADTYPEVAILDFLAG)(
				BT_DEVICE *pBtDevice,
				BT_TEST_MODE TestMode,
				BT_PKT_TYPE PKT_TYPE,
				unsigned int *ValidFlag
	                        );

//------------------------------------------------------------------------------------------------------------------
//-->Control Flow
typedef int(*BT_FP_SET_CONTINUETX_BEGIN)(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);
typedef int(*BT_FP_SET_CONTINUETX_STOP)(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);				
typedef int(*BT_FP_SET_CONTINUETX_UPDATE)(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);				
//-->PKT TX Flow
typedef int(*BT_FP_SET_PKTTX_BEGIN)(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);
typedef int(*BT_FP_SET_PKTTX_STOP)(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);
typedef int(*BT_FP_SET_PKTTX_UPDATE)(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);
//-->PKT RX Flow
typedef int(*BT_FP_SET_PKTRX_BEGIN)(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);
typedef int(*BT_FP_SET_PKTRX_STOP)(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);
typedef int(*BT_FP_SET_PKTRX_UPDATE)(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport);

typedef int(*BT_FP_GET_CHIPID)(BT_DEVICE *pBtDevice);	
typedef int(*BT_FP_GET_ECOVERSION)	(BT_DEVICE *pBtDevice);		
typedef int(*BT_FP_GET_CHIPVERSIONINFO)(BT_DEVICE *pBtDevice);
typedef int(*BT_FP_BT_DL_FW)(BT_DEVICE *pBtDevice,unsigned char *pPatchcode,int patchLength);
typedef int(*BT_FP_BT_DL_MERGER_FW)(BT_DEVICE *pBtDevice,unsigned char *pPatchcode,int patchLength);

//------------------------------------------------------------------------------------------------------------------

typedef struct   BT_TRX_TIME_TAG BT_TRX_TIME;

typedef enum{
        TRX_TIME_STOP =0,
        TX_TIME_RUNING ,
        RX_TIME_RUNING,
        
        //////////////
        NUMOFTRXTIME_TAG
}TRXTIME_TAG;

struct  BT_TRX_TIME_TAG{
	unsigned long beginTimeClockCnt;
	unsigned long UseTimeClockCnt;
	unsigned long endTimeClockCnt;
};
struct BT_DEVICE_TAG  // Chip
{

	//-->Table is base function
	unsigned char TXGainTable[MAX_TXGAIN_TABLE_SIZE];
	unsigned char TXDACTable[MAX_TXDAC_TABLE_SIZE];
	BT_FP_SET_TXGAINTABLE		  SetTxGainTable;
	BT_FP_SET_TXDACTABLE		  SetTxDACTable;
	BT_FP_GET_PAYLOADLENTABLE	  GetPayloadLenTable;
		
	//-->Register Read/Write 
	BT_FP_SET_MD_REG_MASK_BITS        SetMdRegMaskBits;
	BT_FP_GET_MD_REG_MASK_BITS        GetMdRegMaskBits;	

	BT_FP_SET_RF_REG_MASK_BITS        SetRfRegMaskBits;
	BT_FP_GET_RF_REG_MASK_BITS        GetRfRegMaskBits;

	//-->HCI Command & Event
	BT_FP_SEND_HCICOMMANDWITHEVENT	  SendHciCommandWithEvent;
	BT_FP_RECV_ANYEVENT		  RecvAnyHciEvent;
        
        //-->HCI command raw data
	BT_FB_SEND_HCI_CMD		  SendHciCmd;
	BT_FB_RECV_HCI_EVENT		  RecvHciEvent;
	
	//Device member
	//-->Register Control 
	BT_FP_SET_TXCHANNEL			  SetTxChannel;
	BT_FP_SET_TXCHANNEL			  SetLETxChannel;
	BT_FP_SET_RXCHANNEL    		  SetRxChannel;	
	BT_FP_SET_POWERGAININDEX	  SetPowerGainIndex;
	BT_FP_SET_POWERGAIN		  SetPowerGain;
	BT_FP_SET_POWERDAC		  SetPowerDac;
	BT_FP_SET_PAYLOADTYPE	  	  SetPayloadType;	
	BT_FP_SET_WHITENINGCOFFENABLE	  SetWhiteningCoeffEnable;
	BT_FP_SET_PACKETTYPE		  SetPacketType;
	BT_FP_SET_HITTARGET		  SetHitTarget;
	BT_FP_SET_TESTMODE		  SetTestMode;
	BT_FP_SET_MUTIRXENABLE		  SetMutiRxEnable;
	BT_FP_SET_SETRESETMDCOUNT		SetRestMDCount;
	BT_FP_SET_SETPACKHEADER		SetPackHeader;
	BT_FP_SET_PESUDOOUTERSETUP	SetPesudoOuterSetup;
	
	
	
	
	//-->Vendor HCI Command Control 	
	BT_FP_SET_FWPOWERTRACKENABLE		SetFWPowerTrackEnable;
	BT_FP_SET_HOPPINGMODE				SetHoppingMode;
	BT_FP_SET_HCIRESET					SetHciReset;
    BT_FP_GET_BT_CLOCK_TIME				GetBTClockTime;
        
        
        //--->Control Flow
        unsigned int  TRXSTATE;
        unsigned long OldModemReg4Value;
        unsigned long TxTriggerPktCnt;

        //0:TRX STOP  1 : is TX 2: IS rx 
        //Con-TX
        BT_TRX_TIME                             TRxTime[NUMOFTRXTIME_TAG];
        BT_FP_SET_CONTINUETX_BEGIN		SetContinueTxBegin;
        BT_FP_SET_CONTINUETX_STOP		SetContinueTxStop;
        BT_FP_SET_CONTINUETX_UPDATE		SetContinueTxUpdate;
        
        //PKT-TX
        BT_FP_SET_PKTTX_BEGIN			SetPktTxBegin;
        BT_FP_SET_PKTTX_STOP			SetPktTxStop;
        BT_FP_SET_PKTTX_UPDATE			SetPktTxUpdate;        
 
         //PKT-RX
        BT_FP_SET_PKTRX_BEGIN			SetPktRxBegin;
        BT_FP_SET_PKTRX_STOP			SetPktRxStop;
        BT_FP_SET_PKTRX_UPDATE			SetPktRxUpdate;
        
                
        //Base Function
		BT_BASE_FP_GETPAYLOADTYPEVAILDFLAG      GetPayLoadTypeValidFlag;
		BT_BASE_FP_HITTARGETACCRESSCODEGEN		HitTargetAccessCodeGen;

        //interface
		void *pExtra;	
		unsigned char InterfaceType;

		BASE_INTERFACE_MODULE *pBaseInterface;

		BT_CHIPINFO	*pBTInfo;
		BT_CHIPINFO	BaseBTInfoMemory;
		BT_FP_GET_CHIPID						GetChipId;
		BT_FP_GET_ECOVERSION					GetECOVersion;
		BT_FP_GET_CHIPVERSIONINFO				GetChipVersionInfo;
		BT_FP_BT_DL_FW							BTDlFW;
		BT_FP_BT_DL_MERGER_FW					BTDlMERGERFW;
		//Inner
		unsigned long Inner_TotalRXBits;
		unsigned long Inner_TotalRxCounts;
		unsigned long Inner_TotalRxErrorBits;

		unsigned long Inner_TX_MD_0X2E;
		unsigned long Inner_RX_MD_0X2E;

		unsigned char Inner_RX_First; 



};

//-----------------------------------------------------------------------------------------------------------------
//		Module
//-----------------------------------------------------------------------------------------------------------------
typedef enum _BT_REPORT_TAG{
    NO_THING,
    REPORT_TX,
    REPORT_RX,
    REPORT_CHIP,
    REPORT_ALL
}BT_REPORT_TAG;


typedef struct  BT_MODULE_TAG BT_MODULE;
//-->module
typedef int
(*BT_MODULE_FP_ACTION_REPORT)(
				BT_MODULE *pBtModule,
				int ActiceItem,
                BT_DEVICE_REPORT *pReport
	                        );

typedef int
(*BT_MODULE_FP_UPDATA_PARAMETER)(
				BT_MODULE *pBtModule,
                                 BT_PARAMETER 	*pParam
	                        );
typedef int
(*BT_MODULE_FP_ACTION_CONTROLEXCUTE)(
				BT_MODULE *pBtModule
	                        );
typedef int
(*BT_MODULE_FP_ACTION_DLFW)(
				BT_MODULE *pBtModule,
				unsigned char *pPatchcode,
				int patchLength,
				int Mode);

//-->Register Read/Write 
typedef int
  (*BT_MODULE_FP_SET_MD_REG_MASK_BITS)(
	BT_MODULE *pBtModule,
	unsigned char Addr,
	unsigned char Msb,
	unsigned char Lsb,
	const unsigned long UserValue
	);

typedef int
(*BT_MODULE_FP_GET_MD_REG_MASK_BITS)(
	BT_MODULE *pBtModule,
	unsigned char RegStartAddr,
	unsigned char Msb,
	unsigned char Lsb,
	unsigned long *pReadingValue
	);


typedef int
(*BT_MODULE_FP_SET_RF_REG_MASK_BITS)(
	BT_MODULE *pBtModule,
	unsigned char RegStartAddr,
	unsigned char Msb,
	unsigned char Lsb,
	const unsigned long WritingValue
	);

typedef int
(*BT_MODULE_FP_GET_RF_REG_MASK_BITS)(
	BT_MODULE *pBtModule,
	unsigned char RegStartAddr,
	unsigned char Msb,
	unsigned char Lsb,
	unsigned long *pReadingValue
	);
//-->HCI Command & Event
typedef int
(*BT_MODULE_FP_SEND_HCICOMMANDWITHEVENT)(
	BT_MODULE *pBtModule,
	unsigned int  OpCode,
	unsigned char PayLoadLength,
	unsigned char *pPayLoad,
	unsigned char  EventType,
	unsigned char  *pEvent,
       unsigned char  *pEventLen
	);
typedef int
(*BT_MODULE_FP_RECV_ANYEVENT)(
	BT_MODULE *pBtModule,
	unsigned char  *pEvent
	);
//------------------------------------------------------------------------------------------------------------------
struct BT_MODULE_TAG
{
        //Interface 
    //    BASE_INTERFACE_MODULE	*pBaseInterface;

        //Member Value
	BT_PARAMETER	 *pBtParam;
	BT_DEVICE		 *pBtDevice;
	BT_DEVICE_REPORT *pModuleBtReport;

	BT_PARAMETER	  BaseBtParamMemory;
	BT_DEVICE		  BaseBtDeviceMemory;
	BT_DEVICE_REPORT  BaseModuleBtReportMemory;



    //Module Function
	BT_MODULE_FP_UPDATA_PARAMETER   	 UpDataParameter;
	BT_MODULE_FP_ACTION_CONTROLEXCUTE	 ActionControlExcute;
	BT_MODULE_FP_ACTION_REPORT			 ActionReport;

	
	BT_MODULE_FP_ACTION_DLFW			 DownloadPatchCode;

    //register read/write and hci command 
	BT_MODULE_FP_SET_MD_REG_MASK_BITS        SetMdRegMaskBits;
	BT_MODULE_FP_GET_MD_REG_MASK_BITS        GetMdRegMaskBits;	

	BT_MODULE_FP_SET_RF_REG_MASK_BITS        SetRfRegMaskBits;
	BT_MODULE_FP_GET_RF_REG_MASK_BITS        GetRfRegMaskBits;

	//-->HCI Command & Event
	BT_MODULE_FP_SEND_HCICOMMANDWITHEVENT   SendHciCommandWithEvent;
	BT_MODULE_FP_RECV_ANYEVENT				RecvAnyHciEvent;


};

//-----------------------------------------------------------------------------------------------------
//		Base Function
//-----------------------------------------------------------------------------------------------------



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
#define LEN_14_BYTE					14
#define LEN_16_BYTE					16

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



#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

//  Define UART HCI Packet Indicator
#define     IF_UART_CMD	0x01
#define     IF_UART_ACL	0x02
#define     IF_UART_SCO	0x03
#define     IF_UART_EVT	0x04




//  Define HCI IO
#define     HCIIO_EFUSE				0x00
#define     HCIIO_BTRFREG				0x01
#define     HCIIO_BTMODEMREG		0x02
#define     HCIIO_BTCMD				0x03
#define     HCIIO_BTEVT				0x04
#define     HCIIO_BTACLIN				0x05
#define     HCIIO_BTACLOUT			0x06
#define     HCIIO_BTSCOIN				0x07
#define     HCIIO_BTSCOOUT			0x08
#define     HCIIO_PATCHCODE			0x09
#define     HCIIO_UART_H5			0x0A



//  Patch code download size
#define     PATCHCODE_DOWNLOAD_SIZE                     0xFC

#define     RESET_DEFAULTVALUE                          0x00

//Define BTHCI Debug mode
#define BTHCI_DEBUG             1

enum BT_HCI_EVENT_FIELD
{
	EVT_CODE = 0,
	EVT_PARA_LEN,
	EVT_HCI_CMD_NUM,
	EVT_CMD_OPCODE_0,
	EVT_CMD_OPCODE_1,
	EVT_STATUS,
	EVT_BYTE0,
	EVT_BYTE1,
	EVT_BYTE2,
	EVT_BYTE3,
};

//member function
int bt_default_SetMDRegMaskBits(
	BT_DEVICE *pBtDevice,
	unsigned char Addr,
	unsigned char Msb,
	unsigned char Lsb,
	const unsigned long UserValue
	);
int
bt_default_GetMDRegMaskBits(
	BT_DEVICE *pBt,
	unsigned char Addr,
	unsigned char Msb,
	unsigned char Lsb,
	unsigned long *pUserValue
	);
int
bt_default_GetRFRegMaskBits(
	BT_DEVICE *pBt,
	unsigned char Addr,
	unsigned char Msb,
	unsigned char Lsb,
	unsigned long *pUserValue
	);
int
bt_default_SetRFRegMaskBits(
	BT_DEVICE *pBt,
	unsigned char Addr,
	unsigned char Msb,
	unsigned char Lsb,
	const unsigned long UserValue
	) ;
int
bt_default_GetBytes(
	BT_DEVICE *pBt,
	unsigned char Addr,
	unsigned long *pReadingValue,
	unsigned char *pEvtCode,
	unsigned long *pEvtCodeLen
	);
int
bt_default_SetBytes(
	BT_DEVICE *pBt,
	unsigned char Addr,
	unsigned long WritingValue,
	unsigned char *pEvtCode,
	unsigned long *pEvtCodeLen
	);
int
bt_default_RecvHCIEvent(
	BT_DEVICE *pBt,
	unsigned char PktType,
	unsigned char *pReadingBuf,
	unsigned long *pLen
	);
int
bt_default_SendHCICmd(
	BT_DEVICE *pBt,
	unsigned char PktType,
	unsigned char *pWritingBuf,
	unsigned long Len
	);
int
bt_uart_Recv(
	BT_DEVICE *pBt,
	unsigned char PktType,
	unsigned char *pReadingBuf,
	unsigned long *pLen
	);
int
bt_uart_Send(
	BT_DEVICE *pBt,
	unsigned char PktType,
	unsigned char *pWritingBuf,
	unsigned long Len
	);
int
bt_Recv(
	BT_DEVICE *pBt,
	unsigned char PktType,
	unsigned char *pReadingBuf,
	unsigned long *pLen
	);
int
bt_Send(
	BT_DEVICE *pBt,
	unsigned char PktType,
	unsigned char *pWritingBuf,
	unsigned long Len
	);
void
BTHCI_EvtReport(
	unsigned char *pEvtCode,
	unsigned long EvtCodeLen
	);
int bt_default_GetChipId(BT_DEVICE *pBtDevice);
int bt_default_GetECOVersion(BT_DEVICE *pBtDevice);
int bt_default_GetBTChipVersionInfo(BT_DEVICE *pBtDevice);
//int bt_default_GetBTChipDesc(BT_DEVICE *pBtDevice,char *ChipName);
int bt_default_BTDlFW(BT_DEVICE *pBtDevice,unsigned char *pPatchcode,int patchLength);
int bt_default_BTDlMergerFW(BT_DEVICE *pBtDevice,unsigned char *pPatchcode,int patchLength);

#endif
