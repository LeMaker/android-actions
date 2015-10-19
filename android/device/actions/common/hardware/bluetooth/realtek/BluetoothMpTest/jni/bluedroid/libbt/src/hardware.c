/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Realtek Corporation
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
 *  Filename:      hardware.c
 *
 *  Description:   Contains controller-specific functions, like
 *                      firmware patch download
 *                      low power mode operations
 *
 ******************************************************************************/

#define LOG_TAG "bt_hwcfg"

#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include "bt_hci_bdroid.h"
#include "bt_vendor_rtk.h"
#include "userial.h"
#include "userial_vendor.h"
#include "upio.h"
#include <unistd.h>

#include "bt_vendor_lib.h"
#include "hci.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/
#define RTK_VERSION "2.11"

#ifndef BTHW_DBG
#define BTHW_DBG FALSE
#endif

#if (BTHW_DBG == TRUE)
#define BTHWDBG(param, ...) {ALOGD(param, ## __VA_ARGS__);}
#else
#define BTHWDBG(param, ...) {}
#endif


#define HCI_UART_H4 0
#define HCI_UART_3WIRE  2


#define FIRMWARE_DIRECTORY  "/system/etc/firmware/%s"
#define BT_CONFIG_DIRECTORY "/system/etc/firmware/%s"
#define PATCH_DATA_FIELD_MAX_SIZE     252

#define BT_CAL_DIRECTORY "/data/misc/bluedroid/"

struct patch_struct {
    int nTxIndex;   // current sending pkt number
    int nTotal;     // total pkt number
    int nRxIndex;   // ack index from board
    int nNeedRetry; // if no response from board
};
static struct patch_struct rtk_patch;


#define RTK_VENDOR_CONFIG_MAGIC 0x8723ab55
struct rtk_bt_vendor_config_entry{
    uint16_t offset;
    uint8_t entry_len;
    uint8_t entry_data[0];
} __attribute__ ((packed));


struct rtk_bt_vendor_config{
    uint32_t signature;
    uint16_t data_len;
    struct rtk_bt_vendor_config_entry entry[0];
} __attribute__ ((packed));

int gHwFlowControlEnable = 1;
int gNeedToSetHWFlowControl = 0;
int gFinalSpeed = 0;

#define FW_PATCHFILE_EXTENSION      ".hcd"
#define FW_PATCHFILE_EXTENSION_LEN  4
#define FW_PATCHFILE_PATH_MAXLEN    248 /* Local_Name length of return of
                                           HCI_Read_Local_Name */

#define HCI_CMD_MAX_LEN             258

#define HCI_RESET                               0x0C03

#define HCI_VSC_UPDATE_BAUDRATE                 0xFC17
#define HCI_VSC_DOWNLOAD_FW_PATCH               0xFC20
#define HCI_VENDOR_READ_RTK_ROM_VERISION        0xFC6D
#define HCI_READ_LMP                            0x1001

#define ROM_LMP_8723a               0x1200
#define ROM_LMP_8723b               0x8723
#define ROM_LMP_8821a               0X8821
#define ROM_LMP_8761a               0X8761


#define HCI_VSC_H5_INIT                0xFCEE


#define HCI_EVT_CMD_CMPL_STATUS_RET_BYTE        5
#define HCI_EVT_CMD_CMPL_LOCAL_NAME_STRING      6
#define HCI_EVT_CMD_CMPL_LOCAL_BDADDR_ARRAY     6
#define HCI_EVT_CMD_CMPL_OPCODE                 3
#define LPM_CMD_PARAM_SIZE                      12
#define UPDATE_BAUDRATE_CMD_PARAM_SIZE          6
#define HCI_CMD_PREAMBLE_SIZE                   3
#define HCD_REC_PAYLOAD_LEN_BYTE                2
#define BD_ADDR_LEN                             6
#define LOCAL_NAME_BUFFER_LEN                   32
#define LOCAL_BDADDR_PATH_BUFFER_LEN            256


#define H5_SYNC_REQ_SIZE 2
#define H5_SYNC_RESP_SIZE 2
#define H5_CONF_REQ_SIZE 3
#define H5_CONF_RESP_SIZE 2

#define STREAM_TO_UINT16(u16, p) {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2;}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define UINT32_TO_STREAM(p, u32) {*(p)++ = (uint8_t)(u32); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 24);}

/******************************************************************************
**  Local type definitions
******************************************************************************/

/* Hardware Configuration State */
enum {
    HW_CFG_H5_INIT = 1,
    HW_CFG_START,
    HW_CFG_SET_UART_BAUD_HOST,//change FW baudrate
    HW_CFG_SET_UART_BAUD_CONTROLLER,//change Host baudrate
    HW_CFG_DL_FW_PATCH
};

/* h/w config control block */
typedef struct
{
    uint8_t state;                          /* Hardware configuration state */
    int     fw_fd;                          /* FW patch file fd */
    uint8_t f_set_baud_2;                   /* Baud rate switch state */
    char    local_chip_name[LOCAL_NAME_BUFFER_LEN];
} bt_hw_cfg_cb_t;

/* low power mode parameters */
typedef struct
{
    uint8_t sleep_mode;                     /* 0(disable),1(UART),9(H5) */
    uint8_t host_stack_idle_threshold;      /* Unit scale 300ms/25ms */
    uint8_t host_controller_idle_threshold; /* Unit scale 300ms/25ms */
    uint8_t bt_wake_polarity;               /* 0=Active Low, 1= Active High */
    uint8_t host_wake_polarity;             /* 0=Active Low, 1= Active High */
    uint8_t allow_host_sleep_during_sco;
    uint8_t combine_sleep_mode_and_lpm;
    uint8_t enable_uart_txd_tri_state;      /* UART_TXD Tri-State */
    uint8_t sleep_guard_time;               /* sleep guard time in 12.5ms */
    uint8_t wakeup_guard_time;              /* wakeup guard time in 12.5ms */
    uint8_t txd_config;                     /* TXD is high in sleep state */
    uint8_t pulsed_host_wake;               /* pulsed host wake if mode = 1 */
} bt_lpm_param_t;


/******************************************************************************
**  Externs
******************************************************************************/

void hw_config_cback(void *p_evt_buf);
void rtk_get_lmp_cback(void *p_evt_buf);

extern uint8_t vnd_local_bd_addr[BD_ADDR_LEN];


/******************************************************************************
**  Static variables
******************************************************************************/

static char fw_patchfile_path[256] = FW_PATCHFILE_LOCATION;
static char fw_patchfile_name[128] = { 0 };


static bt_hw_cfg_cb_t hw_cfg_cb;

static bt_lpm_param_t lpm_param =
{
    LPM_SLEEP_MODE,
    LPM_IDLE_THRESHOLD,
    LPM_HC_IDLE_THRESHOLD,
    LPM_BT_WAKE_POLARITY,
    LPM_HOST_WAKE_POLARITY,
    LPM_ALLOW_HOST_SLEEP_DURING_SCO,
    LPM_COMBINE_SLEEP_MODE_AND_LPM,
    LPM_ENABLE_UART_TXD_TRI_STATE,
    0,  /* not applicable */
    0,  /* not applicable */
    0,  /* not applicable */
    LPM_PULSED_HOST_WAKE
};

/*********************************add for multi patch start**************************/

//signature: Realtech
const uint8_t RTK_EPATCH_SIGNATURE[8]={0x52,0x65,0x61,0x6C,0x74,0x65,0x63,0x68};
//Extension Section IGNATURE:0x77FD0451
const uint8_t Extension_Section_SIGNATURE[4]={0x51,0x04,0xFD,0x77};

uint16_t project_id[]=
{
    ROM_LMP_8723a,
    ROM_LMP_8723b,
    ROM_LMP_8821a,
    ROM_LMP_8761a
};

typedef struct {
    uint16_t    prod_id;
    char        *patch_name;
    char        *config_name;
} patch_info;

static patch_info patch_table[] = {
    { ROM_LMP_8723a, "mp_rtl8723a_fw", "mp_rtl8723a_config" },    //Rtl8723AS
    { ROM_LMP_8723b, "mp_rtl8723b_fw", "mp_rtl8723b_config"},     //Rtl8723BS
    { ROM_LMP_8821a, "mp_rtl8821a_fw", "mp_rtl8821a_config"},     //Rtl8821AS
    { ROM_LMP_8761a, "mp_rtl8761a_fw", "mp_rtl8761a_config"}      //Rtl8761AW
};

struct rtk_epatch_entry{
    uint16_t chipID;
    uint16_t patch_length;
    uint32_t start_offset;
} __attribute__ ((packed));

struct rtk_epatch{
    uint8_t signature[8];
    uint32_t fm_version;
    uint16_t number_of_total_patch;
    struct rtk_epatch_entry entry[0];
} __attribute__ ((packed));

struct rtk_extension_entry{
    uint8_t opcode;
    uint8_t length;
    uint8_t *data;
} __attribute__ ((packed));

typedef enum _RTK_ROM_VERSION_CMD_STATE
{
    cmd_not_send,
    cmd_has_sent,
    cmd_sent_event_timeout,
    event_received
} RTK_ROM_VERSION_CMD_STATE;


uint8_t gEVersion = 0;
uint8_t need_download_fw = 1;
uint16_t  lmp_version = 0;
uint8_t gRom_version_cmd_state = cmd_not_send;
extern tHCI_IF *p_hci_if;

//#define BT_FW_CAL_ENABLE

#ifdef BT_FW_CAL_ENABLE

//add for FW CAL
uint8_t isFirstBoot = TRUE;
uint8_t is_first_bt_init = FALSE;

#define CAL_INQUIRY_SUCCESS     0
#define CAL_INQUIRY_UNKNOWN     1
#define CAL_INQUIRY_FAIL        2

#define BT_EFUSE_HOST_INFO_DISABLE      0x0001
#define BT_EFUSE_CAL_STS_EN_DISABLE     0x0002
#define IS_FIRST_BT_INIT_AFTER_BOOT     0x0004
#define IS_FIRST_BT_INIT                0x0008
#define IS_LAST_INQUIRY_SUCCESS         0x0010



struct _rtk_bt_cal_info_entry{
    uint16_t offset;
    uint8_t entry_len;
    uint16_t bt_cal_efuse_host_info;
    uint16_t bt_cal_efuse_cal_sts[5];
} __attribute__ ((packed));

struct _rtk_bt_cal_info_entry *rtk_bt_cal_info_entry = NULL;


void rtk_print_host_cal_info(struct _rtk_bt_cal_info_entry *cal_info_entry);
void rtk_print_host_info(uint16_t bt_cal_efuse_host_info);
void rtk_print_cal_info(uint16_t *bt_cal_efuse_cal_sts);


#endif

/*********************************add for multi patch end**************************/

patch_info* get_patch_entry(uint16_t    prod_id)
{
    patch_info  *patch_entry = NULL;
    patch_entry = patch_table;
    while (prod_id != patch_entry->prod_id)
    {
        if (0 == patch_entry->prod_id)
            break;

        patch_entry++;
    }
    return patch_entry;
}


/*******************************************************************************
**
** Function        ms_delay
**
** Description     sleep unconditionally for timeout milliseconds
**
** Returns         None
**
*******************************************************************************/
void ms_delay (uint32_t timeout)
{
    struct timespec delay;
    int err;

    if (timeout == 0)
        return;

    delay.tv_sec = timeout / 1000;
    delay.tv_nsec = 1000 * 1000 * (timeout%1000);

    /* [u]sleep can't be used because it uses SIGALRM */
    do {
        err = nanosleep(&delay, &delay);
    } while (err < 0 && errno ==EINTR);
}

/*******************************************************************************
**
** Function        line_speed_to_userial_baud
**
** Description     helper function converts line speed number into USERIAL baud
**                 rate symbol
**
** Returns         unit8_t (USERIAL baud symbol)
**
*******************************************************************************/
uint8_t line_speed_to_userial_baud(uint32_t line_speed)
{
    uint8_t baud;

    if (line_speed == 4000000)
        baud = USERIAL_BAUD_4M;
    else if (line_speed == 3000000)
        baud = USERIAL_BAUD_3M;
    else if (line_speed == 2000000)
        baud = USERIAL_BAUD_2M;
    else if (line_speed == 1500000)
        baud = USERIAL_BAUD_1_5M;
    else if (line_speed == 1000000)
        baud = USERIAL_BAUD_1M;
    else if (line_speed == 921600)
        baud = USERIAL_BAUD_921600;
    else if (line_speed == 460800)
        baud = USERIAL_BAUD_460800;
    else if (line_speed == 230400)
        baud = USERIAL_BAUD_230400;
    else if (line_speed == 115200)
        baud = USERIAL_BAUD_115200;
    else if (line_speed == 57600)
        baud = USERIAL_BAUD_57600;
    else if (line_speed == 19200)
        baud = USERIAL_BAUD_19200;
    else if (line_speed == 9600)
        baud = USERIAL_BAUD_9600;
    else if (line_speed == 1200)
        baud = USERIAL_BAUD_1200;
    else if (line_speed == 600)
        baud = USERIAL_BAUD_600;
    else
    {
        ALOGE( "userial vendor: unsupported baud speed %d", line_speed);
        baud = USERIAL_BAUD_115200;
    }

    return baud;
}

typedef struct _baudrate_ex
{
    int rtk_speed;
    int uart_speed;
}baudrate_ex;

baudrate_ex baudrates[] =
{

    {0x00006004, 921600},
    {0x05F75004, 921600},//RTL8723BS
    {0x00004003, 1500000},
    {0x04928002, 1500000},//RTL8723BS
    {0x00005002, 2000000},//same as RTL8723AS
    {0x00008001, 3000000},
    {0x04928001, 3000000},//RTL8723BS
    {0x00007001, 3500000},
    {0x052A6001, 3500000},//RTL8723BS
    {0x00005001, 4000000},//same as RTL8723AS
    {0x0000701d, 115200},
    {0x0252C002, 115200}//RTL8723BS
};

/**
* Change realtek Bluetooth speed to uart speed. It is matching in the struct baudrates:
*
* @code
* baudrate_ex baudrates[] =
* {
*   {0x7001, 3500000},
*   {0x6004, 921600},
*   {0x4003, 1500000},
*   {0x5001, 4000000},
*   {0x5002, 2000000},
*   {0x8001, 3000000},
*   {0x701d, 115200}
* };
* @endcode
*
* If there is no match in baudrates, uart speed will be set as #115200.
*
* @param rtk_speed realtek Bluetooth speed
* @param uart_speed uart speed
*
*/
static void rtk_speed_to_uart_speed(uint32_t rtk_speed, uint32_t* uart_speed)
{
    *uart_speed = 115200;

    uint8_t i;
    for (i=0; i< sizeof(baudrates)/sizeof(baudrate_ex); i++)
    {
        if (baudrates[i].rtk_speed == rtk_speed){
            *uart_speed = baudrates[i].uart_speed;
        return;
        }
    }
    return;
}

/**
* Change uart speed to realtek Bluetooth speed. It is matching in the struct baudrates:
*
* @code
* baudrate_ex baudrates[] =
* {
*   {0x7001, 3500000},
*   {0x6004, 921600},
*   {0x4003, 1500000},
*   {0x5001, 4000000},
*   {0x5002, 2000000},
*   {0x8001, 3000000},
*   {0x701d, 115200}
* };
* @endcode
*
* If there is no match in baudrates, realtek Bluetooth speed will be set as #0x701D.
*
* @param uart_speed uart speed
* @param rtk_speed realtek Bluetooth speed
*
*/
static inline void uart_speed_to_rtk_speed(int uart_speed, int* rtk_speed)
{
    *rtk_speed = 0x701D;

    unsigned int i;
    for (i=0; i< sizeof(baudrates)/sizeof(baudrate_ex); i++)
    {
      if (baudrates[i].uart_speed == uart_speed){
          *rtk_speed = baudrates[i].rtk_speed;
          return;
      }
    }
    return;
}


/*******************************************************************************
**
** Function         hw_config_set_bdaddr
**
** Description      Program controller's Bluetooth Device Address
**
** Returns          TRUE, if valid address is sent
**                  FALSE, otherwise
**
*******************************************************************************/
static uint8_t hw_config_set_controller_baudrate(HC_BT_HDR *p_buf, uint32_t baudrate)
{
    uint8_t retval = FALSE;
    uint8_t *p = (uint8_t *) (p_buf + 1);

    UINT16_TO_STREAM(p, HCI_VSC_UPDATE_BAUDRATE);
    *p++ = 4; /* parameter length */
    UINT32_TO_STREAM(p, baudrate);


    p_buf->len = HCI_CMD_PREAMBLE_SIZE + 4;
    hw_cfg_cb.state = HW_CFG_SET_UART_BAUD_HOST;

    retval = bt_vendor_cbacks->xmit_cb(HCI_VSC_UPDATE_BAUDRATE, p_buf, \
                                 hw_config_cback);

    return (retval);
}

static const char *get_firmware_name()
{
    static char firmware_file_name[PATH_MAX] = {0};
    sprintf(firmware_file_name, FIRMWARE_DIRECTORY"rtlbtmp_fw");
    return firmware_file_name;
}

uint32_t rtk_parse_config_file(unsigned char* config_buf, size_t filelen, char bt_addr[6])
{
    struct rtk_bt_vendor_config* config = (struct rtk_bt_vendor_config*) config_buf;
    uint16_t config_len = config->data_len, temp = 0;
    struct rtk_bt_vendor_config_entry* entry = config->entry;
    unsigned int i = 0;
    uint32_t baudrate = 0;



    bt_addr[0] = 0; //reset bd addr byte 0 to zero

    if (config->signature != RTK_VENDOR_CONFIG_MAGIC)
    {
        ALOGE("config signature magic number(%x) is not set to RTK_VENDOR_CONFIG_MAGIC", config->signature);
        return 0;
    }

    if (config_len != filelen - sizeof(struct rtk_bt_vendor_config))
    {
        ALOGE("config len(%x) is not right(%x)", config_len, filelen-sizeof(struct rtk_bt_vendor_config));
        return 0;
    }

    for (i=0; i<config_len;)
    {

        switch(entry->offset)
        {
            //20130621 morgan modify for bt_addr cannot change issue
            /*
            case 0x3c:
            {
                int j=0;
                for (j=0; j<entry->entry_len; j++)
                    entry->entry_data[j] = bt_addr[entry->entry_len - 1 - j];
                break;
            }
            */
            case 0xc:
            {
                baudrate = *(uint32_t*)entry->entry_data;
                if (entry->entry_len >= 12) //0ffset 0x18 - 0xc
                {
                    gNeedToSetHWFlowControl =1;
                    gHwFlowControlEnable = (entry->entry_data[12] & 0x4) ? 1:0; //0x18 byte bit2
                }
                else
                {
                    gNeedToSetHWFlowControl = 0;
                }
                ALOGI("config baud rate to :%08x, hwflowcontrol:%x, %x", baudrate, entry->entry_data[12], gHwFlowControlEnable);
                break;
            }
            default:
                ALOGI("config offset(%x),length(%x)", entry->offset, entry->entry_len);
                break;
        }
        temp = entry->entry_len + sizeof(struct rtk_bt_vendor_config_entry);
        i += temp;
        entry = (struct rtk_bt_vendor_config_entry*)((uint8_t*)entry + temp);
    }


    return baudrate;
}

#ifdef BT_FW_CAL_ENABLE

uint32_t rtk_get_bt_cal_info(unsigned char** cal_buf)
{
    char bt_cal_file_name[PATH_MAX] = {0};

    struct stat st;
    size_t filelen;
    ssize_t ret_len = 0;

    int fd;


    sprintf(bt_cal_file_name, BT_CAL_DIRECTORY"rtlbt_cal");

    if (stat(bt_cal_file_name, &st) < 0)
    {
        ALOGE("can't access bt cal file:%s, errno:%d\n", bt_cal_file_name, errno);
        return -1;
    }

    filelen = st.st_size;
    ALOGI("CAL File Length = %d", filelen);

    if ((fd = open(bt_cal_file_name, O_RDONLY)) < 0)
    {
        ALOGE("Can't open bt cal file, errno = %d", errno);
        return -1;
    }


    if ((*cal_buf = malloc(filelen)) == NULL)
    {
        ALOGE("malloc buffer for cal file fail(%x)\n", filelen);
        return -1;
    }

    //
    //we may need to parse this config file.
    //for easy debug, only get need data.
    ret_len = read(fd, *cal_buf, filelen);
    if ( ret_len < (ssize_t)filelen) {
        ALOGE("Can't load bt cal file, ret_len(%d), errno = %d", ret_len, errno);
        free(*cal_buf);
        close(fd);
        return -1;
    }

    close(fd);
    return filelen;
}

uint32_t rtk_set_bt_cal_info(unsigned char* cal_buf, uint32_t length)
{
    char bt_cal_file_name[PATH_MAX] = {0};

    struct stat st;
    size_t filelen;
    int fd;


    sprintf(bt_cal_file_name, BT_CAL_DIRECTORY"rtlbt_cal");

    if (stat(bt_cal_file_name, &st) < 0)
    {
        ALOGE("can't access bt cal file:%s, errno:%d\n", bt_cal_file_name, errno);
        filelen = length;
    }
    else
    {
        filelen = st.st_size;
    }



    ALOGI("CAL File Length = %d", filelen);

    if ((fd = open(bt_cal_file_name, O_RDWR | O_CREAT, 0666)) < 0)
    {
        ALOGE("Can't open bt cal file, errno = %d", errno);
        return -1;
    }

    //
    //we may need to parse this config file.
    //for easy debug, only get need data.

    if (write(fd, cal_buf, filelen) < (ssize_t)filelen) {
        ALOGE("Can't load bt cal file, errno = %d", errno);

        close(fd);
        return -1;
    }

    close(fd);
    return filelen;
}

#endif

uint32_t rtk_get_bt_config(unsigned char** config_buf,
        uint32_t* config_baud_rate, char * config_file_short_name)
{
    char bt_config_file_name[PATH_MAX] = {0};
    uint8_t* bt_addr_temp = NULL;
    char bt_addr[6]={0x00, 0xe0, 0x4c, 0x88, 0x88, 0x88};
    struct stat st;
    size_t filelen;
    int fd;
    FILE* file = NULL;

    sprintf(bt_config_file_name, BT_CONFIG_DIRECTORY, config_file_short_name);
    ALOGI("BT config file: %s", bt_config_file_name);

    if (stat(bt_config_file_name, &st) < 0)
    {
        ALOGE("can't access bt config file:%s, errno:%d\n", bt_config_file_name, errno);
        return -1;
    }

    filelen = st.st_size;

    if ((fd = open(bt_config_file_name, O_RDONLY)) < 0)
    {
        ALOGE("Can't open bt config file");
        return -1;
    }

    if ((*config_buf = malloc(filelen)) == NULL)
    {
        ALOGE("malloc buffer for config file fail(%x)\n", filelen);
        return -1;
    }

    //
    //we may need to parse this config file.
    //for easy debug, only get need data.

    if (read(fd, *config_buf, filelen) < (ssize_t)filelen) {
        ALOGE("Can't load bt config file");
        free(*config_buf);
        close(fd);
        return -1;
    }

    *config_baud_rate = rtk_parse_config_file(*config_buf, filelen, bt_addr);
    ALOGI("Get config baud rate from config file:%x", *config_baud_rate);

    close(fd);
    return filelen;
}

int rtk_get_bt_firmware(uint8_t** fw_buf, size_t addi_len, char* fw_short_name)
{
    const char *filename[PATH_MAX] = {0};
    struct stat st;
    int fd = -1;
    size_t fwsize = 0;
       size_t buf_size = 0;

    sprintf(filename, FIRMWARE_DIRECTORY, fw_short_name);
    ALOGI("BT fw file: %s", (char*)filename);

    if (stat(filename, &st) < 0) {
        ALOGE("Can't access firmware, errno:%d", errno);
        return -1;
    }

    fwsize = st.st_size;
    buf_size = fwsize + addi_len;

    if ((fd = open(filename, O_RDONLY)) < 0) {
        ALOGE("Can't open firmware, errno:%d", errno);
        return -1;
    }

    if (!(*fw_buf = malloc(buf_size))) {
        ALOGE("Can't alloc memory for fw&config, errno:%d", errno);
        if (fd >= 0)
        close(fd);
        return -1;
    }

    if (read(fd, *fw_buf, fwsize) < (ssize_t) fwsize) {
        free(*fw_buf);
        *fw_buf = NULL;
        if (fd >= 0)
        close(fd);
        return -1;
    }

    if (fd >= 0)
        close(fd);

    ALOGI("Load FW OK");
    return buf_size;
}


static int hci_download_patch_h4(HC_BT_HDR *p_buf, int index, uint8_t *data, int len)
{
    uint8_t retval = FALSE;
    uint8_t *p = (uint8_t *) (p_buf + 1);

    UINT16_TO_STREAM(p, HCI_VSC_DOWNLOAD_FW_PATCH);
    *p++ = 1 + len;  /* parameter length */
    *p++ = index;
    memcpy(p, data, len);


    p_buf->len = HCI_CMD_PREAMBLE_SIZE + 1+len;

    hw_cfg_cb.state = HW_CFG_DL_FW_PATCH;

    retval = bt_vendor_cbacks->xmit_cb(HCI_VSC_DOWNLOAD_FW_PATCH, p_buf, \
                                 hw_config_cback);
    return retval;
}

static void rtk_download_fw_config(HC_BT_HDR *p_buf, uint8_t* buf, size_t filesize, int is_sent_changerate, int proto)
{

    uint8_t iCurIndex = 0;
    uint8_t iCurLen = 0;
    uint8_t iEndIndex = 0;
    uint8_t iLastPacketLen = 0;
    uint8_t iAdditionPkt = 0;
    uint8_t iTotalIndex = 0;

    unsigned char *bufpatch = NULL;

    iEndIndex = (uint8_t)((filesize-1)/PATCH_DATA_FIELD_MAX_SIZE);
    iLastPacketLen = (filesize)%PATCH_DATA_FIELD_MAX_SIZE;

    if (is_sent_changerate)
        iAdditionPkt = (iEndIndex+2)%8?(8-(iEndIndex+2)%8):0;
    else
        iAdditionPkt = (iEndIndex+1)%8?(8-(iEndIndex+1)%8):0;

    iTotalIndex = iAdditionPkt + iEndIndex;
    rtk_patch.nTotal = iTotalIndex; //init TotalIndex

    ALOGI("iEndIndex:%d  iLastPacketLen:%d iAdditionpkt:%d\n", iEndIndex, iLastPacketLen, iAdditionPkt);

    if (iLastPacketLen == 0)
        iLastPacketLen = PATCH_DATA_FIELD_MAX_SIZE;

    bufpatch = buf;

    int i;
    for (i=0; i<=iTotalIndex; i++) {
        if (iCurIndex < iEndIndex) {
            iCurIndex = iCurIndex&0x7F;
            iCurLen = PATCH_DATA_FIELD_MAX_SIZE;
        }
        else if (iCurIndex == iEndIndex) {//send last data packet
            if (iCurIndex == iTotalIndex)
                iCurIndex = iCurIndex | 0x80;
            else
            iCurIndex = iCurIndex&0x7F;
            iCurLen = iLastPacketLen;
        }
        else if (iCurIndex < iTotalIndex) {
            iCurIndex = iCurIndex&0x7F;
            bufpatch = NULL;
            iCurLen = 0;
            //printf("addtional packet index:%d  iCurIndex:%d\n", i, iCurIndex);
        }
        else {  //send end packet
            bufpatch = NULL;
            iCurLen = 0;
            iCurIndex = iCurIndex|0x80;
            //printf("end packet index:%d iCurIndex:%d\n", i, iCurIndex);
        }

        if (iCurIndex & 0x80)
            ALOGI("Send FW last command");

        if (proto == HCI_UART_H4) {
            iCurIndex = hci_download_patch_h4(p_buf, iCurIndex, bufpatch, iCurLen);
            if ((iCurIndex != i) && (i != rtk_patch.nTotal))
            {
                // check index but ignore last pkt
                ALOGE("index mismatch i:%d iCurIndex:%d, patch fail\n", i, iCurIndex);
                return;
            }
        }
        else if(proto == HCI_UART_3WIRE)
            //hci_download_patch(fd, iCurIndex, bufpatch, iCurLen);
            ALOGI("iHCI_UART_3WIRE");

        if (iCurIndex < iEndIndex) {
            bufpatch += PATCH_DATA_FIELD_MAX_SIZE;
        }
        iCurIndex ++;
    }

    //set last ack packet down
    if (proto == HCI_UART_3WIRE)
    {
        //rtk_send_pure_ack_down(fd);
    }
}


void rtk_get_eversion_timeout(int sig)
{
    ALOGE("RTK get eversion timeout\n");
    need_download_fw = 0;
    bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);
}


/**
* Send vendor cmd to get eversion: 0xfc6d
* If Rom code does not support this cmd, use default a.
*/
void rtk_get_eversion(void)
{
    HC_BT_HDR  *p_buf=NULL;

    p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
                                                    HCI_CMD_MAX_LEN);
    p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
    p_buf->offset = 0;
    p_buf->len = 0;
    p_buf->layer_specific = 0;

    ALOGI("bt vendor lib: Get eversion");
    struct sigaction sa;
    uint8_t *p = (uint8_t *) (p_buf + 1);

    UINT16_TO_STREAM(p, HCI_VENDOR_READ_RTK_ROM_VERISION);

    *p++ = 0; /* parameter length */

    p_buf->len = HCI_CMD_PREAMBLE_SIZE;

    bt_vendor_cbacks->xmit_cb(HCI_VENDOR_READ_RTK_ROM_VERISION, p_buf, \
                                 hw_config_cback);

    gRom_version_cmd_state = cmd_has_sent;
    ALOGI("RTK send HCI_VENDOR_READ_RTK_ROM_VERISION_Command\n");

    alarm(0);
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_NOCLDSTOP;
    sa.sa_handler = rtk_get_eversion_timeout;
    sigaction(SIGALRM, &sa, NULL);
    alarm(1);

}

void rtk_get_lmp_timeout(int sig)
{
    ALOGE("RTK get lmp timeout\n");
    need_download_fw = 0;
    bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);
}

void rtk_get_lmp()
{
    HC_BT_HDR  *p_buf=NULL;
    struct sigaction sa;

    p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
                                                        HCI_CMD_MAX_LEN);
    p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
    p_buf->offset = 0;
    p_buf->len = 0;
    p_buf->layer_specific = 0;

    ALOGI("bt vendor lib: Get lmp");
    uint8_t *p = (uint8_t *) (p_buf + 1);

    UINT16_TO_STREAM(p, HCI_READ_LMP);

    *p++ = 0; /* parameter length */

    p_buf->len = HCI_CMD_PREAMBLE_SIZE;

    bt_vendor_cbacks->xmit_cb(HCI_READ_LMP, p_buf, rtk_get_lmp_cback);

    gRom_version_cmd_state = cmd_has_sent;
    ALOGI("RTK send HCI_READ_LMP_Command \n");

    alarm(0);
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_NOCLDSTOP;
    sa.sa_handler = rtk_get_lmp_timeout;
    sigaction(SIGALRM, &sa, NULL);
    alarm(1);

}

void rtk_get_lmp_cback(void *p_mem)
{
    HC_BT_HDR *p_evt_buf = (HC_BT_HDR *) p_mem;
    uint8_t   *p, status;
    gRom_version_cmd_state = event_received;
    alarm(0);

    status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE);

    if (status == 0)
    {
        p = (uint8_t *)(p_evt_buf + 1) + LPM_CMD_PARAM_SIZE;
        STREAM_TO_UINT16(lmp_version,p);
        ALOGI("lmp_version = %x", lmp_version);
        if(lmp_version == ROM_LMP_8723a)
        {
            hw_config_cback(NULL);
        }
        else
        {
            rtk_get_eversion();
        }
    }

    if (bt_vendor_cbacks)
    {
        bt_vendor_cbacks->lpm_cb(status);
        bt_vendor_cbacks->dealloc(p_evt_buf);
    }

}

/*******************************************************************************
**
** Function         hw_config_cback
**
** Description      Callback function for controller configuration
**
** Returns          None
**
*******************************************************************************/
void hw_config_cback(void *p_mem)
{
    HC_BT_HDR   *p_evt_buf = NULL;
    char    *p_name = NULL;
    char    *p_tmp = NULL;
    uint8_t     *p = NULL;
    uint8_t     status = 0;
    uint16_t    opcode = 0;
    HC_BT_HDR  *p_buf=NULL;
    uint8_t     is_proceeding = FALSE;
    int         i = 0;

    static int buf_len = 0;
    static uint8_t* buf = NULL;
    static uint32_t baudrate = 0;

    static uint8_t iCurIndex = 0;
    static uint8_t iCurLen = 0;
    static uint8_t iEndIndex = 0;
    static uint8_t iLastPacketLen = 0;
    static uint8_t iAdditionPkt = 0;
    static uint8_t iTotalIndex = 0;
    static unsigned char *bufpatch = NULL;

    static uint8_t iIndexRx = 0;

    uint8_t     *ph5_ctrl_pkt = NULL;
    uint16_t     h5_ctrl_pkt = 0;

//add for multi epatch
    uint8_t is_multi_epatch = 0;
    uint8_t* epatch_buf = NULL;
    int epatch_length = -1;
    struct rtk_epatch* epatch_info = NULL;
    struct rtk_epatch_entry current_entry = {0};
    patch_info* prtk_patch_file_info = NULL;

#ifdef BT_FW_CAL_ENABLE
    int iBtCalLen = 0;
    uint8_t bt_cal_ext_id = 0;
    uint8_t bt_cal_ext_length = 0;
#endif

#if (USE_CONTROLLER_BDADDR == TRUE)
    const uint8_t null_bdaddr[BD_ADDR_LEN] = {0,0,0,0,0,0};
#endif

    if(p_mem != NULL)
    {
        p_evt_buf = (HC_BT_HDR *) p_mem;
        status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE);
        p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OPCODE;
        STREAM_TO_UINT16(opcode,p);
    }


    /* Ask a new buffer big enough to hold any HCI commands sent in here */
    //if ((status == 0) && bt_vendor_cbacks)
    if (bt_vendor_cbacks) //a fc6d status==1
    {
        p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
                                                       HCI_CMD_MAX_LEN);
    }

    if (p_buf != NULL)
    {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->len = 0;
        p_buf->layer_specific = 0;

        p = (uint8_t *) (p_buf + 1);

        ALOGI("hw_cfg_cb.state = %i", hw_cfg_cb.state);
        switch (hw_cfg_cb.state)
        {
            case HW_CFG_START:
            {
                if((lmp_version != ROM_LMP_8723a)&&(p_mem != NULL))
                {
                    HC_BT_HDR   *p_evt_buf = (HC_BT_HDR *) p_mem;
                    status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE);
                    gRom_version_cmd_state = event_received;
                    alarm(0);

                    if(status == 0)
                    {
                        gEVersion = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE + 1);
                    }
                    else if(1 == status)
                    {
                        gEVersion = 0;  //default a
                    }
                    else
                    {
                        ALOGE("READ_RTK_ROM_VERISION return status error!");
                        //Need to do more
                    }
                    ALOGI("bt vendor lib: READ_RTK_ROM_VERISION status:%i, gEVersion:%i", status, gEVersion);
                }

                uint8_t*config_file_buf = NULL;
                int config_len = -1;

                ALOGI("bt vendor lib:HW_CFG_START");
                //reset all static variable here
                buf_len = -1;
                buf = NULL;
                baudrate = 0;

                iCurIndex = 0;
                iCurLen = 0;
                iEndIndex = 0;
                iLastPacketLen = 0;
                iAdditionPkt = 0;
                iTotalIndex = 0;
                bufpatch = NULL;

                iIndexRx = 0;
                gNeedToSetHWFlowControl = 0;

                //download patch

                //get efuse config file and patch code file

                prtk_patch_file_info = get_patch_entry(lmp_version);

                if(prtk_patch_file_info != NULL)
                {
                    config_len = rtk_get_bt_config(&config_file_buf, &baudrate, prtk_patch_file_info->config_name);
                }
                if (config_len < 0)
                {
                    ALOGE("Get Config file error, just use efuse settings");
                    config_len = 0;
                }
#ifdef BT_FW_CAL_ENABLE
            else
            {
                struct rtk_bt_vendor_config* config = (struct rtk_bt_vendor_config*) config_file_buf;
                config->data_len += 12 + 3;
            }
#endif

                if(prtk_patch_file_info != NULL)
                {
                    buf_len = rtk_get_bt_firmware(&epatch_buf, config_len, prtk_patch_file_info->patch_name);
                }

                if (buf_len < 0)
                {
                    ALOGE("Get BT firmware error, continue without bt firmware");
                }
                else
                {
                    if(lmp_version == ROM_LMP_8723a)
                    {
                        if(memcmp(epatch_buf, RTK_EPATCH_SIGNATURE, 8) == 0)
                        {
                            ALOGE("8723as Check signature error!");
                            need_download_fw = 0;
                        }
                        else
                        {
                            if (!(buf = malloc(buf_len))) {
                                ALOGE("Can't alloc memory for fw&config, errno:%d", errno);
                                buf_len = -1;
                            }
                            else
                            {
                                ALOGI("8723as, fw copy direct");
                                memcpy(buf,epatch_buf,buf_len);
                                free(epatch_buf);
                                epatch_buf = NULL;
                                if (config_len)
                                {
                                    memcpy(&buf[buf_len - config_len], config_file_buf, config_len);
                                }
                            }
                        }
                    }
                    else
                    {
                        struct rtk_extension_entry patch_lmp = {0};
                        //check Extension Section Field
                        if(memcmp(epatch_buf + buf_len-config_len-4 ,Extension_Section_SIGNATURE,4) != 0)
                        {
                            ALOGE("Check Extension_Section_SIGNATURE error! do not download fw");
                            need_download_fw = 0;
                        }
                        else
                        {
                            uint8_t *temp;
                            temp = epatch_buf+buf_len-config_len-5;
                            do{
                                if(*temp == 0x00)
                                {
                                    patch_lmp.opcode = *temp;
                                    patch_lmp.length = *(temp-1);
                                    if ((patch_lmp.data = malloc(patch_lmp.length)))
                                    {
                                        memcpy(patch_lmp.data,temp-2,patch_lmp.length);
                                    }
                                    ALOGI("opcode = 0x%x",patch_lmp.opcode);
                                    ALOGI("length = 0x%x",patch_lmp.length);
                                    ALOGI("data = 0x%x",*(patch_lmp.data));
                                    break;
                                }
                                temp -= *(temp-1)+2;
                            }while(*temp != 0xFF);

                            if(lmp_version != project_id[*(patch_lmp.data)])
                            {
                                ALOGE("lmp_version is %x, project_id is %x, does not match!!!",lmp_version,project_id[*(patch_lmp.data)]);
                                need_download_fw = 0;
                            }
                            else
                            {
                                ALOGI("lmp_version is %x, project_id is %x, match!",lmp_version, project_id[*(patch_lmp.data)]);

                                if(memcmp(epatch_buf, RTK_EPATCH_SIGNATURE, 8) != 0)
                                {
                                    ALOGI("Check signature error!");
                                    need_download_fw = 0;
                                }
                                else
                                {
                                    int i = 0;
                                    epatch_info = (struct rtk_epatch*)epatch_buf;
                                    ALOGI("fm_version = 0x%x",epatch_info->fm_version);
                                    ALOGI("number_of_total_patch = %d",epatch_info->number_of_total_patch);

                                    //get right epatch entry
                                    for(i; i<epatch_info->number_of_total_patch; i++)
                                    {
                                        if(*(uint16_t*)(epatch_buf+14+2*i) == gEVersion + 1)
                                        {
                                            current_entry.chipID = gEVersion + 1;
                                            current_entry.patch_length = *(uint16_t*)(epatch_buf+14+2*epatch_info->number_of_total_patch+2*i);
                                            current_entry.start_offset = *(uint32_t*)(epatch_buf+14+4*epatch_info->number_of_total_patch+4*i);
                                            break;
                                        }
                                    }
                                    ALOGI("chipID = %d",current_entry.chipID);
                                    ALOGI("patch_length = 0x%x",current_entry.patch_length);
                                    ALOGI("start_offset = 0x%x",current_entry.start_offset);

                                    //get right eversion patch: buf, buf_len
                                    buf_len = current_entry.patch_length + config_len;
#ifdef BT_FW_CAL_ENABLE
                                    //add for BT CAL

                                    rtk_get_bt_cal_info((unsigned char**)&rtk_bt_cal_info_entry);

                                    if(rtk_bt_cal_info_entry == NULL)
                                    {
                                        is_first_bt_init = TRUE;
                                        rtk_bt_cal_info_entry = malloc(sizeof(struct _rtk_bt_cal_info_entry));
                                        rtk_bt_cal_info_entry->offset = 0x01E6;
                                        rtk_bt_cal_info_entry->entry_len = 0x0C;
                                        rtk_bt_cal_info_entry->bt_cal_efuse_host_info= 0xFFEC;
                                        rtk_bt_cal_info_entry->bt_cal_efuse_cal_sts[0] = 0xFFFF;
                                        rtk_bt_cal_info_entry->bt_cal_efuse_cal_sts[1] = 0xFFFF;
                                        rtk_bt_cal_info_entry->bt_cal_efuse_cal_sts[2] = 0xFFFF;
                                        rtk_bt_cal_info_entry->bt_cal_efuse_cal_sts[3] = 0xFFFF;
                                        rtk_bt_cal_info_entry->bt_cal_efuse_cal_sts[4] = 0xFFFF;
                                    }
                                    else
                                    {
                                        is_first_bt_init = FALSE;
                                    }

                                    if(rtk_bt_cal_info_entry != NULL)
                                    {
                                        rtk_print_host_cal_info(rtk_bt_cal_info_entry);
                                        if(isFirstBoot == TRUE)
                                        {

                                            isFirstBoot = FALSE;
                                            rtk_bt_cal_info_entry->bt_cal_efuse_host_info |= IS_FIRST_BT_INIT_AFTER_BOOT;
                                            //rtk_bt_cal_info_entry->bt_cal_efuse_host_info= 0xECFF;
                                            ALOGI("isFirstBoot is TRUE");

                                        }
                                        else
                                        {
                                            rtk_bt_cal_info_entry->bt_cal_efuse_host_info &= (~IS_FIRST_BT_INIT_AFTER_BOOT);
                                            //rtk_bt_cal_info_entry->bt_cal_efuse_host_info= 0xE8FF;
                                            ALOGI("isFirstBoot is FALSE");
                                        }

                                        if(is_first_bt_init ==TRUE)
                                        {
                                            rtk_bt_cal_info_entry->bt_cal_efuse_host_info |= IS_FIRST_BT_INIT;
                                            ALOGI("is_first_bt_init is TRUE");
                                        }
                                        else
                                        {
                                            rtk_bt_cal_info_entry->bt_cal_efuse_host_info &= (~IS_FIRST_BT_INIT);
                                            ALOGI("is_first_bt_init is FALSE");
                                        }

                                    }
                                    else
                                    {

                                        ALOGI("rtk_bt_cal_info_entry malloc fail");
                                    }

                                    ALOGE("after update bt_efuse_host_info");
                                    rtk_print_host_info(rtk_bt_cal_info_entry->bt_cal_efuse_host_info);

                                    iBtCalLen = sizeof(struct _rtk_bt_cal_info_entry);
                                    buf_len = buf_len + iBtCalLen;
#endif
                                    ALOGI("buf_len = 0x%x",buf_len);

                                    if (!(buf = malloc(buf_len))) {
                                        ALOGE("Can't alloc memory for multi fw&config, errno:%d", errno);
                                        buf_len = -1;
                                    }
                                    else
                                    {
                                        memcpy(buf,&epatch_buf[current_entry.start_offset],current_entry.patch_length);
                                        memcpy(&buf[current_entry.patch_length-4],&epatch_info->fm_version,4);
                                    }
                                    free(epatch_buf);
                                    epatch_buf = NULL;

                                    if (config_len)
                                    {
#ifdef BT_FW_CAL_ENABLE
                                        memcpy(&buf[buf_len - config_len -  iBtCalLen], config_file_buf, config_len);
#else
                                        memcpy(&buf[buf_len - config_len], config_file_buf, config_len);
#endif
                                    }
#ifdef BT_FW_CAL_ENABLE
                                    if(rtk_bt_cal_info_entry != NULL)
                                    {
                                        memcpy(&buf[buf_len - iBtCalLen], rtk_bt_cal_info_entry, iBtCalLen);
                                    }

#endif
                                }
                            }
                        }
                    }
                }

                if (config_file_buf)
                free(config_file_buf);

                ALOGI("Fw:%s exists, config file:%s exists", (buf_len > 0) ? "":"not", (config_len>0)?"":"not");

                if((buf_len > 0)&&(need_download_fw))
                {
                    iEndIndex = (uint8_t)((buf_len-1)/PATCH_DATA_FIELD_MAX_SIZE);
                    iLastPacketLen = (buf_len)%PATCH_DATA_FIELD_MAX_SIZE;


                    if (baudrate && (lmp_version != ROM_LMP_8723a))
                        iAdditionPkt = (iEndIndex+4)%8?(8-(iEndIndex+4)%8):0;
                    else if((baudrate && (lmp_version == ROM_LMP_8723a))||(!baudrate && (lmp_version != ROM_LMP_8723a)))
                        iAdditionPkt = (iEndIndex+3)%8?(8-(iEndIndex+3)%8):0;
                    else
                        iAdditionPkt = (iEndIndex+2)%8?(8-(iEndIndex+2)%8):0;

                    iTotalIndex = iAdditionPkt + iEndIndex;
                    rtk_patch.nTotal = iTotalIndex; //init TotalIndex

                    ALOGI("iEndIndex:%d  iLastPacketLen:%d iAdditionpkt:%d\n", iEndIndex, iLastPacketLen, iAdditionPkt);

                    if (iLastPacketLen == 0)
                        iLastPacketLen = PATCH_DATA_FIELD_MAX_SIZE;

                    bufpatch = buf;
                }
                else
                {
                    is_proceeding = FALSE;
                    break;
                }

                if ((buf_len>0) && (config_len == 0))
                {
                    goto DOWNLOAD_FW;
                }

            }
            /* fall through intentionally */

            case HW_CFG_SET_UART_BAUD_CONTROLLER:


                ALOGI("bt vendor lib: set CONTROLLER UART baud %x", baudrate);


                is_proceeding = hw_config_set_controller_baudrate(p_buf, baudrate);


                break;

            case HW_CFG_SET_UART_BAUD_HOST:
            {
                uint32_t HostBaudRate = 0;

                ALOGI("========add delay 100 ms");
                ms_delay(100);

                /* update baud rate of host's UART port */
                rtk_speed_to_uart_speed(baudrate, &HostBaudRate);
                ALOGI("bt vendor lib: set HOST UART baud %i", HostBaudRate);
                userial_vendor_set_baud( \
                    line_speed_to_userial_baud(HostBaudRate) \
                );
                ms_delay(100);
            }
             //fall through
DOWNLOAD_FW:
            case HW_CFG_DL_FW_PATCH:

                status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE);
                ALOGI("bt vendor lib: HW_CFG_DL_FW_PATCH status:%i, opcode:%x", status, opcode);

                //recv command complete event for patch code download command
                if(opcode == HCI_VSC_DOWNLOAD_FW_PATCH){
                     iIndexRx = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE + 1);
                     ALOGI("bt vendor lib: HW_CFG_DL_FW_PATCH status:%i, iIndexRx:%i", status, iIndexRx);
                    //update buf of patch and index.
                    if (iCurIndex < iEndIndex) {
                        bufpatch += PATCH_DATA_FIELD_MAX_SIZE;
                    }
                    iCurIndex ++;
                }

                 if( (opcode ==HCI_VSC_DOWNLOAD_FW_PATCH)&&( iIndexRx&0x80 || iIndexRx == iTotalIndex) ){
#ifdef BT_FW_CAL_ENABLE
                     bt_cal_ext_id= *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE + 1 + 1);

            bt_cal_ext_length= *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE + 1 +
            1 +1);

            ALOGI("bt_cal_ext_id = %d, bt_cal_ext_length = %d", bt_cal_ext_id, bt_cal_ext_length);

            if(rtk_bt_cal_info_entry != NULL)
            {
                if(bt_cal_ext_id == 1 && bt_cal_ext_length == 10)
                {

                memcpy(rtk_bt_cal_info_entry->bt_cal_efuse_cal_sts, ((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE + 1 +
                1 +1 +1), bt_cal_ext_length);

                //cancel cal inquiry result
                rtk_bt_cal_info_entry->bt_cal_efuse_host_info &= (~IS_LAST_INQUIRY_SUCCESS);
                rtk_print_cal_info(rtk_bt_cal_info_entry->bt_cal_efuse_cal_sts);
            /*
                rtk_bt_cal_info_entry->bt_cal_efuse_cal_sts[0] = 0x1111;
                rtk_bt_cal_info_entry->bt_cal_efuse_cal_sts[1] = 0x2222;
                rtk_bt_cal_info_entry->bt_cal_efuse_cal_sts[2] = 0x3333;
                rtk_bt_cal_info_entry->bt_cal_efuse_cal_sts[3] = 0x4444;
                rtk_bt_cal_info_entry->bt_cal_efuse_cal_sts[4] = 0x5555;
            */
                ALOGI("rtk_set_bt_cal_info");
                rtk_set_bt_cal_info(rtk_bt_cal_info_entry, sizeof(struct _rtk_bt_cal_info_entry));
                free(rtk_bt_cal_info_entry);
                rtk_bt_cal_info_entry = NULL;
                }

            }


#endif
                    ALOGI("vendor lib fwcfg completed");
                    if(buf) {
                        free(buf);
                        buf = NULL;
                    }
                    bt_vendor_cbacks->dealloc(p_buf);
                    bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);

                    hw_cfg_cb.state = 0;

              if(gNeedToSetHWFlowControl)
              {
                    if(gHwFlowControlEnable)
                    {
                        userial_vendor_set_hw_fctrl(1);
                    }
                    else
                    {
                        userial_vendor_set_hw_fctrl(0);
                    }

            }

                    if (hw_cfg_cb.fw_fd != -1)
                    {
                        close(hw_cfg_cb.fw_fd);
                        hw_cfg_cb.fw_fd = -1;
                    }
                    is_proceeding = TRUE;

                    break;
                 }

                    if (iCurIndex < iEndIndex) {
                            iCurIndex = iCurIndex&0x7F;
                            iCurLen = PATCH_DATA_FIELD_MAX_SIZE;
                    }
                    else if (iCurIndex == iEndIndex) {  //send last data packet
                        if (iCurIndex == iTotalIndex)
                            iCurIndex = iCurIndex | 0x80;
                        else
                            iCurIndex = iCurIndex&0x7F;
                        iCurLen = iLastPacketLen;
                    }
                    else if (iCurIndex < iTotalIndex) {
                            iCurIndex = iCurIndex&0x7F;
                            bufpatch = NULL;
                            iCurLen = 0;
                            //printf("addtional packet index:%d  iCurIndex:%d\n", i, iCurIndex);
                    }
                    else {          //send end packet
                        bufpatch = NULL;
                        iCurLen = 0;
                        iCurIndex = iCurIndex|0x80;
                        //printf("end packet index:%d iCurIndex:%d\n", i, iCurIndex);
                    }

                    if (iCurIndex & 0x80)
                        ALOGI("Send FW last command");

                    ALOGI("iCurIndex = %i, iCurLen = %i", iCurIndex, iCurLen);

                    is_proceeding = hci_download_patch_h4(p_buf, iCurIndex, bufpatch, iCurLen);


                break;

                default:
                    break;
        } // switch(hw_cfg_cb.state)
    } // if (p_buf != NULL)

    /* Free the RX event buffer */
    if ((bt_vendor_cbacks) && (p_mem != NULL))
        bt_vendor_cbacks->dealloc(p_evt_buf);

    if (is_proceeding == FALSE)
    {
        ALOGE("vendor lib fwcfg aborted!!!");
        if (bt_vendor_cbacks)
        {
            if (p_buf != NULL)
                bt_vendor_cbacks->dealloc(p_buf);

            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_FAIL);
        }

        if (hw_cfg_cb.fw_fd != -1)
        {
            close(hw_cfg_cb.fw_fd);
            hw_cfg_cb.fw_fd = -1;
        }

        hw_cfg_cb.state = 0;
    }
}

/******************************************************************************
**   LPM Static Functions
******************************************************************************/

/*******************************************************************************
**
** Function         hw_lpm_ctrl_cback
**
** Description      Callback function for lpm enable/disable rquest
**
** Returns          None
**
*******************************************************************************/
void hw_lpm_ctrl_cback(void *p_mem)
{
    HC_BT_HDR *p_evt_buf = (HC_BT_HDR *) p_mem;
    bt_vendor_op_result_t status = BT_VND_OP_RESULT_FAIL;

    if (*((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE) == 0)
    {
        status = BT_VND_OP_RESULT_SUCCESS;
    }

    if (bt_vendor_cbacks)
    {
        bt_vendor_cbacks->lpm_cb(status);
        bt_vendor_cbacks->dealloc(p_evt_buf);
    }
}




/*****************************************************************************
**   Hardware Configuration Interface Functions
*****************************************************************************/


/*******************************************************************************
**
** Function        hw_config_start
**
** Description     Kick off controller initialization process
**
** Returns         None
**
*******************************************************************************/
void hw_config_start(void)
{
    HC_BT_HDR  *p_buf = NULL;
    uint8_t     *p;

    hw_cfg_cb.state = 0;
    hw_cfg_cb.fw_fd = -1;
    hw_cfg_cb.f_set_baud_2 = FALSE;


    /* Start from sending H5 SYNC */

    if (bt_vendor_cbacks)
    {
        p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
                                                       2);
    }

    if (p_buf)
    {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->layer_specific = 0;
        p_buf->len = 2;

        p = (uint8_t *) (p_buf + 1);
        UINT16_TO_STREAM(p, HCI_VSC_H5_INIT);

        hw_cfg_cb.state = HW_CFG_START;
        ALOGI("hw_config_start:Realtek version %s \n",RTK_VERSION);
        //bt_vendor_cbacks->xmit_cb(HCI_VSC_H5_INIT, p_buf, hw_config_cback);
        bt_vendor_cbacks->xmit_cb(HCI_VSC_H5_INIT, p_buf, rtk_get_lmp);
    }
    else
    {
        if (bt_vendor_cbacks)
        {
            ALOGE("vendor lib fw conf aborted [no buffer]");
            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_FAIL);
        }
    }
}


#ifdef BT_FW_CAL_ENABLE

void rtk_print_host_cal_info(struct _rtk_bt_cal_info_entry *cal_info_entry)
{
    if(cal_info_entry)
    {
        rtk_print_host_info(cal_info_entry->bt_cal_efuse_host_info);
        rtk_print_cal_info(cal_info_entry->bt_cal_efuse_cal_sts);
    }
}

void rtk_print_host_info(uint16_t bt_cal_efuse_host_info)
{
    ALOGI("bt_cal_efuse_host_info = 0x%04x", bt_cal_efuse_host_info);
    ALOGI("BT_EFUSE_HOST_INFO_DISABLE:  %d", bt_cal_efuse_host_info&BT_EFUSE_HOST_INFO_DISABLE);
    ALOGI("BT_EFUSE_CAL_STS_EN_DISABLE: %d",bt_cal_efuse_host_info&BT_EFUSE_CAL_STS_EN_DISABLE);
    ALOGI("IS_FIRST_BT_INIT_AFTER_BOOT: %d", bt_cal_efuse_host_info&IS_FIRST_BT_INIT_AFTER_BOOT);
    ALOGI("IS_FIRST_BT_INIT:            %d", bt_cal_efuse_host_info&IS_FIRST_BT_INIT);
    ALOGI("IS_LAST_INQUIRY_SUCCESS:     %d", bt_cal_efuse_host_info&IS_LAST_INQUIRY_SUCCESS);
}

void rtk_print_cal_info(uint16_t *bt_cal_efuse_cal_sts)
{
    if(bt_cal_efuse_cal_sts)
    {
        ALOGI("BT_CAL_STS: %04X, %04X, %04X, %04X, %04X",
        bt_cal_efuse_cal_sts[0],
        bt_cal_efuse_cal_sts[1],
        bt_cal_efuse_cal_sts[2],
        bt_cal_efuse_cal_sts[3],
        bt_cal_efuse_cal_sts[4]
        );
    }
}

#endif
