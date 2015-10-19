 

#include "foundation.h"
#include <utils/Log.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>


// Base uart interface builder
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
	)
{
	// Set base interface module pointer.
	*ppBaseInterface = pBaseInterfaceModuleMemory;
	
	(*ppBaseInterface)->InterfaceType = TYPE_USB;
	(*ppBaseInterface)->PortNo = PortNo;
	(*ppBaseInterface)->Baudrate = Baudrate;

	// Set all base interface function pointers and arguments.
	(*ppBaseInterface)->Open = Open;
	(*ppBaseInterface)->Send = Send;
	(*ppBaseInterface)->Recv = Recv;
	(*ppBaseInterface)->WaitMs = WaitMs;
	(*ppBaseInterface)->Close = Close;
	
	(*ppBaseInterface)->SetUserDefinedDataPointer = base_interface_SetUserDefinedDataPointer;
	(*ppBaseInterface)->GetUserDefinedDataPointer = base_interface_GetUserDefinedDataPointer;



	return;
}




 
void
base_interface_SetUserDefinedDataPointer(
	BASE_INTERFACE_MODULE *pBaseInterface,
	int UserDefinedData
	)
{
	// Set user defined data pointer of base interface structure with user defined data pointer argument.
	pBaseInterface->UserDefinedData = UserDefinedData;


	return;
}




 
void
base_interface_GetUserDefinedDataPointer(
	BASE_INTERFACE_MODULE *pBaseInterface,
	int *pUserDefinedData
	)
{
	// Get user defined data pointer from base interface structure to the caller user defined data pointer.
	*pUserDefinedData = pBaseInterface->UserDefinedData;


	return;
}




 
unsigned long
SignedIntToBin(
	long Value,
	unsigned char BitNum
	)
{
	unsigned char i;
	unsigned long Mask, Binary;



	// Generate Mask according to BitNum.
	Mask = 0;
	for(i = 0; i < BitNum; i++)
		Mask |= 0x1 << i;


	// Convert signed integer to binary with Mask.
	Binary = Value & Mask;


	return Binary;
}




 
long
BinToSignedInt(
	unsigned long Binary,
	unsigned char BitNum
	)
{
	int i;

	unsigned char SignedBit;
	unsigned long SignedBitExtension;

	long Value;



	// Get signed bit.
	SignedBit = (unsigned char)((Binary >> (BitNum - 1)) & BIT_0_MASK);


	// Generate signed bit extension.
	SignedBitExtension = 0;

	for(i = BitNum; i < LONG_BIT_NUM; i++)
		SignedBitExtension |= SignedBit << i;


	// Combine binary value and signed bit extension to signed integer value.
	Value = (long)(Binary | SignedBitExtension);


	return Value;
}




 
unsigned long
DivideWithCeiling(
	unsigned long Dividend,
	unsigned long Divisor
	)
{
	unsigned long Result;


	// Get primitive division result.
	Result = Dividend / Divisor;

	// Adjust primitive result with ceiling.
	if(Dividend % Divisor > 0)
		Result += 1;


	return Result;
}

#if 0
void  BTDevice_BaseTxGain_Dac_Table(BT_DEVICE *pBtDevice,
                                        unsigned char *pTxGainTable,
                                        unsigned char *pTxDACTable)
{
        int n=0;
        unsigned char *pTxGain=pBtDevice->TXGainTable;
        unsigned char *pTxDac=pBtDevice->TXDACTable;
        if (pTxGainTable != NULL)
        {
                for (n=0;n<MAX_TXGAIN_TABLE_SIZE;n++)
                {
                        *(pTxGain+n)=pTxGainTable[n];
                }
        }
        else
        {
                for (n=0;n<MAX_TXDAC_TABLE_SIZE;n++)
                {
                        *(pTxGain+n)=0;
                }
        }
        if (pTxDACTable != NULL)
        {
                for (n=0;n<MAX_TXDAC_TABLE_SIZE;n++)
                {
                        *(pTxDac+n)=pTxDACTable[n];
                }
        }
        else
        {
                for (n=0;n<MAX_TXDAC_TABLE_SIZE;n++)
                {
                       *(pTxDac+n)=0;
                }
        }
        return ;
}
#endif



#ifndef MPTOOL_LOG_BUF_SIZE
#define MPTOOL_LOG_BUF_SIZE  1024
#endif
#define MPTOOL_LOG_MAX_SIZE  (MPTOOL_LOG_BUF_SIZE - 12)

#define LOGI0(t,s) __android_log_write(ANDROID_LOG_INFO, t, s)

void
bt_mp_LogMsg(const char *fmt_str, ...)
{
    static char buffer[MPTOOL_LOG_BUF_SIZE];

    va_list ap;
    va_start(ap, fmt_str);
    vsnprintf(&buffer[0], MPTOOL_LOG_MAX_SIZE, fmt_str, ap);
    va_end(ap);

    LOGI0("rtlbtmp: ", buffer);
}


/*****************************************************************************
**   Logger API
*****************************************************************************/

void bdt_log(const char *fmt_str, ...)
{
    static char buffer[1024];
    va_list ap;

    va_start(ap, fmt_str);
    vsnprintf(buffer, 1024, fmt_str, ap);
    va_end(ap);

    fprintf(stdout, "%s", buffer);
}
