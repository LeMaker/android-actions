/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
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
 *  Filename:      bte_main.c
 *
 *  Description:   Contains BTE core stack initialization and shutdown code
 *
 ******************************************************************************/
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "gki.h"
//#include "bd.h"
#include "btu.h"
//#include "bte.h"
//#include "bta_api.h"
#include "bt_hci_lib.h"
#include "bt_hci_bdroid.h"

/*******************************************************************************
**  Constants & Macros
*******************************************************************************/

/* Run-time configuration file */
#ifndef BTE_STACK_CONF_FILE
#define BTE_STACK_CONF_FILE "/etc/bluetooth/bt_stack.conf"
#endif

/* if not specified in .txt file then use this as default  */
#ifndef HCI_LOGGING_FILENAME
#define HCI_LOGGING_FILENAME  "/data/misc/bluedroid/btsnoop_hci.log"
#endif

/* Stack preload process timeout period  */
#ifndef PRELOAD_START_TIMEOUT_MS
#define PRELOAD_START_TIMEOUT_MS 5000  // 3 seconds
#endif

/* Stack preload process maximum retry attempts  */
#ifndef PRELOAD_MAX_RETRY_ATTEMPTS
#define PRELOAD_MAX_RETRY_ATTEMPTS 0
#endif

/*******************************************************************************
**  Local type definitions
*******************************************************************************/
/* Preload retry control block */
typedef struct
{
    int     retry_counts;
    BOOLEAN timer_created;
    timer_t timer_id;
} bt_preload_retry_cb_t;


/******************************************************************************
**  Variables
******************************************************************************/
BOOLEAN hci_logging_enabled = FALSE;    /* by default, turn hci log off */
char hci_logfile[256] = HCI_LOGGING_FILENAME;

BOOLEAN trace_h5_enabled = FALSE;    /* by default, turn hci log off */

/*******************************************************************************
**  Static variables
*******************************************************************************/
static bt_hc_interface_t *bt_hc_if=NULL;
static const bt_hc_callbacks_t hc_callbacks;
static BOOLEAN lpm_enabled = FALSE;

static bt_preload_retry_cb_t preload_retry_cb;


/*******************************************************************************
**  Static functions
*******************************************************************************/
static void bte_main_in_hw_init(void);
static void preload_start_wait_timer(void);
static void preload_stop_wait_timer(void);

/*******************************************************************************
**  Externs
*******************************************************************************/
BTU_API extern UINT32 btu_task (UINT32 param);
BTU_API extern void BTE_Init (void);
BT_API extern void BTE_LoadStack(void);
BT_API void BTE_UnloadStack(void);
extern void scru_flip_bda (BD_ADDR dst, const BD_ADDR src);
extern void bte_load_conf(const char *p_path);


/*******************************************************************************
**                        System Task Configuration
*******************************************************************************/

/* bluetooth protocol stack (BTU) task */
#ifndef BTE_BTU_STACK_SIZE
#define BTE_BTU_STACK_SIZE       0//0x2000         /* In bytes */
#endif
#define BTE_BTU_TASK_STR        ((INT8 *) "BTU")
UINT32 bte_btu_stack[(BTE_BTU_STACK_SIZE + 3) / 4];

/******************************************************************************
**
** Function         bte_main_in_hw_init
**
** Description      Internal helper function for chip hardware init
**
** Returns          None
**
******************************************************************************/
void bte_main_in_hw_init(void)
{
    if ( (bt_hc_if = (bt_hc_interface_t *) bt_hc_get_interface()) \
         == NULL)
    {
        APPL_TRACE_ERROR0("!!! Failed to get BtHostControllerInterface !!!");
    }
}

/******************************************************************************
**
** Function         bte_main_boot_entry
**
** Description      BTE MAIN API - Entry point for BTE chip/stack initialization
**
** Returns          None
**
******************************************************************************/
void bte_main_boot_entry(void)
{
    /* initialize OS */
    GKI_init();

    bte_main_in_hw_init();

    bte_load_conf(BTE_STACK_CONF_FILE);

}

/******************************************************************************
**
** Function         bte_main_shutdown
**
** Description      BTE MAIN API - Shutdown code for BTE chip/stack
**
** Returns          None
**
******************************************************************************/
void bte_main_shutdown()
{
    GKI_shutdown();
}

/******************************************************************************
**
** Function         bte_main_enable
**
** Description      BTE MAIN API - Creates all the BTE tasks. Should be called
**                  part of the Bluetooth stack enable sequence
**
** Returns          None
**
******************************************************************************/
void bte_main_enable(uint8_t *local_addr)
{
    APPL_TRACE_DEBUG1("%s", __FUNCTION__);
    
    preload_start_wait_timer();
        

    if (bt_hc_if)
    {
        int result = bt_hc_if->init(&hc_callbacks, local_addr);

        assert(result == BT_HC_STATUS_SUCCESS);

        if (hci_logging_enabled == TRUE)
            bt_hc_if->logging(BT_HC_LOGGING_ON, hci_logfile);


        bt_hc_if->set_power(BT_HC_CHIP_PWR_OFF);

        bt_hc_if->set_power(BT_HC_CHIP_PWR_ON);

        bt_hc_if->preload(NULL);
    }

    GKI_create_task((TASKPTR)btu_task, BTU_TASK, BTE_BTU_TASK_STR,
                    (UINT16 *) ((UINT8 *)bte_btu_stack + BTE_BTU_STACK_SIZE),
                    sizeof(bte_btu_stack));

    GKI_run(0);
}

/******************************************************************************
**
** Function         bte_main_disable
**
** Description      BTE MAIN API - Destroys all the BTE tasks. Should be called
**                  part of the Bluetooth stack disable sequence
**
** Returns          None
**
******************************************************************************/
void bte_main_disable(void)
{
    APPL_TRACE_DEBUG1("%s", __FUNCTION__);
    preload_stop_wait_timer();
    
    if (bt_hc_if)
    {
        bt_hc_if->cleanup();
        bt_hc_if->set_power(BT_HC_CHIP_PWR_OFF);
    }

    GKI_destroy_task(BTU_TASK);

    GKI_freeze();
}

/*******************************************************************************
**
** Function        preload_wait_timeout
**
** Description     Timeout thread of preload watchdog timer
**
** Returns         None
**
*******************************************************************************/
static void preload_wait_timeout(union sigval arg)
{
    APPL_TRACE_ERROR2("...preload_wait_timeout (retried:%d/max-retry:%d)...",
                        preload_retry_cb.retry_counts,
                        PRELOAD_MAX_RETRY_ATTEMPTS);

        /* Notify BTIF_TASK that the init procedure had failed*/
        GKI_send_event(BTIF_TASK, BT_EVT_HARDWARE_INIT_FAIL);
}

/*******************************************************************************
**
** Function        preload_start_wait_timer
**
** Description     Launch startup watchdog timer
**
** Returns         None
**
*******************************************************************************/
static void preload_start_wait_timer(void)
{
    int status;
    struct itimerspec ts;
    struct sigevent se;
    UINT32 timeout_ms = PRELOAD_START_TIMEOUT_MS;

    if (preload_retry_cb.timer_created == FALSE)
    {
        se.sigev_notify = SIGEV_THREAD;
        se.sigev_value.sival_ptr = &preload_retry_cb.timer_id;
        se.sigev_notify_function = preload_wait_timeout;
        se.sigev_notify_attributes = NULL;

        status = timer_create(CLOCK_MONOTONIC, &se, &preload_retry_cb.timer_id);

        if (status == 0)
            preload_retry_cb.timer_created = TRUE;
    }

    if (preload_retry_cb.timer_created == TRUE)
    {
        ts.it_value.tv_sec = timeout_ms/1000;
        ts.it_value.tv_nsec = 1000000*(timeout_ms%1000);
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;

        status = timer_settime(preload_retry_cb.timer_id, 0, &ts, 0);
        if (status == -1)
            APPL_TRACE_ERROR0("Failed to fire preload watchdog timer");
    }
}

/*******************************************************************************
**
** Function        preload_stop_wait_timer
**
** Description     Stop preload watchdog timer
**
** Returns         None
**
*******************************************************************************/
static void preload_stop_wait_timer(void)
{
    if (preload_retry_cb.timer_created == TRUE)
    {
        timer_delete(preload_retry_cb.timer_id);
        preload_retry_cb.timer_created = FALSE;
    }
}

/******************************************************************************
**
** Function         bte_main_postload_cfg
**
** Description      BTE MAIN API - Stack postload configuration
**
** Returns          None
**
******************************************************************************/
void bte_main_postload_cfg(void)
{
    if (bt_hc_if)
        bt_hc_if->postload(NULL);
}

/******************************************************************************
**
** Function         bte_main_hci_send
**
** Description      BTE MAIN API - This function is called by the upper stack to
**                  send an HCI message. The function displays a protocol trace
**                  message (if enabled), and then calls the 'transmit' function
**                  associated with the currently selected HCI transport
**
** Returns          None
**
******************************************************************************/
void bte_main_hci_send (BT_HDR *p_msg, UINT16 event)
{
    UINT16 sub_event = event & BT_SUB_EVT_MASK;  /* local controller ID */

    APPL_TRACE_DEBUG1("%s", __FUNCTION__);
    p_msg->event = event;


    if((sub_event == LOCAL_BR_EDR_CONTROLLER_ID) || \
       (sub_event == LOCAL_BLE_CONTROLLER_ID))
    {
        if (bt_hc_if)
            bt_hc_if->transmit_buf((TRANSAC)p_msg, \
                                       (char *) (p_msg + 1), \
                                        p_msg->len);
        else
            GKI_freebuf(p_msg);
    }
    else
    {
        APPL_TRACE_ERROR0("Invalid Controller ID. Discarding message.");
        GKI_freebuf(p_msg);
    }
}

/******************************************************************************
**
** Function         bte_main_post_reset_init
**
** Description      BTE MAIN API - This function is mapped to BTM_APP_DEV_INIT
**                  and shall be automatically called from BTE after HCI_Reset
**
** Returns          None
**
******************************************************************************/
void bte_main_post_reset_init()
{

}

/*****************************************************************************
**
**   libbt-hci Callback Functions
**
*****************************************************************************/

/******************************************************************************
**
** Function         preload_cb
**
** Description      HOST/CONTROLLER LIB CALLBACK API - This function is called
**                  when the libbt-hci completed stack preload process
**
** Returns          None
**
******************************************************************************/
static void preload_cb(TRANSAC transac, bt_hc_preload_result_t result)
{
    APPL_TRACE_EVENT1("HC preload_cb %d [0:SUCCESS 1:FAIL]", result);

    if (result == BT_HC_PRELOAD_SUCCESS)
    {
        preload_stop_wait_timer();

        /* notify BTU task that libbt-hci is ready */
        GKI_send_event(BTU_TASK, BT_EVT_PRELOAD_CMPL);
    }
}

/******************************************************************************
**
** Function         postload_cb
**
** Description      HOST/CONTROLLER LIB CALLBACK API - This function is called
**                  when the libbt-hci lib completed stack postload process
**
** Returns          None
**
******************************************************************************/
static void postload_cb(TRANSAC transac, bt_hc_postload_result_t result)
{
    APPL_TRACE_EVENT1("HC postload_cb %d", result);
}


/******************************************************************************
**
** Function         alloc
**
** Description      HOST/CONTROLLER LIB CALLOUT API - This function is called
**                  from the libbt-hci to request for data buffer allocation
**
** Returns          NULL / pointer to allocated buffer
**
******************************************************************************/
static char *alloc(int size)
{
    BT_HDR *p_hdr = NULL;

    /*
    APPL_TRACE_DEBUG1("HC alloc size=%d", size);
    */

    p_hdr = (BT_HDR *) GKI_getbuf ((UINT16) size);

    if (p_hdr == NULL)
    {
        APPL_TRACE_WARNING0("alloc returns NO BUFFER!");
    }

    return ((char *) p_hdr);
}

/******************************************************************************
**
** Function         dealloc
**
** Description      HOST/CONTROLLER LIB CALLOUT API - This function is called
**                  from the libbt-hci to release the data buffer allocated
**                  through the alloc call earlier
**
**                  Bluedroid libbt-hci library uses 'transac' parameter to
**                  pass data-path buffer/packet across bt_hci_lib interface
**                  boundary. The 'p_buf' is not intended to be used here
**                  but might point to data portion of data-path buffer.
**
** Returns          bt_hc_status_t
**
******************************************************************************/
static int dealloc(TRANSAC transac, char *p_buf)
{
    GKI_freebuf(transac);
    return BT_HC_STATUS_SUCCESS;
}

/******************************************************************************
**
** Function         data_ind
**
** Description      HOST/CONTROLLER LIB CALLOUT API - This function is called
**                  from the libbt-hci to pass in the received HCI packets
**
**                  The core stack is responsible for releasing the data buffer
**                  passed in from the libbt-hci once the core stack has done
**                  with it.
**
**                  Bluedroid libbt-hci library uses 'transac' parameter to
**                  pass data-path buffer/packet across bt_hci_lib interface
**                  boundary. The 'p_buf' and 'len' parameters are not intended
**                  to be used here but might point to data portion in data-
**                  path buffer and length of valid data respectively.
**
** Returns          bt_hc_status_t
**
******************************************************************************/
static int data_ind(TRANSAC transac, char *p_buf, int len)
{
    BT_HDR *p_msg = (BT_HDR *) transac;

    /*
    APPL_TRACE_DEBUG2("HC data_ind event=0x%04X (len=%d)", p_msg->event, len);
    */

    GKI_send_msg (BTU_TASK, BTU_HCI_RCV_MBOX, transac);
    return BT_HC_STATUS_SUCCESS;
}

/******************************************************************************
**
** Function         tx_result
**
** Description      HOST/CONTROLLER LIB CALLBACK API - This function is called
**                  from the libbt-hci once it has processed/sent the prior data
**                  buffer which core stack passed to it through transmit_buf
**                  call earlier.
**
**                  The core stack is responsible for releasing the data buffer
**                  if it has been completedly processed.
**
**                  Bluedroid libbt-hci library uses 'transac' parameter to
**                  pass data-path buffer/packet across bt_hci_lib interface
**                  boundary. The 'p_buf' is not intended to be used here
**                  but might point to data portion in data-path buffer.
**
** Returns          bt_hc_status_t
**
******************************************************************************/
static int tx_result(TRANSAC transac, char *p_buf, \
                      bt_hc_transmit_result_t result)
{
    /*
    APPL_TRACE_DEBUG2("HC tx_result %d (event=%04X)", result, \
                      ((BT_HDR *)transac)->event);
    */

    if (result == BT_HC_TX_FRAGMENT)
    {
        GKI_send_msg (BTU_TASK, BTU_HCI_RCV_MBOX, transac);
    }
    else
    {
        GKI_freebuf(transac);
    }

    return BT_HC_STATUS_SUCCESS;
}

/*****************************************************************************
**   The libbt-hci Callback Functions Table
*****************************************************************************/
static const bt_hc_callbacks_t hc_callbacks = {
    sizeof(bt_hc_callbacks_t),
    preload_cb,
    postload_cb,
    NULL,
    NULL,
    alloc,
    dealloc,
    data_ind,
    tx_result
};
