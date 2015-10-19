/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tv.settings.util.bluetooth;

import java.nio.ByteBuffer;
import java.util.UUID;

/**
 * Class that contains helper functions and constants related to the communication protocol
 * used between the controller and hub over bluetooth.
 */
public class PairingUtils {

    public static final UUID[] UUIDs = new UUID[] {
      UUID.fromString("578f077f-2bb0-474b-8473-c257d910a09b"),
      UUID.fromString("c9e53035-20e2-4b09-822f-8fe4055063a6"),
      UUID.fromString("ef845772-2a89-420e-9042-181f6134d6ef"),
      UUID.fromString("67debdcf-ffd2-4f1e-a851-424b2a7f9178"),
      UUID.fromString("a3330af0-4b14-46f1-8398-d074e7863e29")
    };

    // Messages sent from the controller to the tungsten.
    // ================================================================================

    // -->> (none)
    // <<-- version
    public static final int MSG_GET_PROTOCOL_VERSION = 110;

    // -->> (n/a)
    // <<-- Error message (length prefix string)
    public static final int MSG_ERROR = 120;

    // -->> Controller protocol version
    // -->> Timezone (length prefix string)
    // -->> Hub name
    // <<-- Status bits
    // <<-- Build fingerprint of the device (added in v7)
    public static final int MSG_START_SETUP = 310;

    // -->> (none)
    // <<-- Error code
    // <<-- Ethernet status (NETWORK_ constants)
    // <<-- Wifi status (NETWORK_ constants)
    // <<-- Wifi registration attempts (int)
    // <<-- Wifi SSID (length prefixed string)
    // <<-- IP address (length prefixed string)
    // <<-- Wifi MAC address (length prefixed string) (add in v4)
    public static final int MSG_GET_NETWORK_STATE = 320;

    // -->> SSID (length prefix string)
    // -->> Password (length prefix string)
    // -->> Auth algorithm (encoded WifiConfiguration.AuthAlgorithm)
    // -->> Key management policy (encoded WifiConfiguration.KeyMgmt)
    // -->> The number of enterprise fields (int) (added in v9)
    // -->> Series of enterprise field names and values (length prefixed strings) (added in v9)
    // <<-- (same data as MSG_GET_NETWORK_STATE what=MSG_SET_WIFI_INFO)
    public static final int MSG_SET_WIFI_INFO = 330;

    // -->> Last ethernet status (NETWORK_ constants)
    // -->> Last wifi status (NETWORK_ constants)
    // -->> Last wifi attempt count
    // -->> Wifi MAC address, if using wifi (added in v4)
    // <<-- (same data as MSG_GET_NETWORK_STATE what=MSG_SET_WIFI_INFO)
    // Waits until the wifi and / or ethernet status is not the same the input
    // values, then returns.
    public static final int MSG_WAIT_FOR_NET_CHANGE = 340;

    // added in v7
    // -->> Test to run (TEST_ constants)
    // <<-- If the test is ready to run (STATE_TEST_ constants)
    public static final int MSG_RUN_NETWORK_TEST = 343;

    // added in v5
    // -->> (none)
    // <<-- placeId (length prefix string)
    // <<-- hub serial (length prefix string)
    // <<-- IP address of the hub
    public static final int MSG_GET_HUB_IDENTIFIERS = 345;

    // -->> Account ID (robot account, not user GAIA)
    // -->> OAuth token to use
    // -->> Service token
    // -->> HubName
    // <<-- 1 or 0
    public static final int MSG_JOIN_CLOUDBRIDGE = 350;

    // -->> Place ID
    // -->> Name (length prefix string)
    // -->> Owner cert (length prefix string, bas64 encoded ???)
    // <<-- EndpointInfo with master address and port
    public static final int MSG_CREATE_PLACE = 360;

    // -->> (none)
    // <<-- Tungsten's certificate (length prefixed)
    //      - Will wait for broker to finish
    public static final int MSG_GET_CERTIFICATE = 370;

    // -->> (none)
    // <<-- OTA required (1 or 0)
    //      - Will wait for first checkin to happen
    public static final int MSG_CHECK_FOR_OTA = 390;

    // -->> (none)
    // <<-- State (See OtaObserver for valid values)
    // <<-- Progress (-1 - 100, or OtaObserver#NOT_SET)
    //     - Sent when waiting for OTA progress
    public static final int MSG_OTA_PROGRESS = 392;

    // -->> (none)
    // <<-- 1 or 0 if the controller should wait for a reboot
    // <<-- Serial number of the device
    // <<-- the IP address of the device
    public static final int MSG_WAIT_FOR_FIRST_RUN = 400;

    // added in v10
    // -->> One of TEST_MODE_* constants indicating the role of the hub (int)
    // -->> IP address of controller, if the hub is running in client mode, otherwise omitted
    //      (length prefixed string)
    // <<-- (none)
    public static final int MSG_RUN_UNICAST_CONNECTION_TEST = 800;

    // added in v10
    // -->> One of TEST_MODE_* constants indicating the role of the hub (int)
    // -->> IP address of controller, if the hub is running in client mode, otherwise omitted
    //      (length prefixed string)
    // <<-- (none)
    public static final int MSG_RUN_BANDWIDTH_TEST = 801;

    // added in v10
    // -->> One of TEST_MODE_* constants indicating the role of the hub (int)
    // -->> IP address of controller, if the hub is running in client mode, otherwise omitted
    //      (length prefixed string)
    // <<-- (none)
    public static final int MSG_RUN_PACKET_LOSS_TEST = 802;

    // The most recent protocol version.
    // ================================================================================
    public static final int PROTOCOL_VERSION = 12;

    // Errors that the networking can return
    public static final int ERROR_NONE = 0;
    public static final int ERROR_UNKNOWN = 1;
    public static final int ERROR_ADDING_NETWORK = 2;
    public static final int ERROR_AUTH_FAILED = 3;
    public static final int ERROR_SETUP_RESET = 4;
    public static final int ERROR_JOIN_CLOUDBRIDGE = 5;
    public static final int ERROR_NETWORK_DISABLED = 6;

    // Wifi status codes (from android.net.NetworkInfo.DetailedState)
    // ================================================================================
    public static final int NETWORK_AUTHENTICATING = 1;
    public static final int NETWORK_BLOCKED = 2;
    public static final int NETWORK_CONNECTED = 3;
    public static final int NETWORK_CONNECTING = 4;
    public static final int NETWORK_DISCONNECTED = 5;
    public static final int NETWORK_DISCONNECTING = 6;
    public static final int NETWORK_FAILED = 7;
    public static final int NETWORK_IDLE = 8;
    public static final int NETWORK_OBTAINING_IPADDR = 9;
    public static final int NETWORK_SCANNING = 10;
    public static final int NETWORK_SUSPENDED = 11;
    // the network is connected, but we have no ip address
    public static final int NETWORK_NO_IP = 12;

    // Test codes
    // ================================================================================
    public static final int TEST_UNICAST = 1;
    public static final int TEST_LAN_BANDWIDTH = 2;
    public static final int TEST_PACKET_LOSS = 3;

    public static final int STATE_TEST_STARTUP_FAILED = 0;
    public static final int STATE_TEST_READY = 1;

    // send a heartbeat every 10 seconds
    public static final int TIMEOUT_IP_WRITE = 10 * 1000;
    // set the read timeout with plenty of buffer in case the write is delayed for some reason
    public static final int TIMEOUT_IP_READ = TIMEOUT_IP_WRITE * 25 / 10;

    public static String getString(ByteBuffer msg) {
        final int len = msg.getInt();
        final byte[] buf = msg.array();
        final int pos = msg.position();
        msg.position(pos + len);
        return new String(buf, pos, len);
    }

    /**
     * Append the length of the given string as an integer into the ByteBuffer followed by the
     * string itself.
     * @param buffer ByteBuffer to add this string to
     * @param toAdd The string to add
     * @return The same buffer that was passed in, just returned for convenience.
     */
    public static ByteBuffer putString(ByteBuffer buffer, String toAdd) {
        buffer.putInt(toAdd.length());
        buffer.put(toAdd.getBytes());
        return buffer;
    }

    public static byte[] getByteArray(ByteBuffer msg) {
        final int len = msg.getInt();
        final byte[] result = new byte[len];
        System.arraycopy(msg.array(), msg.position(), result, 0, len);
        final int pos = msg.position();
        msg.position(pos + len);
        return result;
    }

    // Constants encoding the setup needs of the device
    // bit mask to do place setup
    public static final int TASK_SETUP_PLACE = 1;

    // bit mask to do network setup
    public static final int TASK_SETUP_NETWORK = 2;

    // bit mask to do network setup (if needed), run network tests, and terminate. Generally
    // speaking it should always be possible to run tests
    public static final int TASK_TEST_NETWORK = 4;

    // The hub should run as the server for this test, the meaning of "server" is context sensitive
    public static final int TEST_MODE_SERVER = 0;

    // The hub should run as the client for this test, the meaning of "client" is context sensitive
    public static final int TEST_MODE_CLIENT = 1;

}
