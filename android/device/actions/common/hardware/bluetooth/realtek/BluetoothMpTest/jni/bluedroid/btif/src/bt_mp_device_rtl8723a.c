
#include "bt_mp_device_rtl8723a.h"

//------------------------------------------------------------------------------------------------------------------
//
//Device Level::member Funcion Base Function for RTL8723A
int BTDevice_SetPowerGainIndex_RTL8723A(BT_DEVICE *pBtDevice,int Index)
{
	unsigned char PowerGainValue =0;
	int rtn=BT_FUNCTION_SUCCESS;
	if (Index >=MAX_TXGAIN_TABLE_SIZE)
	{
		rtn=FUNCTION_PARAMETER_ERROR;
		goto exit;
	}
		
	PowerGainValue =pBtDevice->TXGainTable[Index];
	/* set rf standby mode */  
	rtn=pBtDevice->SetRfRegMaskBits(pBtDevice,0x00,15,0,0x1000); 
	if (rtn != BT_FUNCTION_SUCCESS)
	{
		goto exit;
	}
	rtn=BTDevice_SetPowerGain_RTL8723A(pBtDevice,PowerGainValue);
exit:	
	return	rtn;
}
int BTDevice_SetTestMode_RTL8723A(BT_DEVICE *pBtDevice,BT_TEST_MODE TestMode)
{
	int rtn=BT_FUNCTION_SUCCESS;
	//coding....
	switch (TestMode)
	{
		case BT_DUT_MODE:
			if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,11,8,0x0E) !=BT_FUNCTION_SUCCESS)
			{
				rtn=FUNCTION_ERROR;
				goto exit;	 
			}
			
				    
		break;
		case BT_PSEUDO_MODE:
			if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,11,8,0x0F) !=BT_FUNCTION_SUCCESS)
			{
				rtn=FUNCTION_ERROR;
				goto exit;	 
			}
			//WriteGlobalReg(0x8074, 0xffffffff, 0x7d7);
			//WriteGlobalReg(0x8076, 0xffffffff, 0x2196);
			//WriteGlobalReg(0x8076, 0xffffffff, 0x2116);
			
		break;
		default:
		rtn=FUNCTION_PARAMETER_ERROR;
		goto exit;	
		
	}	
	
exit:	
	return rtn; 
}

int BTDevice_SetTxChannel_RTL8723A(BT_DEVICE *pBtDevice,unsigned char ChannelNumber)
{
	int rtn=BT_FUNCTION_SUCCESS;
	if (ChannelNumber >=79)
		return FUNCTION_PARAMETER_INVALID_CHANNEL;

	rtn=pBtDevice->SetRfRegMaskBits(pBtDevice,0x00,15,0,0x1000); 
	if (rtn != BT_FUNCTION_SUCCESS)
	{
		goto exit;
	}
	if (rtn != BT_FUNCTION_SUCCESS)
	{
		goto exit;
	}	
	rtn=pBtDevice->SetRfRegMaskBits(pBtDevice,0x3C,15,8,6+((ChannelNumber*2)&0xFF)); 
exit:	
	return rtn; 
}
int BTDevice_SetRxChannel_RTL8723A(BT_DEVICE *pBtDevice,unsigned char ChannelNumber)
{
	int rtn=BT_FUNCTION_SUCCESS;
	rtn=pBtDevice->SetRfRegMaskBits(pBtDevice,0x00,15,0,0x1000); 
	if (rtn != BT_FUNCTION_SUCCESS)
	{
		goto exit;
	}	
	rtn=pBtDevice->SetRfRegMaskBits(pBtDevice,0x3C,15,8,6+((ChannelNumber*2)&0xFF)-5); 
exit:	
	return rtn; 
}
int BTDevice_SetPowerGain_RTL8723A(BT_DEVICE *pBtDevice,unsigned char PowerGainValue)
{
	int rtn=BT_FUNCTION_SUCCESS;
	/* set rf standby mode */  
	rtn=pBtDevice->SetRfRegMaskBits(pBtDevice,0x00,15,0,0x1000); 
	if (rtn != BT_FUNCTION_SUCCESS)
	{
		goto exit;
	}	
	rtn=pBtDevice->SetRfRegMaskBits(pBtDevice,0x03,15,10,PowerGainValue); 
exit:	
	return rtn; 
}
int BTDevice_SetPowerDac_RTL8723A(BT_DEVICE *pBtDevice,unsigned char DacValue)
{
	int rtn=BT_FUNCTION_SUCCESS;
	
	rtn=pBtDevice->SetMdRegMaskBits(pBtDevice,0x38,7,3,DacValue); 
	
	return rtn; 
}