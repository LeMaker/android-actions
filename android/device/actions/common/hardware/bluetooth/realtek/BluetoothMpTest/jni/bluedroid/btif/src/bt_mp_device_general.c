

#include "bt_mp_device_general.h"

//------------------------------------------------------------------------------------------------------------------
//Device Level::member Funcion Base Function for
//
//-----------------------------------------------------------------------------------------------------


int BTDevice_SetPowerGainIndex(	BT_DEVICE *pBtDevice,	int Index)
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
	rtn=BTDevice_SetPowerGain(pBtDevice,PowerGainValue);
exit:	
	return	rtn;
}
//-----------------------------------------------------------------------------------------------------

int BTDevice_SetTestMode(BT_DEVICE *pBtDevice,BT_TEST_MODE TestMode)
{
	int rtn=BT_FUNCTION_SUCCESS;

	switch (TestMode)
	{
		case BT_DUT_MODE:
			if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,11,8,0x0E) !=BT_FUNCTION_SUCCESS)
			{
				rtn=FUNCTION_ERROR;
				goto exit;	 
			}
			if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x3c,12,12,0x1) !=BT_FUNCTION_SUCCESS)
			{
				rtn=FUNCTION_ERROR;
				goto exit;	 
			}				    
		break;

		case BT_PSEUDO_MODE:
			 /* disable modem fix tx */
			if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x3c,12,12,0x0) !=BT_FUNCTION_SUCCESS)
			{
				rtn=FUNCTION_ERROR;
				goto exit;
			}
			/* enable pesudo outter mode */
             rtn=pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,11,8,0x0F);
			if (rtn !=BT_FUNCTION_SUCCESS)
			{
				rtn=FUNCTION_ERROR;
				goto exit;
			}

                
		break;
		default:
		        rtn=FUNCTION_PARAMETER_ERROR;
		goto exit;	
		
	}	
	
exit:	
	return rtn; 
}

 //-----------------------------------------------------------------------------------------------------
int BTDevice_SetLETxChannel(BT_DEVICE *pBtDevice,unsigned char ChannelNumber)
{
	int rtn=BT_FUNCTION_SUCCESS;
	/* set rf standby mode */  
	ChannelNumber=ChannelNumber*2;
	if (ChannelNumber >=40)
		return FUNCTION_PARAMETER_INVALID_CHANNEL;

	rtn=pBtDevice->SetRfRegMaskBits(pBtDevice,0x00,15,0,0x1000); 
	if (rtn != BT_FUNCTION_SUCCESS)
	{
		goto exit;
	}
	rtn=pBtDevice->SetRfRegMaskBits(pBtDevice,0x3F,15,0,ChannelNumber&0x7F); 
exit:	
	return rtn; 
}
int BTDevice_SetTxChannel(BT_DEVICE *pBtDevice,unsigned char ChannelNumber)
{
	int rtn=BT_FUNCTION_SUCCESS;
	/* set rf standby mode */  
	if (ChannelNumber >=79)
		return FUNCTION_PARAMETER_INVALID_CHANNEL;

	rtn=pBtDevice->SetRfRegMaskBits(pBtDevice,0x00,15,0,0x1000); 
	if (rtn != BT_FUNCTION_SUCCESS)
	{
		goto exit;
	}
	rtn=pBtDevice->SetRfRegMaskBits(pBtDevice,0x3F,15,0,ChannelNumber&0x7F); 
exit:	
	return rtn; 
}
//-----------------------------------------------------------------------------------------------------
int BTDevice_SetRxChannel(BT_DEVICE *pBtDevice,unsigned char ChannelNumber)
{
	int rtn=BT_FUNCTION_SUCCESS;
	if (ChannelNumber >=79)
		return FUNCTION_PARAMETER_INVALID_CHANNEL;
	/* set rf standby mode */  
	rtn=pBtDevice->SetRfRegMaskBits(pBtDevice,0x00,15,0,0x1000); 
	if (rtn != BT_FUNCTION_SUCCESS)
	{
		goto exit;
	}	
	/* set rf channel */  
	rtn=pBtDevice->SetRfRegMaskBits(pBtDevice,0x3F,15,0,0x80 | (ChannelNumber&0x7F)); 
exit:	
	return rtn; 
}
//-----------------------------------------------------------------------------------------------------
int BTDevice_SetPowerGain(BT_DEVICE *pBtDevice,unsigned char PowerGainValue)
{
	int rtn=BT_FUNCTION_SUCCESS;
	/* set rf standby mode */  
	rtn=pBtDevice->SetRfRegMaskBits(pBtDevice,0x00,15,0,0x1000); 
	if (rtn != BT_FUNCTION_SUCCESS)
	{
		goto exit;
	}	
	rtn=pBtDevice->SetRfRegMaskBits(pBtDevice,0x02,15,8,PowerGainValue); 
exit:	
	return rtn; 
}
//-----------------------------------------------------------------------------------------------------
int BTDevice_SetPowerDac(BT_DEVICE *pBtDevice,unsigned char DacValue)
{
	int rtn=BT_FUNCTION_SUCCESS;

	rtn=pBtDevice->SetMdRegMaskBits(pBtDevice,0x38,6,3,DacValue);

	return rtn;
}
//-----------------------------------------------------------------------------------------------------
