/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  This file contains functions that interface with the HCI transport. On
 *  the receive side, it routes events to the appropriate handler, e.g.
 *  L2CAP, ScoMgr. On the transmit side, it manages the command
 *  transmission.
 *
 ******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "gki.h"
#include "bt_types.h"
#include "hcimsgs.h"
#include "btu.h"
#include <utils/Log.h>



//Counter to track number of HCI command timeout
static int num_hci_cmds_timed_out;


static void btu_hcif_command_complete_evt (UINT8 controller_id, UINT8 *p, UINT16 evt_len);
static void btu_hcif_command_status_evt (UINT8 controller_id, UINT8 *p, UINT16 evt_len);
void btu_hcif_mp_test_event (UINT8 controller_id, BT_HDR *p_msg);

/*******************************************************************************
**
** Function         btu_hcif_store_cmd
**
** Description      This function stores a copy of an outgoing command and
**                  and sets a timer waiting for a event in response to the
**                  command.
**
** Returns          void
**
*******************************************************************************/
static void btu_hcif_store_cmd (UINT8 controller_id, BT_HDR *p_buf)
{
    tHCI_CMD_CB * p_hci_cmd_cb = &(btu_cb.hci_cmd_cb[controller_id]);
    UINT16  opcode;
    BT_HDR  *p_cmd;
    UINT8   *p = (UINT8 *)(p_buf + 1) + p_buf->offset;

    /* get command opcode */
    STREAM_TO_UINT16 (opcode, p);

    /* don't do anything for certain commands */
    if ((opcode == HCI_RESET) || (opcode == HCI_HOST_NUM_PACKETS_DONE))
    {
        return;
    }

    /* allocate buffer (HCI_GET_CMD_BUF will either get a buffer from HCI_CMD_POOL or from 'best-fit' pool) */
    if ((p_cmd = HCI_GET_CMD_BUF(p_buf->len + p_buf->offset - HCIC_PREAMBLE_SIZE)) == NULL)
    {
        return;
    }

    /* copy buffer */
    memcpy (p_cmd, p_buf, sizeof(BT_HDR));

    /* If vendor specific save the callback function */
    if ((opcode & HCI_GRP_VENDOR_SPECIFIC) == HCI_GRP_VENDOR_SPECIFIC
#if BLE_INCLUDED == TRUE
        || (opcode == HCI_BLE_RAND )
        || (opcode == HCI_BLE_ENCRYPT)
#endif
       )
    {
#if 0
        BT_TRACE_2 (TRACE_LAYER_HCI, TRACE_TYPE_DEBUG,
                    "Storing VSC callback opcode=0x%04x, Callback function=0x%07x",
                    opcode, *(UINT32 *)(p_buf + 1));
#endif
        memcpy ((UINT8 *)(p_cmd + 1), (UINT8 *)(p_buf + 1), sizeof(void *));
    }

    memcpy ((UINT8 *)(p_cmd + 1) + p_cmd->offset,
            (UINT8 *)(p_buf + 1) + p_buf->offset, p_buf->len);

    /* queue copy of cmd */
    GKI_enqueue(&(p_hci_cmd_cb->cmd_cmpl_q), p_cmd);

    /* start timer */
    if (BTU_CMD_CMPL_TIMEOUT > 0)
    {
#if (defined(BTU_CMD_CMPL_TOUT_DOUBLE_CHECK) && BTU_CMD_CMPL_TOUT_DOUBLE_CHECK == TRUE)
        p_hci_cmd_cb->checked_hcisu = FALSE;
#endif
        btu_start_timer (&(p_hci_cmd_cb->cmd_cmpl_timer),
                         (UINT16)(BTU_TTYPE_BTU_CMD_CMPL + controller_id),
                         BTU_CMD_CMPL_TIMEOUT);
    }
}

static void send_recv_evt_message(short evt, UINT8 evt_opcode, UINT8 *p, UINT16 evt_len)
{
    BT_HDR *p_msg;
    UINT8 *pp;

    if ((p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE + evt_len + sizeof(UINT8) + sizeof(UINT8))) != NULL)
    {
        p_msg->event = evt;
        p_msg->offset = 0;
        pp = (UINT8 *)(p_msg + 1);
        UINT8_TO_STREAM (pp, evt_opcode);
        UINT8_TO_STREAM (pp, evt_len);
        if(evt_len > 0)
            ARRAY_TO_STREAM(pp, p, evt_len);
        GKI_send_msg (BTIF_TASK, BTU_BTIF_MBOX, p_msg);
    }
}



/*******************************************************************************
**
** Function         btu_hcif_process_event
**
** Description      This function is called when an event is received from
**                  the Host Controller.
**
** Returns          void
**
*******************************************************************************/


void btu_hcif_process_event (UINT8 controller_id, BT_HDR *p_msg)
{
    UINT8   *p = (UINT8 *)(p_msg + 1) + p_msg->offset;
    UINT8   hci_evt_code, hci_evt_len;
#if BLE_INCLUDED == TRUE
    UINT8   ble_sub_code;
#endif
    STREAM_TO_UINT8  (hci_evt_code, p);
    STREAM_TO_UINT8  (hci_evt_len, p);

    switch (hci_evt_code)
    {
        case HCI_COMMAND_COMPLETE_EVT:
            btu_hcif_command_complete_evt (controller_id, p, hci_evt_len);
            btu_hcif_mp_test_event(controller_id, p_msg);
            break;
        case HCI_COMMAND_STATUS_EVT:
            btu_hcif_command_status_evt (controller_id, p, hci_evt_len);
            btu_hcif_mp_test_event(controller_id, p_msg);
            break;

    }

}


/*******************************************************************************
**
** Function         btu_hcif_command_complete_evt
**
** Description      Process event HCI_COMMAND_COMPLETE_EVT
**
** Returns          void
**
*******************************************************************************/
static void btu_hcif_command_complete_evt (UINT8 controller_id, UINT8 *p, UINT16 evt_len)
{
    tHCI_CMD_CB *p_hci_cmd_cb = &(btu_cb.hci_cmd_cb[controller_id]);
    UINT16      cc_opcode;
    BT_HDR      *p_cmd;
    void        *p_cplt_cback = NULL;

    STREAM_TO_UINT8  (p_hci_cmd_cb->cmd_window, p);

#if (defined(HCI_MAX_SIMUL_CMDS) && (HCI_MAX_SIMUL_CMDS > 0))
    if (p_hci_cmd_cb->cmd_window > HCI_MAX_SIMUL_CMDS)
        p_hci_cmd_cb->cmd_window = HCI_MAX_SIMUL_CMDS;
#endif

    STREAM_TO_UINT16 (cc_opcode, p);

    evt_len -= 3;

    /* only do this for certain commands */
    if ((cc_opcode != HCI_RESET) && (cc_opcode != HCI_HOST_NUM_PACKETS_DONE) &&
        (cc_opcode != HCI_COMMAND_NONE))
    {
        /* dequeue and free stored command */

/* always use cmd code check, when one cmd timeout waiting for cmd_cmpl,
   it'll cause the rest of the command goes in wrong order                  */
        p_cmd = (BT_HDR *) GKI_getfirst (&p_hci_cmd_cb->cmd_cmpl_q);

        while (p_cmd)
        {
            UINT16 opcode_dequeued;
            UINT8  *p_dequeued;

            /* Make sure dequeued command is for the command_cplt received */
            p_dequeued = (UINT8 *)(p_cmd + 1) + p_cmd->offset;
            STREAM_TO_UINT16 (opcode_dequeued, p_dequeued);

            if (opcode_dequeued != cc_opcode)
            {
                /* opcode does not match, check next command in the queue */
                p_cmd = (BT_HDR *) GKI_getnext(p_cmd);
                continue;
            }
            GKI_remove_from_queue(&p_hci_cmd_cb->cmd_cmpl_q, p_cmd);

            /* If command was a VSC, then extract command_complete callback */
            if ((cc_opcode & HCI_GRP_VENDOR_SPECIFIC) == HCI_GRP_VENDOR_SPECIFIC
#if BLE_INCLUDED == TRUE
                || (cc_opcode == HCI_BLE_RAND )
                || (cc_opcode == HCI_BLE_ENCRYPT)
#endif
               )
            {
                p_cplt_cback = *((void **)(p_cmd + 1));
            }

            GKI_freebuf (p_cmd);

            break;
        }

        /* if more commands in queue restart timer */
        if (BTU_CMD_CMPL_TIMEOUT > 0)
        {
            if (!GKI_queue_is_empty (&(p_hci_cmd_cb->cmd_cmpl_q)))
            {
#if (defined(BTU_CMD_CMPL_TOUT_DOUBLE_CHECK) && BTU_CMD_CMPL_TOUT_DOUBLE_CHECK == TRUE)
                p_hci_cmd_cb->checked_hcisu = FALSE;
#endif
                btu_start_timer (&(p_hci_cmd_cb->cmd_cmpl_timer),
                                 (UINT16)(BTU_TTYPE_BTU_CMD_CMPL + controller_id),
                                 BTU_CMD_CMPL_TIMEOUT);
            }
            else
            {
                btu_stop_timer (&(p_hci_cmd_cb->cmd_cmpl_timer));
            }
        }
    }

    /* handle event */


    /* see if we can send more commands */
    btu_hcif_send_cmd (controller_id, NULL);
}

/*******************************************************************************
**
** Function         btu_hcif_command_status_evt
**
** Description      Process event HCI_COMMAND_STATUS_EVT
**
** Returns          void
**
*******************************************************************************/
static void btu_hcif_command_status_evt (UINT8 controller_id, UINT8 *p, UINT16 evt_len)
{
    tHCI_CMD_CB * p_hci_cmd_cb = &(btu_cb.hci_cmd_cb[controller_id]);
    UINT8       status;
    UINT16      opcode;
    UINT16      cmd_opcode;
    BT_HDR      *p_cmd = NULL;
    UINT8       *p_data = NULL;
    void        *p_vsc_status_cback = NULL;

    STREAM_TO_UINT8  (status, p);
    STREAM_TO_UINT8  (p_hci_cmd_cb->cmd_window, p);

#if (defined(HCI_MAX_SIMUL_CMDS) && (HCI_MAX_SIMUL_CMDS > 0))
    if (p_hci_cmd_cb->cmd_window > HCI_MAX_SIMUL_CMDS)
        p_hci_cmd_cb->cmd_window = HCI_MAX_SIMUL_CMDS;
#endif

    STREAM_TO_UINT16 (opcode, p);

    /* only do this for certain commands */
    if ((opcode != HCI_RESET) && (opcode != HCI_HOST_NUM_PACKETS_DONE) &&
        (opcode != HCI_COMMAND_NONE))
    {
        /*look for corresponding command in cmd_queue*/
        p_cmd = (BT_HDR *) GKI_getfirst(&(p_hci_cmd_cb->cmd_cmpl_q));
        while (p_cmd)
        {
            p_data = (UINT8 *)(p_cmd + 1) + p_cmd->offset;
            STREAM_TO_UINT16 (cmd_opcode, p_data);

            /* Make sure this  command is for the command_status received */
            if (cmd_opcode != opcode)
            {
                /* opcode does not match, check next command in the queue */
                p_cmd = (BT_HDR *) GKI_getnext(p_cmd);
                continue;
            }
            else
            {
                GKI_remove_from_queue(&p_hci_cmd_cb->cmd_cmpl_q, p_cmd);

                /* If command was a VSC, then extract command_status callback */
                 if ((cmd_opcode & HCI_GRP_VENDOR_SPECIFIC) == HCI_GRP_VENDOR_SPECIFIC)
                {
                    p_vsc_status_cback = *((void **)(p_cmd + 1));
                }
                break;
            }
        }

        /* if more commands in queue restart timer */
        if (BTU_CMD_CMPL_TIMEOUT > 0)
        {
            if (!GKI_queue_is_empty (&(p_hci_cmd_cb->cmd_cmpl_q)))
            {
#if (defined(BTU_CMD_CMPL_TOUT_DOUBLE_CHECK) && BTU_CMD_CMPL_TOUT_DOUBLE_CHECK == TRUE)
                p_hci_cmd_cb->checked_hcisu = FALSE;
#endif
                btu_start_timer (&(p_hci_cmd_cb->cmd_cmpl_timer),
                                 (UINT16)(BTU_TTYPE_BTU_CMD_CMPL + controller_id),
                                 BTU_CMD_CMPL_TIMEOUT);
            }
            else
            {
                btu_stop_timer (&(p_hci_cmd_cb->cmd_cmpl_timer));
            }
        }
    }

    /* handle command */
   

    /* free stored command */
    if (p_cmd != NULL)
    {
        GKI_freebuf (p_cmd);
    }
    else
    {
        BT_TRACE_1 (TRACE_LAYER_HCI, TRACE_TYPE_WARNING,
                    "No command in queue matching opcode %d", opcode);
    }

    /* See if we can forward any more commands */
    btu_hcif_send_cmd (controller_id, NULL);
}


void btu_hcif_mp_test_event (UINT8 controller_id, BT_HDR *p_msg)
{
    UINT8   *p = (UINT8 *)(p_msg + 1) + p_msg->offset;
    UINT8   hci_evt_code, hci_evt_len;

    STREAM_TO_UINT8  (hci_evt_code, p);
    STREAM_TO_UINT8  (hci_evt_len, p);


    send_recv_evt_message(BT_EVT_RX, hci_evt_code, p, hci_evt_len);

    num_hci_cmds_timed_out = 0;
}


void btu_hcif_mp_notify_event (BT_HDR *p_msg)
{
    UINT8   *p = (UINT8 *)(p_msg + 1) + p_msg->offset;
    UINT8   mp_op_code;
    UINT8   mp_op_buffer_len;

    STREAM_TO_UINT8  (mp_op_code, p);
    STREAM_TO_UINT8  (mp_op_buffer_len, p);

    send_recv_evt_message(BT_EVT_MP_NOTIFY_BTIF, mp_op_code, p, mp_op_buffer_len);

}





/*******************************************************************************
**
** Function         btu_hcif_send_cmd
**
** Description      This function is called to check if it can send commands
**                  to the Host Controller. It may be passed the address of
**                  a packet to send.
**
** Returns          void
**
*******************************************************************************/
void btu_hcif_send_cmd (UINT8 controller_id, BT_HDR *p_buf)
{
    tHCI_CMD_CB * p_hci_cmd_cb = &(btu_cb.hci_cmd_cb[controller_id]);

#if ((L2CAP_HOST_FLOW_CTRL == TRUE)||defined(HCI_TESTER))
    UINT8 *pp;
    UINT16 code;
#endif
    ALOGE("btu_hcif_send_cmd++");
    /* If there are already commands in the queue, then enqueue this command */
    if ((p_buf) && (p_hci_cmd_cb->cmd_xmit_q.count))
    {
        GKI_enqueue (&(p_hci_cmd_cb->cmd_xmit_q), p_buf);
        p_buf = NULL;
    }

    if ((controller_id == LOCAL_BR_EDR_CONTROLLER_ID)
         && (p_hci_cmd_cb->cmd_window == 0))
    {
        p_hci_cmd_cb->cmd_window = p_hci_cmd_cb->cmd_xmit_q.count + 1;
    }

    /* See if we can send anything */
    while (p_hci_cmd_cb->cmd_window != 0)
    {
        if (!p_buf)
            p_buf = (BT_HDR *)GKI_dequeue (&(p_hci_cmd_cb->cmd_xmit_q));

        if (p_buf)
        {
            btu_hcif_store_cmd(controller_id, p_buf);

#if ((L2CAP_HOST_FLOW_CTRL == TRUE)||defined(HCI_TESTER))
            pp = (UINT8 *)(p_buf + 1) + p_buf->offset;

            STREAM_TO_UINT16 (code, pp);

            /*
             * We do not need to decrease window for host flow control,
             * host flow control does not receive an event back from controller
             */
            if (code != HCI_HOST_NUM_PACKETS_DONE)
#endif
                p_hci_cmd_cb->cmd_window--;

            if (controller_id == LOCAL_BR_EDR_CONTROLLER_ID)
            {
                ALOGE("HCI_CMD_TO_LOWER");
                HCI_CMD_TO_LOWER(p_buf);
            }
            else
            {
                /* Unknown controller */
                BT_TRACE_1 (TRACE_LAYER_HCI, TRACE_TYPE_WARNING, "BTU HCI(ctrl id=%d) controller ID not recognized", controller_id);
                GKI_freebuf(p_buf);;
            }

            p_buf = NULL;
        }
        else
            break;
    }
    ALOGE("btu_hcif_send_cmd--");
    if (p_buf)
        GKI_enqueue (&(p_hci_cmd_cb->cmd_xmit_q), p_buf);


}

/******************************************************************************
*
**
** Function         btu_hcif_cmd_timeout
**
** Description      Handle a command timeout
**
** Returns          void
**
*******************************************************************************/
void btu_hcif_cmd_timeout (UINT8 controller_id)
{

}
