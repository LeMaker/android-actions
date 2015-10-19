
#include "bt_mp_module_base.h"
//------------------------------------------------------------------------------------------------------------------


int BTModule_ActionReport(      
				BT_MODULE *pBtModule,
				int ActiceItem,
                BT_DEVICE_REPORT *pReport
	            )
{
	int rtn=BT_FUNCTION_SUCCESS;
	BT_DEVICE 	 *pModuleBtDevice	=pBtModule->pBtDevice;
	BT_PARAMETER 	 *pModuleBtParam	=pBtModule->pBtParam;
	BT_DEVICE_REPORT *pModuleBtReport =pBtModule->pModuleBtReport;

	switch (ActiceItem)
	{
	case NO_THING:
	break;
	case REPORT_TX:
		pReport->TotalTXBits=pModuleBtReport->TotalTXBits;
		pReport->TotalTxCounts=pModuleBtReport->TotalTxCounts;
//		pReport->TXPktUpdateCnts=pModuleBtReport->TXPktUpdateCnts;
	//	pReport->TXUpdateBits=pModuleBtReport->TXUpdateBits;


	break;
	case REPORT_RX:
		pReport->TotalRXBits=pModuleBtReport->TotalRXBits;
		pReport->TotalRxCounts=pModuleBtReport->TotalRxCounts;
		pReport->TotalRxErrorBits=pModuleBtReport->TotalRxErrorBits;
		pReport->ber=pModuleBtReport->ber;
		pReport->IsRxRssi=pModuleBtReport->IsRxRssi;
	//	pReport->RXPktUpdateCnts=pModuleBtReport->RXPktUpdateCnts;
	//	pReport->RXUpdateBits=pModuleBtReport->RXUpdateBits;

	break;
	case REPORT_CHIP:
		if (pModuleBtDevice->GetChipVersionInfo(pModuleBtDevice) != BT_FUNCTION_SUCCESS)
		{
			rtn=FUNCTION_ERROR;
		}
		else
		{
				pReport->pBTInfo=pModuleBtDevice->pBTInfo;
		}
	break;
	default:
	case REPORT_ALL:
		pReport->TotalTXBits=pModuleBtReport->TotalTXBits;
		pReport->TotalTxCounts=pModuleBtReport->TotalTxCounts;
		pReport->TotalRXBits=pModuleBtReport->TotalRXBits;
		pReport->TotalRxCounts=pModuleBtReport->TotalRxCounts;
		pReport->TotalRxErrorBits=pModuleBtReport->TotalRxErrorBits;
		pReport->ber=pModuleBtReport->ber;
		pReport->IsRxRssi=pModuleBtReport->IsRxRssi;
	break;
	}
	return rtn;
}

//------------------------------------------------------------------------------------------------------------------
int BTModule_UpDataParameter(
                                BT_MODULE *pBtModule,
                                BT_PARAMETER 	*pParam
                            )
{
	int rtn=BT_FUNCTION_SUCCESS;
	BT_PARAMETER 	*pBtModuleParam		=pBtModule->pBtParam;

	memcpy(pBtModuleParam,pParam,sizeof(BT_PARAMETER));

	return rtn;
}
//------------------------------------------------------------------------------------------------------------------

int BTModule_ActionControlExcute(
				BT_MODULE *pBtModule
	                        )
{
	int rtn=BT_FUNCTION_SUCCESS;
        int Item=pBtModule->pBtParam->ParameterIndex;
	BT_DEVICE 	 *pModuleBtDevice	=pBtModule->pBtDevice;
	BT_PARAMETER 	 *pModuleBtParam	=pBtModule->pBtParam;
	BT_DEVICE_REPORT *pModuleBtReport =pBtModule->pModuleBtReport;

      	switch(Item)
	{

                ////////////////////////// MODULE INIT   /////////////////////////////////////////////////////////
		case MODULE_INIT:
                break;
                ////////////////////////// TABLE          /////////////////////////////////////////////////////// 
                case SETTXGAINTABLE:
                        rtn=pModuleBtDevice->SetTxGainTable(pModuleBtDevice,pModuleBtParam->TXGainTable);
                break;
                case SETTXDACTABLE:
                        rtn=pModuleBtDevice->SetTxDACTable(pModuleBtDevice,pModuleBtParam->TXDACTable);
                break;
                ////////////////////////// DEVICE MEMBER /////////////////////////////////////////////////////////
		case SET_TXCHANNEL:
                        rtn=pModuleBtDevice->SetTxChannel(pModuleBtDevice,pModuleBtParam->mChannelNumber);
                break;
		case SET_RXCHANNEL:
                        rtn=pModuleBtDevice->SetRxChannel(pModuleBtDevice,pModuleBtParam->mChannelNumber);
                break;
                
                case SET_POWERGAININDEX:
                	 rtn=pModuleBtDevice->SetPowerGainIndex(pModuleBtDevice,pModuleBtParam->mTxGainIndex);
                break;	 
		case SET_POWERGAIN:
                	 rtn=pModuleBtDevice->SetPowerGain(pModuleBtDevice,pModuleBtParam->mTxGainValue);
                break;			
		case SET_POWERDAC:
                	 rtn=pModuleBtDevice->SetPowerDac(pModuleBtDevice,pModuleBtParam->mTxDAC);
                break;			
		case SET_PAYLOADTYPE:
                	 rtn=pModuleBtDevice->SetPayloadType(pModuleBtDevice,pModuleBtParam->mPayloadType);
                break;			
		case SET_WHITENINGCOFFENABLE:
                	 rtn=pModuleBtDevice->SetWhiteningCoeffEnable(pModuleBtDevice,pModuleBtParam->mWhiteningCoeffEnable);
                break;			
		case SET_PACKETTYPE:
                	 rtn=pModuleBtDevice->SetPacketType(pModuleBtDevice,pModuleBtParam-> mPacketType);
                break;			
		case SET_HITTARGET:
                	 rtn=pModuleBtDevice->SetHitTarget(pModuleBtDevice,pModuleBtParam->mHitTarget);
                break;			
		case SET_TESTMODE:
                	 rtn=pModuleBtDevice->SetTestMode(pModuleBtDevice,pModuleBtParam->mTestMode);
                break;			
		case SET_MUTIRXENABLE:
                	 rtn=pModuleBtDevice->SetMutiRxEnable(pModuleBtDevice,pModuleBtParam->mMutiRxEnable);
                break;			
                ////////////////////////// HCI RESET /////////////////////////////////////////////////////////
		case HCI_RESET:
                        rtn=pModuleBtDevice->SetHciReset(pModuleBtDevice,600);
                break;
                /////////////////////////// PACKET_TX /////////////////////////////////////////////////////////
		case PACKET_TX_START:
                        rtn=pModuleBtDevice->SetPktTxBegin(pModuleBtDevice,pModuleBtParam,pModuleBtReport);
                break;
		case PACKET_TX_UPDATE:
                        rtn=pModuleBtDevice->SetPktTxUpdate(pModuleBtDevice,pModuleBtParam,pModuleBtReport);
                break;
		case PACKET_TX_STOP:
                        rtn=pModuleBtDevice->SetPktTxStop(pModuleBtDevice,pModuleBtParam,pModuleBtReport);
                break;
                ////////////////////////// PACKET_RX /////////////////////////////////////////////////////////
		case PACKET_RX_START:
                          rtn=pModuleBtDevice->SetPktRxBegin(pModuleBtDevice,pModuleBtParam,pModuleBtReport);
                break;
		case PACKET_RX_UPDATE:
                        rtn=pModuleBtDevice->SetPktRxUpdate(pModuleBtDevice,pModuleBtParam,pModuleBtReport);
                break;
		case PACKET_RX_STOP:
                        rtn=pModuleBtDevice->SetPktRxStop(pModuleBtDevice,pModuleBtParam,pModuleBtReport);
                break;

                /////////////////////////// CONTINUE_TX /////////////////////////////////////////////////////////
                case CONTINUE_TX_LE_START:
		case CONTINUE_TX_START:
                        rtn=pModuleBtDevice->SetContinueTxBegin(pModuleBtDevice,pModuleBtParam,pModuleBtReport);
                break;
                case CONTINUE_TX_LE_STOP:
		case CONTINUE_TX_STOP:
                        rtn=pModuleBtDevice->SetContinueTxStop(pModuleBtDevice,pModuleBtParam,pModuleBtReport);
                break;
                case CONTINUE_TX_LE_UPDATE:
	        case CONTINUE_TX_UPDATE:
                        rtn=pModuleBtDevice->SetContinueTxUpdate(pModuleBtDevice,pModuleBtParam,pModuleBtReport);
                break;
                /////////////////////////// HOPPING  /////////////////////////////////////////////////////////
		case HOPPING_DWELL_TIME:
                        rtn=pModuleBtDevice->SetHoppingMode(pModuleBtDevice,pModuleBtParam->mPacketType);
                break;
                /////////////////////////// REPORT /////////////////////////////////////////////////////////
        case REPORT_CLEAR:
					pModuleBtReport->TotalTXBits=0;
					pModuleBtReport->TXUpdateBits=0;
					pModuleBtReport->TotalTxCounts=0;
					pModuleBtReport->TXPktUpdateCnts=0;

					pModuleBtReport->TotalRXBits=0;
					pModuleBtReport->RXUpdateBits=0;
					pModuleBtReport->RXPktUpdateCnts=0;
					pModuleBtReport->TotalRxCounts=0;
					pModuleBtReport->TotalRxErrorBits=0;
					pModuleBtReport->IsRxRssi=-90;
					rtn=pModuleBtDevice->SetRestMDCount(pModuleBtDevice);
                break;
		default:
                break;
	}

	return rtn;
}
//------------------------------------------------------------------------------------------------------------------

int BTModule_DownloadPatchCode(
				BT_MODULE *pBtModule
				,unsigned char *pPatchcode
				,int patchLength
				,int Mode)
{
				int rtn=BT_FUNCTION_SUCCESS;

				BT_DEVICE 	 *pModuleBtDevice	=pBtModule->pBtDevice;
				
				if (Mode)
				{
					if (pModuleBtDevice->GetChipVersionInfo(pModuleBtDevice) != BT_FUNCTION_SUCCESS)
					{
							rtn=FUNCTION_ERROR;
							goto exit;
					}

					rtn=pModuleBtDevice->BTDlMERGERFW(pModuleBtDevice,pPatchcode,patchLength);
				}
				else
				{
					rtn=pModuleBtDevice->BTDlFW(pModuleBtDevice,pPatchcode,patchLength);
				}
exit:
				return rtn;

}
//------------------------------------------------------------------------------------------------------------------
int
BTModule_RecvAnyHciEvent(
	BT_MODULE *pBtModule,
	unsigned char  *pEvent
	)
{

		BT_DEVICE *pBtDevice = pBtModule->pBtDevice;

		return pBtDevice->RecvAnyHciEvent(pBtDevice,pEvent);
}
//------------------------------------------------------------------------------------------------------------------
int
BTModule_SendHciCommandWithEvent(
	BT_MODULE *pBtModule,
	unsigned int  OpCode,
	unsigned char PayLoadLength,
	unsigned char *pPayLoad,
	unsigned char  EventType,
	unsigned char  *pEvent,
       unsigned char  *pEventLen
	)
{
    BT_DEVICE *pBtDevice = pBtModule->pBtDevice;
	
	return pBtDevice->SendHciCommandWithEvent(pBtDevice,OpCode,PayLoadLength,pPayLoad,EventType,pEvent, pEventLen);

}
//------------------------------------------------------------------------------------------------------------------
int
BTModule_GetMDRegMaskBits(
	BT_MODULE *pBtModule,
	unsigned char Addr,
	unsigned char Msb,
	unsigned char Lsb,
	unsigned long *pUserValue
	)
{
	 BT_DEVICE *pBtDevice = pBtModule->pBtDevice;
	 return pBtDevice->GetMdRegMaskBits(pBtDevice,Addr,Msb,Lsb,pUserValue);
}
//------------------------------------------------------------------------------------------------------------------

int
BTModule_SetMDRegMaskBits(
	BT_MODULE *pBtModule,
	unsigned char Addr,
	unsigned char Msb,
	unsigned char Lsb,
	const unsigned long UserValue
	)
{
	 BT_DEVICE *pBtDevice = pBtModule->pBtDevice;
	return pBtDevice->SetMdRegMaskBits(pBtDevice,Addr,Msb,Lsb,UserValue);
}
//------------------------------------------------------------------------------------------------------------------

int
BTModule_GetRFRegMaskBits(
	BT_MODULE *pBtModule,
	unsigned char Addr,
	unsigned char Msb,
	unsigned char Lsb,
	unsigned long *pUserValue
	)
{
	 BT_DEVICE *pBtDevice = pBtModule->pBtDevice;
	 return pBtDevice->GetRfRegMaskBits(pBtDevice,Addr,Msb,Lsb,pUserValue);
}
//------------------------------------------------------------------------------------------------------------------

int
BTModule_SetRFRegMaskBits(
	BT_MODULE *pBtModule,
	unsigned char Addr,
	unsigned char Msb,
	unsigned char Lsb,
	const unsigned long UserValue
	)
{
	 BT_DEVICE *pBtDevice = pBtModule->pBtDevice;

	 return pBtDevice->SetRfRegMaskBits(pBtDevice,Addr,Msb,Lsb,UserValue);
}
//------------------------------------------------------------------------------------------------------------------
