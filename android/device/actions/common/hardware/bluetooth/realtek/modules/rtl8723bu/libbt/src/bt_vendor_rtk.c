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
 *  Filename:      bt_vendor_rtk.c
 *
 *  Description:   Broadcom vendor specific library implementation
 *
 ******************************************************************************/

#define LOG_TAG "bt_vendor"

#include <fcntl.h>
#include <errno.h>
#include <utils/Log.h>
#include "bt_vendor_rtk.h"

#ifndef BTVND_DBG
#define BTVND_DBG FALSE
#endif

#if (BTVND_DBG == TRUE)
#define BTVNDDBG(param, ...) {ALOGD(param, ## __VA_ARGS__);}
#else
#define BTVNDDBG(param, ...) {}
#endif

/******************************************************************************
**  Local type definitions
******************************************************************************/
#define VND_PORT_NAME_MAXLEN    256
/* vendor serial control block */
typedef struct
{
    int fd;                     /* fd to Bluetooth device */
    uint16_t dev_id;
    char port_name[VND_PORT_NAME_MAXLEN];
} vnd_userial_cb_t;

/******************************************************************************
**  Variables
******************************************************************************/

bt_vendor_callbacks_t *bt_vendor_cbacks = NULL;
uint8_t vnd_local_bd_addr[6]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static vnd_userial_cb_t vnd_userial;
int poweruse_flag = 0;
/*****************************************************************************
**
**   BLUETOOTH VENDOR INTERFACE LIBRARY FUNCTIONS
**
*****************************************************************************/

static int init(const bt_vendor_callbacks_t* p_cb, unsigned char *local_bdaddr)
{
    ALOGI("init");

    if (p_cb == NULL)
    {
        ALOGE("init failed with no user callbacks!");
        return -1;
    }

    userial_vendor_init();

    /* store reference to user callbacks */
    bt_vendor_cbacks = (bt_vendor_callbacks_t *) p_cb;

    /* This is handed over from the stack */
    memcpy(vnd_local_bd_addr, local_bdaddr, 6);

    return 0;
}

int set_bluetooth_power(int on)
{
    int sz;
    int fd = -1;
    int ret = -1;
    char buffer;
    char rfkill_state_path[64];
    char propBuf[64];

    property_get("ro.bt.useusb0", propBuf, "0");
    if (strcmp(propBuf, "9") == 0) {
        return 0;
    } else if (strcmp(propBuf, "0") == 0) {
        buffer = (on ? 'a' : 'b');
    } else {
        buffer = (on ? 'c' : 'd');
    }

    ALOGE("set_bluetooth_power:%d-%d-%c", poweruse_flag, on, buffer);
    if (!poweruse_flag)
    {
         if (!on)
         {
             return 0;
         }
    }
    else
    {
         if (on)
         {
             return 0;
         }
    }

    //strcpy(rfkill_state_path, "/proc/acts_hub");

    fd = open(rfkill_state_path, O_WRONLY);
    if (fd < 0) {
        ALOGE("open (%s) for write failed: %s (%d)", rfkill_state_path, 
        strerror(errno), errno);
        return ret;
    }
    sz = write(fd, &buffer, 1);
    if (sz < 0) {
        ALOGE("write(%s) failed: %s (%d)", rfkill_state_path, strerror(errno),
             errno);
        close(fd);
        return ret;;
    }
    ret = 0;
    if (on)
    {
        usleep(1000000);
        poweruse_flag = 1;
    }
    else
    {
        usleep(200000);
        poweruse_flag = 0;
    }

    close(fd);
    return ret;
}

/** Requested operations */
static int op(bt_vendor_opcode_t opcode, void *param)
{
    int retval = 0;

    BTVNDDBG("op for %d", opcode);

    switch(opcode)
    {
        case BT_VND_OP_POWER_CTRL:
            {
                int *state = (int *) param;
                if (*state == BT_VND_PWR_OFF)
                    set_bluetooth_power(0);
                else if (*state == BT_VND_PWR_ON)
                    set_bluetooth_power(1);
            }
            break;

        case BT_VND_OP_FW_CFG:
            {
            	bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);
            }
            break;

        case BT_VND_OP_SCO_CFG:
            {
                retval = -1;
            }
            break;

        case BT_VND_OP_USERIAL_OPEN:
            {
                int (*fd_array)[] = (int (*)[]) param;
                int fd, idx;
				int i = 0;

				for (i=0; i<5; i++)
				{
                    fd = userial_vendor_open();
					if (fd != -1)
					{
					    break;
					}
					else
					{
					    usleep(1000000);
					}
				}

                if (fd != -1)
                {
                    for (idx=0; idx < CH_MAX; idx++)
                        (*fd_array)[idx] = fd;

                    retval = 1;
                }
                /* retval contains numbers of open fd of HCI channels */
            }
            break;

        case BT_VND_OP_USERIAL_CLOSE:
            {
                userial_vendor_close();
            }
            break;

        case BT_VND_OP_GET_LPM_IDLE_TIMEOUT:
            {
                uint32_t *timeout_ms = (uint32_t *) param;
                *timeout_ms = 250;
            }
            break;

        case BT_VND_OP_LPM_SET_MODE:
            {
        		if (bt_vendor_cbacks)
        			bt_vendor_cbacks->lpm_cb(BT_VND_OP_RESULT_SUCCESS);
            }
            break;

        case BT_VND_OP_LPM_WAKE_SET_STATE:
            break;
    }

    return retval;
}

/*******************************************************************************
**
** Function        userial_vendor_init
**
** Description     Initialize userial vendor-specific control block
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_init(void)
{
    vnd_userial.fd = -1;
    vnd_userial.dev_id = 0;
    snprintf(vnd_userial.port_name, VND_PORT_NAME_MAXLEN, "%s", \
            BLUETOOTH_UART_DEVICE_PORT);
}

/*******************************************************************************
**
** Function        userial_vendor_open
**
** Description     Open the serial port with the given configuration
**
** Returns         device fd
**
*******************************************************************************/
int userial_vendor_open(void)
{
	ALOGI("userial vendor open: opening %s", vnd_userial.port_name);

	if ((vnd_userial.fd = open(vnd_userial.port_name, O_RDWR)) == -1)
	{
	    ALOGE("userial vendor open: unable to open %s: %s", vnd_userial.port_name, strerror(errno));
	    return -1;
	}

	ALOGI("device fd = %d open", vnd_userial.fd);

	return vnd_userial.fd;
}

/*******************************************************************************
**
** Function        userial_vendor_close
**
** Description     Conduct vendor-specific close work
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_close(void)
{
    int result;

    if (vnd_userial.fd == -1)
        return;

    ALOGI("device fd = %d close", vnd_userial.fd);

    if ((result = close(vnd_userial.fd)) < 0)
        ALOGE( "close(fd:%d) FAILED result:%d", vnd_userial.fd, result);

    vnd_userial.fd = -1;
}

/** Closes the interface */
static void cleanup( void )
{
    BTVNDDBG("cleanup");
    bt_vendor_cbacks = NULL;
}

static int detect_bluetooth(void)
{
	#define		BT_PROC_FILE_NAME	"/proc/rtk_bt_8723bu"
	int fd = -1;
	char buffer;

	ALOGI("detect_bluetooth rtl8723bu");
	set_bluetooth_power(1);
	
	fd = open(BT_PROC_FILE_NAME, O_RDONLY);
    if (fd < 0) {
        ALOGE("detect_bluetooth open (%s) for read failed: %s (%d)", BT_PROC_FILE_NAME, strerror(errno), errno);
        return 0;
    }
	
    if (read(fd, &buffer, 1) < 0) {
        ALOGE("read(%s) failed: %s (%d)", BT_PROC_FILE_NAME, strerror(errno), errno);
		close(fd);
        return 0;
    }

	close(fd);

	ALOGI("detect_bluetooth rtl8723bu:%c", buffer);
		
	set_bluetooth_power(0);
	if ('1' == buffer)
		return 1;
	else
		return 0;
}

static int get_hci_version(void)
{
	#define		HCI_VERSION_H4 	4
	return HCI_VERSION_H4;
}

// Entry point of DLib
const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE = {
    sizeof(bt_vendor_interface_t),
    init,
    op,
    cleanup,
    detect_bluetooth,
    get_hci_version
};
