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
 *  Filename:      bluedroidtest.c
 *
 *  Description:   Bluedroid Test application
 *
 ***********************************************************************************/

#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <linux/capability.h>

#include <stdarg.h>


#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <private/android_filesystem_config.h>
#include <android/log.h>
#include <utils/Log.h>
#include <cutils/log.h>

#include <hardware/hardware.h>
#include <hardware/bluetoothmp.h>
#include "bluedroidtest.h"
#include "bluetooth_mp_opcode.h"

#ifndef MPTOOL_LOG_BUF_SIZE
#define MPTOOL_LOG_BUF_SIZE  1024
#endif
#define MPTOOL_LOG_MAX_SIZE  (MPTOOL_LOG_BUF_SIZE - 12)

#define LOGI0(t,s) __android_log_write(ANDROID_LOG_INFO, t, s)


void bt_mp_LogMsg(const char *fmt_str, ...);

void bdt_log(const char *fmt_str, ...);



/************************************************************************************
**  Constants & Macros
************************************************************************************/

#define PID_FILE "/data/.bdt_pid"

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif





/************************************************************************************
**  Local type definitions
************************************************************************************/

/************************************************************************************
**  Static variables
************************************************************************************/

static unsigned char main_done = 0;
static bt_status_t status;

/* Main API */
static bluetooth_device_t* bt_device;

const bt_interface_t* sBtInterface = NULL;

static gid_t groups[] = { AID_NET_BT, AID_INET, AID_NET_BT_ADMIN,
                          AID_SYSTEM, AID_MISC, AID_SDCARD_RW,
                          AID_NET_ADMIN, AID_VPN};

/* Set to 1 when the Bluedroid stack is enabled */
static unsigned char bt_enabled = 0;

    
/************************************************************************************
**  Static functions
************************************************************************************/

static void process_cmd(char *p, unsigned char is_job);
static void job_handler(void *param);

typedef unsigned char UINT8;

/************************************************************************************
**  Externs
************************************************************************************/

/************************************************************************************
**  Functions
************************************************************************************/
#define UINT32_TO_STREAM(p, u32) {*(p)++ = (UINT8)(u32); *(p)++ = (UINT8)((u32) >> 8); *(p)++ = (UINT8)((u32) >> 16); *(p)++ = (UINT8)((u32) >> 24);}
#define UINT24_TO_STREAM(p, u24) {*(p)++ = (UINT8)(u24); *(p)++ = (UINT8)((u24) >> 8); *(p)++ = (UINT8)((u24) >> 16);}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (UINT8)(u16); *(p)++ = (UINT8)((u16) >> 8);}
#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (UINT8)(u8);}

/************************************************************************************
**  Shutdown helper functions
************************************************************************************/

static void bdt_shutdown(void)
{
    bt_mp_LogMsg("shutdown bdroid test app\n");
    main_done = 1;
}


/*****************************************************************************
** Android's init.rc does not yet support applying linux capabilities
*****************************************************************************/

static void config_permissions(void)
{
    struct __user_cap_header_struct header;
    struct __user_cap_data_struct cap;

    bt_mp_LogMsg("set_aid_and_cap : pid %d, uid %d gid %d\n", getpid(), getuid(), getgid());

    header.pid = 0;

    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);

    setuid(AID_BLUETOOTH);
    setgid(AID_BLUETOOTH);

    header.version = _LINUX_CAPABILITY_VERSION;

    cap.effective = cap.permitted =  cap.inheritable =
                    1 << CAP_NET_RAW |
                    1 << CAP_NET_ADMIN |
                    1 << CAP_NET_BIND_SERVICE |
                    1 << CAP_SYS_RAWIO |
                    1 << CAP_SYS_NICE |
                    1 << CAP_SETGID;

    capset(&header, &cap);
    setgroups(sizeof(groups)/sizeof(groups[0]), groups);
}




static void hex_dump(char *msg, void *data, int size, int trunc)
{
    unsigned char *p = data;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};

    bt_mp_LogMsg("%s  \n", msg);

    /* truncate */
    if(trunc && (size>32))
        size = 32;

    for(n=1;n<=size;n++) {
        if (n%16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x",
               ((unsigned int)p-(unsigned int)data) );
        }

        c = *p;
        if (isalnum(c) == 0) {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);

        if(n%16 == 0) {
            /* line completed */
            bt_mp_LogMsg("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;
        } else if(n%8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
        }
        p++; /* next byte */
    }

    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        bt_mp_LogMsg("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
    }
}

/*******************************************************************************
 ** Console helper functions
 *******************************************************************************/

void skip_blanks(char **p)
{
  while (**p == ' ')
    (*p)++;
}

uint32_t get_int(char **p, int DefaultValue)
{
  uint32_t Value = 0;
  unsigned char   UseDefault;

  UseDefault = 1;
  skip_blanks(p);

  while ( ((**p)<= '9' && (**p)>= '0') )
    {
      Value = Value * 10 + (**p) - '0';
      UseDefault = 0;
      (*p)++;
    }

  if (UseDefault)
    return DefaultValue;
  else
    return Value;
}

int get_signed_int(char **p, int DefaultValue)
{
  int    Value = 0;
  unsigned char   UseDefault;
  unsigned char  NegativeNum = 0;

  UseDefault = 1;
  skip_blanks(p);

  if ( (**p) == '-')
    {
      NegativeNum = 1;
      (*p)++;
    }
  while ( ((**p)<= '9' && (**p)>= '0') )
    {
      Value = Value * 10 + (**p) - '0';
      UseDefault = 0;
      (*p)++;
    }

  if (UseDefault)
    return DefaultValue;
  else
    return ((NegativeNum == 0)? Value : -Value);
}

void get_str(char **p, char *Buffer)
{
  skip_blanks(p);

  while (**p != 0 && **p != ' ')
    {
      *Buffer = **p;
      (*p)++;
      Buffer++;
    }

  *Buffer = 0;
}

uint32_t get_hex(char **p, int DefaultValue)
{
  uint32_t Value = 0;
  unsigned char   UseDefault;

  UseDefault = 1;
  skip_blanks(p);

  while ((**p == '0') && ( (*(*p+1) == 'x') ||(*(*p+1) == 'X') ))
    (*p) = (*p)+2;

  while ( ((**p)<= '9' && (**p)>= '0') ||
          ((**p)<= 'f' && (**p)>= 'a') ||
          ((**p)<= 'F' && (**p)>= 'A') )
    {
      if (**p >= 'a')
        Value = Value * 16 + (**p) - 'a' + 10;
      else if (**p >= 'A')
        Value = Value * 16 + (**p) - 'A' + 10;
      else
        Value = Value * 16 + (**p) - '0';
      UseDefault = 0;
      (*p)++;
    }

  if (UseDefault)
    return DefaultValue;
  else
    return Value;
}

void get_bdaddr(const char *str, bt_bdaddr_t *bd) {
    char *d = ((char *)bd), *endp;
    int i;
    for(i = 0; i < 6; i++) {
        *d++ = strtol(str, &endp, 16);
        if (*endp != ':' && i != 5) {
            memset(bd, 0, sizeof(bt_bdaddr_t));
            return;
        }
        str = endp + 1;
    }
}

#define is_cmd(str) ((strlen(str) == strlen(cmd)) && strncmp((const char *)&cmd, str, strlen(str)) == 0)
#define if_cmd(str)  if (is_cmd(str))

typedef void (t_console_cmd_handler) (char *p);

typedef struct {
    const char *name;
    t_console_cmd_handler *handler;
    const char *help;
    unsigned char is_job;
} t_cmd;

t_console_cmd_handler *mp_cur_handler = NULL;

const t_cmd console_cmd_list[];
static int console_cmd_maxlen = 0;

static void cmdjob_handler(void *param)
{
    char *job_cmd = (char*)param;

    bt_mp_LogMsg("cmdjob starting (%s)\n", job_cmd);

    process_cmd(job_cmd, 1);

    bt_mp_LogMsg("cmdjob terminating\n");

    free(job_cmd);
}

static int create_cmdjob(char *cmd)
{
    pthread_t thread_id;
    char *job_cmd;

    job_cmd = malloc(strlen(cmd)+1); /* freed in job handler */
    strcpy(job_cmd, cmd);

    if (pthread_create(&thread_id, NULL,
                       (void*)cmdjob_handler, (void*)job_cmd)!=0)
      perror("pthread_create");

    return 0;
}

/*******************************************************************************
 ** Load stack lib
 *******************************************************************************/

int HAL_load(void)
{
    int err = 0;

    hw_module_t* module;
    hw_device_t* device;

    bt_mp_LogMsg("Loading HAL lib + extensions\n");

    err = hw_get_module("bluetoothmp", (hw_module_t const**)&module);
    if (err == 0)
    {
        err = module->methods->open(module, "bluetoothmp", &device);
        if (err == 0) {
            bt_device = (bluetooth_device_t *)device;
            sBtInterface = bt_device->get_bluetooth_interface();
        }
    }

    bt_mp_LogMsg("HAL library loaded (%s)\n", strerror(err));

    return err;
}

int HAL_unload(void)
{
    int err = 0;

    bt_mp_LogMsg("Unloading HAL lib\n");

    sBtInterface = NULL;

    bt_mp_LogMsg("HAL library unloaded (%s)\n", strerror(err));

    return err;
}

/*******************************************************************************
 ** HAL test functions & callbacks
 *******************************************************************************/

void setup_test_env(void)
{
    int i = 0;

    while (console_cmd_list[i].name != NULL)
    {
        console_cmd_maxlen = MAX(console_cmd_maxlen, (int)strlen(console_cmd_list[i].name));
        i++;
    }
}

void check_return_status(bt_status_t status)
{
    if (status != BT_STATUS_SUCCESS)
    {

    }
    else
    {

    }
}

static void adapter_state_changed(bt_state_t state)
{
    bt_mp_LogMsg("ADAPTER STATE UPDATED : %s\n", (state == BT_STATE_OFF)?"OFF":"ON");
    if (state == BT_STATE_ON) {
        bdt_log("%s%s%x\n", STR_BT_MP_ENABLE, STR_BT_MP_RX_RESULT_DELIM, BT_FUNCTION_SUCCESS);
        bt_enabled = 1;
    } else {
        bt_enabled = 0;
        bdt_log("%s%s%x\n", STR_BT_MP_ENABLE, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_DISABLE);
    }

}
//bufer will be free

static void dut_mode_recv(uint8_t evtcode, char *buf)
{
    bdt_log(buf);
}

static bt_callbacks_t bt_callbacks = {
    sizeof(bt_callbacks_t),
    adapter_state_changed,
    NULL, /* thread_evt_cb */
    dut_mode_recv, /*dut_mode_recv_cb */
};

void bdt_init(void)
{
    bt_mp_LogMsg("INIT BT\n");
    status = sBtInterface->init(&bt_callbacks);
    check_return_status(status);
}

void bdt_enable(void)
{
    bt_mp_LogMsg("ENABLE BT\n");
    if (bt_enabled) {
        bt_mp_LogMsg("Bluetooth is already enabled\n");
        bdt_log("%s%s%x\n", STR_BT_MP_ENABLE, STR_BT_MP_RX_RESULT_DELIM, BT_FUNCTION_SUCCESS);
        return;
    }
    status = sBtInterface->init(&bt_callbacks);
    status = sBtInterface->enable();
    
    check_return_status(status);
}

void bdt_disable(void)
{
    bt_mp_LogMsg("DISABLE BT\n");
    if (!bt_enabled) {
        bt_mp_LogMsg("Bluetooth is already disabled\n");
        bdt_log("%s%s%x\n", STR_BT_MP_DISABLE, STR_BT_MP_RX_RESULT_DELIM, BT_FUNCTION_SUCCESS);
        return;
    }
    status = sBtInterface->disable();
    sBtInterface->cleanup();
    bt_enabled = 0;
    check_return_status(status);
        bdt_log("%s%s%x\n", STR_BT_MP_DISABLE, STR_BT_MP_RX_RESULT_DELIM, BT_FUNCTION_SUCCESS);
}
void bdt_dut_mode_configure(char *p)
{
    int32_t mode = -1;

    bt_mp_LogMsg("BT DUT MODE CONFIGURE\n");
    if (!bt_enabled) {
        bt_mp_LogMsg("Bluetooth must be enabled for test_mode to work.\n");
        bdt_log("%s%s%x\n", STR_BT_MP_DUT_MODE, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_DISABLE);
        return;
    }

    status = sBtInterface->hal_mp_op_send(BT_MP_OP_HCI_SEND_CMD, p, 0);

    check_return_status(status);
}


void bdt_hci(char *p)
{
    int i;

    uint32_t tmp;
    uint16_t opcode;
    uint8_t buf[260];
    uint8_t len;

    if (!bt_enabled) {
        bt_mp_LogMsg("Bluetooth must be enabled for test:%s\n", STR_BT_MP_HCI_CMD);
        bdt_log("%s%s%x\n", STR_BT_MP_HCI_CMD, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_DISABLE);
        return;
    }

     status = sBtInterface->hal_mp_op_send(BT_MP_OP_HCI_SEND_CMD, p, 0);

    check_return_status(status);
}

void bdt_GetPara(char *p)
{
    if (!bt_enabled) {
        bt_mp_LogMsg("Bluetooth must be enabled for test:%s\n", STR_BT_MP_GET_PARA);        
        bdt_log("%s%s%x\n", STR_BT_MP_GET_PARA, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_DISABLE);
        return;
    }

    status = sBtInterface->hal_mp_op_send(BT_MP_OP_USER_DEF_GetPara, p, 0);

    check_return_status(status);

}

void bdt_SetHit(char *p)
{
    if (!bt_enabled) {
        bt_mp_LogMsg("Bluetooth must be enabled for test:%s\n", STR_BT_MP_SET_HIT);        
        bdt_log("%s%s%x\n", STR_BT_MP_SET_HIT, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_DISABLE);
        return;
    }
    
//    BT_SetHit(&BtModuleMemory, p);    
    status = sBtInterface->hal_mp_op_send(BT_MP_OP_USER_DEF_SetHit, p, 0);
}

void bdt_SetPara1(char *p)
{
    if (!bt_enabled) {
        bt_mp_LogMsg("Bluetooth must be enabled for test:%s\n", STR_BT_MP_SET_PARA1);        
        bdt_log("%s%s%x\n", STR_BT_MP_SET_PARA1, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_DISABLE);
        return;
    }
    
//    BT_SetPara1(&BtModuleMemory, p);
    status = sBtInterface->hal_mp_op_send(BT_MP_OP_USER_DEF_SetPara1, p, 0);
}

void bdt_SetPara2(char *p)
{
    if (!bt_enabled) {
        bt_mp_LogMsg("Bluetooth must be enabled for test:%s\n", STR_BT_MP_SET_PARA2);        
        bdt_log("%s%s%x\n", STR_BT_MP_SET_PARA2, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_DISABLE);
        return;
    }
    
//    BT_SetPara2(&BtModuleMemory, p);
    status = sBtInterface->hal_mp_op_send(BT_MP_OP_USER_DEF_SetPara2, p, 0);
}


void bdt_SetGainTable(char *p)
{
    if (!bt_enabled) {
        bt_mp_LogMsg("Bluetooth must be enabled for test:%s\n", STR_BT_MP_SET_GAIN_TABLE);
        bdt_log("%s%s%x\n", STR_BT_MP_SET_GAIN_TABLE, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_DISABLE);
        return;
    }

//    BT_SetGainTable(&BtModuleMemory, p);
    status = sBtInterface->hal_mp_op_send(BT_MP_OP_USER_DEF_SetGainTable, p, 0);
}

void bdt_SetDacTable(char *p)
{
    if (!bt_enabled) {
        bt_mp_LogMsg("Bluetooth must be enabled for test:%s\n", STR_BT_MP_SET_DAC_TABLE);
        bdt_log("%s%s%x\n", STR_BT_MP_SET_DAC_TABLE, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_DISABLE);
        return;
    }

//    BT_SetDacTable(&BtModuleMemory, p);
    status = sBtInterface->hal_mp_op_send(BT_MP_OP_USER_DEF_SetDacTable, p, 0);
}

void bdt_Exec(char *p)
{
    if (!bt_enabled) {
        bt_mp_LogMsg("Bluetooth must be enabled for test:%s\n", STR_BT_MP_EXEC);        
        bdt_log("%s%s%x\n", STR_BT_MP_EXEC, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_DISABLE);
        return;
    }

//    BT_Exec(&BtModuleMemory, p);
    status = sBtInterface->hal_mp_op_send(BT_MP_OP_USER_DEF_Exec, p, 0);
}

void bdt_ReportTx(char *p)
{
    if (!bt_enabled) {
        bt_mp_LogMsg("Bluetooth must be enabled for test:%s\n", STR_BT_MP_REPORTTX);        
        bdt_log("%s%s%x\n", STR_BT_MP_REPORTTX, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_DISABLE);
        return;
    }

//    BT_ReportTx(&BtModuleMemory);
    status = sBtInterface->hal_mp_op_send(BT_MP_OP_USER_DEF_ReportTx, p, 0);
}

void bdt_ReportRx(char *p)
{
    if (!bt_enabled) {
        bt_mp_LogMsg("Bluetooth must be enabled for test:%s\n", STR_BT_MP_REPORTRX);        
        bdt_log("%s%s%x\n", STR_BT_MP_REPORTRX, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_DISABLE);
        return;
    }

//    BT_ReportRx(&BtModuleMemory);
    status = sBtInterface->hal_mp_op_send(BT_MP_OP_USER_DEF_ReportRx, p, 0);
}

void bdt_RegRf(char *p)
{
    if (!bt_enabled) {
        bt_mp_LogMsg("Bluetooth must be enabled for test:%s\n", STR_BT_MP_REG_RF);        
        bdt_log("%s%s%x\n", STR_BT_MP_REG_RF, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_DISABLE);
        return;
    }

    status = sBtInterface->hal_mp_op_send(BT_MP_OP_USER_DEF_REG_RF, p, 0);
}

void bdt_RegMd(char *p)
{
    if (!bt_enabled) {
        bt_mp_LogMsg("Bluetooth must be enabled for test:%s\n", STR_BT_MP_REG_MD);        
        bdt_log("%s%s%x\n", STR_BT_MP_REG_MD, STR_BT_MP_RX_RESULT_DELIM, ERROR_BT_DISABLE);
        return;
    }

    status = sBtInterface->hal_mp_op_send(BT_MP_OP_USER_DEF_REG_RF, p, 0);
}


void bdt_cleanup(void)
{
    bt_mp_LogMsg("CLEANUP\n");
    sBtInterface->cleanup();
}

/*******************************************************************************
 ** Console commands
 *******************************************************************************/

void do_help(char *p)
{
    int i = 0;
    int max = 0;
    char line[128];
    int pos = 0;

    while (console_cmd_list[i].name != NULL)
    {
        pos = sprintf(line, "%s", (char*)console_cmd_list[i].name);
        bdt_log("%s %s\n", (char*)line, (char*)console_cmd_list[i].help);
        i++;
    }
}

void do_quit(char *p)
{
    bdt_shutdown();
}

/*******************************************************************
 *
 *  BT TEST  CONSOLE COMMANDS
 *
 *  Parses argument lists and passes to API test function
 *
*/

void do_enable(char *p)
{
    bdt_enable();
}

void do_disable(char *p)
{
    bdt_disable();
}
void do_dut_mode_configure(char *p)
{
    bdt_dut_mode_configure(p);
}

void do_hci(char *p)
{
    bdt_hci(p);
}

void do_GetPara(char *p)
{
    bdt_GetPara(p);
}

void do_SetHit(char *p)
{
    bdt_SetHit(p);
}

void do_SetPara1(char *p)
{
    bdt_SetPara1(p);
}
void do_SetPara2(char *p)
{
    bdt_SetPara2(p);
}

void do_SetGainTable(char *p)
{
    bdt_SetGainTable(p);
}

void do_SetDacTable(char *p)
{
    bdt_SetDacTable(p);
}

void do_Exec(char *p)
{
    bdt_Exec(p);
}

void do_ReportTx(char *p)
{
    bdt_ReportTx(p);
}

void do_ReportRx(char *p)
{
    bdt_ReportRx(p);
}

void do_RegRf(char *p)
{
    bdt_RegRf(p);
}

void do_RegMd(char *p)
{
    bdt_RegMd(p);
}

void do_cleanup(char *p)
{
    bdt_cleanup();
}

/*******************************************************************
 *
 *  CONSOLE COMMAND TABLE
 *
*/

const t_cmd console_cmd_list[] =
{
    /*
     * INTERNAL
     */

    { "help", do_help, "lists all available console commands", 0 },
    { "quit", do_quit, "", 0},

    /*
     * API CONSOLE COMMANDS
     */
     /* Init and Cleanup shall be called automatically */
    { STR_BT_MP_ENABLE, do_enable, ":: enables bluetooth", 0 },
    { STR_BT_MP_DISABLE, do_disable, ":: disables bluetooth", 0 },
    { STR_BT_MP_DUT_MODE, do_dut_mode_configure, ":: DUT mode - 1 to enter,0 to exit", 0 },



    { STR_BT_MP_GET_PARA, do_GetPara, ":: do_GetPara", 0 },
    { STR_BT_MP_SET_HIT, do_SetHit, ":: do_SetHit", 0 },
    { STR_BT_MP_SET_PARA1, do_SetPara1, ":: do_SetPara1", 0 },
    { STR_BT_MP_SET_PARA2, do_SetPara2, ":: do_SetPara2", 0 },
    { STR_BT_MP_SET_GAIN_TABLE, do_SetGainTable, ":: do_SetGainTable", 0 },
    { STR_BT_MP_SET_DAC_TABLE, do_SetDacTable, ":: do_SetDacTable", 0 },

    { STR_BT_MP_SET_DAC_TABLE, do_SetDacTable, ":: do_SetDacTable", 0 },
    
    { STR_BT_MP_EXEC, do_Exec, ":: do_Exec", 0 },
    { STR_BT_MP_REPORTTX, do_ReportTx, ":: do_ReportTx", 0 },
    { STR_BT_MP_REPORTRX, do_ReportRx, ":: do_ReportRx", 0 },
    
    { STR_BT_MP_REG_RF, do_RegRf, ":: do_RegRf", 0 },
    { STR_BT_MP_REG_MD, do_RegMd, ":: do_RegMd", 0 },


    
    { STR_BT_MP_HCI_CMD, do_hci, ":: send hci command", 0 },


    /* add here */

    /* last entry */
    {NULL, NULL, "", 0},
};

/*
 * Main console command handler
*/

static void process_cmd(char *p, unsigned char is_job)
{
    char cmd[64];
    int i = 0;
    char *p_saved = p;

    get_str(&p, cmd);

    /* table commands */
    while (console_cmd_list[i].name != NULL)
    {
        if (is_cmd(console_cmd_list[i].name))
        {
            
            if (!is_job && console_cmd_list[i].is_job)
            {
                
                create_cmdjob(p_saved);
            }
            else
            {
                console_cmd_list[i].handler(p);
            }
            return;
        }
        i++;
    }
    bdt_log("%s : unknown command\n", p_saved);
    do_help(NULL);
}

int main (int argc, char * argv[])
{
    int opt;
    char cmd[128];
    int args_processed = 0;
    int pid = -1;


    config_permissions();
    bt_mp_LogMsg("\n:::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    bt_mp_LogMsg(":: Bluedroid test app starting\n");

    if ( HAL_load() < 0 ) {
        perror("HAL failed to initialize, exit\n");
        unlink(PID_FILE);
        exit(0);
    }

    setup_test_env();

    /* Automatically perform the init */
//    bdt_init();

    while(!main_done)
    {
        char line[128];

        /* command prompt */
        bt_mp_LogMsg( ">" );
        fflush(stdout);

        fgets (line, 128, stdin);

        if (line[0]!= '\0')
        {
            /* remove linefeed */
            line[strlen(line)-1] = 0;

            process_cmd(line, 0);
            memset(line, '\0', 128);
        }
    }

    /* FIXME: Commenting this out as for some reason, the application does not exit otherwise*/
    //bdt_cleanup();

    HAL_unload();

    bt_mp_LogMsg(":: Bluedroid test app terminating\n");

    return 0;
}

void bt_mp_LogMsg(const char *fmt_str, ...)
{
    static char buffer[MPTOOL_LOG_BUF_SIZE];

    va_list ap;
    va_start(ap, fmt_str);
    vsnprintf(&buffer[0], MPTOOL_LOG_MAX_SIZE, fmt_str, ap);
    va_end(ap);

    LOGI0("rtlbtmp: ", buffer);
}


/*****************************************************************************
**   Logger API
*****************************************************************************/

void bdt_log(const char *fmt_str, ...)
{
    static char buffer[1024];
    va_list ap;

    va_start(ap, fmt_str);
    vsnprintf(buffer, 1024, fmt_str, ap);
    va_end(ap);

    fprintf(stdout, "%s", buffer);
}

