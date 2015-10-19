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
 *  Filename:      hci_h5.c
 *
 *  Description:   Contains HCI transport send/receive functions
 *
 ******************************************************************************/

#define LOG_TAG "bt_h5"
#include <utils/Log.h>
#include <stdlib.h>
#include <fcntl.h>
#include "bt_hci_bdroid.h"
#include "hci.h"
#include "userial.h"
#include "utils.h"
#include <termios.h>
#include <errno.h>
#include "bt_skbuff.h"
#include "bt_list.h"

#include <signal.h>
#include <time.h>

#include <sys/prctl.h>

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#define H5_TRACE_DATA_ENABLE 1//if you want to see data tx and rx, set H5_TRACE_DATA_ENABLE 1


uint8_t h5_log_enable = 0;

#ifndef H5_LOG_BUF_SIZE
#define H5_LOG_BUF_SIZE  1024
#endif
#define H5_LOG_MAX_SIZE  (H5_LOG_BUF_SIZE - 12)


#ifndef H5_LOG_BUF_SIZE
#define H5_LOG_BUF_SIZE  1024
#endif
#define H5_LOG_MAX_SIZE  (H5_LOG_BUF_SIZE - 12)


#define TIMER_H5_DATA_RETRANS (SIGRTMAX)
#define TIMER_H5_SYNC_RETRANS (SIGRTMAX -1)
#define TIMER_H5_CONF_RETRANS (SIGRTMAX -2)
#define TIMER_H5_WAIT_CT_BAUDRATE_READY (SIGRTMAX -3)
#define TIMER_H5_HW_INIT_READY        (SIGRTMAX -4)

#define DATA_RETRANS_COUNT  40  //40*100 = 4000ms(4s)
#define SYNC_RETRANS_COUNT  20  //20*250 = 5000ms(5s)
#define CONF_RETRANS_COUNT  20


#define DATA_RETRANS_TIMEOUT_VALUE  100 //ms
#define SYNC_RETRANS_TIMEOUT_VALUE   250
#define CONF_RETRANS_TIMEOUT_VALUE   250
#define WAIT_CT_BAUDRATE_READY_TIMEOUT_VALUE   400
#define H5_HW_INIT_READY_TIMEOUT_VALUE   4000//4

#define HCI_VSC_H5_INIT                0xFCEE


/* Preamble length for HCI Commands:
**      2-bytes for opcode and 1 byte for length
*/
#define HCI_CMD_PREAMBLE_SIZE   3

/* Preamble length for HCI Events:
**      1-byte for opcode and 1 byte for length
*/
#define HCI_EVT_PREAMBLE_SIZE   2

/* Preamble length for SCO Data:
**      2-byte for Handle and 1 byte for length
*/
#define HCI_SCO_PREAMBLE_SIZE   3

/* Preamble length for ACL Data:
**      2-byte for Handle and 2 byte for length
*/
#define HCI_ACL_PREAMBLE_SIZE   4



#define ACL_RX_PKT_START        2
#define ACL_RX_PKT_CONTINUE     1
#define L2CAP_HEADER_SIZE       4

/* Maximum numbers of allowed internal
** outstanding command packets at any time
*/
#define INT_CMD_PKT_MAX_COUNT       8
#define INT_CMD_PKT_IDX_MASK        0x07


//HCI Event codes
#define HCI_CONNECTION_COMP_EVT             0x03
#define HCI_DISCONNECTION_COMP_EVT          0x05
#define HCI_COMMAND_COMPLETE_EVT    0x0E
#define HCI_COMMAND_STATUS_EVT      0x0F
#define HCI_NUM_OF_CMP_PKTS_EVT     0x13
#define HCI_BLE_EVT     0x3E

//HCI Command opcodes
#define HCI_READ_BUFFER_SIZE        0x1005
#define HCI_LE_READ_BUFFER_SIZE     0x2002



#define PATCH_DATA_FIELD_MAX_SIZE     252
#define READ_DATA_SIZE  16

// HCI data types //
#define H5_ACK_PKT              0x00
#define HCI_COMMAND_PKT         0x01
#define HCI_ACLDATA_PKT         0x02
#define HCI_SCODATA_PKT         0x03
#define HCI_EVENT_PKT           0x04
#define H5_VDRSPEC_PKT          0x0E
#define H5_LINK_CTL_PKT         0x0F

//#define BT_FW_CAL_ENABLE

#ifdef BT_FW_CAL_ENABLE
#define CAL_INQUIRY_SUCCESS     0
#define CAL_INQUIRY_UNKNOWN     1
#define CAL_INQUIRY_FAIL        2
#define IS_LAST_INQUIRY_SUCCESS     0x0010
#define BT_CAL_DIRECTORY "/data/misc/bluedroid/"
uint32_t rtk_set_bt_cal_inqury_result(uint8_t result);
#endif

/* BD Address */
typedef struct {
    uint8_t b[6];
} __packed bdaddr_t;


/******************************************************************************
**  Local type definitions
******************************************************************************/

static const uint16_t msg_evt_table[] =
{
    MSG_HC_TO_STACK_HCI_ERR,       /* H4_TYPE_COMMAND */
    MSG_HC_TO_STACK_HCI_ACL,       /* H4_TYPE_ACL_DATA */
    MSG_HC_TO_STACK_HCI_SCO,       /* H4_TYPE_SCO_DATA */
    MSG_HC_TO_STACK_HCI_EVT        /* H4_TYPE_EVENT */
};

/* Callback function for the returned event of internal issued command */
typedef void (*tINT_CMD_CBACK)(void *p_mem);

typedef struct
{
    uint16_t opcode;        /* OPCODE of outstanding internal commands */
    tINT_CMD_CBACK cback;   /* Callback function when return of internal
                             * command is received */
} tINT_CMD_Q;

typedef RTK_BUFFER sk_buff;

/* Skb helpers */
struct bt_skb_cb
{
    uint8_t pkt_type;
    uint8_t incoming;
    uint8_t expect;
    uint8_t tx_seq;
    uint8_t retries;
    uint8_t sar;
    unsigned short channel;
};

typedef struct {
    uint8_t index;
    uint8_t data[252];
} __attribute__ ((packed)) download_vendor_patch_cp;


#define HCI_COMMAND_HDR_SIZE 3
#define HCI_EVENT_HDR_SIZE   2

struct hci_command_hdr {
 uint16_t opcode;     /* OCF & OGF */
 uint8_t    plen;
} __attribute__ ((packed));

struct hci_event_hdr {
 uint8_t    evt;
 uint8_t    plen;
 } __attribute__ ((packed));

struct hci_ev_cmd_complete {
uint8_t     ncmd;
uint16_t   opcode;
} __attribute__ ((packed));

#define HCI_CMD_READ_BD_ADDR 0x1009
#define HCI_VENDOR_CHANGE_BDRATE 0xfc17

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

#pragma pack(1)
#if __BYTE_ORDER == __LITTLE_ENDIAN
typedef struct _H5_PKT_HEADER
{
    uint8_t SeqNumber:3;
    uint8_t AckNumber:3;
    uint8_t DicPresent:1; // Data Integrity Check Present
    uint8_t ReliablePkt:1;

    uint16_t PktType:4;
    uint16_t PayloadLen:12;

    uint8_t HdrChecksum;
}H5_PKT_HEADER;

typedef struct _H5_CONFIG_FIELD
{
    uint8_t SlidingWindowSize:3;
    uint8_t OofFlowControl:1;
    uint8_t DicType:1;
    uint8_t VersionNumber:3;
} H5_CONFIG_FIELD;

#else
typedef struct _H5_PKT_HEADER
{
    uint8_t ReliablePkt:1;
    uint8_t DicPresent:1; // Data Integrity Check Present
    uint8_t AckNumber:3;
    uint8_t SeqNumber:3;

    uint16_t PayloadLen:12;
    uint16_t PktType:4;

    uint8_t HdrChecksum;
}H5_PKT_HEADER;

typedef struct _H5_CONFIG_FIELD
{
    uint8_t VersionNumber:3;
    uint8_t DicType:1;
    uint8_t OofFlowControl:1;
    uint8_t SlidingWindowSize:3;
} H5_CONFIG_FIELD;

#endif


typedef enum _H5_RX_STATE
{
    H5_W4_PKT_DELIMITER,
    H5_W4_PKT_START,
    H5_W4_HDR,
    H5_W4_DATA,
    H5_W4_CRC
} H5_RX_STATE;

typedef enum _H5_RX_ESC_STATE
{
    H5_ESCSTATE_NOESC,
    H5_ESCSTATE_ESC
} H5_RX_ESC_STATE;

typedef enum _H5_LINK_STATE
{
    H5_UNINITIALIZED,
    H5_INITIALIZED,
    H5_ACTIVE
} H5_LINK_STATE;

typedef enum _PATCH_PROTOCOL
{
    PATCH_PROTOCAL_H4,
    PATCH_PROTOCAL_H5
} PATCH_PROTOCOL;


typedef struct _HCI_CONN {
    RT_LIST_ENTRY List;
    uint16_t handle;
    bdaddr_t bd_addr;
    uint8_t link_type;
    uint8_t encrypt_enabled;
    uint16_t NumOfNotCmpAclPkts;
    RTB_QUEUE_HEAD *pending_pkts;    // pending pkts to send
    sk_buff *rx_skb;
}HCI_CONN, *PHCI_CONN;


#define H5_EVENT_RX                    0x0001
#define H5_EVENT_EXIT                  0x0200

static volatile uint8_t h5_retransfer_running = 0;
static volatile uint16_t h5_ready_events = 0;


/* Control block for HCISU_H5 */
struct tHCI_H5_CB
{
    HC_BT_HDR *p_rcv_msg;          /* Buffer to hold current rx HCI message */
    uint16_t rcv_len;               /* Size of current incoming message */
    uint8_t rcv_msg_type;           /* Current incoming message type */
//    tHCI_H4_RCV_STATE rcv_state;    /* Receive state of current rx message */
    uint16_t hc_acl_data_size;      /* Controller's max ACL data length */
    uint8_t   hc_sco_data_size;
    uint16_t hc_acl_total_num;      /* Controller's total ACL number packets */
    uint16_t hc_sco_total_num;      /* Controller's total ACL number packets */
    uint16_t hc_ble_acl_data_size;  /* Controller's max BLE ACL data length */
    uint8_t hc_ble_acl_total_num;  /* Controller's max BLE ACL data length */
    BUFFER_Q acl_rx_q;      /* Queue of base buffers for fragmented ACL pkts */
    uint8_t preload_count;          /* Count numbers of preload bytes */
    uint8_t preload_buffer[6];      /* HCI_ACL_PREAMBLE_SIZE + 2 */
    int int_cmd_rsp_pending;        /* Num of internal cmds pending for ack */
    uint8_t int_cmd_rd_idx;         /* Read index of int_cmd_opcode queue */
    uint8_t int_cmd_wrt_idx;        /* Write index of int_cmd_opcode queue */
    tINT_CMD_Q int_cmd[INT_CMD_PKT_MAX_COUNT]; /* FIFO queue */

    tINT_CMD_CBACK cback_h5sync;   /* Callback function when h5 sync*/

    uint8_t sliding_window_size;
    uint8_t oof_flow_control;
    uint8_t dic_type;


    RTB_QUEUE_HEAD *unack;      // Unack'ed packets queue
    RTB_QUEUE_HEAD *rel;        // Reliable packets queue

    RTB_QUEUE_HEAD *unrel;      // Unreliable packets queue


    uint8_t rxseq_txack;        // rxseq == txack. // expected rx SeqNumber
    uint8_t	 rxack;             // Last packet sent by us that the peer ack'ed //

    uint8_t	 use_crc;
    uint8_t	 is_txack_req;      // txack required? Do we need to send ack's to the peer? //

    // Reliable packet sequence number - used to assign seq to each rel pkt. */
    uint8_t msgq_txseq;         //next pkt seq

    uint16_t    message_crc;
    uint32_t    rx_count;       //expected pkts to recv

    H5_RX_STATE rx_state;
    H5_RX_ESC_STATE rx_esc_state;
    H5_LINK_STATE link_estab_state;

    sk_buff *rx_skb;
    sk_buff* host_last_cmd;

    timer_t  timer_data_retrans;
    timer_t  timer_sync_retrans;
    timer_t  timer_conf_retrans;
    timer_t  timer_wait_ct_baudrate_ready;
    timer_t  timer_h5_hw_init_ready;

    uint32_t data_retrans_count;
    uint32_t sync_retrans_count;
    uint32_t conf_retrans_count;

    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    pthread_t thread_data_retrans;


    RT_LIST_HEAD    HciConnHash;

    uint16_t hc_cur_acl_total_num;
    uint8_t   cleanuping;

#ifdef BT_FW_CAL_ENABLE
    uint8_t  bHasUpdateCalInquiryState;
#endif
};

static struct tHCI_H5_CB rtk_h5;
static pthread_mutex_t h5_wakeup_mutex = PTHREAD_MUTEX_INITIALIZER;

/******************************************************************************
**  Variables
******************************************************************************/

/* Num of allowed outstanding HCI CMD packets */
volatile int num_hci_cmd_pkts = 1;

/******************************************************************************
**  Static variables
******************************************************************************/

struct patch_struct {
    int nTxIndex;   // current sending pkt number
    int nTotal;     // total pkt number
    int nRxIndex;   // ack index from board
    int nNeedRetry; // if no response from board
};
static struct patch_struct rtk_patch;



/******************************************************************************
**  Externs
******************************************************************************/

extern BUFFER_Q tx_q;

void btsnoop_init(void);
void btsnoop_close(void);
void btsnoop_cleanup (void);
void btsnoop_capture(HC_BT_HDR *p_buf, uint8_t is_rcvd);
uint8_t hci_h5_send_int_cmd(uint16_t opcode, HC_BT_HDR *p_buf, \
                                  tINT_CMD_CBACK p_cback);
void lpm_wake_assert(void);
void lpm_tx_done(uint8_t is_tx_done);
static void h5_wake_up();

//timer API for retransfer
int h5_alloc_data_retrans_timer();
int h5_free_data_retrans_timer();
int h5_stop_data_retrans_timer();
int h5_start_data_retrans_timer();

int h5_alloc_sync_retrans_timer();
int h5_free_sync_retrans_timer();
int h5_stop_sync_retrans_timer();
int h5_start_sync_retrans_timer();

int h5_alloc_conf_retrans_timer();
int h5_free_conf_retrans_timer();
int h5_stop_conf_retrans_timer();
int h5_start_conf_retrans_timer();

int h5_alloc_wait_controller_baudrate_ready_timer();
int h5_free_wait_controller_baudrate_ready_timer();
int h5_stop_wait_controller_baudrate_ready_timer();
int h5_start_wait_controller_baudrate_ready_timer();

int h5_alloc_hw_init_ready_timer();
int h5_free_hw_init_ready_timer();
int h5_stop_hw_init_ready_timer();
int h5_start_hw_init_ready_timer();


// bite reverse in bytes
// 00000001 -> 10000000
// 00000100 -> 00100000
const uint8_t byte_rev_table[256] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};
#ifndef H5_LOG_BUF_SIZE
#define H5_LOG_BUF_SIZE  1024
#endif
#define H5_LOG_MAX_SIZE  (H5_LOG_BUF_SIZE - 12)

#define LOGI0(t,s) __android_log_write(ANDROID_LOG_INFO, t, s)

void
LogMsg(const char *fmt_str, ...)
{
    static char buffer[H5_LOG_BUF_SIZE];
    if(h5_log_enable == 1)
    {
        va_list ap;
        va_start(ap, fmt_str);
        vsnprintf(&buffer[0], H5_LOG_MAX_SIZE, fmt_str, ap);
        va_end(ap);

        LOGI0("H5: ", buffer);
     }
     else
     {
        return;
     }
}

/* Copy, swap, convert BD Address */
static inline int bacmp(bdaddr_t *ba1, bdaddr_t *ba2)
{
    return memcmp(ba1, ba2, sizeof(bdaddr_t));
}
static inline void bacpy(bdaddr_t *dst, bdaddr_t *src)
{
    memcpy(dst, src, sizeof(bdaddr_t));
}

static inline void baPrint(bdaddr_t ba)
{
    LogMsg("BT_ADDR: 0x%02X%02X%02X%02X%02X%02X",ba.b[5], ba.b[4], ba.b[3], ba.b[2], ba.b[1], ba.b[0]);
}
// reverse bit
static __inline uint8_t bit_rev8(uint8_t byte)
{
    return byte_rev_table[byte];
}

// reverse bit
static __inline uint16_t bit_rev16(uint16_t x)
{
    return (bit_rev8(x & 0xff) << 8) | bit_rev8(x >> 8);
}

static const uint16_t crc_table[] =
{
    0x0000, 0x1081, 0x2102, 0x3183,
    0x4204, 0x5285, 0x6306, 0x7387,
    0x8408, 0x9489, 0xa50a, 0xb58b,
    0xc60c, 0xd68d, 0xe70e, 0xf78f
};

// Initialise the crc calculator
#define H5_CRC_INIT(x) x = 0xffff


/*******************************************************************************
**
** Function        h5_ms_delay
**
** Description     sleep unconditionally for timeout milliseconds
**
** Returns         None
**
*******************************************************************************/
void h5_ms_delay (uint32_t timeout)
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
/***********************************************
//
//skb related functions
//
//
//
***********************************************/
uint8_t *
skb_get_data(
    IN sk_buff *skb
    )
{
    return skb->Data;
}

uint32_t
skb_get_data_length(
    IN sk_buff *skb
    )
{
    return skb->Length;
}

uint32_t
skb_get_hci_data_length(
    IN sk_buff *skb_acl
    )
{
    uint8_t     *data;
    uint16_t   data_len = 0;

    uint16_t    handle, hci_len, l2cap_len;

    uint8_t     frame_end=TRUE;


    data = skb_get_data(skb_acl);
    STREAM_TO_UINT16 (handle, data);
    STREAM_TO_UINT16 (hci_len, data);

    return hci_len;

}

uint32_t
skb_get_l2cap_data_length(
    IN sk_buff *skb_acl
    )
{
    uint8_t     *data;
    uint16_t   data_len = 0;

    uint16_t    handle, hci_len, l2cap_len;

    uint8_t     frame_end=TRUE;


    data = skb_get_data(skb_acl);
    STREAM_TO_UINT16 (handle, data);
    STREAM_TO_UINT16 (hci_len, data);
    STREAM_TO_UINT16 (l2cap_len, data);


    return l2cap_len;

}

void
skb_set_hci_data_length(
    IN sk_buff *skb_acl,
    IN uint32_t hci_len
    )
{
    uint8_t     *data;

    data = skb_get_data(skb_acl);
    data = data + 2;
    UINT16_TO_STREAM(data, hci_len);
}

sk_buff *
skb_alloc(
    IN unsigned int len
    )
{
    sk_buff * skb = (sk_buff * )RtbAllocate(len, 0);
    return skb;
}

void
skb_free(
    IN OUT sk_buff **skb
    )
{
    RtbFree(*skb);
    *skb = NULL;
    return;
}

static  void skb_unlink(
    sk_buff *skb,
    struct _RTB_QUEUE_HEAD * list
)
{
    RtbRemoveNode(list, skb);
}

// increase the date length in sk_buffer by len,
// and return the increased header pointer
uint8_t *skb_put(OUT sk_buff* skb, IN uint32_t len)
{
    RTK_BUFFER * rtb = (RTK_BUFFER * )skb;

    return RtbAddTail(rtb, len);
}

// change skb->len to len
// !!! len should less than skb->len
void skb_trim( sk_buff *skb, unsigned int len)
{
    RTK_BUFFER * rtb = (RTK_BUFFER * )skb;
    uint32_t skb_len = skb_get_data_length(skb);

    RtbRemoveTail(rtb, (skb_len - len));
    return;
}

uint8_t skb_get_pkt_type( sk_buff *skb)
{
    return BT_CONTEXT(skb)->PacketType;
}

void skb_set_pkt_type( sk_buff *skb, uint8_t pkt_type)
{
    BT_CONTEXT(skb)->PacketType = pkt_type;
}

uint16_t skb_get_acl_handle( sk_buff *skb)
{
    return BT_CONTEXT(skb)->Handle;
}

void skb_set_acl_handle( sk_buff *skb, uint16_t handle)
{
    BT_CONTEXT(skb)->Handle = handle;
}


// decrease the data length in sk_buffer by len,
// and move the content forward to the header.
// the data in header will be removed.
void skb_pull(OUT  sk_buff * skb, IN uint32_t len)
{
    RTK_BUFFER * rtb = (RTK_BUFFER * )skb;
    RtbRemoveHead(rtb, len);
    return;
}

sk_buff *
skb_alloc_and_init(
    IN uint8_t PktType,
    IN uint8_t * Data,
    IN uint32_t  DataLen
    )
{
    sk_buff * skb = skb_alloc(DataLen);
    if (NULL == skb)
    return NULL;
    memcpy(skb_put(skb, DataLen), Data, DataLen);
    skb_set_pkt_type(skb, PktType);

    return skb;
}

static void
skb_queue_head(
    IN RTB_QUEUE_HEAD * skb_head,
    IN RTK_BUFFER * skb
)
{
    RtbQueueHead(skb_head, skb);
}

static void
skb_queue_tail(
    IN RTB_QUEUE_HEAD * skb_head,
    IN RTK_BUFFER * skb
)
{
    RtbQueueTail(skb_head, skb);
}

static RTK_BUFFER *
skb_dequeue_head(IN RTB_QUEUE_HEAD * skb_head)
{
    return RtbDequeueHead(skb_head);
}

static RTK_BUFFER *
skb_dequeue_tail(IN RTB_QUEUE_HEAD * skb_head)
{
    return RtbDequeueTail(skb_head);
}

static uint32_t
skb_queue_get_length(
    IN RTB_QUEUE_HEAD * skb_head
)
{
    return RtbGetQueueLen(skb_head);
}


/**
*/
HCI_CONN*
HciConnAllocate(
    uint16_t handle
  )
{
    HCI_CONN * phci_conn = NULL;
    phci_conn = malloc(sizeof(HCI_CONN));
    if(phci_conn)
    {
        phci_conn->handle = handle;
        phci_conn->pending_pkts = RtbQueueInit();
        phci_conn->rx_skb = NULL;
    }
    return phci_conn;
}

/**
*/
void
HciConnFree(
    HCI_CONN* phci_conn
  )
{
    if(phci_conn)
    {
        RtbQueueFree(phci_conn->pending_pkts);
        if(phci_conn->rx_skb)
            skb_free(&phci_conn->rx_skb);

        free(phci_conn);
    }
}

/**
    HCI connection related APIs.

*/
void ConnHashInit( struct tHCI_H5_CB* h5)
{
    RT_LIST_HEAD* Head = &h5->HciConnHash;
    ListInitializeHeader(Head);
}

void ConnHashFlush(struct tHCI_H5_CB* h5)
{
    RT_LIST_HEAD* Head = &h5->HciConnHash;
    RT_LIST_ENTRY* Iter = NULL, *Temp = NULL;
    HCI_CONN* Desc = NULL;

    LIST_FOR_EACH_SAFELY(Iter, Temp, Head)
    {
        Desc = LIST_ENTRY(Iter, HCI_CONN, List);
        if (Desc)
        {
            ListDeleteNode(Iter);
            HciConnFree(Desc);
        }
    }
    ListInitializeHeader(Head);
}

void ConnHashAdd(struct tHCI_H5_CB* h5, HCI_CONN* Desc)
{
    RT_LIST_HEAD* Head = &h5->HciConnHash;
    ListAddToTail(&Desc->List, Head);

}

void ConnHashDelete(HCI_CONN* Desc)
{
    if (Desc)
        ListDeleteNode(&Desc->List);
}

HCI_CONN* ConnHashLookupByHandle(struct tHCI_H5_CB* h5, uint16_t Handle)
{
    RT_LIST_HEAD* Head = &h5->HciConnHash;
    RT_LIST_ENTRY* Iter = NULL;
    HCI_CONN* Desc = NULL;

    LIST_FOR_EACH(Iter, Head)
    {
        Desc = LIST_ENTRY(Iter, HCI_CONN, List);
        if ((Handle & 0xEFF) == Desc->handle )  //only last 12 bit are meanful for hci handle
        {
            return Desc;
        }
    }
    return NULL;
}




/**
* Add "d" into crc scope, caculate the new crc value
*
* @param crc crc data
* @param d one byte data
*/
static void h5_crc_update(uint16_t *crc, uint8_t d)
{
    uint16_t reg = *crc;

    reg = (reg >> 4) ^ crc_table[(reg ^ d) & 0x000f];
    reg = (reg >> 4) ^ crc_table[(reg ^ (d >> 4)) & 0x000f];

    *crc = reg;
}

struct __una_u16 { uint16_t x; };
static __inline uint16_t __get_unaligned_cpu16(const void *p)
{
    const struct __una_u16 *ptr = (const struct __una_u16 *)p;
    return ptr->x;
}


static __inline uint16_t get_unaligned_be16(const void *p)
{
    return __get_unaligned_cpu16((const uint8_t *)p);
}
/**
* Get crc data.
*
* @param h5 realtek h5 struct
* @return crc data
*/
static uint16_t h5_get_crc(struct tHCI_H5_CB *h5)
{
   uint16_t crc = 0;
   uint8_t * data = skb_get_data(h5->rx_skb) + skb_get_data_length(h5->rx_skb) - 2;
   crc = data[1] + (data[0] << 8);
   return crc;
}

/**
* Just add 0xc0 at the end of skb,
* we can also use this to add 0xc0 at start while there is no data in skb
*
* @param skb socket buffer
*/
static void h5_slip_msgdelim(sk_buff *skb)
{
    const char pkt_delim = 0xc0;
    memcpy(skb_put(skb, 1), &pkt_delim, 1);
}

/**
* Slip ecode one byte in h5 proto, as follows:
* 0xc0 -> 0xdb, 0xdc
* 0xdb -> 0xdb, 0xdd
* 0x11 -> 0xdb, 0xde
* 0x13 -> 0xdb, 0xdf
* others will not change
*
* @param skb socket buffer
* @c pure data in the one byte
*/
static void h5_slip_one_byte(sk_buff *skb, uint8_t c)
{
    const signed char esc_c0[2] = { 0xdb, 0xdc };
    const signed char esc_db[2] = { 0xdb, 0xdd };
    const signed char esc_11[2] = { 0xdb, 0xde };
    const signed char esc_13[2] = { 0xdb, 0xdf };

    switch (c)
    {
    case 0xc0:
        memcpy(skb_put(skb, 2), &esc_c0, 2);
        break;
    case 0xdb:
        memcpy(skb_put(skb, 2), &esc_db, 2);
        break;

    case 0x11:
    {
        if(rtk_h5.oof_flow_control)
        {
            memcpy(skb_put(skb, 2), &esc_11, 2);
        }
        else
        {
            memcpy(skb_put(skb, 1), &c, 1);
        }
    }
    break;

    case 0x13:
    {
        if(rtk_h5.oof_flow_control)
        {
            memcpy(skb_put(skb, 2), &esc_13, 2);
        }
        else
        {
            memcpy(skb_put(skb, 1), &c, 1);
        }
    }
    break;

    default:
        memcpy(skb_put(skb, 1), &c, 1);
    }
}

/**
* Decode one byte in h5 proto, as follows:
* 0xdb, 0xdc -> 0xc0
* 0xdb, 0xdd -> 0xdb
* 0xdb, 0xde -> 0x11
* 0xdb, 0xdf -> 0x13
* others will not change
*
* @param h5 realtek h5 struct
* @byte pure data in the one byte
*/
static void h5_unslip_one_byte(struct tHCI_H5_CB *h5, unsigned char byte)
{
    const uint8_t c0 = 0xc0, db = 0xdb;
    const uint8_t oof1 = 0x11, oof2 = 0x13;
    H5_PKT_HEADER * hdr = (H5_PKT_HEADER *)skb_get_data(h5->rx_skb);

    if (H5_ESCSTATE_NOESC == h5->rx_esc_state)
    {
        if (0xdb == byte)
        {
            h5->rx_esc_state = H5_ESCSTATE_ESC;
        }
        else
        {
            memcpy(skb_put(h5->rx_skb, 1), &byte, 1);
            //Check Pkt Header's CRC enable bit
            if (hdr->DicPresent && h5->rx_state != H5_W4_CRC)
            {
                h5_crc_update(&h5->message_crc, byte);
            }
            h5->rx_count--;
        }
    }
    else if(H5_ESCSTATE_ESC == h5->rx_esc_state)
    {
        switch (byte)
        {
        case 0xdc:
            memcpy(skb_put(h5->rx_skb, 1), &c0, 1);
            if (hdr->DicPresent && h5->rx_state != H5_W4_CRC)
                h5_crc_update(&h5-> message_crc, 0xc0);
            h5->rx_esc_state = H5_ESCSTATE_NOESC;
            h5->rx_count--;
            break;
        case 0xdd:
            memcpy(skb_put(h5->rx_skb, 1), &db, 1);
             if (hdr->DicPresent && h5->rx_state != H5_W4_CRC)
                h5_crc_update(&h5-> message_crc, 0xdb);
            h5->rx_esc_state = H5_ESCSTATE_NOESC;
            h5->rx_count--;
            break;
        case 0xde:
            memcpy(skb_put(h5->rx_skb, 1), &oof1, 1);
            if (hdr->DicPresent && h5->rx_state != H5_W4_CRC)
                h5_crc_update(&h5-> message_crc, oof1);
            h5->rx_esc_state = H5_ESCSTATE_NOESC;
            h5->rx_count--;
            break;
        case 0xdf:
            memcpy(skb_put(h5->rx_skb, 1), &oof2, 1);
            if (hdr->DicPresent && h5->rx_state != H5_W4_CRC)
                h5_crc_update(&h5-> message_crc, oof2);
            h5->rx_esc_state = H5_ESCSTATE_NOESC;
            h5->rx_count--;
            break;
        default:
            ALOGE("Error: Invalid byte %02x after esc byte", byte);
            skb_free(&h5->rx_skb);
            h5->rx_skb = NULL;
            h5->rx_state = H5_W4_PKT_DELIMITER;
            h5->rx_count = 0;
            break;
        }
    }
}
/**
* Prepare h5 packet, packet format as follow:
*  | LSB 4 octets  | 0 ~4095| 2 MSB
*  |packet header | payload | data integrity check |
*
* pakcket header fromat is show below:
*  | LSB 3 bits         | 3 bits             | 1 bits                       | 1 bits          |
*  | 4 bits     | 12 bits        | 8 bits MSB
*  |sequence number | acknowledgement number | data integrity check present | reliable packet |
*  |packet type | payload length | header checksum
*
* @param h5 realtek h5 struct
* @param data pure data
* @param len the length of data
* @param pkt_type packet type
* @return socket buff after prepare in h5 proto
*/
static sk_buff * h5_prepare_pkt(struct tHCI_H5_CB *h5, uint8_t *data, signed long len, signed long pkt_type)
{
    sk_buff *nskb;
    uint8_t hdr[4];
    uint16_t H5_CRC_INIT(h5_txmsg_crc);
    int rel, i;
    LogMsg("HCI h5_prepare_pkt");

    switch (pkt_type)
    {
    case HCI_ACLDATA_PKT:
    case HCI_COMMAND_PKT:
    case HCI_EVENT_PKT:
    rel = 1;// reliable
    break;
    case H5_ACK_PKT:
    case H5_VDRSPEC_PKT:
    case H5_LINK_CTL_PKT:
    rel = 0;// unreliable
    break;
    default:
    ALOGE("Unknown packet type");
    return NULL;
    }

    // Max len of packet: (original len +4(h5 hdr) +2(crc))*2
    //   (because bytes 0xc0 and 0xdb are escaped, worst case is
    //   when the packet is all made of 0xc0 and 0xdb :) )
    //   + 2 (0xc0 delimiters at start and end).

    nskb = skb_alloc((len + 6) * 2 + 2);
    if (!nskb)
    {
        LogMsg("nskb is NULL");
        return NULL;
    }

    //Add SLIP start byte: 0xc0
    h5_slip_msgdelim(nskb);
    // set AckNumber in SlipHeader
    hdr[0] = h5->rxseq_txack << 3;
    h5->is_txack_req = 0;

    LogMsg("We request packet no(%u) to card", h5->rxseq_txack);
    LogMsg("Sending packet with seqno %u and wait %u", h5->msgq_txseq, h5->rxseq_txack);
    if (rel)
    {
        // set reliable pkt bit and SeqNumber
        hdr[0] |= 0x80 + h5->msgq_txseq;
        //LogMsg("Sending packet with seqno(%u)", h5->msgq_txseq);
        ++(h5->msgq_txseq);
        h5->msgq_txseq = (h5->msgq_txseq) & 0x07;
    }

    // set DicPresent bit
    if (h5->use_crc)
    hdr[0] |= 0x40;

    // set packet type and payload length
    hdr[1] = ((len << 4) & 0xff) | pkt_type;
    hdr[2] = (uint8_t)(len >> 4);
    // set checksum
    hdr[3] = ~(hdr[0] + hdr[1] + hdr[2]);

    // Put h5 header */
    for (i = 0; i < 4; i++)
    {
        h5_slip_one_byte(nskb, hdr[i]);

        if (h5->use_crc)
            h5_crc_update(&h5_txmsg_crc, hdr[i]);
    }

    // Put payload */
    for (i = 0; i < len; i++)
    {
        h5_slip_one_byte(nskb, data[i]);

       if (h5->use_crc)
       h5_crc_update(&h5_txmsg_crc, data[i]);
    }

    // Put CRC */
    if (h5->use_crc)
    {
        h5_txmsg_crc = bit_rev16(h5_txmsg_crc);
        h5_slip_one_byte(nskb, (uint8_t) ((h5_txmsg_crc >> 8) & 0x00ff));
        h5_slip_one_byte(nskb, (uint8_t) (h5_txmsg_crc & 0x00ff));
    }

    // Add SLIP end byte: 0xc0
    h5_slip_msgdelim(nskb);
    return nskb;
}
/**
* Removed controller acked packet from Host's unacked lists
*
* @param h5 realtek h5 struct
*/
static void h5_remove_acked_pkt(struct tHCI_H5_CB *h5)
{
    RT_LIST_HEAD* Head = NULL;
    RT_LIST_ENTRY* Iter = NULL, *Temp = NULL;
    RTK_BUFFER *skb = NULL;

    int pkts_to_be_removed = 0;
    int seqno = 0;
    int i = 0;

    pthread_mutex_lock(&h5_wakeup_mutex);

    seqno = h5->msgq_txseq;
    pkts_to_be_removed = RtbGetQueueLen(h5->unack);

    while (pkts_to_be_removed)
    {
        if (h5->rxack == seqno)
        break;

        pkts_to_be_removed--;
        seqno = (seqno - 1) & 0x07;
    }

    if (h5->rxack != seqno)
    {
        LogMsg("Peer acked invalid packet");
    }


    // remove ack'ed packet from bcsp->unack queue
    i = 0;//  number of pkts has been removed from un_ack queue.
    Head = (RT_LIST_HEAD *)(h5->unack);
    LIST_FOR_EACH_SAFELY(Iter, Temp, Head)
    {
        skb = LIST_ENTRY(Iter, sk_buff, List);
        if (i >= pkts_to_be_removed)
            break;

        skb_unlink(skb, h5->unack);
        skb_free(&skb);
        i++;
    }

    if (0 == skb_queue_get_length(h5->unack))
    {
        h5_stop_data_retrans_timer();
        rtk_h5.data_retrans_count = 0;
    }

    if (i != pkts_to_be_removed)
    {
        LogMsg("Removed only (%u) out of (%u) pkts", i, pkts_to_be_removed);
    }

    pthread_mutex_unlock(&h5_wakeup_mutex);

}

/**
* Realtek send pure ack, send a packet only with an ack
*
* @param fd uart file descriptor
*
*/

static void hci_h5_send_pure_ack(void)
{

    //convert h4 data to h5
    uint16_t bytes_sent = 0;

    sk_buff *nskb = h5_prepare_pkt(&rtk_h5, NULL, 0, H5_ACK_PKT);
    if(nskb == NULL)
    {
        ALOGE("h5_prepare_pkt allocate memory fail");
        return;
    }
    LogMsg("H5: --->>>send pure ack");
    uint8_t * data = skb_get_data(nskb);

#if H5_TRACE_DATA_ENABLE
    {
        uint32_t iTemp = 0;
        uint32_t iTempTotal = 16;

        LogMsg("H5 TX: length(%d)", skb_get_data_length(nskb));
        if(iTempTotal > skb_get_data_length(nskb))
        {
            iTempTotal = skb_get_data_length(nskb);
        }

        for(iTemp = 0; iTemp < iTempTotal; iTemp++)
        {
            LogMsg("0x%x", data[iTemp]);
        }
    }
#endif

    bytes_sent = userial_write(MSG_STACK_TO_HC_HCI_CMD, data, skb_get_data_length(nskb));
    LogMsg("bytes_sent(%d)", bytes_sent);

    skb_free(&nskb);

    return;

}

static void hci_h5_send_sync_req()
{
    uint16_t bytes_sent = 0;
    unsigned char    h5sync[2]     = {0x01, 0x7E};

    sk_buff *nskb = h5_prepare_pkt(&rtk_h5, h5sync, sizeof(h5sync), H5_LINK_CTL_PKT);
    if(nskb == NULL)
    {
        ALOGE("h5_prepare_pkt allocate memory fail");
        return;
    }
    LogMsg("H5: --->>>send sync req");
    uint8_t * data = skb_get_data(nskb);

#if H5_TRACE_DATA_ENABLE
    {
        uint32_t iTemp = 0;
        uint32_t iTempTotal = 16;
        LogMsg("H5 TX: length(%d)", skb_get_data_length(nskb));
        if(iTempTotal > skb_get_data_length(nskb))
        {
            iTempTotal = skb_get_data_length(nskb);
        }
        for(iTemp = 0; iTemp < iTempTotal; iTemp++)
        {
            LogMsg("0x%x", data[iTemp]);
        }
    }
#endif

    bytes_sent = userial_write(MSG_STACK_TO_HC_HCI_CMD, data, skb_get_data_length(nskb));
    LogMsg("bytes_sent(%d)", bytes_sent);

    skb_free(&nskb);

    return;
}

static void hci_h5_send_sync_resp()
{
    uint16_t bytes_sent = 0;
    unsigned char h5syncresp[2] = {0x02, 0x7D};

    sk_buff *nskb = h5_prepare_pkt(&rtk_h5, h5syncresp, sizeof(h5syncresp), H5_LINK_CTL_PKT);
    if(nskb == NULL)
    {
        ALOGE("h5_prepare_pkt allocate memory fail");
        return;
    }
    LogMsg("H5: --->>>send sync resp");
    uint8_t * data = skb_get_data(nskb);

#if H5_TRACE_DATA_ENABLE
    {
        uint32_t iTemp = 0;
        uint32_t iTempTotal = 16;
        LogMsg("H5 TX: length(%d)", skb_get_data_length(nskb));
        if(iTempTotal > skb_get_data_length(nskb))
        {
            iTempTotal = skb_get_data_length(nskb);
        }
        for(iTemp = 0; iTemp < iTempTotal; iTemp++)
        {
            LogMsg("0x%x", data[iTemp]);
        }
    }
#endif

    bytes_sent = userial_write(MSG_STACK_TO_HC_HCI_CMD, data, skb_get_data_length(nskb));
    LogMsg("bytes_sent(%d)", bytes_sent);

    skb_free(&nskb);

    return;
}

static void hci_h5_send_conf_req()
{
    uint16_t bytes_sent = 0;
    unsigned char h5conf[3] = {0x03, 0xFC, 0x14};

    sk_buff *nskb = h5_prepare_pkt(&rtk_h5, h5conf, sizeof(h5conf), H5_LINK_CTL_PKT);
    if(nskb == NULL)
    {
        ALOGE("h5_prepare_pkt allocate memory fail");
        return;
    }
    LogMsg("H5: --->>>send conf req");
    uint8_t * data = skb_get_data(nskb);

#if H5_TRACE_DATA_ENABLE
    {
        uint32_t iTemp = 0;
        uint32_t iTempTotal = 16;
        LogMsg("H5 TX: length(%d)", skb_get_data_length(nskb));
        if(iTempTotal > skb_get_data_length(nskb))
        {
            iTempTotal = skb_get_data_length(nskb);
        }
        for(iTemp = 0; iTemp < iTempTotal; iTemp++)
        {
            LogMsg("0x%x", data[iTemp]);
        }
    }
#endif

    bytes_sent = userial_write(MSG_STACK_TO_HC_HCI_CMD, data, skb_get_data_length(nskb));
    LogMsg("bytes_sent(%d)", bytes_sent);

    skb_free(&nskb);

    return;
}


static void hci_h5_send_conf_resp()
{
    uint16_t bytes_sent = 0;
    unsigned char h5confresp[2] = {0x04, 0x7B};

    sk_buff *nskb = h5_prepare_pkt(&rtk_h5, h5confresp, sizeof(h5confresp), H5_LINK_CTL_PKT);
    if(nskb == NULL)
    {
        ALOGE("h5_prepare_pkt allocate memory fail");
        return;
    }
    LogMsg("H5: --->>>send conf resp");
    uint8_t * data = skb_get_data(nskb);

#if H5_TRACE_DATA_ENABLE
    {
        uint32_t iTemp = 0;
        uint32_t iTempTotal = 16;
        LogMsg("H5 TX: length(%d)", skb_get_data_length(nskb));
        if(iTempTotal > skb_get_data_length(nskb))
        {
            iTempTotal = skb_get_data_length(nskb);
        }
        for(iTemp = 0; iTemp < iTempTotal; iTemp++)
        {
            LogMsg("0x%x", data[iTemp]);
        }
    }
#endif

    bytes_sent = userial_write(MSG_STACK_TO_HC_HCI_CMD, data, skb_get_data_length(nskb));
    LogMsg("bytes_sent(%d)", bytes_sent);

    skb_free(&nskb);

    return;
}

static void rtk_notify_hw_h5_init_result(uint8_t result)
{
        uint16_t len = 0;
        HC_BT_HDR *pH5InitResultPkt = NULL;
        uint8_t * p = NULL;
        unsigned char h5InitOk[6] = {0x0e, 0x04, 0x02, 0xEE, 0xFC, 0x00};

        /* Allocate a buffer for message */
        if (bt_hc_cbacks)
        {
            len = BT_HC_HDR_SIZE + sizeof(h5InitOk);
            pH5InitResultPkt = (HC_BT_HDR *) bt_hc_cbacks->alloc(len);
        }

        if (pH5InitResultPkt)
        {
            /* Initialize buffer with received h5 data */
            pH5InitResultPkt->offset = 0;
            pH5InitResultPkt->layer_specific = 0;
            pH5InitResultPkt->event = MSG_STACK_TO_HC_HCI_CMD;
            pH5InitResultPkt->len = sizeof(h5InitOk);

            memcpy((uint8_t *)(pH5InitResultPkt + 1), h5InitOk, sizeof(h5InitOk));

            if(rtk_h5.cback_h5sync)
                rtk_h5.cback_h5sync(pH5InitResultPkt);

        }

}


static sk_buff *
h5_dequeue(
    )
{
    sk_buff *skb = NULL;
    //   First of all, check for unreliable messages in the queue,
    //   since they have higher priority
    LogMsg("h5_dequeue++");
    if ((skb = (sk_buff*)skb_dequeue_head(rtk_h5.unrel)) != NULL)
    {
        LogMsg("h5_dequeue11");
        sk_buff *nskb = h5_prepare_pkt(&rtk_h5,
                                         skb_get_data(skb),
                                         skb_get_data_length(skb),
                                         skb_get_pkt_type(skb));
        if (nskb)
        {
            LogMsg("h5_dequeue12");
            skb_free(&skb);
            return nskb;
        }
        else
        {
            LogMsg("h5_dequeue13");
            skb_queue_head(rtk_h5.unrel, skb);
        }
    }
    //   Now, try to send a reliable pkt. We can only send a
    //   reliable packet if the number of packets sent but not yet ack'ed
    //   is < than the winsize

//    LogMsg("RtbGetQueueLen(rtk_h5.unack) = (%d), sliding_window_size = (%d)", RtbGetQueueLen(rtk_h5.unack), rtk_h5.sliding_window_size);

    if (RtbGetQueueLen(rtk_h5.unack)< rtk_h5.sliding_window_size &&
        (skb = (sk_buff *)skb_dequeue_head(rtk_h5.rel)) != NULL)
    {
        LogMsg("h5_dequeue21");
        sk_buff *nskb = h5_prepare_pkt(&rtk_h5,
                                         skb_get_data(skb),
                                         skb_get_data_length(skb),
                                         skb_get_pkt_type(skb));
        if (nskb)
        {
            LogMsg("h5_dequeue22");
            skb_queue_tail(rtk_h5.unack, skb);
            h5_start_data_retrans_timer();
            LogMsg("h5_dequeue23");
            return nskb;
        }
        else
        {
            LogMsg("h5_dequeue24");
            skb_queue_head(rtk_h5.rel, skb);
        }
    }
    //   We could not send a reliable packet, either because there are
    //   none or because there are too many unack'ed packets. Did we receive
    //   any packets we have not acknowledged yet
    LogMsg("h5_dequeue3");
    if (rtk_h5.is_txack_req)
    {
        // if so, craft an empty ACK pkt and send it on BCSP unreliable
        // channel
        sk_buff *nskb = h5_prepare_pkt(&rtk_h5, NULL, 0, H5_ACK_PKT);
        return nskb;
    }
    // We have nothing to send
    LogMsg("h5_dequeue4");
    return NULL;
}

int
h5_enqueue(
    IN sk_buff *skb
    )
{
    //Pkt length must be less than 4095 bytes
    if (skb_get_data_length(skb) > 0xFFF)
    {
        ALOGE("skb len > 0xFFF");
        skb_free(&skb);
        return 0;
    }

    switch (skb_get_pkt_type(skb))
    {
    case HCI_ACLDATA_PKT:
    case HCI_COMMAND_PKT:
        skb_queue_tail(rtk_h5.rel, skb);
        break;


    case H5_LINK_CTL_PKT:
    case H5_ACK_PKT:
    case H5_VDRSPEC_PKT:
        skb_queue_tail(rtk_h5.unrel, skb);/* 3-wire LinkEstablishment*/
        break;
    default:
        skb_free(&skb);
        break;
    }
    return 0;
}


static void h5_wake_up()
{
    uint16_t bytes_sent = 0;
    sk_buff *skb = NULL;
    uint8_t * data = NULL;
    uint32_t data_len = 0;

    pthread_mutex_lock(&h5_wakeup_mutex);
    LogMsg("h5_wake_up++");
    while (NULL != (skb = h5_dequeue()))
    {
        data = skb_get_data(skb);
        data_len = skb_get_data_length(skb);
        bytes_sent = userial_write(0, data, data_len);

        LogMsg("bytes_sent(%d)", bytes_sent);

#if H5_TRACE_DATA_ENABLE
        {
            uint32_t iTemp = 0;
            uint32_t iTempTotal = 16;
            LogMsg("H5 TX: length(%d)", data_len);
            if(iTempTotal > data_len)
            {
                iTempTotal = data_len;
            }
            for(iTemp = 0; iTemp < iTempTotal; iTemp++)
            {
                LogMsg("0x%x", data[iTemp]);
            }
        }
#endif
        skb_free(&skb);
    }
    LogMsg("h5_wake_up--");

    pthread_mutex_unlock(&h5_wakeup_mutex);
}


/*******************************************************************************
**
** Function         acl_rx_frame_end_chk
**
** Description      This function is called from the HCI transport when the last
**                  byte of an HCI ACL packet has been received. It checks if
**                  the L2CAP message is complete, i.e. no more continuation
**                  packets are expected.
**
** Returns          TRUE if message complete, FALSE if continuation expected
**
*******************************************************************************/
static uint8_t acl_rx_frame_end_chk (sk_buff * skb_acl)
{
    uint8_t     *data;
    uint16_t   data_len = 0;

    uint16_t    handle, hci_len, l2cap_len;

    uint8_t     frame_end=TRUE;


    data = skb_get_data(skb_acl);
    data_len = skb_get_data_length(skb_acl);
    if(data_len < 6)
    {
        frame_end = FALSE;
        return frame_end;
    }

    STREAM_TO_UINT16 (handle, data);
    STREAM_TO_UINT16 (hci_len, data);
    STREAM_TO_UINT16 (l2cap_len, data);
    LogMsg("handle(0x%x), hci_len(%d), l2cap_len(%d)", handle, hci_len, l2cap_len);

    if (hci_len > 0)
    {
        if (l2cap_len > (data_len - (HCI_ACL_PREAMBLE_SIZE+L2CAP_HEADER_SIZE)) )
        {
            /* If the L2CAP length has not been reached, tell H5 not to send
             * this buffer to stack */
            frame_end = FALSE;
            LogMsg("H5: L2CAP fragment PKT, data_len(%d)", data_len);
        }
        else
        {
            LogMsg("H5: L2CAP Complete PKT, data_len(%d)", data_len);
        }
    }

    return frame_end;
}

void h5_process_ctl_pkts(void)
{
    //process h5 link establish
    int len;
    H5_CONFIG_FIELD f = {0,0,0,0};

    struct  tHCI_H5_CB  *p_cb=&rtk_h5;
    sk_buff * skb = rtk_h5.rx_skb;

    unsigned char    h5sync[2]     = {0x01, 0x7E},
                            h5syncresp[2] = {0x02, 0x7D},
                            h5conf[3]     = {0x03, 0xFC, 0x14},
                            h5confresp[2] = {0x04, 0x7B},
                            h5InitOk[2] = {0xF1, 0xF1};

    uint8_t *ph5_payload = NULL;
    ph5_payload = (uint8_t *)(p_cb->p_rcv_msg + 1);


    if(rtk_h5.link_estab_state == H5_UNINITIALIZED) {  //sync
        if (!memcmp(skb_get_data(skb), h5sync, 2))
        {
            LogMsg("H5: <<<---recv sync req");
            hci_h5_send_sync_resp();
        }
        else if (!memcmp(skb_get_data(skb), h5syncresp, 2))
        {
            LogMsg("H5: <<<---recv sync resp");
            h5_stop_sync_retrans_timer();
            rtk_h5.sync_retrans_count  = 0;
            rtk_h5.link_estab_state = H5_INITIALIZED;

              //send config req
              hci_h5_send_conf_req();
              h5_start_conf_retrans_timer();
        }

    }
    else if(rtk_h5.link_estab_state == H5_INITIALIZED) {  //config
        if (!memcmp(skb_get_data(skb), h5sync, 0x2)) {

            LogMsg("H5: <<<---recv sync req in H5_INITIALIZED");
            hci_h5_send_sync_resp();
        }
        else if (!memcmp(skb_get_data(skb), h5conf, 0x2)) {
             LogMsg("H5: <<<---recv conf req");
             hci_h5_send_conf_resp();
        }
        else if (!memcmp(skb_get_data(skb), h5confresp,  0x2)) {
            LogMsg("H5: <<<---recv conf resp");
            h5_stop_conf_retrans_timer();
            rtk_h5.conf_retrans_count  = 0;

            rtk_h5.link_estab_state = H5_ACTIVE;
            //notify hw to download patch
            memcpy(&f, skb_get_data(skb)+2, sizeof(f));
            rtk_h5.sliding_window_size = f.SlidingWindowSize;
            rtk_h5.oof_flow_control = f.OofFlowControl;
            rtk_h5.dic_type = f.DicType;
            LogMsg("rtk_h5.sliding_window_size(%d), oof_flow_control(%d), dic_type(%d)",
            rtk_h5.sliding_window_size, rtk_h5.oof_flow_control, rtk_h5.dic_type);
         if(rtk_h5.dic_type)
            rtk_h5.use_crc = 1;

            rtk_notify_hw_h5_init_result(1);
        }
        else {
            LogMsg("H5_INITIALIZED receive event, ingnore");
        }
    }
    else if(rtk_h5.link_estab_state == H5_ACTIVE) {
        if (!memcmp(skb_get_data(skb), h5sync, 0x2)) {

            LogMsg("H5: <<<---recv sync req in H5_ACTIVE");
            hci_h5_send_sync_resp();
            LogMsg("H5 : H5_ACTIVE transit to H5_UNINITIALIZED");
            rtk_h5.link_estab_state = H5_UNINITIALIZED;
            hci_h5_send_sync_req();
            h5_start_sync_retrans_timer();
        }
        else if (!memcmp(skb_get_data(skb), h5conf, 0x2)) {
             LogMsg("H5: <<<---recv conf req in H5_ACTIVE");
             hci_h5_send_conf_resp();
        }
        else if (!memcmp(skb_get_data(skb), h5confresp,  0x2)) {
            LogMsg("H5: <<<---recv conf resp in H5_ACTIVE, discard");
        }
        else {
            LogMsg("H5_ACTIVE receive unknown link control msg, ingnore");
        }

    }
}

uint8_t isRtkInternalCommand(uint16_t opcode)
{
    if(opcode == 0xFC17
        || opcode == 0xFC6D
        || opcode == 0xFC20)
    {
        return 1;
    }
    else
    {
        return 0;
    }

}

/*******************************************************************************
**
** Function         internal_event_intercept_h5
**
** Description      This function is called to parse received HCI event and
**                  - update the Num_HCI_Command_Packets
**                  - intercept the event if it is the result of an early
**                    issued internal command.
**
** Returns          TRUE : if the event had been intercepted for internal process
**                  FALSE : send this event to core stack
**
*******************************************************************************/
uint8_t internal_event_intercept_h5(void)
{
    uint8_t internal_command = 0;//if it is internal event, you need to set internal_command = 1;
    struct  tHCI_H5_CB  *p_cb=&rtk_h5;
    sk_buff * skb = rtk_h5.rx_skb;
    uint8_t *ph5_payload = NULL;
    ph5_payload = (uint8_t *)(p_cb->p_rcv_msg + 1);

    //process fw change baudrate and patch download
    uint8_t     *p;
    uint8_t     event_code;
    uint16_t    opcode, len;
    p = (uint8_t *)(p_cb->p_rcv_msg + 1);

    event_code = *p++;
    len = *p++;
    LogMsg("event_code(0x%x)", event_code);

    if (event_code == HCI_COMMAND_COMPLETE_EVT)
    {
        num_hci_cmd_pkts = *p++;
        STREAM_TO_UINT16(opcode, p)

    if (opcode == HCI_READ_BUFFER_SIZE)
    {
        uint8_t status = 0;
        status = *p++;
        if (status == 0)
        {
            uint16_t   hc_acl_data_size = 0;
            uint8_t   hc_sco_data_size = 0;
            uint16_t hc_acl_total_num = 0;
            uint16_t hc_sco_total_num = 0;

            STREAM_TO_UINT16(hc_acl_data_size, p)
            hc_sco_data_size = *p++;
            STREAM_TO_UINT16(hc_acl_total_num, p);
            STREAM_TO_UINT16(hc_sco_total_num, p);

            rtk_h5.hc_acl_data_size = hc_acl_data_size;
            rtk_h5.hc_sco_data_size = hc_sco_data_size;
            rtk_h5.hc_acl_total_num = hc_acl_total_num;
            rtk_h5.hc_sco_total_num = hc_sco_total_num;

            LogMsg("hc_acl_data_size:(%d), hc_sco_data_size:(%d), hc_acl_total_num:(%d), hc_sco_total_num(%d)", \
            hc_acl_data_size, hc_sco_data_size, hc_acl_total_num, hc_sco_total_num);

            rtk_h5.hc_cur_acl_total_num = hc_acl_total_num;
        }
    }
    else
    if (opcode == HCI_LE_READ_BUFFER_SIZE)
    {
        uint8_t status = 0;
        status = *p++;
        if (status == 0)
        {
            uint16_t   hc_ble_acl_data_size = 0;
            uint8_t hc_ble_acl_total_num = 0;

            STREAM_TO_UINT16(hc_ble_acl_data_size, p)
            hc_ble_acl_total_num = *p++;
            rtk_h5.hc_ble_acl_data_size = (hc_ble_acl_data_size) ? hc_ble_acl_data_size : rtk_h5.hc_acl_data_size;
            rtk_h5.hc_ble_acl_total_num = hc_ble_acl_total_num;
            LogMsg("hc_ble_acl_data_size:(%d), hc_ble_acl_total_num:(%d)",  \
            hc_ble_acl_data_size, hc_ble_acl_total_num);
        }
    }

        if (p_cb->int_cmd_rsp_pending > 0)
        {

            LogMsg("CommandCompleteEvent for command (0x%04X)", opcode);
            if (opcode == p_cb->int_cmd[p_cb->int_cmd_rd_idx].opcode)
            {
                if(opcode == 0xFC17)
                {
                    //need to set a timer, add wait for retransfer packet from controller.
                    //if there is no packet rx from controller, we can assure baudrate change success.
                    h5_start_wait_controller_baudrate_ready_timer();
                    internal_command = 1;
                }
                else
                {

                    if(opcode == 0xFC6D)
                        internal_command = 1;

                    if (p_cb->int_cmd[p_cb->int_cmd_rd_idx].cback != NULL)
                    {
                        p_cb->int_cmd[p_cb->int_cmd_rd_idx].cback(p_cb->p_rcv_msg);
                    }
                    else
                    {
                        // Missing cback function!
                        // Release the p_rcv_msg buffer.
                        if (bt_hc_cbacks)
                        {
                            bt_hc_cbacks->dealloc((TRANSAC) p_cb->p_rcv_msg, \
                                                  (char *) (p_cb->p_rcv_msg + 1));
                        }
                    }
                }

                p_cb->int_cmd_rd_idx = ((p_cb->int_cmd_rd_idx+1) & INT_CMD_PKT_IDX_MASK);
                p_cb->int_cmd_rsp_pending--;
                internal_command = 1;
            }
        }
    }
    else if (event_code == HCI_COMMAND_STATUS_EVT)
    {

        num_hci_cmd_pkts = *(++p);
        STREAM_TO_UINT16(opcode, p);
        if(opcode == 0x0001)
        {
            //it is internal command
            internal_command = 1;
        }
    }
    else if(event_code == HCI_NUM_OF_CMP_PKTS_EVT)
    {
        uint8_t i = 0;
        uint8_t num_of_handle =  *p++;
        uint16_t handle = 0;
        uint16_t NumOcp = 0;
        uint8_t *phandle_list  = NULL;
        uint8_t *pNocp_list = NULL;
        HCI_CONN *phci_conn = NULL;

        phandle_list = p;
        pNocp_list = p + num_of_handle*sizeof(handle);

        if (len < num_of_handle*4+sizeof(num_of_handle))
        {
            ALOGE("len is error");
            internal_command = 0;
            return internal_command;
        }

        for (i = 0; i< num_of_handle; i++)
        {
            STREAM_TO_UINT16(handle, phandle_list)
            phci_conn = ConnHashLookupByHandle(&rtk_h5, handle);
            if (phci_conn)
            {
                STREAM_TO_UINT16(NumOcp, pNocp_list)
                LogMsg("NumOcp(%d), hc_cur_acl_total_num(%d)", NumOcp, rtk_h5.hc_cur_acl_total_num);
                rtk_h5.hc_cur_acl_total_num += NumOcp;
                phci_conn->NumOfNotCmpAclPkts -= NumOcp;

                //queue data to h5 to send
                while(rtk_h5.hc_cur_acl_total_num>0
                &&(NULL != (skb = skb_dequeue_head(phci_conn->pending_pkts))) )
                {
                    h5_enqueue(skb);
                    rtk_h5.hc_cur_acl_total_num--;
                    phci_conn->NumOfNotCmpAclPkts++;
                    h5_wake_up();
                }

            }
        }

    }
    else if(event_code == HCI_CONNECTION_COMP_EVT)
    {
        uint8_t status = 0;
        uint16_t handle = 0;
        bdaddr_t bd_addr ;
        uint8_t link_type = 0;
        uint8_t encrypt_enabled = 0;

        HCI_CONN * hci_conn = NULL;

        status = *p++;
        STREAM_TO_UINT16 (handle, p);
        memcpy(&bd_addr, p, 6);
        p +=6;
        link_type = *p++;
        encrypt_enabled = *p;

        LogMsg("CCP EVT: status(%d), handle(0x%x), link_type(0x%x), encrypt_enabled(%d)", status, handle, link_type, encrypt_enabled);
        baPrint(bd_addr);

        if(status == 0)
        {
            hci_conn = ConnHashLookupByHandle(&rtk_h5, handle);
            if(hci_conn == NULL)
            {
                hci_conn = HciConnAllocate(handle);
                if(hci_conn)
                {
                    ConnHashAdd(&rtk_h5, hci_conn);
                    bacpy(&hci_conn->bd_addr, &bd_addr);
                    hci_conn->link_type = link_type;
                    hci_conn->encrypt_enabled = encrypt_enabled;
                    hci_conn->NumOfNotCmpAclPkts = 0;

                }
                else
                {
                    ALOGE("HciConnAllocate fail");
                }
            }
            else
            {
                ALOGE("HCI Connection handle(0x%x) has already exist!", handle);
                bacpy(&hci_conn->bd_addr, &bd_addr);
                hci_conn->link_type = link_type;
                hci_conn->encrypt_enabled = encrypt_enabled;
                hci_conn->NumOfNotCmpAclPkts = 0;

            }
            LogMsg("hc_cur_acl_total_num(%d)", rtk_h5.hc_cur_acl_total_num);
        }

    }
    //add for le connection start
    else if(event_code == HCI_BLE_EVT)
    {
        uint8_t status = 0;
        uint16_t handle = 0;
        bdaddr_t bd_addr ;
        uint8_t link_type = 0;
        uint8_t encrypt_enabled = 0;
        uint8_t role;
        uint8_t addr_type;
        uint8_t ble_sub_code;

        HCI_CONN * hci_conn = NULL;
        ble_sub_code = *p++;
        LogMsg("HCI_BLE_EVT with sub event code 0x%x", ble_sub_code);
        if(ble_sub_code != 0x01)
            return 0;
        status = *p++;
        STREAM_TO_UINT16 (handle, p);
        p += 2;
        memcpy(&bd_addr, p, 6);
        p +=6;
        baPrint(bd_addr);

        if(status == 0)
        {
            hci_conn = ConnHashLookupByHandle(&rtk_h5, handle);
            if(hci_conn == NULL)
            {
                hci_conn = HciConnAllocate(handle);
                if(hci_conn)
                {
                    ConnHashAdd(&rtk_h5, hci_conn);
                    bacpy(&hci_conn->bd_addr, &bd_addr);
                    hci_conn->link_type = link_type;
                    hci_conn->encrypt_enabled = encrypt_enabled;
                    hci_conn->NumOfNotCmpAclPkts = 0;

                }
                else
                {
                    ALOGE("HciConnAllocate fail");
                }
            }
            else
            {
                ALOGE("HCI Connection handle(0x%x) has already exist!", handle);
                bacpy(&hci_conn->bd_addr, &bd_addr);
                hci_conn->link_type = link_type;
                hci_conn->encrypt_enabled = encrypt_enabled;
                hci_conn->NumOfNotCmpAclPkts = 0;

            }
            ALOGI("hc_cur_acl_total_num(%d)", rtk_h5.hc_cur_acl_total_num);
        }

    }
    //add for le connection end
    else if(event_code == HCI_DISCONNECTION_COMP_EVT)
    {
        uint8_t status = 0;
        uint16_t handle = 0;
        uint8_t reason = 0;
        HCI_CONN * hci_conn = NULL;
        status = *p++;
        STREAM_TO_UINT16(handle, p);
        reason = *p;
        LogMsg("DCP EVT: status(%d), handle(0x%x), reason(0x%x)", status, handle, reason);
        LogMsg("hc_cur_acl_total_num(%d)", rtk_h5.hc_cur_acl_total_num);
        {
            hci_conn = ConnHashLookupByHandle(&rtk_h5, handle);
            if(hci_conn)
            {
                rtk_h5.hc_cur_acl_total_num += hci_conn->NumOfNotCmpAclPkts;
                LogMsg("hc_cur_acl_total_num(%d)", rtk_h5.hc_cur_acl_total_num);
                ConnHashDelete(hci_conn);
            }
            else
            {
                ALOGE("HCI Connection handle(0x%x) not found", handle);
            }
        }
    }

#ifdef BT_FW_CAL_ENABLE
   else if(event_code == 0x02||
        event_code == 0x22||
        event_code == 0x2f)
    {
        if(rtk_h5.bHasUpdateCalInquiryState == FALSE)
        {
            LogMsg("H5: Update CAL Inquiry success info to cal file");
            rtk_set_bt_cal_inqury_result(CAL_INQUIRY_SUCCESS);
            rtk_h5.bHasUpdateCalInquiryState = TRUE;
        }
    }
#endif

    return internal_command;

}


/**
* Check if it's a hci frame, if it is, complete it with response or parse the cmd complete event
*
* @param skb socket buffer
*
*/
static void hci_recv_frame(sk_buff *skb, uint8_t pkt_type)
{
    uint8_t intercepted = FALSE;
    uint32_t i = 0 ;
    uint8_t *data = skb_get_data(skb);
    uint32_t data_len = skb_get_data_length(skb);

    LogMsg("UART H5 RX: length = %d", data_len);

#if H5_TRACE_DATA_ENABLE
    {
        uint32_t iTemp = 0;
        uint32_t iTempTotal = 16;
        LogMsg("H5 RX: length(%d)", data_len);
        if(iTempTotal > data_len)
        {
            iTempTotal = data_len;
        }
        for(iTemp = 0; iTemp < iTempTotal; iTemp++)
        {
            LogMsg("0x%x", data[iTemp]);
        }
    }
#endif
    //we only intercept evt packet here
    if(pkt_type == HCI_EVENT_PKT)
    {
        intercepted = internal_event_intercept_h5();
    }

    //if you want to intercept more pakcets such as acl data, you can add code here

    LogMsg("intercepted = %d", intercepted);
    if ((bt_hc_cbacks) && (intercepted == FALSE))
    {
        bt_hc_cbacks->data_ind((TRANSAC) rtk_h5.p_rcv_msg, \
                                                (char *) (rtk_h5.p_rcv_msg + 1), \
                                                rtk_h5.p_rcv_msg->len + BT_HC_HDR_SIZE);
    }

}

/**


    @return 1- complete_pkt, 0 not a complete pkt.
*/


uint8_t hci_rx_dispatch_by_handle(sk_buff* rx_skb)
{
    uint8_t complete_pkt = FALSE;
    uint16_t handle = 0;
    HCI_CONN *hci_conn = NULL;

    uint8_t     *rx_skb_data = NULL;
    uint16_t   rx_skb_data_len = 0;

    //get acl handle from acl data
    rx_skb_data = skb_get_data(rx_skb);
    rx_skb_data_len = skb_get_data_length(rx_skb);

    //print snoop log
    if(h5_log_enable == 1)
    {
        HC_BT_HDR *p_rcv_msg = NULL;          /* Buffer to hold current rx HCI message */
        p_rcv_msg = (HC_BT_HDR *) bt_hc_cbacks->alloc(BT_HC_HDR_SIZE + rx_skb_data_len);
        if (p_rcv_msg != NULL)
        {
            /* Initialize buffer with received h5 data */
            p_rcv_msg->offset = 0;
            p_rcv_msg->layer_specific = 0;
            p_rcv_msg->event = MSG_HC_TO_STACK_HCI_ACL;
            p_rcv_msg->len = rx_skb_data_len;
            memcpy((uint8_t *)(p_rcv_msg + 1), rx_skb_data, rx_skb_data_len);
            btsnoop_capture(p_rcv_msg, TRUE);
            bt_hc_cbacks->dealloc((TRANSAC) p_rcv_msg, (char *) (p_rcv_msg + 1));
        }
    }
    //end print snoop log

    STREAM_TO_UINT16 (handle, rx_skb_data);
    //get acl handle, 12bit only.
    handle = handle & 0x0FFF ;
    LogMsg("hci_rx_dispatch_by_handle: (0x%x)",handle);
    //look up hci connection by handle
    hci_conn = ConnHashLookupByHandle(&rtk_h5, handle);
    if(hci_conn == NULL)
    {
        hci_conn = HciConnAllocate(handle);
        if(hci_conn)
        {
            ConnHashAdd(&rtk_h5, hci_conn);
        }
        else
        {
            ALOGE("HciConnAllocate fail");
            return 0;
        }
    }

    //if it is first packet
    if(hci_conn->rx_skb == NULL)
    {
        uint8_t* data = NULL;
        hci_conn->rx_skb = skb_alloc(skb_get_data_length(rx_skb));
        data = skb_get_data(hci_conn->rx_skb);
        memcpy(data, skb_get_data(rx_skb), skb_get_data_length(rx_skb));
        hci_conn->rx_skb->Length = skb_get_data_length(rx_skb);
    }
    else
    {
        sk_buff* skb_total = NULL;

        uint16_t len_total = 0;
        uint8_t * data_total = NULL;

        uint32_t hci_len_rx_skb_cur = skb_get_hci_data_length(rx_skb);
        uint32_t hci_len_rx_skb_last = skb_get_hci_data_length(hci_conn->rx_skb);

        //pull hci header here, now rx_skb only contains L2CAP packets
        skb_pull(rx_skb, HCI_ACL_PREAMBLE_SIZE);

        //add rx_skb to hci_conn->rx_skb
        len_total = skb_get_data_length(rx_skb) + skb_get_data_length(hci_conn->rx_skb);
        skb_total = skb_alloc(len_total);
        if(skb_total)
        {

            data_total = skb_get_data(skb_total);
            skb_total->Length = len_total;
            memcpy(data_total, skb_get_data(hci_conn->rx_skb), skb_get_data_length(hci_conn->rx_skb));
            memcpy(data_total + skb_get_data_length(hci_conn->rx_skb), skb_get_data(rx_skb), skb_get_data_length(rx_skb));

            //free unused buffer
            skb_free(&hci_conn->rx_skb);

            //
            hci_conn->rx_skb = skb_total;
            //update hci total length
            skb_set_hci_data_length(hci_conn->rx_skb, hci_len_rx_skb_cur + hci_len_rx_skb_last);

        }
        else
        {
            ALOGE("skb_alloc fail");
            return 0;
        }
    }

    //check if it is a complete acl packet, if it is, then replace hci_conn->rx_skb with orignal rx_skb
    if(TRUE == acl_rx_frame_end_chk(hci_conn->rx_skb))
    {
        skb_free(&rx_skb);
        rtk_h5.rx_skb = hci_conn->rx_skb;
        complete_pkt = TRUE;

        //reset hci_conn->rx_skb to NULL
        hci_conn->rx_skb = NULL;
    }
    else
    {
        complete_pkt = FALSE;
    }
    return complete_pkt;
 }

/**
* after rx data is parsed, and we got a rx frame saved in h5->rx_skb,
* this routinue is called.
* things todo in this function:
* 1. check if it's a hci frame, if it is, complete it with response or ack
* 2. see the ack number, free acked frame in queue
* 3. reset h5->rx_state, set rx_skb to null.
*
* @param h5 realtek h5 struct
*
*/
static void h5_complete_rx_pkt(struct tHCI_H5_CB *h5)
{
    int pass_up = 1;
    uint16_t eventtype = 0;
    H5_PKT_HEADER* h5_hdr = NULL;
    uint8_t complete_pkt = TRUE;
    uint8_t pkt_type = 0;
    struct tHCI_H5_CB *p_cb=&rtk_h5;

    //LogMsg("HCI 3wire h5_complete_rx_pkt");
    h5_hdr = (H5_PKT_HEADER * )skb_get_data(h5->rx_skb);
    LogMsg("SeqNumber(%d), AckNumber(%d)", h5_hdr->SeqNumber, h5_hdr->AckNumber);

    if (h5_hdr->ReliablePkt)
    {
        LogMsg("Received reliable seqno %u from card", h5->rxseq_txack);
        h5->rxseq_txack = h5_hdr->SeqNumber + 1;
        h5->rxseq_txack %= 8;
        h5->is_txack_req = 1;
    // send down an empty ack if needed.
        h5_wake_up();
    }

    h5->rxack = h5_hdr->AckNumber;
    pkt_type = h5_hdr->PktType;
    switch (h5_hdr->PktType)
    {
        case HCI_ACLDATA_PKT:
            pass_up = 1;
            eventtype = MSG_HC_TO_STACK_HCI_ACL;
        break;

        case HCI_EVENT_PKT:
            pass_up = 1;
            eventtype = MSG_HC_TO_STACK_HCI_EVT;
            break;

        case HCI_SCODATA_PKT:
            pass_up = 1;
            eventtype = MSG_HC_TO_STACK_HCI_SCO;
            break;
        case HCI_COMMAND_PKT:
            pass_up = 1;
            eventtype = MSG_HC_TO_STACK_HCI_ERR;
            break;

        case H5_LINK_CTL_PKT:
            pass_up = 0;
        break;

        case H5_ACK_PKT:
            pass_up = 0;
            break;

        default:
          ALOGE("Unknown pkt type(%d)", h5_hdr->PktType);
          eventtype = MSG_HC_TO_STACK_HCI_ERR;
          pass_up = 0;
          break;
    }

    // remove h5 header and send packet to hci
    h5_remove_acked_pkt(h5);

    if(h5_hdr->PktType == H5_LINK_CTL_PKT)
    {
        skb_pull(h5->rx_skb, sizeof(H5_PKT_HEADER));
        h5_process_ctl_pkts();
    }

    // decide if we need to pass up.
    if (pass_up)
    {

        skb_pull(h5->rx_skb, sizeof(H5_PKT_HEADER));

        if(eventtype == MSG_HC_TO_STACK_HCI_ACL)
        {
            complete_pkt = hci_rx_dispatch_by_handle(h5->rx_skb);
        }

        //send command or complete acl data it to bluedroid stack
        if(complete_pkt)
        {
            uint16_t len = 0;
            sk_buff * skb_complete_pkt = h5->rx_skb;

            /* Allocate a buffer for message */
            if (bt_hc_cbacks)
            {
                len = BT_HC_HDR_SIZE + skb_get_data_length(skb_complete_pkt);
                h5->p_rcv_msg = (HC_BT_HDR *) bt_hc_cbacks->alloc(len);
            }

            if (h5->p_rcv_msg)
            {
                /* Initialize buffer with received h5 data */
                h5->p_rcv_msg->offset = 0;
                h5->p_rcv_msg->layer_specific = 0;
                h5->p_rcv_msg->event = eventtype;
                h5->p_rcv_msg->len = skb_get_data_length(skb_complete_pkt);
                memcpy((uint8_t *)(h5->p_rcv_msg + 1), skb_get_data(skb_complete_pkt), skb_get_data_length(skb_complete_pkt));
            }

            /* generate snoop trace message */
            /* ACL packet tracing had done in acl_rx_frame_end_chk() */
            if (p_cb->p_rcv_msg->event != MSG_HC_TO_STACK_HCI_ACL)
                btsnoop_capture(p_cb->p_rcv_msg, TRUE);

            hci_recv_frame(skb_complete_pkt, pkt_type);
        }

    }


    skb_free(&h5->rx_skb);

    h5->rx_state = H5_W4_PKT_DELIMITER;
    h5->rx_skb = NULL;
}

/**
* Parse the receive data in h5 proto.
*
* @param h5 realtek h5 struct
* @param data point to data received before parse
* @param count num of data
* @return reserved count
*/
static int h5_recv(struct tHCI_H5_CB *h5, void *data, int count)
{
    unsigned char *ptr;
    uint8_t * skb_data = NULL;
    H5_PKT_HEADER * hdr = NULL;

    //LogMsg("count %d rx_state %d rx_count %ld", count, h5->rx_state, h5->rx_count);
    ptr = (unsigned char *)data;

    while (count)
    {
        if (h5->rx_count)
        {
            if (*ptr == 0xc0)
            {
                ALOGE("short h5 packet");
                skb_free(&h5->rx_skb);
                h5->rx_state = H5_W4_PKT_START;
                h5->rx_count = 0;
            } else
                h5_unslip_one_byte(h5, *ptr);

            ptr++; count--;
            continue;
        }

        switch (h5->rx_state)
        {
        case H5_W4_HDR:
            // check header checksum. see Core Spec V4 "3-wire uart" page 67
            skb_data = skb_get_data(h5->rx_skb);
            hdr = (H5_PKT_HEADER *)skb_data;

            if ((0xff & (uint8_t) ~ (skb_data[0] + skb_data[1] +
                                   skb_data[2])) != skb_data[3])
            {
                ALOGE("h5 hdr checksum error!!!");
                skb_free(&h5->rx_skb);
                h5->rx_state = H5_W4_PKT_DELIMITER;
                h5->rx_count = 0;
                continue;
            }

              if (hdr->ReliablePkt
                && (hdr->SeqNumber!= h5->rxseq_txack))
            {
                ALOGE("Out-of-order packet arrived, got(%u)expected(%u)",
                   hdr->SeqNumber, h5->rxseq_txack);
                h5->is_txack_req = 1;
                h5_wake_up();

                skb_free(&h5->rx_skb);
                h5->rx_state = H5_W4_PKT_DELIMITER;
                h5->rx_count = 0;

                continue;
            }
            h5->rx_state = H5_W4_DATA;
            //payload length: May be 0
            h5->rx_count = hdr->PayloadLen;
            continue;
        case H5_W4_DATA:
            hdr = (H5_PKT_HEADER *)skb_get_data(h5->rx_skb);
            if (hdr->DicPresent)
            {   // pkt with crc /
                h5->rx_state = H5_W4_CRC;
                h5->rx_count = 2;
            }
            else
            {
                h5_complete_rx_pkt(h5); //Send ACK
                //LogMsg(DF_SLIP,("--------> H5_W4_DATA ACK\n"));
            }
            continue;

        case H5_W4_CRC:
            if (bit_rev16(h5->message_crc) != h5_get_crc(h5))
            {
                ALOGE("Checksum failed, computed(%04x)received(%04x)",
                    bit_rev16(h5->message_crc), h5_get_crc(h5));
                skb_free(&h5->rx_skb);
                h5->rx_state = H5_W4_PKT_DELIMITER;
                h5->rx_count = 0;
                continue;
            }
            skb_trim(h5->rx_skb, skb_get_data_length(h5->rx_skb) - 2);
            h5_complete_rx_pkt(h5);
            continue;

        case H5_W4_PKT_DELIMITER:
            switch (*ptr)
            {
            case 0xc0:
                h5->rx_state = H5_W4_PKT_START;
                break;
            default:
                break;
            }
            ptr++; count--;
            break;

        case H5_W4_PKT_START:
            switch (*ptr)
            {
            case 0xc0:
                ptr++; count--;
                break;
            default:
                h5->rx_state = H5_W4_HDR;
                h5->rx_count = 4;
                h5->rx_esc_state = H5_ESCSTATE_NOESC;
                H5_CRC_INIT(h5->message_crc);

                // Do not increment ptr or decrement count
                // Allocate packet. Max len of a H5 pkt=
                // 0xFFF (payload) +4 (header) +2 (crc)
                h5->rx_skb = skb_alloc(0x1005);
                if (!h5->rx_skb)
                {
                    h5->rx_state = H5_W4_PKT_DELIMITER;
                    h5->rx_count = 0;
                    return 0;
                }
                break;
            }
            break;
        }
    }
    return count;
}

/******************************************************************************
**  Static functions
******************************************************************************/

/*******************************************************************************
**
** Function         get_acl_data_length_cback_h5
**
** Description      Callback function for HCI_READ_BUFFER_SIZE and
**                  HCI_LE_READ_BUFFER_SIZE commands if they were sent because
**                  of internal request.
**
Parameters:HCI_READ_BUFFER_SIZE
Status,                                                 1byte
HC_ACL_Data_Packet_Length,              2byte
HC_Synchronous_Data_Packet_Length, 1byte
HC_Total_Num_ACL_Data_Packets,       2byte
HC_Total_Num_Synchronous_Data_Packets 2byte

Parameters:HCI_LE_READ_BUFFER_SIZE
Status,                                                     1byte
HC_LE_ACL_Data_Packet_Length,               2byte
HC_Total_Num_LE_ACL_Data_Packets        1byte

** Returns          None
**
*******************************************************************************/

    uint16_t hc_acl_data_size;      /* Controller's max ACL data length */
    uint8_t   hc_sco_data_size;
    uint16_t hc_acl_total_num;      /* Controller's total ACL number packets */
    uint16_t hc_sco_total_num;      /* Controller's total ACL number packets */
    uint16_t hc_ble_acl_data_size;  /* Controller's max BLE ACL data length */
    uint16_t hc_ble_acl_total_num;  /* Controller's max BLE ACL data length */

void get_acl_data_length_cback_h5(void *p_mem)
{
    uint8_t     *p, status;
    uint16_t    opcode, len=0;
    HC_BT_HDR   *p_buf = (HC_BT_HDR *) p_mem;

    p = (uint8_t *)(p_buf + 1) + 3;
    STREAM_TO_UINT16(opcode, p)
    status = *p++;
    if (status == 0) /* Success */
    {
        STREAM_TO_UINT16(len, p);
    }


    LogMsg("get_acl_data_length_cback_h5: opcode(0x%x)", opcode);
    if (opcode == HCI_READ_BUFFER_SIZE)
    {
        if (status == 0)
        {
            uint8_t   hc_sco_data_size = 0;
            uint16_t hc_acl_total_num = 0;
            uint16_t hc_sco_total_num = 0;
            hc_sco_data_size = *p++;
            STREAM_TO_UINT16(hc_acl_total_num, p);
            STREAM_TO_UINT16(hc_sco_total_num, p);

            rtk_h5.hc_acl_data_size = len;
            rtk_h5.hc_sco_data_size = hc_sco_data_size;
            rtk_h5.hc_acl_total_num = hc_acl_total_num;
            rtk_h5.hc_sco_total_num = hc_sco_total_num;

            LogMsg("hc_acl_data_size:(%d), hc_sco_data_size:(%d), hc_acl_total_num:(%d), hc_sco_total_num(%d)", \
               len, hc_sco_data_size, hc_acl_total_num, hc_sco_total_num);

            rtk_h5.hc_cur_acl_total_num = hc_acl_total_num;
        }

        /* reuse the rx buffer for sending HCI_LE_READ_BUFFER_SIZE command */
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->layer_specific = 0;
        p_buf->len = 3;

        p = (uint8_t *) (p_buf + 1);
        UINT16_TO_STREAM(p, HCI_LE_READ_BUFFER_SIZE);
        *p = 0;

        if ((status = hci_h5_send_int_cmd(HCI_LE_READ_BUFFER_SIZE, p_buf, \
                                           get_acl_data_length_cback_h5)) == FALSE)
        {
            bt_hc_cbacks->dealloc((TRANSAC) p_buf, (char *) (p_buf + 1));
            bt_hc_cbacks->postload_cb(NULL, BT_HC_POSTLOAD_SUCCESS);
        }
    }
    else if (opcode == HCI_LE_READ_BUFFER_SIZE)
    {
        if (status == 0)
        {
            uint8_t hc_ble_acl_total_num = 0;
            hc_ble_acl_total_num = *p++;
            rtk_h5.hc_ble_acl_data_size = (len) ? len : rtk_h5.hc_acl_data_size;
            rtk_h5.hc_ble_acl_total_num = hc_ble_acl_total_num;
            LogMsg("hc_ble_acl_data_size:(%d), hc_ble_acl_total_num:(%d)",  \
               len, hc_ble_acl_total_num);
        }

        if (bt_hc_cbacks)
        {
            bt_hc_cbacks->dealloc((TRANSAC) p_buf, (char *) (p_buf + 1));
            ALOGE("vendor lib postload completed");
            bt_hc_cbacks->postload_cb(NULL, BT_HC_POSTLOAD_SUCCESS);
        }
    }
}

static void data_retransfer_thread(void *arg)
{
    uint16_t events;
    uint32_t data_len = 0;
    uint8_t* pdata = NULL;
    uint32_t i = 0;

    LogMsg("data_retransfer_thread started");

    prctl(PR_SET_NAME, (unsigned long)"data_retransfer_thread", 0, 0, 0);

    while (h5_retransfer_running)
    {
        pthread_mutex_lock(&rtk_h5.mutex);
        while (h5_ready_events == 0)
        {
            pthread_cond_wait(&rtk_h5.cond, &rtk_h5.mutex);
        }
        events = h5_ready_events;
        h5_ready_events = 0;
        pthread_mutex_unlock(&rtk_h5.mutex);

        if (events & H5_EVENT_RX)
        {
            sk_buff *skb;
            ALOGE("retransmitting (%u) pkts, retransfer count(%d)", skb_queue_get_length(rtk_h5.unack), rtk_h5.data_retrans_count);
            if(rtk_h5.data_retrans_count < DATA_RETRANS_COUNT)
            {
                while ((skb = skb_dequeue_tail(rtk_h5.unack)) != NULL)
                {
                    data_len = skb_get_data_length(skb);
                    pdata = skb_get_data(skb);
                    if(data_len>16)
                     data_len=16;

                    for(i = 0 ; i < data_len; i++)
                        ALOGE("0x%02X", pdata[i]);

                    rtk_h5.msgq_txseq = (rtk_h5.msgq_txseq - 1) & 0x07;
                    skb_queue_head(rtk_h5.rel, skb);

                }
                rtk_h5.data_retrans_count++;
                h5_wake_up();

            }
            else
            {
            //do not put packet to rel queue, and do not send
            //Kill bluetooth
            kill(getpid(), SIGKILL);
            }

        }
        else
        if (events & H5_EVENT_EXIT)
        {
            break;
        }

}

    LogMsg("data_retransfer_thread exiting");
    pthread_exit(NULL);

}

void h5_retransfer_signal_event(uint16_t event)
{
    pthread_mutex_lock(&rtk_h5.mutex);
    h5_ready_events |= event;
    pthread_cond_signal(&rtk_h5.cond);
    pthread_mutex_unlock(&rtk_h5.mutex);
}



static int create_data_retransfer_thread()
{
    struct sched_param param;
    int policy;

    pthread_attr_t thread_attr;


    if (h5_retransfer_running)
    {
        ALOGW("create_data_retransfer_thread has been called repeatedly without calling cleanup ?");
    }

    h5_retransfer_running = 1;
    h5_ready_events = 0;

    pthread_attr_init(&thread_attr);
    pthread_mutex_init(&rtk_h5.mutex, NULL);
    pthread_cond_init(&rtk_h5.cond, NULL);

    if (pthread_create(&rtk_h5.thread_data_retrans, &thread_attr, \
               (void*)data_retransfer_thread, NULL) != 0)
    {
        ALOGE("pthread_create failed!");
        h5_retransfer_running = 0;
        return -1 ;
    }
/*
    if(pthread_getschedparam(hc_cb.worker_thread, &policy, &param)==0)
    {
        policy = BTHC_LINUX_BASE_POLICY;

#if (BTHC_LINUX_BASE_POLICY!=SCHED_NORMAL)
        param.sched_priority = BTHC_MAIN_THREAD_PRIORITY;
#endif
        result = pthread_setschedparam(hc_cb.worker_thread, policy, &param);
        if (result != 0)
        {
            ALOGW("create_data_retransfer_thread pthread_setschedparam failed (%s)", \
            strerror(result));
        }
    }
*/
    return 0;

}

/*****************************************************************************
**   HCI H5 INTERFACE FUNCTIONS
*****************************************************************************/

/*******************************************************************************
**
** Function        hci_h5_init
**
** Description     Initialize H5 module
**
** Returns         None
**
*******************************************************************************/
void hci_h5_init(void)
{
    LogMsg("hci_h5_init");

    memset(&rtk_h5, 0, sizeof(struct tHCI_H5_CB));
    utils_queue_init(&(rtk_h5.acl_rx_q));

    /* Per HCI spec., always starts with 1 */
    num_hci_cmd_pkts = 1;
    h5_log_enable = 0;
    /* Give an initial values of Host Controller's ACL data packet length
     * Will update with an internal HCI(_LE)_Read_Buffer_Size request
     */
    rtk_h5.hc_acl_data_size = 820;//default value for 8723
    rtk_h5.hc_ble_acl_data_size = 27;
    rtk_h5.hc_cur_acl_total_num = 8;


    h5_alloc_data_retrans_timer();
    h5_alloc_sync_retrans_timer();
    h5_alloc_conf_retrans_timer();
    h5_alloc_wait_controller_baudrate_ready_timer();
    h5_alloc_hw_init_ready_timer();

    rtk_h5.thread_data_retrans = -1;

    if(create_data_retransfer_thread() != 0)
        ALOGE("H5 create_data_retransfer_thread failed");


    rtk_h5.unack = RtbQueueInit();
    rtk_h5.rel = RtbQueueInit();
    rtk_h5.unrel = RtbQueueInit();
    ConnHashInit(&rtk_h5);


    rtk_h5.rx_state = H5_W4_PKT_DELIMITER;
    rtk_h5.rx_esc_state = H5_ESCSTATE_NOESC;

#ifdef BT_FW_CAL_ENABLE
    rtk_h5.bHasUpdateCalInquiryState = FALSE;
#endif

    btsnoop_init();
}

/*******************************************************************************
**
** Function        hci_h5_cleanup
**
** Description     Clean H5 module
**
** Returns         None
**
*******************************************************************************/
void hci_h5_cleanup(void)
{
    LogMsg("hci_h5_cleanup++");
    uint8_t try_cnt=10;
    int result;

    rtk_h5.cleanuping = 1;

    btsnoop_close();
    btsnoop_cleanup();

    h5_free_data_retrans_timer();
    h5_free_sync_retrans_timer();
    h5_free_conf_retrans_timer();
    h5_free_wait_controller_baudrate_ready_timer();
    h5_free_hw_init_ready_timer();


    if (h5_retransfer_running)
    {
        h5_retransfer_running = 0;
        h5_retransfer_signal_event(H5_EVENT_EXIT);
        if ((result=pthread_join(rtk_h5.thread_data_retrans, NULL)) < 0)
        ALOGE( "H5 pthread_join() FAILED result:%d", result);
    }

    h5_ms_delay(200);

    pthread_mutex_destroy(&rtk_h5.mutex);
    pthread_cond_destroy(&rtk_h5.cond);

    RtbQueueFree(rtk_h5.unack);
    RtbQueueFree(rtk_h5.rel);
    RtbQueueFree(rtk_h5.unrel);
    ConnHashFlush(&rtk_h5);

    LogMsg("hci_h5_cleanup--");

}





/*******************************************************************************
**
** Function        hci_h5_send_msg
**
** Description     Determine message type, set HCI H5 packet indicator, and
**                 send message through USERIAL driver
**
** Returns         None
**
*******************************************************************************/
void hci_h5_send_msg(HC_BT_HDR *p_msg)
{
    uint8_t type = 0;
    uint16_t handle;
    uint16_t bytes_to_send, lay_spec;
    uint8_t *p = ((uint8_t *)(p_msg + 1)) + p_msg->offset;
    uint16_t event = p_msg->event & MSG_EVT_MASK;
    uint16_t sub_event = p_msg->event & MSG_SUB_EVT_MASK;
    uint16_t acl_pkt_size = 0, acl_data_size = 0;
    uint16_t bytes_sent;

    uint8_t *pdata_h5 = NULL;
    uint16_t len_h5_data = 0;

    uint8_t *p_h5 = NULL;
    uint32_t iTemp = 0;
    uint32_t iTempTotal = 16;

    sk_buff * skb = NULL;
    HCI_CONN * hci_conn = NULL;

    LogMsg("hci_h5_send_msg, len =%d", p_msg->len);


    if (event == MSG_STACK_TO_HC_HCI_ACL)
       type = HCI_ACLDATA_PKT;
    else if (event == MSG_STACK_TO_HC_HCI_SCO)
        type = HCI_SCODATA_PKT;
    else if (event == MSG_STACK_TO_HC_HCI_CMD)
            type = HCI_COMMAND_PKT;

   if (sub_event == LOCAL_BR_EDR_CONTROLLER_ID)
    {
        acl_data_size = rtk_h5.hc_acl_data_size;
        acl_pkt_size = rtk_h5.hc_acl_data_size + HCI_ACL_PREAMBLE_SIZE;
    }
    else
    {
        acl_data_size = rtk_h5.hc_ble_acl_data_size;
        acl_pkt_size = rtk_h5.hc_ble_acl_data_size + HCI_ACL_PREAMBLE_SIZE;
    }
    LogMsg("acl_data_size = %d, acl_pkt_size = (%d), p_msg->offset=(%d)", acl_data_size, acl_pkt_size, p_msg->offset);

    //
    if (event == MSG_STACK_TO_HC_HCI_ACL)
    {
        uint8_t *pTemp = p;
        STREAM_TO_UINT16 (handle, pTemp);
        //get acl handle, 12bit only.
        handle = handle & 0x0FFF ;
        hci_conn = ConnHashLookupByHandle(&rtk_h5, handle);
        if(hci_conn == NULL)
        {
            ALOGE("HCI connection handle (0x%x) has been disconnected or not connected", handle);
            ALOGE("Return NOT Send this packet!!");
            return;
        }
    }

    /* Check if sending ACL data that needs fragmenting */
    if ((event == MSG_STACK_TO_HC_HCI_ACL) && (p_msg->len > acl_pkt_size))
    {
        /* Get the handle from the packet */
        STREAM_TO_UINT16 (handle, p);

        /* Set packet boundary flags to "continuation packet" */
        handle = (handle & 0xCFFF) | 0x1000;

        /* Do all the first chunks */
        while (p_msg->len > acl_pkt_size)
        {
            /* remember layer_specific because uart borrow
               one byte from layer_specific for packet type */
            lay_spec = p_msg->layer_specific;

            p = ((uint8_t *)(p_msg + 1)) + p_msg->offset;

            //
            skb = skb_alloc_and_init(type, p, acl_pkt_size);


            //check whether this ACL conn has ability to send
            LogMsg("hc_cur_acl_total_num(%d), hc_acl_total_num(%d)", rtk_h5.hc_cur_acl_total_num, rtk_h5.hc_acl_total_num);
            if(rtk_h5.hc_cur_acl_total_num>0)
            {
                h5_enqueue(skb);
                rtk_h5.hc_cur_acl_total_num--;
                hci_conn->NumOfNotCmpAclPkts++;
            }
            else
            {
                //store skb in pending pkts queue
                ALOGE("******************(%d)", rtk_h5.hc_cur_acl_total_num);

                skb_queue_tail(hci_conn->pending_pkts, skb);
            }


            /* generate snoop trace message */
            btsnoop_capture(p_msg, FALSE);

            p_msg->layer_specific = lay_spec;
            /* Adjust offset and length for what we just sent */
            p_msg->offset += acl_data_size;
            p_msg->len    -= acl_data_size;

            p = ((uint8_t *)(p_msg + 1)) + p_msg->offset;

            UINT16_TO_STREAM (p, handle);

            if (p_msg->len > acl_pkt_size)
            {
                UINT16_TO_STREAM (p, acl_data_size);
            }
            else
            {
                UINT16_TO_STREAM (p, p_msg->len - HCI_ACL_PREAMBLE_SIZE);
            }

            /* If we were only to send partial buffer, stop when done.    */
            /* Send the buffer back to L2CAP to send the rest of it later */
            if (p_msg->layer_specific)
            {
                if (--p_msg->layer_specific == 0)
                {
                    p_msg->event = MSG_HC_TO_STACK_L2C_SEG_XMIT;

                    if (bt_hc_cbacks)
                    {
                        bt_hc_cbacks->tx_result((TRANSAC) p_msg, \
                                                    (char *) (p_msg + 1), \
                                                    BT_HC_TX_FRAGMENT);
                    }

                    return;
                }
            }
        }
    }

    /* remember layer_specific because uart borrow
       one byte from layer_specific for packet type */
    lay_spec = p_msg->layer_specific;

    /* Put the HCI Transport packet type 1 byte before the message */
    p = ((uint8_t *)(p_msg + 1)) + p_msg->offset -1;


    //
    skb = skb_alloc_and_init(type, p+1, p_msg->len);
    if (event == MSG_STACK_TO_HC_HCI_ACL)
    {
            //check whether this ACL conn has ability to send
            LogMsg("hc_cur_acl_total_num(%d), hc_acl_total_num(%d)", rtk_h5.hc_cur_acl_total_num, rtk_h5.hc_acl_total_num);

            if(rtk_h5.hc_cur_acl_total_num>0)
            {
                h5_enqueue(skb);
                rtk_h5.hc_cur_acl_total_num--;
                hci_conn->NumOfNotCmpAclPkts++;
            }
            else
            {
                //store skb in pending pkts queue
                skb_queue_tail(hci_conn->pending_pkts, skb);
            }
    }
    else
    {
        h5_enqueue(skb);
    }

    //

    p_msg->layer_specific = lay_spec;

    if (event == MSG_STACK_TO_HC_HCI_CMD)
    {
        num_hci_cmd_pkts--;

        /* If this is an internal Cmd packet, the layer_specific field would
         * have stored with the opcode of HCI command.
         * Retrieve the opcode from the Cmd packet.
         */
         p++;
        STREAM_TO_UINT16(lay_spec, p);
        LogMsg("HCI Command opcode(0x%04X)", lay_spec);
        if(lay_spec == 0x0c03)
        {
            LogMsg("RX HCI RESET Command, stop hw init timer");
            h5_stop_hw_init_ready_timer();
        }
    }

    /* generate snoop trace message */
    btsnoop_capture(p_msg, FALSE);

    if (bt_hc_cbacks)
    {
        if ((event == MSG_STACK_TO_HC_HCI_CMD) && \
            (rtk_h5.int_cmd_rsp_pending > 0) && \
            (p_msg->layer_specific == lay_spec))
        {
            /* dealloc buffer of internal command */
            bt_hc_cbacks->dealloc((TRANSAC) p_msg, (char *) (p_msg + 1));
        }
        else
        {
            bt_hc_cbacks->tx_result((TRANSAC) p_msg, (char *) (p_msg + 1), \
                                        BT_HC_TX_SUCCESS);
        }
    }

    h5_wake_up();
    return;
}


/*******************************************************************************
**
** Function        hci_h5_receive_msg
**
** Description     Construct HCI EVENT/ACL packets and send them to stack once
**                 complete packet has been received.
**
** Returns         Number of read bytes
**
*******************************************************************************/
uint16_t hci_h5_receive_msg(void)
{
    uint16_t    bytes_read = 0;
    uint8_t     byte;
    uint16_t    msg_len, len;
    uint8_t     msg_received;

    while (TRUE)
    {
        /* Read one byte to see if there is anything waiting to be read */
        if (userial_read(0 /*dummy*/, &byte, 1) == 0)
        {
            break;
        }

        bytes_read++;
        msg_received = FALSE;

        h5_recv(&rtk_h5, &byte, 1);
    }

    return (bytes_read);
}


/*******************************************************************************
**
** Function        hci_h5_send_int_cmd
**
** Description     Place the internal commands (issued internally by vendor lib)
**                 in the tx_q.
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t hci_h5_send_int_cmd(uint16_t opcode, HC_BT_HDR *p_buf, \
                                  tINT_CMD_CBACK p_cback)
{
    uint8_t * p =  (uint8_t *) (p_buf + 1);

    if(rtk_h5.link_estab_state == H5_UNINITIALIZED)
    {
        if(opcode == HCI_VSC_H5_INIT)
        {
            rtk_h5.cback_h5sync = p_cback;
            hci_h5_send_sync_req();
            h5_start_sync_retrans_timer();
        }
    }
    else if(rtk_h5.link_estab_state == H5_ACTIVE)
    {
        if(opcode == 0xFC17)
            rtk_h5.cback_h5sync = p_cback;

        LogMsg("hci_h5_send_int_cmd(0x%x)", opcode);
        if (rtk_h5.int_cmd_rsp_pending > INT_CMD_PKT_MAX_COUNT)
        {
            ALOGE( \
            "Allow only %d outstanding internal commands at a time [Reject 0x%04X]"\
            , INT_CMD_PKT_MAX_COUNT, opcode);
            return FALSE;
        }

        rtk_h5.int_cmd_rsp_pending++;
        rtk_h5.int_cmd[rtk_h5.int_cmd_wrt_idx].opcode = opcode;
        rtk_h5.int_cmd[rtk_h5.int_cmd_wrt_idx].cback = p_cback;
        rtk_h5.int_cmd_wrt_idx = ((rtk_h5.int_cmd_wrt_idx+1) & INT_CMD_PKT_IDX_MASK);

        p_buf->layer_specific = opcode;

        if(opcode == 0xFC17
            ||opcode == 0xFC20
            ||opcode == 0xFC6D
            ||opcode == 0x1001
            ||opcode == HCI_READ_BUFFER_SIZE
            ||opcode ==HCI_LE_READ_BUFFER_SIZE )
        {
            /* stamp signature to indicate an internal command */
            hci_h5_send_msg(p_buf);
        }
    }


    return TRUE;
}


/*******************************************************************************
**
** Function        hci_h5_get_acl_data_length
**
** Description     Issue HCI_READ_BUFFER_SIZE command to retrieve Controller's
**                 ACL data length setting
**
** Returns         None
**
*******************************************************************************/
void hci_h5_get_acl_data_length(void)
{
#if 0
    HC_BT_HDR  *p_buf = NULL;
    uint8_t     *p, ret;

    if (bt_hc_cbacks)
    {
        p_buf = (HC_BT_HDR *) bt_hc_cbacks->alloc(BT_HC_HDR_SIZE + \
                                                       HCI_CMD_PREAMBLE_SIZE);


    if (p_buf)
    {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->layer_specific = 0;
        p_buf->len = HCI_CMD_PREAMBLE_SIZE;

        p = (uint8_t *) (p_buf + 1);
        UINT16_TO_STREAM(p, HCI_READ_BUFFER_SIZE);
        *p = 0;

        if ((ret = hci_h5_send_int_cmd(HCI_READ_BUFFER_SIZE, p_buf, \
                                       get_acl_data_length_cback_h5)) == FALSE)
        {
            bt_hc_cbacks->dealloc((TRANSAC) p_buf, (char *) (p_buf + 1));
        }
        else
            return;
    }

    if (bt_hc_cbacks)
    {
        ALOGE("vendor lib postload aborted");
        bt_hc_cbacks->postload_cb(NULL, BT_HC_POSTLOAD_FAIL);
    }
#endif
}


/***
    Timer related functions
*/
static timer_t OsAllocateTimer(int signo)
{
    struct sigevent sigev;
    timer_t timerid = -1;

    // Create the POSIX timer to generate signo
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = signo;
    sigev.sigev_value.sival_ptr = &timerid;

    //Create the Timer using timer_create signal

    if (timer_create(CLOCK_REALTIME, &sigev, &timerid) == 0)
    {
            return timerid;
    }
    else
    {
        ALOGE("timer_create error!");
        return -1;
    }
}

 int OsFreeTimer(timer_t timerid)
{
    int ret = 0;
    ret = timer_delete(timerid);
    if(ret != 0)
        ALOGE("timer_delete fail with errno(%d)", errno);

    return ret;
}


 static int OsStartTimer(timer_t timerid, int msec, int mode)
 {
    struct itimerspec itval;

    itval.it_value.tv_sec = msec / 1000;
    itval.it_value.tv_nsec = (long)(msec % 1000) * (1000000L);

    if (mode == 1)

    {
        itval.it_interval.tv_sec    = itval.it_value.tv_sec;
        itval.it_interval.tv_nsec = itval.it_value.tv_nsec;
    }
    else
    {
        itval.it_interval.tv_sec = 0;
        itval.it_interval.tv_nsec = 0;
    }

    //Set the Timer when to expire through timer_settime

    if (timer_settime(timerid, 0, &itval, NULL) != 0)
    {
        ALOGE("time_settime error!");
        return -1;
    }

    return 0;

}

 static int OsStopTimer(timer_t timerid)
 {
    return OsStartTimer(timerid, 0, 0);
 }

static void h5_timeout_handler(int signo, siginfo_t * info, void *context)
 {

    ALOGE("h5_timeout_handler");
    if(rtk_h5.cleanuping)
    {
        ALOGE("H5 is cleanuping, EXIT here!");
        return;
    }
    if (signo == TIMER_H5_DATA_RETRANS)
    {
        h5_retransfer_signal_event(H5_EVENT_RX);
    }
    else
    if (signo == TIMER_H5_SYNC_RETRANS)
    {
        ALOGE("Wait H5 Sync Resp timeout, %d times", rtk_h5.sync_retrans_count);
        if(rtk_h5.sync_retrans_count < SYNC_RETRANS_COUNT)
        {
            hci_h5_send_sync_req();
            rtk_h5.sync_retrans_count ++;
        }
        else
        {
            h5_stop_sync_retrans_timer();
        }
    }
    else
    if (signo == TIMER_H5_CONF_RETRANS)
    {
        ALOGE("Wait H5 Conf Resp timeout, %d times", rtk_h5.conf_retrans_count);
        if(rtk_h5.conf_retrans_count < CONF_RETRANS_COUNT)
        {
            hci_h5_send_conf_req();
            rtk_h5.conf_retrans_count++;
        }
        else
        {
            h5_stop_conf_retrans_timer();
        }
    }
    else
    if (signo == TIMER_H5_WAIT_CT_BAUDRATE_READY)
    {
        LogMsg("No Controller retransfer, baudrate of controller ready");
        if (rtk_h5.cback_h5sync!= NULL)
        {
            rtk_h5.cback_h5sync(rtk_h5.p_rcv_msg);
        }
        else
        {
            // Missing cback function!
            // Release the p_rcv_msg buffer.
            if (bt_hc_cbacks)
            {
                bt_hc_cbacks->dealloc((TRANSAC) rtk_h5.p_rcv_msg, \
                                      (char *) (rtk_h5.p_rcv_msg + 1));
            }
        }
    }
    else
    if (signo == TIMER_H5_HW_INIT_READY)
    {
        LogMsg("TIMER_H5_HW_INIT_READY timeout, kill restart BT");
        kill(getpid(), SIGKILL);

    }
    else
    {
        ALOGE("H5 timer rx unspported signo(%d)", signo);
    }
}

int h5_alloc_data_retrans_timer()
 {
    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO;

    //register the Signal Handler
    sigact.sa_sigaction = h5_timeout_handler;

    // Set up sigaction to catch signal first timer
    if (sigaction(TIMER_H5_DATA_RETRANS, &sigact, NULL) == -1)
    {
        LogMsg("sigaction failed");
        return -1;
    }

    // Create and set the timer when to expire
    rtk_h5.timer_data_retrans = OsAllocateTimer(TIMER_H5_DATA_RETRANS);

    return 0;

 }


int h5_free_data_retrans_timer()
{
    return OsFreeTimer(rtk_h5.timer_data_retrans);
}


int h5_start_data_retrans_timer()
{
    return OsStartTimer(rtk_h5.timer_data_retrans, DATA_RETRANS_TIMEOUT_VALUE, 0);
}

int h5_stop_data_retrans_timer()
{
    return OsStopTimer(rtk_h5.timer_data_retrans);
}

int h5_alloc_sync_retrans_timer()
 {
    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO;

    //register the Signal Handler
    sigact.sa_sigaction = h5_timeout_handler;

    // Set up sigaction to catch signal first timer
    if (sigaction(TIMER_H5_SYNC_RETRANS, &sigact, NULL) == -1)
    {
        LogMsg("sigaction failed");
        return -1;
    }

    // Create and set the timer when to expire
    rtk_h5.timer_sync_retrans = OsAllocateTimer(TIMER_H5_SYNC_RETRANS);

    return 0;

 }

int h5_free_sync_retrans_timer()
{
    return OsFreeTimer(rtk_h5.timer_sync_retrans);
}


int h5_start_sync_retrans_timer()
{
    return OsStartTimer(rtk_h5.timer_sync_retrans, SYNC_RETRANS_TIMEOUT_VALUE, 1);
}

int h5_stop_sync_retrans_timer()
{
    return OsStopTimer(rtk_h5.timer_sync_retrans);
}


int h5_alloc_conf_retrans_timer()
 {
    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO;

    //register the Signal Handler
    sigact.sa_sigaction = h5_timeout_handler;

    // Set up sigaction to catch signal first timer
    if (sigaction(TIMER_H5_CONF_RETRANS, &sigact, NULL) == -1)
    {
        LogMsg("sigaction failed");
        return -1;
    }

    // Create and set the timer when to expire
    rtk_h5.timer_conf_retrans = OsAllocateTimer(TIMER_H5_CONF_RETRANS);

    return 0;

 }

int h5_free_conf_retrans_timer()
{
    return OsFreeTimer(rtk_h5.timer_conf_retrans);
}


int h5_start_conf_retrans_timer()
{
    return OsStartTimer(rtk_h5.timer_conf_retrans, CONF_RETRANS_TIMEOUT_VALUE, 1);
}

int h5_stop_conf_retrans_timer()
{
    return OsStopTimer(rtk_h5.timer_conf_retrans);
}



int h5_alloc_wait_controller_baudrate_ready_timer()
 {
    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO;

    //register the Signal Handler
    sigact.sa_sigaction = h5_timeout_handler;

    // Set up sigaction to catch signal first timer
    if (sigaction(TIMER_H5_WAIT_CT_BAUDRATE_READY, &sigact, NULL) == -1)
    {
        LogMsg("sigaction failed");
        return -1;
    }

    // Create and set the timer when to expire
    rtk_h5.timer_wait_ct_baudrate_ready = OsAllocateTimer(TIMER_H5_WAIT_CT_BAUDRATE_READY);

    return 0;

 }

int h5_free_wait_controller_baudrate_ready_timer()
{
    return OsFreeTimer(rtk_h5.timer_wait_ct_baudrate_ready);
}


int h5_start_wait_controller_baudrate_ready_timer()
{
    return OsStartTimer(rtk_h5.timer_wait_ct_baudrate_ready, WAIT_CT_BAUDRATE_READY_TIMEOUT_VALUE, 0);
}

int h5_stop_wait_controller_baudrate_ready_timer()
{
    return OsStopTimer(rtk_h5.timer_wait_ct_baudrate_ready);
}


int h5_alloc_hw_init_ready_timer()
 {
    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO;

    //register the Signal Handler
    sigact.sa_sigaction = h5_timeout_handler;

    // Set up sigaction to catch signal first timer
    if (sigaction(TIMER_H5_HW_INIT_READY, &sigact, NULL) == -1)
    {
        LogMsg("sigaction failed");
        return -1;
    }

    // Create and set the timer when to expire
    rtk_h5.timer_h5_hw_init_ready = OsAllocateTimer(TIMER_H5_HW_INIT_READY);

    return 0;

 }

int h5_free_hw_init_ready_timer()
{
    return OsFreeTimer(rtk_h5.timer_h5_hw_init_ready);
}


int h5_start_hw_init_ready_timer()
{
    return OsStartTimer(rtk_h5.timer_h5_hw_init_ready, H5_HW_INIT_READY_TIMEOUT_VALUE, 0);
}

int h5_stop_hw_init_ready_timer()
{
    return OsStopTimer(rtk_h5.timer_h5_hw_init_ready);
}


/******************************************************************************
**  HCI H5 Services interface table
******************************************************************************/

const tHCI_IF hci_h5_func_table =
{
    hci_h5_init,
    hci_h5_cleanup,
    hci_h5_send_msg,
    hci_h5_send_int_cmd,
    hci_h5_get_acl_data_length,
    hci_h5_receive_msg
};

#ifdef BT_FW_CAL_ENABLE
uint32_t rtk_set_bt_cal_inqury_result(uint8_t result)
{
    char bt_cal_file_name[PATH_MAX] = {0};
    int fd = -1;
    off_t offset = 0;
ssize_t ret_len = 0;
uint16_t host_info = 0;

    sprintf(bt_cal_file_name, BT_CAL_DIRECTORY"rtlbt_cal");
    if ((fd = open(bt_cal_file_name, O_RDWR)) < 0)
    {
        ALOGE("Can't open bt cal file, errno = %d", errno);
        return -1;
    }

    offset = lseek(fd, 3, SEEK_SET);
    if(offset != 3)
    {
        ALOGE("lseek for read fail return, errno = %d", errno);
        close(fd);
        return -1;
    }

    ret_len = read(fd, &host_info, sizeof(host_info));
    if ( ret_len != sizeof(host_info)) {
        ALOGE("read host_info fail, ret_len(%d), errno = %d", ret_len, errno);
        close(fd);
        return -1;
    }
    LogMsg("Get CAL Host info success, host_info = 0x%04x", host_info);
    if(result == CAL_INQUIRY_SUCCESS)
    {
        host_info |= IS_LAST_INQUIRY_SUCCESS;
    }
    else
    {
        host_info &= (~IS_LAST_INQUIRY_SUCCESS);
    }

    LogMsg("Update CAL Host info to file, host_info = 0x%04x", host_info);

    offset = lseek(fd, 3, SEEK_SET);
    if(offset != 3)
    {
        ALOGE("lseek for write fail return, errno = %d", errno);
        close(fd);
        return -1;
    }

    ret_len = write(fd, &host_info, sizeof(host_info));
    if ( ret_len != sizeof(host_info)) {
        ALOGE("write host_info fail, ret_len = %d, errno = %d", ret_len, errno);
        close(fd);
        return -1;
    }

    close(fd);
    return ret_len;
}
#endif
