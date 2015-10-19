#include <stdio.h>

#include "bt_mp_transport.h"

#include <hardware/bluetoothmp.h>
#include "btif_api.h"




int bt_transport_SendHciCmd(
    BASE_INTERFACE_MODULE *pBaseInterface,
    unsigned char *pCmdBuffer,
    unsigned char bufferLen
	)
{
    unsigned short opcode;
    unsigned char paraLen = 0;
    unsigned char *pParaBuffer = NULL;

    opcode = *(unsigned short*)(pCmdBuffer);
    paraLen = *(unsigned char*)(pCmdBuffer + sizeof(opcode));
    
    pParaBuffer = pCmdBuffer +sizeof(opcode) + sizeof(paraLen);

    return btif_dut_mode_send(opcode, pParaBuffer, paraLen);    
}    


void bt_transport_signal_event(BASE_INTERFACE_MODULE *pBaseInterface, unsigned short event)
{

    pthread_mutex_lock(&pBaseInterface->mutex);
    pBaseInterface->rx_ready_events |= event;
    pthread_cond_signal(&pBaseInterface->cond);
    pthread_mutex_unlock(&pBaseInterface->mutex);
}

int bt_transport_RecvHciEvt(
    BASE_INTERFACE_MODULE *pBaseInterface,
    unsigned char *pEvtBuffer,
    unsigned char *pRetEvtLen
	)
{
    unsigned short events = 0;

    while(1)
    {
        pthread_mutex_lock(&pBaseInterface->mutex);
        while (pBaseInterface->rx_ready_events == 0)
        {
            pthread_cond_wait(&pBaseInterface->cond, &pBaseInterface->mutex);
        }

        events = pBaseInterface->rx_ready_events;
        pBaseInterface->rx_ready_events = 0;
        pthread_mutex_unlock(&pBaseInterface->mutex);    

        if(events & MP_TRANSPORT_EVENT_RX_HCIEVT)
        {
            *pRetEvtLen = pBaseInterface->evtLen;
            memcpy(pEvtBuffer, pBaseInterface->evtBuffer, pBaseInterface->evtLen);
            break;
        }
        else
        if(events & MP_TRANSPORT_EVENT_RX_EXIT)
        {
            break;
        }
        
    }
    return 0;
}    
