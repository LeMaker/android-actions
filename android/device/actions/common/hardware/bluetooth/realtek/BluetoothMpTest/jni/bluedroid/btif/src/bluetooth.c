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

/************************************************************************************
 *
 *  Filename:      bluetooth.c
 *
 *  Description:   Bluetooth HAL implementation
 *
 ***********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "hardware/bluetoothmp.h"

#define LOG_NDDEBUG 0
#define LOG_TAG "bluedroid"

#include "btif_api.h"

#include "foundation.h"
#include "bt_mp_base.h"
#include "bt_mp_api.h"
#include "bluetooth_mp_opcode.h"
#include "gki.h"


bt_callbacks_t *bt_hal_cbacks = NULL;


/************************************************************************************
**  Static functions
************************************************************************************/

/************************************************************************************
**  Externs
************************************************************************************/
BT_MODULE               BtModuleMemory;
BASE_INTERFACE_MODULE   BaseInterfaceModuleMemory;

/************************************************************************************
**  Functions
************************************************************************************/

uint8_t hal_interface_ready(void)
{
    /* add checks here that would prevent API calls other than init to be executed */
    if (bt_hal_cbacks == NULL)
        return FALSE;

    return TRUE;
}


/*****************************************************************************
**
**   BLUETOOTH HAL INTERFACE FUNCTIONS
**
*****************************************************************************/

int hal_init(bt_callbacks_t* callbacks )
{
    ALOGI("init");

    /* sanity check */
    if (hal_interface_ready() == TRUE)
        return BT_STATUS_DONE;

    /* store reference to user callbacks */
    bt_hal_cbacks = callbacks;
    bt_mp_module_init(&BaseInterfaceModuleMemory, &BtModuleMemory);

    /* init btif */
    btif_init_bluetooth();

    return BT_STATUS_SUCCESS;
}

int hal_enable( void )
{
    ALOGI("enable");

    /* sanity check */
    if (hal_interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_enable_bluetooth();
}

int hal_disable(void)
{
    /* sanity check */
    if (hal_interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_disable_bluetooth();
}

void hal_cleanup( void )
{
    /* sanity check */
    if (hal_interface_ready() == FALSE)
        return;

    btif_shutdown_bluetooth();

    bt_hal_cbacks = NULL;
    /* hal callbacks reset upon shutdown complete callback */

    return;
}

int hal_dut_mode_configure(uint8_t enable)
{
    ALOGI("hal_dut_mode_configure");

    /* sanity check */
    if (hal_interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dut_mode_configure(enable);
}

extern void btu_hcif_mp_notify_event (BT_HDR *p_msg);

int hal_mp_op_send(uint16_t opcode, char* buf, uint8_t len)
{
    ALOGI("hal_mp_op_send");
    char *pParaBuf = NULL;
    char *pRetBuf = NULL;
    BT_HDR      *p_buf = NULL;

    char pNotifyBuffer[1024] = {0};
    
    pRetBuf = pNotifyBuffer;
    
    UINT8   mp_op_code = 0;
    UINT8   mp_op_buffer_len = 0;

    memset(pNotifyBuffer, 0, sizeof(pNotifyBuffer));
    
    p_buf = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) + 1026);//1024 + 1 + 1
    p_buf->offset = 0;
    pParaBuf = (char*)(p_buf + 1);
    memset(pParaBuf, 0, 1026);

    /* sanity check */
    if (hal_interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    switch(opcode)
    {
        case BT_MP_OP_HCI_SEND_CMD:
        {
            pRetBuf = BT_SendHciCmd(&BtModuleMemory, buf, pNotifyBuffer);
        }
        break;
        
       case BT_MP_OP_DUT_MODE_CONFIGURE:
        {
            
        }
        break;        

        case BT_MP_OP_USER_DEF_GetPara:
        {
            pRetBuf = BT_GetPara(&BtModuleMemory, pNotifyBuffer);

        }
        break;

        case BT_MP_OP_USER_DEF_SetPara1:
        {
            pRetBuf = BT_SetPara1(&BtModuleMemory, buf, pNotifyBuffer);
        }
        break;

        case BT_MP_OP_USER_DEF_SetPara2:
        {
            pRetBuf = BT_SetPara2(&BtModuleMemory, buf, pNotifyBuffer);
        }
        break;

        case BT_MP_OP_USER_DEF_SetHit:
        {
            pRetBuf = BT_SetHit(&BtModuleMemory, buf, pNotifyBuffer);
        }
        break;

        case BT_MP_OP_USER_DEF_SetDacTable:
        {
            pRetBuf = BT_SetDacTable(&BtModuleMemory, buf, pNotifyBuffer);
        }
        break;

        case BT_MP_OP_USER_DEF_SetGainTable:
        {
            pRetBuf = BT_SetGainTable(&BtModuleMemory, buf, pNotifyBuffer);
        }
        break;
        
        case BT_MP_OP_USER_DEF_Exec:
        {
            pRetBuf = BT_Exec(&BtModuleMemory, buf, pNotifyBuffer);
        }
        break;

        case BT_MP_OP_USER_DEF_ReportTx:
        {
            pRetBuf = BT_ReportTx(&BtModuleMemory, pNotifyBuffer);
        }
        break;

        case BT_MP_OP_USER_DEF_ReportRx:
        {
            pRetBuf = BT_ReportRx(&BtModuleMemory, pNotifyBuffer);
        }
        break;

        case BT_MP_OP_USER_DEF_REG_RF:
        {
            pRetBuf = BT_RegRf(&BtModuleMemory, buf, pNotifyBuffer);
        }
        break;

        case BT_MP_OP_USER_DEF_REG_MD:
        {
            pRetBuf = BT_RegMd(&BtModuleMemory, buf, pNotifyBuffer);
        }
        break;

        default:
            break;

    }
    mp_op_code = opcode;
    mp_op_buffer_len = strlen(pRetBuf);

    UINT8_TO_STREAM(pParaBuf, mp_op_code);
    UINT8_TO_STREAM(pParaBuf, mp_op_buffer_len);

    ALOGI("pRetBuf: %s, %d", pRetBuf, mp_op_buffer_len);
    memcpy(pParaBuf, pRetBuf, mp_op_buffer_len);
    ALOGI("pParaBuf: %s, %d", pParaBuf, mp_op_buffer_len);
    btu_hcif_mp_notify_event(p_buf);

    GKI_freebuf(p_buf);


    return 0;
}

static const bt_interface_t bluetoothInterface = {
    sizeof(bt_interface_t),
    hal_init,
    hal_enable,
    hal_disable,
    hal_cleanup,
    hal_dut_mode_configure,
    hal_mp_op_send
};


const bt_interface_t* bluetooth__get_bluetooth_interface ()
{
    /* fixme -- add property to disable bt interface ? */

    return &bluetoothInterface;
}

static int close_bluetooth_stack(struct hw_device_t* device)
{
    hal_cleanup();
    return 0;
}

static int open_bluetooth_stack (const struct hw_module_t* module, char const
* name,
struct hw_device_t** abstraction)
{
    bluetooth_device_t *stack = malloc(sizeof(bluetooth_device_t) );
    memset(stack, 0, sizeof(bluetooth_device_t) );
    stack->common.tag = HARDWARE_DEVICE_TAG;
    stack->common.version = 0;
    stack->common.module = (struct hw_module_t*)module;
    stack->common.close = close_bluetooth_stack;
    stack->get_bluetooth_interface = bluetooth__get_bluetooth_interface;
    *abstraction = (struct hw_device_t*)stack;
    return 0;
}


static struct hw_module_methods_t bt_stack_module_methods = {
    .open = open_bluetooth_stack,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = BT_HARDWARE_MODULE_ID,
    .name = "Bluetooth MP Stack",
    .author = "The Android Open Source Project",
    .methods = &bt_stack_module_methods
};
