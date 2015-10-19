#include "bt_mp_device_base.h"

#ifndef BOOL 
typedef enum _BOOL{false, true}BOOL;
#endif

//  1 slot = 625u  
//  con-tx : 	DH1   625*1   625 u      1M/625 = 160;
//	     	DH3   625*3   1875u 	 1M/1875= 54;
//	     	DH5   625*5   3125u	 1M/3125= 32
//  pkt-tx :  packet --> packet There will be a slot interval.
//		DH1   625*2   1250u      80
//	     	DH3   625*4   2500u	 40
//	     	DH5   625*6   3750u      26.666 
#define PKTTX_SEC_PACKET_CNT_DH1   160	
#define PKTTX_SEC_PACKET_CNT_DH3   54
#define PKTTX_SEC_PACKET_CNT_DH5   32

#define CONTX_SEC_PACKET_CNT_DH1   80	
#define CONTX_SEC_PACKET_CNT_DH3   40
#define CONTX_SEC_PACKET_CNT_DH5   27

//#define MultiPktInterval	   1	//(n+1) * 100us
#define MULTIPKTINTERVAL	   0x01
//#define TxDummyBits		   16		//0~31
#define TXDUMMYBITS		   0x10	
//#define TxDummyPattern		   0		//0:all zero, 1: 1010 toggle
#define TXDUMMYPATTEN		   0	

#define MAX_RX_READ_COUNT_ADR_0X72	60000
#define MAX_RX_READ_ERRORBITS_ADR_0X78  60000

                                 //dh1     dh3     dh5    2dh1    2dh3     2dh5    3dh1    3dh3    3dh5   poll null
static unsigned char Arrary_Hopping_ptt[10]     ={0x00   ,0x00   ,0x00   ,0x01   ,0x01   ,0x01   ,0x01   ,0x01   ,0x01,0x00};
static unsigned char Arrary_Hopping_pkt_type[10]={0x04   ,0x0b   ,0x0f   ,0x04   ,0x0a   ,0x0e   ,0x08   ,0x0b   ,0x0f,0x01};
static unsigned int  Arrary_Hopping_pkt_len[10] ={27     ,183    ,339    ,54     ,367    ,679    ,83     ,552    ,1021,0   };
static unsigned char Arrary_Interval_slot_number[]={1,3,5,1,3,5,1,3,5,1};
static unsigned int  Arrary_PayloadLength[BT_PKT_TYPE_NUM]={
	
	216,1464,2712,		//1M
	432,2936,5432,          //2M   
	664,4416,8168,		//3M
	296,296,296		//LE
};
int BTDevice_SetPesudoOuterSetup(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam)
{
   int rtn=BT_FUNCTION_SUCCESS;

   /* disable modem fix tx */
   // RTK_UPDATE_MODEM_REG(TRANS_MODEM_REG(0x3C), BIT12, 0);
	if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x3c,12,12,0x0) !=BT_FUNCTION_SUCCESS)
	{
				rtn=FUNCTION_ERROR;
				goto exit;	 
	}

    /* enable pesudo outter mode */
   // RTK_UPDATE_MODEM_REG(TRANS_MODEM_REG(0x2E), BIT8, BIT8);
	if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2E,8,8,0x1) !=BT_FUNCTION_SUCCESS)
	{
				rtn=FUNCTION_ERROR;
				goto exit;	 
	}

    /* set payload type */

	if (pBtDevice->SetPayloadType(pBtDevice,pParam->mPayloadType)  !=BT_FUNCTION_SUCCESS)
	{
				rtn=FUNCTION_ERROR;
				goto exit;	 
	}	
    /* set rate and payload length */
    if (pBtDevice->SetPacketType(pBtDevice,pParam->mPacketType) !=BT_FUNCTION_SUCCESS)
    {
		rtn=FUNCTION_ERROR;
		goto exit;	 
    }     
    /* set packet header */
    if (pBtDevice->SetPackHeader(pBtDevice,pParam->mPacketHeader)!=BT_FUNCTION_SUCCESS)
    {
		rtn=FUNCTION_ERROR;
		goto exit;	 
    } 
    

    /* set syncword */
        //set target bd address
        rtn=pBtDevice->SetHitTarget(pBtDevice,pParam->mHitTarget);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        } 

    /* set whitenning setting */
 	        // set WhiteningCoeffEnable
        rtn=pBtDevice->SetWhiteningCoeffEnable(pBtDevice,pParam->mWhiteningCoeffEnable);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        }
exit:
	return rtn;

}
int BTDevice_SetPackHeader(BT_DEVICE *pBtDevice,unsigned int packHeader)
{
	int rtn=BT_FUNCTION_SUCCESS;
    	if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x30,15,0,packHeader &0xFFFF) != BT_FUNCTION_SUCCESS) 
    	{
        		rtn = FUNCTION_HCISEND_ERROR;
          		goto exit;
	}
    	if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x32,2,0,(packHeader>>16) &0x3) != BT_FUNCTION_SUCCESS) 
    	{
        		rtn = FUNCTION_HCISEND_ERROR;
          		goto exit;
	}		
exit:		
	return rtn;
}	
int BTDevice_SetResetMDCount(BT_DEVICE *pBtDevice)
{
	int rtn=BT_FUNCTION_SUCCESS;


    pBtDevice->TxTriggerPktCnt=0;
	pBtDevice->Inner_TotalRXBits=0;
	pBtDevice->Inner_TotalRxCounts=0;
	pBtDevice->Inner_TotalRxErrorBits=0;
	

	/* reset report counter */
    if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,11,9,0x00) != BT_FUNCTION_SUCCESS) 
    {
        		rtn = FUNCTION_HCISEND_ERROR;
          		goto exit;
	}
    if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,11,9,0x07) != BT_FUNCTION_SUCCESS) 
    {
        		rtn = FUNCTION_HCISEND_ERROR;
          		goto exit;
	}

    if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,1,1,0x00) != BT_FUNCTION_SUCCESS) 
    {
        		rtn = FUNCTION_HCISEND_ERROR;
          		goto exit;
	}
    if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,1,1,0x01) != BT_FUNCTION_SUCCESS) 
    {
        		rtn = FUNCTION_HCISEND_ERROR;
          		goto exit;
	}
    if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,1,1,0x00) != BT_FUNCTION_SUCCESS) 
    {
        		rtn = FUNCTION_HCISEND_ERROR;
          		goto exit;
	}
exit:
	
	return rtn;
}
#define CON_TX  0
#define PKT_TX  1


int BTDevice_GetPayloadLenTable(BT_DEVICE *pBtDevice,unsigned char  *pTable,int length)
{
	int rtn=BT_FUNCTION_SUCCESS;
	int n=0;
	if (length >BT_PKT_TYPE_NUM)
		length=BT_PKT_TYPE_NUM;
	for (n=0;n<length;n++)
	{
		pTable[n]=Arrary_PayloadLength[n];	
	}	
	return rtn;
}	
//-----------------------------------------------------------------------------------------------------
int BTDevice_SetPktRxStop(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport)
{

	int rtn=BT_FUNCTION_SUCCESS;
   	unsigned char pPayload[MAX_HCI_COMANND_BUF_SIZ];
   	unsigned char pEvent[MAX_HCI_EVENT_BUF_SIZ];
	unsigned char  EventLen = 0;    
   	unsigned int  OpCode=0x0000;
	int pktType=pParam->mPacketType;

	
	if (pBtDevice->TRXSTATE == TX_TIME_RUNING)
	{
  		rtn = FUNCTION_TX_RUNNING;
  		return rtn;		
	}
	pBtDevice->Inner_RX_First=0;
	if (pParam->mTestMode == BT_DUT_MODE)
	{
   		if (pktType == BT_PKT_LE)
   		{
   			OpCode=0x201F;
		}
		else
		{
			rtn = FUNCTION_NO_SUPPORT;	
			goto exit;
		}	
		if (OpCode >0x0000)
		{
     			if (pBtDevice->SendHciCommandWithEvent(pBtDevice,OpCode,0,pPayload,0x0E,pEvent, &EventLen) != BT_FUNCTION_SUCCESS)
 			{
 				rtn=FUNCTION_HCISEND_ERROR;
 				goto exit;
			}
 			if (pEvent[5] != 0x00)
 			{
 				rtn=FUNCTION_HCISEND_STAUTS_ERROR;
 				goto exit;
 			}		
		
		}			
	}
	else
	{
	      	//Back to Shut Down mode
        	if (pBtDevice->SetRfRegMaskBits(pBtDevice,0x00,15,0,0x0000) != BT_FUNCTION_SUCCESS) 
        	{
        		goto exit;
			}
			//Reset Modem
        	if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,1,1,0x00) != BT_FUNCTION_SUCCESS) 
        	{
          		goto exit;
			}
			//Disable multi-packet Tx

        	if ( pBtDevice->SetMutiRxEnable(pBtDevice,0x00) != BT_FUNCTION_SUCCESS) 
        	{
        		rtn = FUNCTION_HCISEND_ERROR;
          		goto exit;
			}
        	if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,8,8,0x00) != BT_FUNCTION_SUCCESS) 
        	{
        		rtn = FUNCTION_HCISEND_ERROR;
          		goto exit;
			}
        	if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,8,8,0x01) != BT_FUNCTION_SUCCESS) 
        	{
        		rtn = FUNCTION_HCISEND_ERROR;
          		goto exit;
			}			

	}			
	// stop process report
	if (pBtDevice->SetPktRxUpdate(pBtDevice,pParam,pBtReport) != BT_FUNCTION_SUCCESS)
	{
		//don't care report process
		rtn = BT_FUNCTION_SUCCESS;
	}	
exit:	
	pBtDevice->TRXSTATE = TRX_TIME_STOP;	
	return rtn;	
	
}
int BTDevice_SetPktRxBegin(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport)
{

	int rtn=BT_FUNCTION_SUCCESS;
    unsigned long btClockTime=0;
 	unsigned char pPayload[MAX_HCI_COMANND_BUF_SIZ];
   	unsigned char pEvent[MAX_HCI_EVENT_BUF_SIZ];
   	unsigned char EventLen = 0;
   	unsigned int  OpCode=0x0000;
   	int pktType=pParam->mPacketType;
   	unsigned char pPayload_Len=0;
	unsigned long tmp=0;
   	        
	BT_TRX_TIME   *pRxTime = &pBtDevice->TRxTime[RX_TIME_RUNING];	
	if (pBtDevice->TRXSTATE == TX_TIME_RUNING)
	{
  		rtn = FUNCTION_TX_RUNNING;
  		return rtn;		
	}
   	if (pParam->mPacketType == BT_PKT_LE)
   	{
   	 	if (pParam->mChannelNumber > 39)
   	 	{
   	 		rtn=FUNCTION_PARAMETER_INVALID_CHANNEL;
   	 		return rtn;
   		}
	}
	
	//Time clear  		
  	pRxTime->beginTimeClockCnt=0;
    	pRxTime->UseTimeClockCnt=0;
    	pRxTime->endTimeClockCnt=0;
        
//	pBtDevice->Inner_TotalRXBits=0;
//	pBtDevice->Inner_TotalRxCounts=0;
//	pBtDevice->Inner_TotalRxErrorBits=0;  
        pBtDevice->Inner_RX_MD_0X2E=0;
		pBtDevice->Inner_RX_First=0;
        if (pBtReport != NULL)
        {
		// RX report Clear  
			pBtReport->TotalRXBits=0;
			pBtReport->RXUpdateBits=0;
			pBtReport->RXPktUpdateCnts=0;
			pBtReport->TotalRxCounts=0;
			pBtReport->TotalRxErrorBits=0;
			pBtReport->IsRxRssi=-90;
        }
    //disable modem fix tx //
   // RTK_UPDATE_MODEM_REG(TRANS_MODEM_REG(0x3C), BIT12, 0);
	if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x3c,12,12,0x0) !=BT_FUNCTION_SUCCESS)
	{
				rtn=FUNCTION_ERROR;
				goto exit;	 
	}

    // enable pesudo outter mode //
   // RTK_UPDATE_MODEM_REG(TRANS_MODEM_REG(0x2E), BIT8, BIT8);
//	if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2E,8,8,0x1) !=BT_FUNCTION_SUCCESS)
//	{
//				rtn=FUNCTION_ERROR;
//				goto exit;	 
//	}

    // set payload type //

	if (pBtDevice->SetPayloadType(pBtDevice,pParam->mPayloadType)  !=BT_FUNCTION_SUCCESS)
	{
				rtn=FUNCTION_ERROR;
				goto exit;	 
	}	
    // set rate and payload length //
    if (pBtDevice->SetPacketType(pBtDevice,pParam->mPacketType) !=BT_FUNCTION_SUCCESS)
    {
		rtn=FUNCTION_ERROR;
		goto exit;	 
    }     
    // set packet header 
    if (pBtDevice->SetPackHeader(pBtDevice,pParam->mPacketHeader)!=BT_FUNCTION_SUCCESS)
    {
		rtn=FUNCTION_ERROR;
		goto exit;	 
    } 
    


        //set target bd address
        rtn=pBtDevice->SetHitTarget(pBtDevice,pParam->mHitTarget);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        } 

  	    // set WhiteningCoeffEnable
        rtn=pBtDevice->SetWhiteningCoeffEnable(pBtDevice,pParam->mWhiteningCoeffEnable);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        }		
      
  
        //set test mode
        rtn=pBtDevice->SetTestMode(pBtDevice,pParam->mTestMode);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        }		

	//	rtn=pBtDevice->SetPesudoOuterSetup(pBtDevice,pParam);
     //   if (rtn != BT_FUNCTION_SUCCESS)
    //    {
     //   	goto exit;
    //    }  
/*
        //set target bd address
        rtn=pBtDevice->SetHitTarget(pBtDevice,pParam->mHitTarget);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        } 
        // set WhiteningCoeffEnable
        rtn=pBtDevice->SetWhiteningCoeffEnable(pBtDevice,pParam->mWhiteningCoeffEnable);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        }   
	//set pkt type
        rtn=pBtDevice->SetPacketType(pBtDevice,pParam->mPacketType);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        }
*/		
        //set channel
        rtn=pBtDevice->SetRxChannel(pBtDevice,pParam->mChannelNumber);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        }
		//multi-packet Rx 
		rtn=pBtDevice->SetMutiRxEnable(pBtDevice,pParam->mMutiRxEnable) ;
		if (rtn != BT_FUNCTION_SUCCESS) 
			goto exit;
		

                
	if (pParam->mTestMode == BT_DUT_MODE)
	{
		if (pktType == BT_PKT_LE)
		{
			pPayload[0]=pParam->mChannelNumber;
			OpCode=0x201d;
			
		}
		if (OpCode > 0)
		{
			if (pBtDevice->SendHciCommandWithEvent(pBtDevice,OpCode,pPayload_Len,pPayload,0x0E,pEvent, &EventLen) != BT_FUNCTION_SUCCESS)
			{
				rtn=FUNCTION_HCISEND_ERROR;
			}
 			if (pEvent[5] != 0x00)
 			{
 				rtn=FUNCTION_HCISEND_STAUTS_ERROR;
 			}			
		}
		else
		{
			rtn = FUNCTION_NO_SUPPORT;
				
		}
		return rtn;	
	}
	else
	{
		//Back to Standby Mode from Shut Down mode
     //   if (pBtDevice->SetRfRegMaskBits(pBtDevice,0x00,15,0,0x1000) != BT_FUNCTION_SUCCESS) 
      //  {
     //   		goto exit;
	//	}

		
	}

        
	   if (pBtDevice->SetRestMDCount(pBtDevice) !=BT_FUNCTION_SUCCESS)
	   {
		   
			goto exit; 	
	   }	
	//get begin clock
        if (BTDevice_GetBTClockTime(pBtDevice,&btClockTime) ==BT_FUNCTION_SUCCESS)
        {
                pRxTime->beginTimeClockCnt=btClockTime;
        }
		if (pBtDevice->Inner_RX_MD_0X2E ==0)
        {
			if (pBtDevice->GetMdRegMaskBits(pBtDevice,0x2e,15,0,&tmp) !=BT_FUNCTION_SUCCESS)
			{
				rtn=FUNCTION_ERROR;
				goto exit;	 
			}
			if (tmp == 0)
			{
				rtn=FUNCTION_ERROR;
				goto exit;	 
			}

			pBtDevice->Inner_RX_MD_0X2E=tmp;
			
        }        

        pBtDevice->TRXSTATE = RX_TIME_RUNING;
        return rtn;         
exit:  
		pBtDevice->TRXSTATE = TRX_TIME_STOP;
        rtn = FUNCTION_ERROR;       	
	return rtn;	
	
}
int BTDevice_SetPktRxUpdate(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport)
{

	int rtn=BT_FUNCTION_SUCCESS;
    unsigned long btClockTime=0; 
    unsigned long rxCount=0; 
    unsigned long rxErrbits=0; 
    unsigned long rxPin=0;   
	unsigned long reg_0x7a=0; 
	unsigned long tmp=0;
    int pktType=pParam->mPacketType;
	int resetCountFlag=0;
	BT_TRX_TIME   *pRxTime = &pBtDevice->TRxTime[RX_TIME_RUNING];	
	
	if (pBtDevice->TRXSTATE == TX_TIME_RUNING)
	{
  		rtn = FUNCTION_TX_RUNNING;
  		return rtn;		
	}	
        if (BTDevice_GetBTClockTime(pBtDevice,&btClockTime) ==BT_FUNCTION_SUCCESS)
        {
                pRxTime->endTimeClockCnt=btClockTime;
                if (pRxTime->endTimeClockCnt > pRxTime->beginTimeClockCnt)
                        pRxTime->UseTimeClockCnt=pRxTime->endTimeClockCnt- pRxTime->beginTimeClockCnt;
                else
                        pRxTime->UseTimeClockCnt=pRxTime->beginTimeClockCnt- pRxTime->endTimeClockCnt;

        }
        if (pBtReport == NULL)
        {
			goto exit;
        }
		if (pBtDevice->SetRfRegMaskBits(pBtDevice,0x00,15,0,0x3000) != BT_FUNCTION_SUCCESS) 
        {
                goto exit;
		}
		if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,15,0,pBtDevice->Inner_RX_MD_0X2E|0x0002 ) != BT_FUNCTION_SUCCESS) 
        {
                goto exit;
		}


	//rx Pin
	if (pBtDevice->GetMdRegMaskBits(pBtDevice,0x70,15,10,&rxPin) != BT_FUNCTION_SUCCESS) 
    {
               	goto exit;
	} 

	pBtReport->IsRxRssi = (int) (rxPin);
	pBtReport->IsRxRssi = ((pBtReport->IsRxRssi >> 2) *2) -90;
	if (pBtReport->IsRxRssi > -88)
	{
		if (pBtDevice->Inner_RX_First ==0)
        {
			pBtDevice->Inner_RX_First=1;
			//reject first recv count ,to reset report 
            pBtDevice->SetRestMDCount(pBtDevice);
        }
        //rx Count	
		if (pBtDevice->GetMdRegMaskBits(pBtDevice,0x72,15,0,&rxCount) != BT_FUNCTION_SUCCESS) 
		{
               	goto exit;
		}
#ifdef DBG_REG_SETTING
	DBGPRINTF(">>>>[MD] REG_RX_PKT_Cnt=%x REG_RX_ERR_Bits=%x REG_RX_Pin=%d\n",rxCount,rxErrbits,rxPin);
#endif


		pBtReport->RXPktUpdateCnts=rxCount;
	
        //rx Bits
        if (rxCount >0)
        {

			pBtDevice->Inner_TotalRxCounts = rxCount;
        	pBtReport->TotalRxCounts = rxCount +pBtDevice->Inner_TotalRxCounts;

        	if (pktType <= BT_PKT_LE)
        	{
			//	if (pBtReport->IsRxPin > -90)
				{
        			pBtReport->RXUpdateBits = rxCount * Arrary_PayloadLength[pktType];

        			//pBtReport->TotalRXBits  += pBtReport->RXUpdateBits;
					  pBtReport->TotalRXBits  =  pBtDevice->Inner_TotalRXBits + pBtReport->RXUpdateBits;

				}
        	}	
        }
		
        //rx error Bit
		if (pBtDevice->GetMdRegMaskBits(pBtDevice,0x78,15,0,&rxErrbits) != BT_FUNCTION_SUCCESS) 
		{
               	goto exit;
		}   

		pBtReport->TotalRxErrorBits=pBtDevice->Inner_TotalRxErrorBits+rxErrbits;

		
		pBtReport->ber=( (float)(((float)(pBtReport->TotalRxErrorBits))/((float)(pBtReport->TotalRxErrorBits+pBtReport->TotalRXBits))));
	}
	else
	{
		resetCountFlag =0;
	}
	
	if ((rxCount >MAX_RX_READ_COUNT_ADR_0X72) || (rxErrbits >MAX_RX_READ_ERRORBITS_ADR_0X78) )
	{
		resetCountFlag=1;
	}// max bits process..
	else if( (pBtReport->TotalRXBits > 0x3fffffff) || (pBtReport->TotalRxErrorBits > 0x3fffffff) )
	{
		 rtn = FUNCTION_RX_MAXCOUNT;
		 pBtReport->TotalRXBits=0;
		 pBtReport->TotalRxErrorBits=0;
		 pBtDevice->Inner_TotalRxErrorBits=0;
		 pBtDevice->Inner_TotalRxCounts=0;
		 pBtReport->RXUpdateBits=0;
		 rxErrbits=0;
		 resetCountFlag=1;
		 rxCount=0;
		 rtn=BT_FUNCTION_SUCCESS;
		  
	}

	if (pBtDevice->GetMdRegMaskBits(pBtDevice,0x7a,1,0,&reg_0x7a) != BT_FUNCTION_SUCCESS) 
    {
               	goto exit;
	}
#ifdef DBG_REG_SETTING
	DBGPRINTF(">>>>[MD] REG_RX_PKT_Cnt=%x REG_RX_ERR_Bits=%x REG_RX_Pin=%d 0xREG_72=%x\n",rxCount,rxErrbits,rxPin,reg_0x7a);
#endif
		
	if (((reg_0x7a & 0x01) == 0x01) || (resetCountFlag == 1))
	{

		if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,10,10,0x00) != BT_FUNCTION_SUCCESS) 
        	{
               		goto exit;
		}
		if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,10,10,0x01) != BT_FUNCTION_SUCCESS) 
        	{
               		goto exit;
		}
		if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,9,9,0x00) != BT_FUNCTION_SUCCESS) 
        	{
               		goto exit;
		}
		if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,9,9,0x01) != BT_FUNCTION_SUCCESS) 
        	{
               		goto exit;
		}

		resetCountFlag=1;
	}
exit:	
	if (((reg_0x7a >> 1) &0x01) == 0)
	{
		//Back to Shut Down mode
		if (pBtDevice->SetRfRegMaskBits(pBtDevice,0x00,15,0,0x00) != BT_FUNCTION_SUCCESS) 
       {
               		goto exit;
		}

	}
/////////////////////////////////// debug
//	{
//		unsigned long tmp=0;
//	rtn=pBtDevice->SetRfRegMaskBits(pBtDevice,0x00,15,0,0x1000); 
//	if (rtn != BT_FUNCTION_SUCCESS)
//	{
//		goto exit;
//	}
//		if (pBtDevice->SetRfRegMaskBits(pBtDevice,0x3f,15,0,0x8A) != BT_FUNCTION_SUCCESS) 
   //    {
   //            		goto exit;
//}
#ifdef DBG_REG_SETTING		
	DBGPRINTF(">>>>[MD] REG_RX_PKT_Cnt=%x REG_RX_ERR_Bits=%x REG_RX_Pin=%d 0xREG_72=%x\n",rxCount,rxErrbits,rxPin,reg_0x7a);
	DBGPRINTF(">>>>[MD] REG_0x3f =%x \n",tmp);
#endif		
//	}
///////////////////////////////////////////////////
	  if (resetCountFlag)
	  {
		 pBtDevice->Inner_TotalRxCounts +=rxCount;
		 pBtDevice->Inner_TotalRXBits += pBtReport->RXUpdateBits;
		 pBtDevice->Inner_TotalRxErrorBits+=rxErrbits;
	  }	
      return rtn;		
	
}

//-----------------------------------------------------------------------------------------------------
int BTDevice_SetPktTxStop(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport)
{

	int rtn=BT_FUNCTION_SUCCESS;
   	unsigned char pPayload[MAX_HCI_COMANND_BUF_SIZ];
   	unsigned char pEvent[MAX_HCI_EVENT_BUF_SIZ];
   	unsigned char EventLen = 0;    
   	unsigned int  OpCode=0x0000;
	int pktType=pParam->mPacketType;
	if (pBtDevice->TRXSTATE == RX_TIME_RUNING)
	{
  		rtn = FUNCTION_RX_RUNNING;
  		return rtn;		
	}
	if (pParam->mTestMode == BT_DUT_MODE)
	{	
   		if (pktType == BT_PKT_LE)
   		{
   			OpCode=0x201F;
     		if (pBtDevice->SendHciCommandWithEvent(pBtDevice,OpCode,0,pPayload,0x0E,pEvent, &EventLen) != BT_FUNCTION_SUCCESS)
 			{
 				rtn=FUNCTION_HCISEND_ERROR;
 				goto exit;
			}
		}
		else
		{
			rtn = FUNCTION_NO_SUPPORT;	
			goto exit;
		}	
	}
	else
	{
		//disable multi-packet Tx
        if ( pBtDevice->SetMutiRxEnable(pBtDevice,0x00) != BT_FUNCTION_SUCCESS) 
        {
        		rtn = FUNCTION_HCISEND_ERROR;
          		goto exit;
		}
		//Back to Shut Down mode
       		if ( pBtDevice->SetRfRegMaskBits(pBtDevice,0x00,15,0,0x00) != BT_FUNCTION_SUCCESS) 
        	{
        		rtn = FUNCTION_HCISEND_ERROR;
          		goto exit;
		}
        	if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x04,15,0,pBtDevice->OldModemReg4Value) != BT_FUNCTION_SUCCESS) 
        	{
        		rtn = FUNCTION_HCISEND_ERROR;
          		goto exit;
		}							
	}


    if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,8,8,0x00) != BT_FUNCTION_SUCCESS) 
    {
          	goto exit;
	}
        if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,8,8,0x01) != BT_FUNCTION_SUCCESS) 
        {
          	goto exit;
	}		
	pBtDevice->TRXSTATE = TRX_TIME_STOP;

    if ((pBtReport != NULL) && (pParam !=NULL))
    {
         //report
        ///////////////////////////////////////////////////////////////////////////////////////////////
        BTDevice_CalculatedTxBits(pBtDevice,pParam,pBtReport,PKT_TX,&pBtReport->TXUpdateBits,&pBtReport->TXPktUpdateCnts);
        pBtReport->TotalTXBits += pBtReport->TXUpdateBits;
		pBtDevice->TxTriggerPktCnt += pBtReport->TXPktUpdateCnts;
		pBtReport->TotalTxCounts += pBtReport->TXPktUpdateCnts;
        //////////////////////////////////////////////////////////////////////////////////////////////	
	}	
exit:		
	return rtn;	
	
}
int BTDevice_SetPktTxBegin_PSEUDOMODE(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam)
{
	int rtn=BT_FUNCTION_SUCCESS;
	unsigned long NewModemReg4Value =0;

	 
	
	 /* disable continous tx mode */  
    if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,12,12,0x00) != BT_FUNCTION_SUCCESS) 
    {
          	goto exit;
	}
	/* save modem register 0x04 value */
    if ( pBtDevice->GetMdRegMaskBits(pBtDevice,0x04,15,0,&pBtDevice->OldModemReg4Value) != BT_FUNCTION_SUCCESS)
    {
          	goto exit;
	}
	/* set tx pkt count and interval */
	if ((pParam->mTxPacketCount >= 0xFFF) || (pParam->mTxPacketCount == 0))
	{
		NewModemReg4Value = 0xFFF;
	}
	else
	{
		NewModemReg4Value=pParam->mTxPacketCount;
	}		
	NewModemReg4Value =( NewModemReg4Value <<4 ) | (MULTIPKTINTERVAL&0x000F);
    if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x04,15,0,NewModemReg4Value) != BT_FUNCTION_SUCCESS)
    {
          	goto exit;
	}
	

	//Dummy Tx bits 
	//[a] switch page-2
   if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x00,15,0,0x02) != BT_FUNCTION_SUCCESS) 
    {
          	goto exit;
	}

    if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x3e,6,0, ((TXDUMMYPATTEN & 0x03)<<5)|(TXDUMMYBITS & 0x1F)) != BT_FUNCTION_SUCCESS) 
    {
          	goto exit;
	}		 
	
	//[b] switch page-0
    if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x00,15,0,0x00) != BT_FUNCTION_SUCCESS) 
    {
          	goto exit;
	}
	

	
	 //Reset Counter, Reset Report Count
	 if(pBtDevice->SetRestMDCount(pBtDevice) != BT_FUNCTION_SUCCESS) 
		 goto exit;

	/* generate neg-edge pulse to trigger */   

    if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,0,0,0x01) != BT_FUNCTION_SUCCESS) 
    {
          	goto exit;
	}
    if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,0,0,0x00) != BT_FUNCTION_SUCCESS) 
    {
          	goto exit;
	}
	 return rtn;				
exit:	
	rtn =FUNCTION_ERROR;
	
	return rtn;
}
	
int BTDevice_SetPktTxBegin_DUTMODE(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam)
{
	int rtn=BT_FUNCTION_SUCCESS;
 	unsigned char pPayload[MAX_HCI_COMANND_BUF_SIZ];
   	unsigned char pEvent[MAX_HCI_EVENT_BUF_SIZ];
	unsigned char  EventLen = 0;        
   	unsigned int  OpCode=0x0000;
   	int pktType=pParam->mPacketType;
   	unsigned char pPayload_Len=0;
	if (pBtDevice->TRXSTATE == RX_TIME_RUNING)
	{
  		rtn = FUNCTION_RX_RUNNING;
  		return rtn;		
	}

   	
   	pPayload[0]= pParam->mChannelNumber;
   	
   	if (pktType == BT_PKT_LE)
   	{
   		pPayload[1]=37;
   		pPayload[2]=pParam->mPayloadType;
   		pPayload_Len=3;
   		OpCode=0x201e;
   		
	}
	else 
	{
   		pPayload_Len=0;
   		OpCode=0xFD42;
	}
	
	
	if (OpCode>0)
	{
		if (pBtDevice->SendHciCommandWithEvent(pBtDevice,OpCode,pPayload_Len,pPayload,0x0E,pEvent, &EventLen) != BT_FUNCTION_SUCCESS)
		{
			rtn=FUNCTION_HCISEND_ERROR;
			goto exit;
		}
 		if (pEvent[5] != 0x00)
 		{
 			rtn=FUNCTION_HCISEND_STAUTS_ERROR;
 		}		
	}	

		
exit:	
	return rtn;	
			
}	
int BTDevice_SetPktTxBegin(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport)
{

	int rtn=BT_FUNCTION_SUCCESS;
        unsigned long btClockTime=0;
	unsigned long tmp=0;
	BT_TRX_TIME   *pTxTime = &pBtDevice->TRxTime[TX_TIME_RUNING];
	
	
	if (pBtDevice->TRXSTATE == RX_TIME_RUNING)
	{
  		rtn = FUNCTION_RX_RUNNING;
  		return rtn;		
	}
   	if (pParam->mPacketType == BT_PKT_LE)
   	{
   	 	if (pParam->mChannelNumber > 39)
   	 	{
   	 		rtn=FUNCTION_PARAMETER_INVALID_CHANNEL;
   	 		return rtn;
   		}
	} 
  		pTxTime->beginTimeClockCnt=0;
        pTxTime->UseTimeClockCnt=0;
        pTxTime->endTimeClockCnt=0;
     
        pBtDevice->TxTriggerPktCnt=0;
		pBtDevice->Inner_TX_MD_0X2E=0;
        
        if (pBtReport != NULL)
        {	
		pBtReport->TotalTXBits=0;
		pBtReport->TXUpdateBits=0;
		pBtReport->TotalTxCounts=0;
		pBtReport->TXPktUpdateCnts=0;        	
        }

        
	//	rtn=pBtDevice->SetPesudoOuterSetup(pBtDevice,pParam);
    //    if (rtn != BT_FUNCTION_SUCCESS)
    //    {
     //   	goto exit;
    //    }  
        
      //disable modem fix tx //
   // RTK_UPDATE_MODEM_REG(TRANS_MODEM_REG(0x3C), BIT12, 0);
	if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x3c,12,12,0x0) !=BT_FUNCTION_SUCCESS)
	{
				rtn=FUNCTION_ERROR;
				goto exit;	 
	}

    // enable pesudo outter mode //
   // RTK_UPDATE_MODEM_REG(TRANS_MODEM_REG(0x2E), BIT8, BIT8);
//	if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2E,8,8,0x1) !=BT_FUNCTION_SUCCESS)
//	{
//				rtn=FUNCTION_ERROR;
//				goto exit;	 
//	}

    // set payload type //

	if (pBtDevice->SetPayloadType(pBtDevice,pParam->mPayloadType)  !=BT_FUNCTION_SUCCESS)
	{
				rtn=FUNCTION_ERROR;
				goto exit;	 
	}	
    // set rate and payload length //
    if (pBtDevice->SetPacketType(pBtDevice,pParam->mPacketType) !=BT_FUNCTION_SUCCESS)
    {
		rtn=FUNCTION_ERROR;
		goto exit;	 
    }     
    // set packet header 
    if (pBtDevice->SetPackHeader(pBtDevice,pParam->mPacketHeader)!=BT_FUNCTION_SUCCESS)
    {
		rtn=FUNCTION_ERROR;
		goto exit;	 
    } 
    


        //set target bd address
        rtn=pBtDevice->SetHitTarget(pBtDevice,pParam->mHitTarget);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        } 

  	    // set WhiteningCoeffEnable
        rtn=pBtDevice->SetWhiteningCoeffEnable(pBtDevice,pParam->mWhiteningCoeffEnable);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        }		
      
  
        //set test mode
        rtn=pBtDevice->SetTestMode(pBtDevice,pParam->mTestMode);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        }

        //set channel
        rtn=pBtDevice->SetTxChannel(pBtDevice,pParam->mChannelNumber);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        }
        //set outpower 
		if (pParam->mTxGainIndex<=7)
			rtn=pBtDevice->SetPowerGainIndex(pBtDevice,pParam->mTxGainIndex);
		else
			rtn=pBtDevice->SetPowerGain(pBtDevice,pParam->mTxGainValue);
        
		if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
      	}          
        rtn=pBtDevice->SetMutiRxEnable(pBtDevice,pParam->mMutiRxEnable);
		if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
      	} 
        //begin con-tx setting
	if (pParam->mTestMode == BT_DUT_MODE)
	{
		if (pParam->mPacketType <= BT_PKT_LE)//1DH1~33DH5
		{
		 rtn= BTDevice_SetPktTxBegin_DUTMODE(pBtDevice,pParam);
		}
		else
		{
            //con-Tx don't support DUT MODE
			rtn= FUNCTION_NO_SUPPORT;
			goto exit;		
		}
	}
	else
	{
		if (pParam->mPacketType <= BT_PKT_LE)
		{
			if (BTDevice_SetPktTxBegin_PSEUDOMODE(pBtDevice,pParam) != BT_FUNCTION_SUCCESS)
			{
				goto exit;	
			}		
		}
		else
		{
			rtn= FUNCTION_NO_SUPPORT;
			goto exit;		
		}		
	}                       	
	//get begin clock
        if (BTDevice_GetBTClockTime(pBtDevice,&btClockTime) ==BT_FUNCTION_SUCCESS)
        {
                pTxTime->beginTimeClockCnt=btClockTime;
        }
		
		if (pBtDevice->Inner_TX_MD_0X2E ==0)
        {
			if (pBtDevice->GetMdRegMaskBits(pBtDevice,0x2e,15,0,&tmp) !=BT_FUNCTION_SUCCESS)
			{
				rtn=FUNCTION_ERROR;
				goto exit;	 
			}
			if (tmp == 0)
			{
				rtn=FUNCTION_ERROR;
				goto exit;	 
			}

			pBtDevice->Inner_TX_MD_0X2E=tmp;
			
        }
        pBtDevice->TRXSTATE = TX_TIME_RUNING;
		

        return rtn; 

exit:
        if (rtn == FUNCTION_NO_SUPPORT) 
        {
        	return rtn;
	}	
//	 rtn = FUNCTION_ERROR;
	//if Error to reset 
      // 	if (pBtDevice->SetContinueTxStop(pBtDevice,pParam,pBtReport) != BT_FUNCTION_SUCCESS)  
     //   {
      //  	rtn = FUNCTION_HCISEND_ERROR;	
      //    	return rtn;	
     //   }

	pBtDevice->TRXSTATE = TRX_TIME_STOP;
	
	return rtn;
	
}
int BTDevice_SetPktTxUpdate(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport)
{

	int rtn=BT_FUNCTION_SUCCESS;
	unsigned long NewModemReg4Value=0;
	unsigned long tmp=0;
	if (pBtDevice->TRXSTATE == RX_TIME_RUNING)
	{
  		rtn = FUNCTION_RX_RUNNING;
  		return rtn;		
	}
    if (pBtDevice->GetMdRegMaskBits(pBtDevice,0x2e,15,0,&tmp) != BT_FUNCTION_SUCCESS) 
    {
                goto exit;
	}
	if (tmp == 0)
    {
		if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,15,0,pBtDevice->Inner_TX_MD_0X2E |0x01) != BT_FUNCTION_SUCCESS) 
         {
                goto exit;
		}
		if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,15,0,pBtDevice->Inner_TX_MD_0X2E |0x00) != BT_FUNCTION_SUCCESS) 
         {
                goto exit;
		}

	}
         //report
        ///////////////////////////////////////////////////////////////////////////////////////////////
        BTDevice_CalculatedTxBits(pBtDevice,pParam,pBtReport,PKT_TX,&pBtReport->TXUpdateBits,&pBtReport->TXPktUpdateCnts);
        pBtReport->TotalTXBits += pBtReport->TXUpdateBits;
		pBtDevice->TxTriggerPktCnt += pBtReport->TXPktUpdateCnts;
		pBtReport->TotalTxCounts += pBtReport->TXPktUpdateCnts;	        
        //////////////////////////////////////////////////////////////////////////////////////////////	

	bt_mp_LogMsg("TxTriggerPktCnt= %ld (%ld)\n",pBtDevice->TxTriggerPktCnt ,pBtReport->TXPktUpdateCnts);
	if ((pBtReport->TotalTxCounts >= pParam->mTxPacketCount) && (pParam->mTxPacketCount !=0 ))
	{
		//stop
		rtn = pBtDevice->SetPktTxStop(pBtDevice,pParam,pBtReport);
		rtn = FUNCTION_TX_FINISH;
		return rtn;
	}
	if (pBtDevice->TxTriggerPktCnt >= 0xFFF) 
	{
	        pBtDevice->TxTriggerPktCnt=0;
	        
	        if (pParam->mTxPacketCount == 0)
	        {
	        	NewModemReg4Value = 0xFFF;		        	
	        }
	        else
	        {	
	
	        	tmp=pParam->mTxPacketCount- pBtReport->TotalTxCounts;
	        		
			if ( tmp >= 0xFFF)
			{
				NewModemReg4Value = 0xFFF;
			}
			else
			{
				NewModemReg4Value=tmp;
			}
		}	
		NewModemReg4Value =( NewModemReg4Value <<4 ) | (MULTIPKTINTERVAL&0x000F);
        if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x04,15,0,NewModemReg4Value) != BT_FUNCTION_SUCCESS)
        {
          		goto exit;
		}
		if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,11,9,0x00) != BT_FUNCTION_SUCCESS) 
         {
                goto exit;
		}
		if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,11,9,0x07) != BT_FUNCTION_SUCCESS) 
         {
                goto exit;
		}		
		if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,0,0,0x01) != BT_FUNCTION_SUCCESS) 
        	{
               		goto exit;
		}
		if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,0,0,0x00) != BT_FUNCTION_SUCCESS) 
        	{
               		goto exit;
		}
		

	}
        if ((pBtReport == NULL) || (pParam ==NULL))
	  goto exit;

         //report
        ///////////////////////////////////////////////////////////////////////////////////////////////
 //       BTDevice_CalculatedTxBits(pBtDevice,pParam,pBtReport,PKT_TX,&pBtReport->TXUpdateBits,&pBtReport->TXPktUpdateCnts);
    //    pBtReport->TotalTXBits += pBtReport->TXUpdateBits;
        //////////////////////////////////////////////////////////////////////////////////////////////
		return rtn;
exit:
	
	rtn=FUNCTION_ERROR;	
	if ((pBtReport->TotalTxCounts >=  pParam->mTxPacketCount) && (pParam->mTxPacketCount !=0) && (pParam->mTxPacketCount !=0xFFF))
	{
		rtn =FUNCTION_TX_FINISH; 
	}
	
	return rtn;	
	
}
//-----------------------------------------------------------------------------------------------------
int BTDevice_SetHoppingMode(BT_DEVICE *pBtDevice,BT_PKT_TYPE pktType)
{
	int rtn=BT_FUNCTION_SUCCESS;
   	unsigned char pEvent[MAX_HCI_EVENT_BUF_SIZ];
	unsigned char  EventLen = 0;        

     	unsigned char pPayLoad[]={0x00,0x04, 0x00          ,0x00   };
      	unsigned char *ptt	=Arrary_Hopping_ptt;
      	unsigned char *pkt_type	=Arrary_Hopping_pkt_type;
        unsigned int  *pkt_len	=Arrary_Hopping_pkt_len;
        int Index=0;


	switch (pktType)
	{
		case BT_PKT_1DH1:  
		case BT_PKT_1DH3:
		case BT_PKT_1DH5:
		case BT_PKT_2DH1:
		case BT_PKT_2DH3:
		case BT_PKT_2DH5:
		case BT_PKT_3DH1:
		case BT_PKT_3DH3:
		case BT_PKT_3DH5:  	Index=pktType;
		break;
		case BT_PKT_TYPE_NULL:
		    	Index=9;
                break;
		case BT_PKT_LE:
			Index=-1;	
		break;
	//////////////////////////////////////////////////
		default:
			rtn=FUNCTION_PARAMETER_ERROR;
		goto exit;
	}
	
	if (Index >=0)
	{
        	pPayLoad[0]= ptt[Index];
        	pPayLoad[1]= pkt_type[Index];
        	pPayLoad[2]= pkt_len[Index]&0xFF;
        	pPayLoad[3]= (pkt_len[Index]>>8)&0xFF;

 		if (pBtDevice->SendHciCommandWithEvent(pBtDevice,0xFD45,4,pPayLoad,0x0E,pEvent, &EventLen) != BT_FUNCTION_SUCCESS)
 		{
 			rtn=FUNCTION_HCISEND_ERROR;
 			goto exit;
		}
 		if (pEvent[5] != 0x00)
 		{
 			rtn=FUNCTION_HCISEND_STAUTS_ERROR;
 			goto exit;
 		}			
		pPayLoad[0]=0;
	}
	else
	{
		pPayLoad[0]=1;	
	}	
 	if (pBtDevice->SendHciCommandWithEvent(pBtDevice,0xFD45,1,pPayLoad,0x0E,pEvent, &EventLen) != BT_FUNCTION_SUCCESS)
 	{
 		rtn=FUNCTION_HCISEND_ERROR;
 		goto exit;
	}	
 	if (pEvent[5] != 0x00)
 		rtn=FUNCTION_HCISEND_STAUTS_ERROR;
 	
 exit:		
 	return rtn;
}

//-----------------------------------------------------------------------------------------------------
static int BTDevice_SetContinueTxBegin_PSEUDOMODE(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam)
{
         int rtn=BT_FUNCTION_SUCCESS;
         int trxState =pParam->mPacketType;
   	 //LE
   	 if (trxState == BT_PKT_LE)
   	 {
   	 	if (pParam->mChannelNumber > 39)
   	 	{
   	 		rtn=FUNCTION_PARAMETER_INVALID_CHANNEL;
   	 		goto exit;
   		}
         if(pBtDevice->SetMdRegMaskBits(pBtDevice,0x3c,5,5,0x01) != BT_FUNCTION_SUCCESS)
         {
         		goto exit;
		 }
	 }	
         //Continue Tx mode
     if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,12,12,0x01) != BT_FUNCTION_SUCCESS) 
     {
                goto exit;
	 }
	 //------------------------------------------------------
	 //Reset Counter, Reset Report Count
	 if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,11,9,0x00) != BT_FUNCTION_SUCCESS) 
         {
                goto exit;
	 }
	 if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,11,9,0x07) != BT_FUNCTION_SUCCESS) 
         {
                goto exit;
	 }
	 //Generate Negedge Pulse
	 if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,0,0,0x01) != BT_FUNCTION_SUCCESS) 
         {
                goto exit;
	 }
	 if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,0,0,0x00) != BT_FUNCTION_SUCCESS) 
     {
                goto exit;
	 }
         return rtn;
exit:
         rtn= FUNCTION_ERROR;
         return rtn;         
}
static int BTDevice_SetContinueTxBegin_DUTMODE(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam)
{
         return FUNCTION_NO_SUPPORT;
}
int BTDevice_SetContinueTxBegin(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport)
{
	int rtn=BT_FUNCTION_SUCCESS;
        unsigned long btClockTime=0;
	BT_TRX_TIME   *pTxTime = &pBtDevice->TRxTime[TX_TIME_RUNING];
	
	
	if (pBtDevice->TRXSTATE == RX_TIME_RUNING)
	{
  		rtn = FUNCTION_RX_RUNNING;
  		return rtn;		
	}

  		pTxTime->beginTimeClockCnt=0;
        pTxTime->UseTimeClockCnt=0;
        pTxTime->endTimeClockCnt=0;
        
        if (pBtReport != NULL)
        {	
			pBtReport->TotalTXBits=0;
			pBtReport->TXUpdateBits=0;
			pBtReport->TotalTxCounts=0;
			pBtReport->TXPktUpdateCnts=0;        	
        }
 //       rtn=pBtDevice->SetPesudoOuterSetup(pBtDevice,pParam);
  //      if (rtn != BT_FUNCTION_SUCCESS)
   //     {
   ////     	goto exit;
   //     }  
  //disable modem fix tx //
   // RTK_UPDATE_MODEM_REG(TRANS_MODEM_REG(0x3C), BIT12, 0);
	if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x3c,12,12,0x0) !=BT_FUNCTION_SUCCESS)
	{
				rtn=FUNCTION_ERROR;
				goto exit;	 
	}

    // enable pesudo outter mode //
   // RTK_UPDATE_MODEM_REG(TRANS_MODEM_REG(0x2E), BIT8, BIT8);
//	if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2E,8,8,0x1) !=BT_FUNCTION_SUCCESS)
//	{
//				rtn=FUNCTION_ERROR;
//				goto exit;	 
//	}

    // set payload type //

	if (pBtDevice->SetPayloadType(pBtDevice,pParam->mPayloadType)  !=BT_FUNCTION_SUCCESS)
	{
				rtn=FUNCTION_ERROR;
				goto exit;	 
	}	
    // set rate and payload length //
    if (pBtDevice->SetPacketType(pBtDevice,pParam->mPacketType) !=BT_FUNCTION_SUCCESS)
    {
		rtn=FUNCTION_ERROR;
		goto exit;	 
    }     
    // set packet header 
    if (pBtDevice->SetPackHeader(pBtDevice,pParam->mPacketHeader)!=BT_FUNCTION_SUCCESS)
    {
		rtn=FUNCTION_ERROR;
		goto exit;	 
    } 
    


        //set target bd address
        rtn=pBtDevice->SetHitTarget(pBtDevice,pParam->mHitTarget);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        } 

  	    // set WhiteningCoeffEnable
        rtn=pBtDevice->SetWhiteningCoeffEnable(pBtDevice,pParam->mWhiteningCoeffEnable);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        }		
      
  
        //set test mode
        rtn=pBtDevice->SetTestMode(pBtDevice,pParam->mTestMode);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        }
        
		

	     	 
        //set channel
        rtn=pBtDevice->SetTxChannel(pBtDevice,pParam->mChannelNumber);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
        }
        //set outpower 
		if (pParam->mTxGainIndex<=7)
			rtn=pBtDevice->SetPowerGainIndex(pBtDevice,pParam->mTxGainIndex);
		else
			rtn=pBtDevice->SetPowerGain(pBtDevice,pParam->mTxGainValue);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
      	}

       
		rtn=pBtDevice->SetMutiRxEnable(pBtDevice,0);
        if (rtn != BT_FUNCTION_SUCCESS)
        {
        	goto exit;
      	}

		
         
        
        //begin con-tx setting
	if (pParam->mTestMode == BT_DUT_MODE)
	{
		if (pParam->mPacketType <= BT_PKT_LE)//1DH1~33DH5
		{
			if (BTDevice_SetContinueTxBegin_DUTMODE(pBtDevice,pParam)!= BT_FUNCTION_SUCCESS)
			{
				goto exit;
			}	
		}
		else
		{
                        //con-Tx don't support DUT MODE
			rtn= FUNCTION_NO_SUPPORT;
			return 	rtn;	
		}
	}
	else
	{
		if (pParam->mPacketType <= BT_PKT_LE)
		{
			if (BTDevice_SetContinueTxBegin_PSEUDOMODE(pBtDevice,pParam)!=BT_FUNCTION_SUCCESS)
			{
				goto exit;
			}		
		}
		else
		{
			rtn= FUNCTION_NO_SUPPORT;
			return rtn;		
		}		
	}                       	
	//get begin clock
        if (BTDevice_GetBTClockTime(pBtDevice,&btClockTime) == BT_FUNCTION_SUCCESS)
        {
                pTxTime->beginTimeClockCnt=btClockTime;
        }
        pBtDevice->TRXSTATE = TX_TIME_RUNING;
        return rtn;
        
exit:
        if (rtn == FUNCTION_NO_SUPPORT)
        {
        	return rtn;
	}	
	rtn = FUNCTION_ERROR;
	//if Error to reset 
     //  	if (pBtDevice->SetContinueTxStop(pBtDevice,pParam,pBtReport) != BT_FUNCTION_SUCCESS)  
   //     {
   //     	rtn = FUNCTION_HCISEND_ERROR;
  //        	return rtn;
   //     }

	pBtDevice->TRXSTATE = TRX_TIME_STOP;
	
	return rtn;
}
//-----------------------------------------------------------------------------------------------------
static int BTDevice_SetContinueTxStop_PSEUDOMODE(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam)
{

	int rtn=BT_FUNCTION_SUCCESS;

	 //Back to Standby Mode from Shut Down mode
        if (pBtDevice->SetRfRegMaskBits(pBtDevice,0x00,15,0,0x0000) != BT_FUNCTION_SUCCESS) 
        {
        	goto exit;
	}
	//LE Con-Tx Disable
        if ( pBtDevice->SetMdRegMaskBits(pBtDevice,0x3c,5,5,0x00) != BT_FUNCTION_SUCCESS) 
        {
          	goto exit;
	}
        //Continue Tx mode disable
        if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,12,12,0x00) != BT_FUNCTION_SUCCESS) 
        {
          	goto exit;
	}
	//enable multi-packet Tx
        if (pBtDevice->SetMutiRxEnable(pBtDevice,0x00) != BT_FUNCTION_SUCCESS)  
        {
          	goto exit; 	
		}	

      return rtn;
 exit:
       rtn = FUNCTION_ERROR;
      //if Error to reset 
      if (pBtDevice->SetHciReset(pBtDevice,600) != BT_FUNCTION_SUCCESS)  
      {
      	  rtn = FUNCTION_HCISEND_ERROR;	
          return rtn;	
      }    
      return rtn;     
}
static int BTDevice_SetContinueTxStop_DUTMODE(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam)
{
	int rtn=BT_FUNCTION_SUCCESS;
	
   	unsigned char pPayload[MAX_HCI_COMANND_BUF_SIZ];
   	unsigned char pEvent[MAX_HCI_EVENT_BUF_SIZ];
	unsigned char  EventLen = 0;        
   	unsigned int  OpCode=0x0000;
   	int pktType =pParam->mPacketType;
   	
   	if (pktType == BT_PKT_LE)
   	{
   		OpCode=0x201F;
	}
	else if (pktType < BT_PKT_LE)
	{
		OpCode=0xFD43;	
	}
	else
	{
		rtn = FUNCTION_PARAMETER_ERROR;	
	}	
	if (OpCode >0x0000)
	{
     		if (pBtDevice->SendHciCommandWithEvent(pBtDevice,OpCode,0,pPayload,0x0E,pEvent, &EventLen) != BT_FUNCTION_SUCCESS)
 		{
 			rtn=FUNCTION_HCISEND_ERROR;
 			goto exit;
		}
 		if (pEvent[5] != 0x00)
 		{
 			rtn=FUNCTION_HCISEND_STAUTS_ERROR;
 			goto exit;
 		}		
		
	}
exit:	
	return rtn;
}
int BTDevice_CalculatedTxBits(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport,int pktTx_conTx,unsigned long  *txbits,unsigned long *txpkt_cnt)
{

        int rtn=BT_FUNCTION_SUCCESS;
        unsigned long  pkt_cnt=0;
	unsigned long  pkt_Interval_slot_number=0;
        unsigned long  pkt_Len=0;
	BT_PKT_TYPE PacketType=0;
	unsigned long use_uSec=0;
        unsigned long btClockTime=0;
        
	BT_TRX_TIME   *pTxTime = &pBtDevice->TRxTime[TX_TIME_RUNING];
	
        if (BTDevice_GetBTClockTime(pBtDevice,&btClockTime) ==BT_FUNCTION_SUCCESS)
        {
                pTxTime->endTimeClockCnt=btClockTime;
                if (pTxTime->endTimeClockCnt > pTxTime->beginTimeClockCnt)
                        pTxTime->UseTimeClockCnt=pTxTime->endTimeClockCnt- pTxTime->beginTimeClockCnt;
                else
                        pTxTime->UseTimeClockCnt=pTxTime->beginTimeClockCnt- pTxTime->endTimeClockCnt;

        }

	if ((pBtReport == NULL) || (pParam ==NULL))
                goto exit;
	  	
	PacketType = pParam->mPacketType;
	
        use_uSec = pTxTime->UseTimeClockCnt * 3125;  // 1 clock =312.5u
		
	if (pktTx_conTx == PKT_TX)
		pkt_Interval_slot_number=1 + Arrary_Interval_slot_number[PacketType];   //pkt-tx 
	else  if(pktTx_conTx == CON_TX)
		pkt_Interval_slot_number= Arrary_Interval_slot_number[PacketType];	//con-tx
        else
        {
                rtn = FUNCTION_PARAMETER_ERROR;
                goto exit;
	}
	pkt_cnt = use_uSec/ (pkt_Interval_slot_number * 6250);
        *txpkt_cnt= pkt_cnt;
	pkt_Len= Arrary_Hopping_pkt_len[PacketType];
	*txbits=pkt_cnt * pkt_Len *8;
exit:
	return rtn;
}			
int BTDevice_SetContinueTxStop(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport)
{

	int rtn=BT_FUNCTION_SUCCESS;
	if (pBtDevice->TRXSTATE == RX_TIME_RUNING)
	{
  		rtn = FUNCTION_RX_RUNNING;
  		return rtn;		
	}
	if (pParam->mTestMode == BT_DUT_MODE)
	{
		if (pParam->mPacketType <= BT_PKT_LE)//1DH1~3DH5,LE
		{
			if( BTDevice_SetContinueTxStop_DUTMODE(pBtDevice,pParam) != BT_FUNCTION_SUCCESS)
			{
				goto exit;
			}	
		}
		else
		{
			rtn= FUNCTION_NO_SUPPORT;
			goto exit;		
		}
	}
	else
	{
		if (pParam->mPacketType <= BT_PKT_LE)//1DH1~3DH5,LE
		{
			if(BTDevice_SetContinueTxStop_PSEUDOMODE(pBtDevice,pParam)!= BT_FUNCTION_SUCCESS)
			{
				goto exit;
			}	
		}
		else
		{
			rtn= FUNCTION_NO_SUPPORT;
			goto exit;		
		}		
	}		
	pBtDevice->TRXSTATE = TRX_TIME_STOP;

        if ((pBtReport != NULL) && (pParam !=NULL))
        {
         //report
        ///////////////////////////////////////////////////////////////////////////////////////////////
        	BTDevice_CalculatedTxBits(pBtDevice,pParam,pBtReport,PKT_TX,&pBtReport->TXUpdateBits,&pBtReport->TXPktUpdateCnts);
        	pBtReport->TotalTXBits += pBtReport->TXUpdateBits;
		pBtDevice->TxTriggerPktCnt += pBtReport->TXPktUpdateCnts;
		pBtReport->TotalTxCounts += pBtReport->TXPktUpdateCnts;
        //////////////////////////////////////////////////////////////////////////////////////////////	
}
	return rtn;
exit:
        if (rtn == FUNCTION_NO_SUPPORT) 
        {
        	return rtn;
	}	
	 rtn = FUNCTION_ERROR;
	//if Error to reset 
       	if (pBtDevice->SetContinueTxStop(pBtDevice,pParam,pBtReport) != BT_FUNCTION_SUCCESS)  
        {
        	rtn = FUNCTION_HCISEND_ERROR;	
          	return rtn;	
        }
	return rtn;	
	
}
//-----------------------------------------------------------------------------------------------------
int BTDevice_SetContinueTxUpdate(BT_DEVICE *pBtDevice,BT_PARAMETER *pParam,BT_DEVICE_REPORT *pBtReport)
{
	int rtn=BT_FUNCTION_SUCCESS;
	if (pBtDevice->TRXSTATE == RX_TIME_RUNING)
	{
  		rtn = FUNCTION_RX_RUNNING;
  		return rtn;		
	}	
        if ((pBtReport == NULL) || (pParam ==NULL))
	  goto exit;

         //report
        ///////////////////////////////////////////////////////////////////////////////////////////////
        BTDevice_CalculatedTxBits(pBtDevice,pParam,pBtReport,PKT_TX,&pBtReport->TXUpdateBits,&pBtReport->TXPktUpdateCnts);
        pBtReport->TotalTXBits += pBtReport->TXUpdateBits;
		pBtDevice->TxTriggerPktCnt += pBtReport->TXPktUpdateCnts;
		pBtReport->TotalTxCounts += pBtReport->TXPktUpdateCnts;	        
        //////////////////////////////////////////////////////////////////////////////////////////////	

exit:
	return rtn;
	
}	
//-----------------------------------------------------------------------------------------------------
int BTDevice_GetBTClockTime(
	BT_DEVICE *pBtDevice,
	unsigned long *btClockTime
	)
{
        int rtn=BT_FUNCTION_SUCCESS;
        int i=0;
        unsigned long Time=0;
   	unsigned char pPayload[MAX_HCI_COMANND_BUF_SIZ];
   	unsigned char pEvent[MAX_HCI_EVENT_BUF_SIZ];
	unsigned char  EventLen = 0;        
   	pPayload[0]=0x00;
   	pPayload[1]=0x00;
   	pPayload[2]=0x00;
     	if (pBtDevice->SendHciCommandWithEvent(pBtDevice,0x1407,3,pPayload,0x0E,pEvent, &EventLen) != BT_FUNCTION_SUCCESS)
 	{
 			rtn=FUNCTION_HCISEND_ERROR;
 			goto exit;
	}
 	if (pEvent[5] != 0x00)
 	{
 			rtn=FUNCTION_HCISEND_STAUTS_ERROR;
 	}
 	for (i=0;i<4;i++)
 	{
 		Time= pEvent[8+i]; 
 		
 		*btClockTime += (Time << (i*8));	
	}
exit:

        return rtn;
}
//-----------------------------------------------------------------------------------------------------
int BTDevice_SetMutiRxEnable(BT_DEVICE *pBtDevice,int IsMultiPktRx)
{
	int rtn=BT_FUNCTION_SUCCESS;
	if (IsMultiPktRx)
	{
		rtn = pBtDevice->SetMdRegMaskBits(pBtDevice,0x32,9,9,1);	
	}
	else
	{
		rtn = pBtDevice->SetMdRegMaskBits(pBtDevice,0x32,9,9,0);
			
	}
	
	
	return rtn;
}
//-----------------------------------------------------------------------------------------------------
int BTDevice_SetTxGainTable(	BT_DEVICE *pBtDevice,unsigned char  *pTable)
{
	int rtn=BT_FUNCTION_SUCCESS;
        int n=0;
        for (n=0;n<MAX_TXGAIN_TABLE_SIZE;n++)
        {
                pBtDevice->TXGainTable[n]=pTable[n];
        }
	return rtn; 
}
//-----------------------------------------------------------------------------------------------------
int BTDevice_SetTxDACTable(BT_DEVICE *pBtDevice,unsigned char  *pTable)
{
	int rtn=BT_FUNCTION_SUCCESS;
        int n=0;
        for (n=0;n<MAX_TXDAC_TABLE_SIZE;n++)
        {
                pBtDevice->TXDACTable[n]=pTable[n];
        }
	return rtn;
}
//-----------------------------------------------------------------------------------------------------
int BTDevice_SetWhiteningCoeffEnable(BT_DEVICE *pBtDevice,unsigned char WhiteningCoeffEnable)
{
	int rtn=BT_FUNCTION_SUCCESS;
	
	if (WhiteningCoeffEnable ==0)
	{
		rtn=pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,7,7,0);
	}
	else
	{
		rtn=pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,7,7,1); 
		if (rtn != BT_FUNCTION_SUCCESS)
			goto exit;

		rtn=pBtDevice->SetMdRegMaskBits(pBtDevice,0x32,8,2,WhiteningCoeffEnable);
	}
exit:
	
	return rtn; 
}
//-----------------------------------------------------------------------------------------------------
int BTDevice_SetPayloadType(BT_DEVICE *pBtDevice,BT_PAYLOAD_TYPE PayloadType)
{
	int rtn=BT_FUNCTION_SUCCESS;
	
	rtn=pBtDevice->SetMdRegMaskBits(pBtDevice,0x2e,6,4,PayloadType);
	
	 
	
	return rtn; 
}
//-----------------------------------------------------------------------------------------------------
int BTDevice_SetPacketType(BT_DEVICE *pBtDevice,BT_PKT_TYPE PktType)
{
	int rtn=BT_FUNCTION_SUCCESS;
	unsigned char PktBandWidth=0;
	unsigned long Payload_length=0;
	switch (PktType)
	{
		case BT_PKT_1DH1: PktBandWidth=1;  break;
		case BT_PKT_1DH3: PktBandWidth=1;  break;
		case BT_PKT_1DH5: PktBandWidth=1;  break;
		case BT_PKT_2DH1: PktBandWidth=2;  break;
		case BT_PKT_2DH3: PktBandWidth=2;  break;
		case BT_PKT_2DH5: PktBandWidth=2;  break;
		case BT_PKT_3DH1: PktBandWidth=3;  break;
		case BT_PKT_3DH3: PktBandWidth=3;  break;
		case BT_PKT_3DH5: PktBandWidth=3;  break;
		case BT_PKT_LE:   PktBandWidth=0;  break;
		default:
	 		rtn=FUNCTION_ERROR;		
	}
	if (rtn != FUNCTION_ERROR)
	{
		Payload_length=Arrary_PayloadLength[PktType];	
	}	
	if (PktBandWidth !=0)
	{

		rtn=pBtDevice->SetMdRegMaskBits(pBtDevice,0x2c,15,14,PktBandWidth); 
		if (rtn != BT_FUNCTION_SUCCESS)
		{
			 		goto exit;
		}

		rtn=pBtDevice->SetMdRegMaskBits(pBtDevice,0x2c,12,0,Payload_length); 
	}
	else
	{
		rtn=pBtDevice->SetMdRegMaskBits(pBtDevice,0x2c,12,0,Payload_length);	
	}		

exit:

	return rtn; 
}
//-----------------------------------------------------------------------------------------------------
int BTDevice_SetFWPowerTrackEnable(BT_DEVICE *pBtDevice,unsigned char FWPowerTrackEnable)
{
	int rtn=BT_FUNCTION_SUCCESS;
   	unsigned char pPayload[MAX_HCI_COMANND_BUF_SIZ];
   	unsigned char pEvent[MAX_HCI_EVENT_BUF_SIZ];
    	unsigned char  EventLen = 0;    
	switch (FWPowerTrackEnable)
	{
		case 0:		*(pPayload+0) = 0;  //disable
		case 0xFF:	*(pPayload+0) = 2;  //default vale
		default:	*(pPayload+0) = FWPowerTrackEnable;  //any value  	
		
 	}
 	
 	if (pBtDevice->SendHciCommandWithEvent(pBtDevice,0xFC69,1,pPayload,0x0E,pEvent, &EventLen) != BT_FUNCTION_SUCCESS)
 	{
 		rtn=FUNCTION_HCISEND_ERROR;
 		goto exit;
	}	
 	if (pEvent[5] != 0x00)
 		rtn=FUNCTION_HCISEND_STAUTS_ERROR;
exit: 	

 	return rtn;	

}
//-----------------------------------------------------------------------------------------------------
int BTDevice_SetHciReset(BT_DEVICE *pBtDevice,int Delay_mSec)
{
	int rtn=BT_FUNCTION_SUCCESS;
   	unsigned char pEvent[MAX_HCI_EVENT_BUF_SIZ];
    	unsigned char  EventLen = 0;    
   	unsigned char pPayload[1]={0};
   	
   	//TX/RX State = STOP
	pBtDevice->TRXSTATE = TRX_TIME_STOP;
	
	//Send reset command
 	if (pBtDevice->SendHciCommandWithEvent(pBtDevice,0x0C03,0,pPayload,0x0E,pEvent, &EventLen) != BT_FUNCTION_SUCCESS)
 	{
 			rtn=FUNCTION_HCISEND_ERROR;
 			goto exit;
	}
 	if (pEvent[5] != 0x00)
 	{
 			rtn=FUNCTION_HCISEND_STAUTS_ERROR;
 	}
 
exit:
	return rtn;
}
//-----------------------------------------------------------------------------------------------------	
int BTDevice_SetHitTarget(BT_DEVICE *pBtDevice,ULONG64 HitTarget)
{
	int rtn=BT_FUNCTION_SUCCESS;
	unsigned long pAccessCode[4];
        int i=0;
     //   BT_DEVICE *p1BtDevice=pBtDevice;
        for (i=0;i<4;i++)
             pAccessCode[i]=0;
             
	if (pBtDevice->HitTargetAccessCodeGen(pBtDevice,HitTarget,pAccessCode) != BT_FUNCTION_SUCCESS)
	{
		rtn=FUNCTION_ERROR;
		goto exit;
	}
//	DBGPRINTF(">>Write Modem 0x1c AccessCode[52:67]\n");
	if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x1c,15,0,pAccessCode[0]) != BT_FUNCTION_SUCCESS)
	{
		rtn=FUNCTION_ERROR;	
		goto exit;		
	}	
//	DBGPRINTF(">>Write Modem 0x1e AccessCode[36:51]  \n");
	if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x1e,15,0,pAccessCode[1]) != BT_FUNCTION_SUCCESS)
	{
		rtn=FUNCTION_ERROR;	
		goto exit;		
	}	
	
//	DBGPRINTF(">>Write Modem 0x20 AccessCode[20:35]\n");
	if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x20,15,0,pAccessCode[2]) != BT_FUNCTION_SUCCESS)
	{
		rtn=FUNCTION_ERROR;	
		goto exit;		
	}	
	
//	DBGPRINTF(">>Write Modem 0x22 AccessCode[4:19]\n");
	if (pBtDevice->SetMdRegMaskBits(pBtDevice,0x22,15,0,pAccessCode[3]) != BT_FUNCTION_SUCCESS)
	{
		rtn=FUNCTION_ERROR;	
		goto exit;		
	}		
 
exit:	
	
		
	return rtn;
}	
//-----------------------------------------------------------------------------------------------------
int BTBASE_GetPayLoadTypeValidFlag(BT_DEVICE *pBtDevice,BT_TEST_MODE TestMode,BT_PKT_TYPE PKT_TYPE,unsigned int *ValidFlag)
{
	int rtn=BT_FUNCTION_SUCCESS;
        int i=0,index=0;
        int *pFlag=NULL;
        int Arrary_PayloadType_PseudoOuterMODE_ValidFlag[3][BT_PAYLOAD_TYPE_NUM]=
        {

	{1,1,1,1,1,1,1,1},				//General
	{1,1,0,1,0,0,1,1},				//LE
	{1,1,1,1,1,0,0,0}				//RTL8723A

        };
         int Arrary_PayloadType_DUTMODE_ValidFlag[3][BT_PAYLOAD_TYPE_NUM]=
        {
	{1,1,1,0,0,0,1,0},				//General
	{1,1,0,1,0,0,1,1},				//LE
	{1,1,1,0,0,0,1,0}				//RTL8723A
	
        };
        if (PKT_TYPE == BT_PKT_LE)
                index=1;
        else if( PKT_TYPE== BT_PKT_TYPE_RTL8723A)
                index=2;
        else
                index=0;
	switch (TestMode)
	{
		case BT_DUT_MODE:
                        pFlag=Arrary_PayloadType_DUTMODE_ValidFlag[index];
		break;
		case BT_PSEUDO_MODE:
                        pFlag=Arrary_PayloadType_PseudoOuterMODE_ValidFlag[index];
		break;
		default:
		        rtn=FUNCTION_PARAMETER_ERROR;
		goto exit;		
	}
        if (ValidFlag != NULL)
        {
                for (i=0;i<BT_PAYLOAD_TYPE_NUM;i++)
                    ValidFlag[i]=pFlag[i];
        }
exit:	
	return rtn; 	
	
}
//-----------------------------------------------------------------------------------------------------
int BTBASE_HitTargetAccessCodeGen(BT_DEVICE *pBtDevice,ULONG64 HitTarget,unsigned long *pAccessCode)
{
	int rtn=BT_FUNCTION_SUCCESS;
#if 1
	BOOL AccessCode[72];
   	BOOL LC_PN_SEQ_MSB[32]   = {1,0,0,0,0,0,1,1,1,0,0,0,0,1,0,0,1,0,0,0,1,1,0,1,1,0,0,1,0,1,1,0};      //LC_PN_SEQ_MSB=0x83848d96
   	BOOL LC_GEN_POLY_MSB[32] = {0,1,1,0,0,0,0,1,0,1,0,1,1,1,0,0,0,1,0,0,1,1,1,1,0,1,1,0,1,0,1,0};      //LC_GEN_POLY_MSB=0x615c4f6a
   	BOOL LC_GEN_POLY_LSB[32] = {0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};      //LC_GEN_POLY_LSB=0x40000000
   	BOOL p33_p0[34]          = {1,0,1,0,1,1,1,0,1,1,1,1,0,0,1,1,0,0,0,1,0,1,0,1,0,0,1,1,1,1,1,1,0,0};  //PN_seq(P33~P0) = 0x2bbcc54fc
	BOOL BDAddress[48]; //6 byte  ,48bit
	BOOL LAP_INV[25], LAP_BK[31], barker_seq[33], temp[33], mod2[33], mod1[33];
    BOOL Parity[35], hold_mod2[33], hold_mod1[33], tmp;      //Bit0 is not valid

	int i=0, j=0;

        for (i=0;i<72;i++)
        {
                AccessCode[i]=0;
        }

	for (i=47;i>=0;i--)
	{
		BDAddress[i]=(int)((HitTarget>>i) &0x01) ;	
	}	

	//Extract LAP from BD Address
   	for (i=1; i<=24; i++)
    	{
      		LAP_INV[i]     =  *(BDAddress + i-1);
      		LAP_BK[i]      =  LAP_INV[i];
    	}
    	if (LAP_INV[24] == 0)
    	{
      		LAP_BK[25] =  0;
      		LAP_BK[26] =  0;
      		LAP_BK[27] =  1;
      		LAP_BK[28] =  1;
      		LAP_BK[29] =  0;
      		LAP_BK[30] =  1;
    	}
   	else
    	{
      		LAP_BK[25] =  1;
      		LAP_BK[26] =  1;
      		LAP_BK[27] =  0;
     		LAP_BK[28] =  0;
      		LAP_BK[29] =  1;
      		LAP_BK[30] =  0;
    	}
   	for (i=1; i<=30; i++)
   	{
    		barker_seq[i] = LAP_BK[31-i];
    	}	
   	barker_seq[31] = 0;
   	barker_seq[32] = 0;

   	for (i=1; i<=30; i++)
   	{
    		mod2[i] = barker_seq[i] ^ LC_PN_SEQ_MSB[i-1];
	}
   	mod2[31] = 0;
   	mod2[32] = 0;

   	for (i=1; i<=30; i++)
    	{
    		mod1[i] = 0;
	}
	
   	for (i=1; i<=30; i++)
    	{
       		if (mod2[1] == 1)
        	{
          		if (mod1[1] == 1)
           		{
             			for (j=1; j<=31; j++)  //shift
              				mod2[j] = mod2[j+1];
             			mod2[32] = 1;
           		}
          		else
           		{
            			for (j=1; j<=31; j++)  //shift
              				mod2[j] = mod2[j+1];

             			mod2[32] = 0;
           		}//end if (mod1[1] == 1)

           		for (j=1; j<=31; j++)
            			mod1[j] = mod1[j+1];

           		mod1[32] = 0;

           		for (j=1; j<=32; j++)
            		{
              			mod2[j] = mod2[j] ^ LC_GEN_POLY_MSB[j-1];
              			mod1[j] = mod1[j] ^ LC_GEN_POLY_LSB[j-1];
            		}//end for (j=1; j<=32; j++)
        	}
       		else //if (mod2[1] == 1) else 
        	{
          		if (mod1[1] == 1)
           		{
             			for (j=1; j<=31; j++)  //shift
             				mod2[j] = mod2[j+1];

             			mod2[32] = 1;
           		}
          		else
           		{
            			for (j=1; j<=31; j++)  //shift
              				mod2[j] = mod2[j+1];

             			mod2[32] = 0;
           		}//end (mod1[1] == 1)

           		for (j=1; j<=31; j++)
            			mod1[j] = mod1[j+1];

           		mod1[32] = 0;
        	}//end if (mod2[1] == 1)
    	} //end for (i=1; i<=30; i++)

   	for (i=1; i<=30; i++)
    		hold_mod2[i] = mod2[i+2];

  	hold_mod2[31] = 0;
  	hold_mod2[32] = 0;

  	for (i=1; i<=30; i++)
    		hold_mod1[i] = 0;

  	hold_mod1[31] = mod1[1];
  	hold_mod1[32] = mod1[2];

  	for (i=1; i<=32; i++)
   	{
     		temp[i] = hold_mod2[i] | hold_mod1[i];
     		Parity[i+2] = temp[i];
  	}

  	Parity[1] = mod2[1];
  	Parity[2] = mod2[2];

	//assign AccessCode
  	for (i=1; i<=34; i++)
   	{
     		tmp = Parity[35-i] ^ p33_p0[34-i];
     		AccessCode[i+3] = tmp;
   	}

  	for (i=38; i<=67; i++)
   		AccessCode[i] = LAP_BK[i-37];


   	if (AccessCode[4] == 1)
    	{
      		AccessCode[0] = 0;
      		AccessCode[1] = 1;
      		AccessCode[2] = 0;
      		AccessCode[3] = 1;
      		AccessCode[4] = 0;
    	}
   	else
    	{
      		AccessCode[0] = 1;
      		AccessCode[1] = 0;
      		AccessCode[2] = 1;
      		AccessCode[3] = 0;
      		AccessCode[4] = 1;
    	} //end if (AccessCode[4] == 1)

   	if (AccessCode[67] == 1)
    	{
      		AccessCode[68] = 0;
      		AccessCode[69] = 1;
      		AccessCode[70] = 0;
      		AccessCode[71] = 1;
    	}
   	else
    	{
      		AccessCode[68] = 1;
      		AccessCode[69] = 0;
      		AccessCode[70] = 1;
      		AccessCode[71] = 0;
    	}   //end  if (AccessCode[67] == 1)

 	for (i=0;i<4;i++)
 	{
 		for (j=0;j<16;j++)
 		{
 			if (AccessCode[67-((i*16)+j)] == 0)
 				pAccessCode[i]=pAccessCode[i]|(0x0<<j);
 			else	
 				pAccessCode[i]=pAccessCode[i]|(0x1<<j);
 		}		
 	
	}

#else    		   		
	
#endif		
	return rtn;
}
//----------------------------------------------------------------------------------------------------
 
int
BTDevice_SendHciCommandWithEvent(
	BT_DEVICE *pBtDevice,
	unsigned int  OpCode,
	unsigned char PayLoadLength,
	unsigned char *pPayLoad,
	unsigned char  EventType,
       unsigned char  *pEvent,
	unsigned char  *pEventLen

	)
{
 	unsigned long len;
	unsigned char pWritingBuf[MAX_HCI_COMANND_BUF_SIZ];
    unsigned long Retlen =0;
    int n=0;
    unsigned char hci_rtn=0;

 	len = PayLoadLength +3;
	pWritingBuf[0x00] = OpCode & 0xFF;
	pWritingBuf[0x01] = (OpCode >> 0x08) & 0xFF;
	pWritingBuf[0x02] = PayLoadLength;
    for (n=0;n<PayLoadLength;n++)
    {
           pWritingBuf[0x03+n]=pPayLoad[n];
    }

 	if (pBtDevice->SendHciCmd(pBtDevice, HCIIO_BTCMD, pWritingBuf, len) != BT_FUNCTION_SUCCESS)
        {		         
               goto exit;
        }
	if (OpCode==0xFC20)
	{
		pEvent[5]=0x00;
		return BT_FUNCTION_SUCCESS;	
	}
	if (pBtDevice->RecvHciEvent(pBtDevice, HCIIO_BTEVT, pEvent, pEventLen) != BT_FUNCTION_SUCCESS)
        {
                goto exit;
        }
        switch (EventType)
        {
                case 0x0e:
                        hci_rtn =pEvent[5];
                break;
                case 0x0F:
                break;
                default:
                break;
        }

        if (hci_rtn != 0x00)
        {
                 goto exit;
        }

        return BT_FUNCTION_SUCCESS;
exit:
        return FUNCTION_ERROR;
}
//-----------------------------------------------------------------------------------------------------
int
BTDevice_RecvAnyHciEvent(
	BT_DEVICE *pBtDevice,
	unsigned char  *pEvent
	)
{

        unsigned long Retlen =0;
	if (pBtDevice->RecvHciEvent(pBtDevice, HCIIO_BTEVT, pEvent, &Retlen) != BT_FUNCTION_SUCCESS)
        {
                goto exit;
        }
        return BT_FUNCTION_SUCCESS;
exit:
        return FUNCTION_ERROR;

}
//-----------------------------------------------------------------------------------------------------
