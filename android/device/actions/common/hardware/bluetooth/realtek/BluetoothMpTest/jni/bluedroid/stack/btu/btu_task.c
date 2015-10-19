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
 *  This file contains the main Bluetooth Upper Layer processing loop.
 *  The Broadcom implementations of L2CAP RFCOMM, SDP and the BTIf run as one
 *  GKI task. This btu_task switches between them.
 *
 *  Note that there will always be an L2CAP, but there may or may not be an
 *  RFCOMM or SDP. Whether these layers are present or not is determined by
 *  compile switches.
 *
 ******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bt_target.h"
#include "gki.h"
#include "bt_types.h"
#include "hcimsgs.h"

#include "btu.h"
#include "bt_utils.h"




#include "bt_trace.h"


/* Define BTU storage area
*/
#if BTU_DYNAMIC_MEMORY == FALSE
tBTU_CB  btu_cb;
#endif


/* Define a function prototype to allow a generic timeout handler */
typedef void (tUSER_TIMEOUT_FUNC) (TIMER_LIST_ENT *p_tle);

/*******************************************************************************
**
** Function         btu_task
**
** Description      This is the main task of the Bluetooth Upper Layers unit.
**                  It sits in a loop waiting for messages, and dispatches them
**                  to the appropiate handlers.
**
** Returns          should never return
**
*******************************************************************************/
BTU_API UINT32 btu_task (UINT32 param)
{
    UINT16           event;
    BT_HDR          *p_msg;
    UINT8            i;
    UINT16           mask;
    BOOLEAN          handled;

    /* wait an event that HCISU is ready */
    BT_TRACE_0(TRACE_LAYER_BTU, TRACE_TYPE_API,
                "btu_task pending for preload complete event");

    for (;;)
    {
        event = GKI_wait (0xFFFF, 0);
        if (event & EVENT_MASK(GKI_SHUTDOWN_EVT))
        {
            /* indicates BT ENABLE abort */
            BT_TRACE_0(TRACE_LAYER_BTU, TRACE_TYPE_WARNING,
                        "btu_task start abort!");
            return (0);
        }
        else if (event & BT_EVT_PRELOAD_CMPL)
        {
            break;
        }
        else
        {
            BT_TRACE_1(TRACE_LAYER_BTU, TRACE_TYPE_WARNING,
                "btu_task ignore evt %04x while pending for preload complete",
                event);
        }
    }

    BT_TRACE_0(TRACE_LAYER_BTU, TRACE_TYPE_API,
                "btu_task received preload complete event");
    


    /* Send a startup evt message to BTIF_TASK to kickstart the init procedure */
    GKI_send_event(BTIF_TASK, BT_EVT_TRIGGER_STACK_INIT);



    /* Wait for, and process, events */
    for (;;)
    {
        event = GKI_wait (0xFFFF, 0);

        if (event & TASK_MBOX_0_EVT_MASK)
        {
            /* Process all messages in the queue */
            while ((p_msg = (BT_HDR *) GKI_read_mbox (BTU_HCI_RCV_MBOX)) != NULL)
            {
                /* Determine the input message type. */
                switch (p_msg->event & BT_EVT_MASK)
                {
                    case BT_EVT_TO_BTU_HCI_ACL:
                        /* All Acl Data goes to L2CAP */
                        //l2c_rcv_acl_data (p_msg);
                        break;

                    case BT_EVT_TO_BTU_L2C_SEG_XMIT:
                        /* L2CAP segment transmit complete */
                        break;



                    case BT_EVT_TO_BTU_HCI_EVT:
                        btu_hcif_process_event ((UINT8)(p_msg->event & BT_SUB_EVT_MASK), p_msg);
                        GKI_freebuf(p_msg);


                        break;

                    case BT_EVT_TO_BTU_HCI_CMD:
                        btu_hcif_send_cmd ((UINT8)(p_msg->event & BT_SUB_EVT_MASK), p_msg);
                        break;


                    case BT_EVT_TO_START_TIMER :
                        /* Start free running 1 second timer for list management */
                        GKI_start_timer (TIMER_0, GKI_SECS_TO_TICKS (1), TRUE);
                        GKI_freebuf (p_msg);
                        break;

#if defined(QUICK_TIMER_TICKS_PER_SEC) && (QUICK_TIMER_TICKS_PER_SEC > 0)
                    case BT_EVT_TO_START_QUICK_TIMER :
                        GKI_start_timer (TIMER_2, QUICK_TIMER_TICKS, TRUE);
                        GKI_freebuf (p_msg);
                        break;
#endif

                    default:
                        i = 0;
                        mask = (UINT16) (p_msg->event & BT_EVT_MASK);
                        handled = FALSE;

                        for (; !handled && i < BTU_MAX_REG_EVENT; i++)
                        {
                            if (btu_cb.event_reg[i].event_cb == NULL)
                                continue;

                            if (mask == btu_cb.event_reg[i].event_range)
                            {
                                if (btu_cb.event_reg[i].event_cb)
                                {
                                    btu_cb.event_reg[i].event_cb(p_msg);
                                    handled = TRUE;
                                }
                            }
                        }

                        if (handled == FALSE)
                            GKI_freebuf (p_msg);

                        break;
                }
            }
        }


        if (event & TIMER_0_EVT_MASK)
        {
            TIMER_LIST_ENT  *p_tle;

            GKI_update_timer_list (&btu_cb.timer_queue, 1);

            while ((btu_cb.timer_queue.p_first) && (!btu_cb.timer_queue.p_first->ticks))
            {
                p_tle = btu_cb.timer_queue.p_first;
                GKI_remove_from_timer_list (&btu_cb.timer_queue, p_tle);

                switch (p_tle->event)
                {


                    case BTU_TTYPE_BTU_CMD_CMPL:
                        btu_hcif_cmd_timeout((UINT8)(p_tle->event - BTU_TTYPE_BTU_CMD_CMPL));
                        break;


                    case BTU_TTYPE_USER_FUNC:
                        {
                            tUSER_TIMEOUT_FUNC  *p_uf = (tUSER_TIMEOUT_FUNC *)p_tle->param;
                            (*p_uf)(p_tle);
                        }
                        break;

                    default:
                        i = 0;
                        handled = FALSE;

                        for (; !handled && i < BTU_MAX_REG_TIMER; i++)
                        {
                            if (btu_cb.timer_reg[i].timer_cb == NULL)
                                continue;
                            if (btu_cb.timer_reg[i].p_tle == p_tle)
                            {
                                btu_cb.timer_reg[i].timer_cb(p_tle);
                                handled = TRUE;
                            }
                        }
                        break;
                }
            }

            /* if timer list is empty stop periodic GKI timer */
            if (btu_cb.timer_queue.p_first == NULL)
            {
                GKI_stop_timer(TIMER_0);
            }
        }

#if defined(QUICK_TIMER_TICKS_PER_SEC) && (QUICK_TIMER_TICKS_PER_SEC > 0)
        if (event & TIMER_2_EVT_MASK)
        {
            btu_process_quick_timer_evt();
        }
#endif




#if (defined(BTU_BTA_INCLUDED) && BTU_BTA_INCLUDED == TRUE)
        if (event & TASK_MBOX_2_EVT_MASK)
        {
            while ((p_msg = (BT_HDR *) GKI_read_mbox(TASK_MBOX_2)) != NULL)
            {
                //bta_sys_event(p_msg);
            }
        }

        if (event & TIMER_1_EVT_MASK)
        {
            //bta_sys_timer_update();
        }
#endif

        if (event & EVENT_MASK(APPL_EVT_7))
            break;
    }

    return(0);
}

/*******************************************************************************
**
** Function         btu_start_timer
**
** Description      Start a timer for the specified amount of time.
**                  NOTE: The timeout resolution is in SECONDS! (Even
**                          though the timer structure field is ticks)
**
** Returns          void
**
*******************************************************************************/
void btu_start_timer (TIMER_LIST_ENT *p_tle, UINT16 type, UINT32 timeout)
{
    BT_HDR *p_msg;
    /* if timer list is currently empty, start periodic GKI timer */
    if (btu_cb.timer_queue.p_first == NULL)
    {
        /* if timer starts on other than BTU task */
        if (GKI_get_taskid() != BTU_TASK)
        {
            /* post event to start timer in BTU task */
            if ((p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE)) != NULL)
            {
                p_msg->event = BT_EVT_TO_START_TIMER;
                GKI_send_msg (BTU_TASK, TASK_MBOX_0, p_msg);
            }
        }
        else
        {
            /* Start free running 1 second timer for list management */
            GKI_start_timer (TIMER_0, GKI_SECS_TO_TICKS (1), TRUE);
        }
    }

    GKI_remove_from_timer_list (&btu_cb.timer_queue, p_tle);

    p_tle->event = type;
    p_tle->ticks = timeout;         /* Save the number of seconds for the timer */

    GKI_add_to_timer_list (&btu_cb.timer_queue, p_tle);
}

/*******************************************************************************
**
** Function         btu_remaining_time
**
** Description      Return amount of time to expire
**
** Returns          time in second
**
*******************************************************************************/
UINT32 btu_remaining_time (TIMER_LIST_ENT *p_tle)
{
    return(GKI_get_remaining_ticks (&btu_cb.timer_queue, p_tle));
}

/*******************************************************************************
**
** Function         btu_stop_timer
**
** Description      Stop a timer.
**
** Returns          void
**
*******************************************************************************/
void btu_stop_timer (TIMER_LIST_ENT *p_tle)
{
    GKI_remove_from_timer_list (&btu_cb.timer_queue, p_tle);

    /* if timer list is empty stop periodic GKI timer */
    if (btu_cb.timer_queue.p_first == NULL)
    {
        GKI_stop_timer(TIMER_0);
    }

}


/*******************************************************************************
**
** Function         btu_start_quick_timer
**
** Description      Start a timer for the specified amount of time.
**                  NOTE: The timeout resolution depends on including modules.
**                  QUICK_TIMER_TICKS_PER_SEC should be used to convert from
**                  time to ticks.
**
**
** Returns          void
**
*******************************************************************************/
void btu_start_quick_timer (TIMER_LIST_ENT *p_tle, UINT16 type, UINT32 timeout)
{
    BT_HDR *p_msg;

    /* if timer list is currently empty, start periodic GKI timer */
    if (btu_cb.quick_timer_queue.p_first == NULL)
    {
        /* script test calls stack API without posting event */
        if (GKI_get_taskid() != BTU_TASK)
        {
            /* post event to start timer in BTU task */
            if ((p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE)) != NULL)
            {
                p_msg->event = BT_EVT_TO_START_QUICK_TIMER;
                GKI_send_msg (BTU_TASK, TASK_MBOX_0, p_msg);
            }
        }
        else
            GKI_start_timer(TIMER_2, QUICK_TIMER_TICKS, TRUE);
    }

    GKI_remove_from_timer_list (&btu_cb.quick_timer_queue, p_tle);

    p_tle->event = type;
    p_tle->ticks = timeout; /* Save the number of ticks for the timer */

    GKI_add_to_timer_list (&btu_cb.quick_timer_queue, p_tle);
}


/*******************************************************************************
**
** Function         btu_stop_quick_timer
**
** Description      Stop a timer.
**
** Returns          void
**
*******************************************************************************/
void btu_stop_quick_timer (TIMER_LIST_ENT *p_tle)
{
    GKI_remove_from_timer_list (&btu_cb.quick_timer_queue, p_tle);

    /* if timer list is empty stop periodic GKI timer */
    if (btu_cb.quick_timer_queue.p_first == NULL)
    {
        GKI_stop_timer(TIMER_2);
    }
}

/*******************************************************************************
**
** Function         btu_process_quick_timer_evt
**
** Description      Process quick timer event
**
** Returns          void
**
*******************************************************************************/
void btu_process_quick_timer_evt(void)
{

    /* if timer list is empty stop periodic GKI timer */
    if (btu_cb.quick_timer_queue.p_first == NULL)
    {
        GKI_stop_timer(TIMER_2);
    }
}





void btu_register_timer (TIMER_LIST_ENT *p_tle, UINT16 type, UINT32 timeout, tBTU_TIMER_CALLBACK timer_cb)
{
    UINT8 i = 0;
    INT8  first = -1;
    for (; i < BTU_MAX_REG_TIMER; i++)
    {
        if (btu_cb.timer_reg[i].p_tle == NULL && first < 0)
            first = i;
        if (btu_cb.timer_reg[i].p_tle == p_tle)
        {
            btu_cb.timer_reg[i].timer_cb = timer_cb;
            btu_start_timer(p_tle, type, timeout);
            first = -1;
            break;
        }
    }

    if (first >= 0 && first < BTU_MAX_REG_TIMER)
    {
        btu_cb.timer_reg[first].timer_cb = timer_cb;
        btu_cb.timer_reg[first].p_tle = p_tle;
        btu_start_timer(p_tle, type, timeout);
    }

}


void btu_deregister_timer(TIMER_LIST_ENT *p_tle)
{
    UINT8 i = 0;

    for (; i < BTU_MAX_REG_TIMER; i++)
    {
        if (btu_cb.timer_reg[i].p_tle == p_tle)
        {
            btu_stop_timer(p_tle);
            btu_cb.timer_reg[i].timer_cb = NULL;
            btu_cb.timer_reg[i].p_tle = NULL;
            break;
        }
    }
}

void btu_register_event_range (UINT16 start, tBTU_EVENT_CALLBACK event_cb)
{
    UINT8 i = 0;
    INT8  first = -1;

    for (; i < BTU_MAX_REG_EVENT; i++)
    {
        if (btu_cb.event_reg[i].event_cb == NULL && first < 0)
            first = i;

        if (btu_cb.event_reg[i].event_range == start)
        {
            btu_cb.event_reg[i].event_cb = event_cb;

            if (!event_cb)
                btu_cb.event_reg[i].event_range = 0;

            first = -1;
        }
    }

    /* if not deregistering && an empty index was found in range, register */
    if (event_cb && first >= 0 && first < BTU_MAX_REG_EVENT)
    {
        btu_cb.event_reg[first].event_range = start;
        btu_cb.event_reg[first].event_cb = event_cb;
    }
}


void btu_deregister_event_range (UINT16 range)
{
    btu_register_event_range(range, NULL);
}
