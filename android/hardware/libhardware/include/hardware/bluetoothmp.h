/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_INCLUDE_BLUETOOTH_H
#define ANDROID_INCLUDE_BLUETOOTH_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <hardware/hardware.h>


__BEGIN_DECLS
/**
 * The Bluetooth Hardware Module ID
 */

#define BT_HARDWARE_MODULE_ID "bluetoothmp"
#define BT_STACK_MODULE_ID "bluetoothmp"
#define BT_STACK_TEST_MODULE_ID "bluetoothmp_test"

/** Bluetooth Address */
typedef struct {
    uint8_t address[6];
} __attribute__((packed))bt_bdaddr_t;

/** Bluetooth Device Name */
typedef struct {
    uint8_t name[248];
} __attribute__((packed))bt_bdname_t;

/** Bluetooth Adapter Visibility Modes*/
typedef enum {
    BT_SCAN_MODE_NONE,
    BT_SCAN_MODE_CONNECTABLE,
    BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE
} bt_scan_mode_t;

/** Bluetooth Adapter State */
typedef enum {
    BT_STATE_OFF,
    BT_STATE_ON
}   bt_state_t;

/** Bluetooth Error Status */
/** We need to build on this */

typedef enum {
    BT_STATUS_SUCCESS,
    BT_STATUS_FAIL,
    BT_STATUS_NOT_READY,
    BT_STATUS_NOMEM,
    BT_STATUS_BUSY,
    BT_STATUS_DONE,        /* request already completed */
    BT_STATUS_UNSUPPORTED,
    BT_STATUS_PARM_INVALID,
    BT_STATUS_UNHANDLED,
    BT_STATUS_AUTH_FAILURE,
    BT_STATUS_RMT_DEV_DOWN

} bt_status_t;

/** Bluetooth PinKey Code */
typedef struct {
    uint8_t pin[16];
} __attribute__((packed))bt_pin_code_t;

/** Bluetooth Adapter Discovery state */
typedef enum {
    BT_DISCOVERY_STOPPED,
    BT_DISCOVERY_STARTED
} bt_discovery_state_t;

/** Bluetooth ACL connection state */
typedef enum {
    BT_ACL_STATE_CONNECTED,
    BT_ACL_STATE_DISCONNECTED
} bt_acl_state_t;

/** Bluetooth 128-bit UUID */
typedef struct {
   uint8_t uu[16];
} bt_uuid_t;




/** Bluetooth Interface callbacks */

/** Bluetooth Enable/Disable Callback. */
typedef void (*adapter_state_changed_callback)(bt_state_t state);



typedef enum {
    ASSOCIATE_JVM,
    DISASSOCIATE_JVM
} bt_cb_thread_evt;

/** Thread Associate/Disassociate JVM Callback */
/* Callback that is invoked by the callback thread to allow upper layer to attach/detach to/from
 * the JVM */
typedef void (*callback_thread_event)(bt_cb_thread_evt evt);

/** Bluetooth Test Mode Callback */
/* Receive any HCI event from controller. Must be in DUT Mode for this callback to be received */
typedef void (*dut_mode_recv_callback)(uint8_t opcode, char *buf);

/** TODO: Add callbacks for Link Up/Down and other generic
  *  notifications/callbacks */

/** Bluetooth DM callback structure. */
typedef struct {
    /** set to sizeof(bt_callbacks_t) */
    size_t size;
    adapter_state_changed_callback adapter_state_changed_cb;
    callback_thread_event thread_evt_cb;
    dut_mode_recv_callback dut_mode_recv_cb;
} bt_callbacks_t;


/** Represents the standard Bluetooth DM interface. */
typedef struct {
    /** set to sizeof(bt_interface_t) */
    size_t size;
    /**
     * Opens the interface and provides the callback routines
     * to the implemenation of this interface.
     */
    int (*init)(bt_callbacks_t* callbacks );

    /** Enable Bluetooth. */
    int (*enable)(void);

    /** Disable Bluetooth. */
    int (*disable)(void);

    /** Closes the interface. */
    void (*cleanup)(void);

    /** Bluetooth Test Mode APIs - Bluetooth must be enabled for these APIs */
    /* Configure DUT Mode - Use this mode to enter/exit DUT mode */
    int (*dut_mode_configure)(uint8_t enable);

    /* Send any test HCI (vendor-specific) command to the controller. Must be
    in DUT Mode */
    int (*hal_mp_op_send)(uint16_t opcode, char *buf, uint8_t len);
} bt_interface_t;


typedef struct {
    struct hw_device_t common;
    const bt_interface_t* (*get_bluetooth_interface)();
} bluetooth_device_t;

typedef bluetooth_device_t bluetooth_module_t;
__END_DECLS

#endif /* ANDROID_INCLUDE_BLUETOOTH_H */
