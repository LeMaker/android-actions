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

package com.android.omadm.service;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.NetworkRequest;
import android.os.IBinder;
import android.util.Log;

/**
 * This service monitors the data connection and call state and brings up the FOTA APN when
 * started by an {@link DMIntent#ACTION_START_DATA_CONNECTION_SERVICE} intent.
 *
 * TODO: handle operators which disallow OMA DM sessions over Wi-Fi.
 * TODO: handle mobile data disabled and roaming disabled cases.
 */
public class DMDataConnectionService extends Service {
    private static final String TAG = "DMDataConnectionService";
    private static final boolean DBG = true;

    private ConnectivityManager mConnectivityManager;

    /** Start ID of current network request. */
    private int mCurrentStartId;

    /** The active NetworkCallback for the FOTA APN, or null. */
    private ConnectivityManager.NetworkCallback mCellNetworkCallback;

    /** The active NetworkCallback for WiFi or Ethernet, or null. */
    private ConnectivityManager.NetworkCallback mWiFiNetworkCallback;

    /** The active FOTA APN network, or null. */
    private Network mActiveCellNetwork;

    /** The active WiFi or Ethernet network, or null. */
    private Network mActiveWiFiNetwork;

    /** Wait for 120 seconds after requesting the desired network types. */
    private static final int WAIT_FOR_NETWORK_TIMEOUT_MS = 120 * 1000;

    @Override
    public void onCreate() {
        if (DBG) logd("onCreate()");
        mConnectivityManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
    }

    @Override
    public void onDestroy() {
        if (DBG) logd("onDestroy()");

        mCurrentStartId = 0;

        if (mCellNetworkCallback != null) {
            mConnectivityManager.unregisterNetworkCallback(mCellNetworkCallback);
            if (DBG) logd("unregistered cell network callback");
            mCellNetworkCallback = null;
        }

        if (mWiFiNetworkCallback != null) {
            mConnectivityManager.unregisterNetworkCallback(mWiFiNetworkCallback);
            if (DBG) logd("unregistered WiFi network callback");
            mWiFiNetworkCallback = null;
        }
        // Unbind from any network.
        ConnectivityManager.setProcessDefaultNetwork(null);
    }

    private boolean isWifiConnected() {
        NetworkInfo ni = mConnectivityManager.getActiveNetworkInfo();
        return ni != null && ni.isConnected()
                && (ni.getType() == ConnectivityManager.TYPE_WIFI
                        || ni.getType() == ConnectivityManager.TYPE_ETHERNET);
    }

    /**
     * Request route using FOTA APN on the cell network and/or Wi-Fi or Ethernet transport.
     * @param wifiOrEthernet true if Wi-Fi transport type is allowed; false for mobile data only
     */
    private void requestNetworks(boolean wifiOrEthernet) {
        if (wifiOrEthernet && isWifiConnected()) {
            if (DBG) logd("requesting WiFi or Ethernet network transport");
            NetworkRequest request = new NetworkRequest.Builder()
                    .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
                    .addTransportType(NetworkCapabilities.TRANSPORT_ETHERNET).build();

            mWiFiNetworkCallback = new WiFiCallback();
            mConnectivityManager.requestNetwork(request, mWiFiNetworkCallback,
                    WAIT_FOR_NETWORK_TIMEOUT_MS);
        } else {
            if (DBG) logd("requesting cell network with FOTA capability");
            NetworkRequest request = new NetworkRequest.Builder()
                    .addCapability(NetworkCapabilities.NET_CAPABILITY_FOTA).build();

            mCellNetworkCallback = new CellCallback();
            mConnectivityManager.requestNetwork(request, mCellNetworkCallback,
                    WAIT_FOR_NETWORK_TIMEOUT_MS);
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (DBG) logd("onStartCommand: startID " + startId);

        if (intent != null) {
            String action = intent.getAction();
            if (DMIntent.ACTION_START_DATA_CONNECTION_SERVICE.equals(action)) {
                int lastStartId = mCurrentStartId;
                mCurrentStartId = startId;

                if (lastStartId == 0) {
                    requestNetworks(true);    // request FOTA APN or Wi-Fi/Ethernet networks
                }
            } else {
                if (DBG) loge("unexpected intent: " + action);
                stopSelf(startId);
            }
        } else {
            if (DBG) logd("unexpected null intent");
            stopSelf(startId);
        }
        return Service.START_REDELIVER_INTENT;
    }

    private void sendDataConnectionReady() {
        Intent intent = new Intent(this, DMIntentReceiver.class);
        intent.setAction(DMIntent.ACTION_DATA_CONNECTION_READY);
        sendBroadcast(intent);
    }

    // Callback for FOTA APN of cellular network.
    private class CellCallback extends ConnectivityManager.NetworkCallback {
        @Override
        public void onAvailable(Network network) {
            if (mCurrentStartId != 0) {
                if (DBG) logd("CellCallback.onAvailable() for network: " + network);
                mActiveCellNetwork = network;
                if (mActiveWiFiNetwork == null) {
                    if (DBG) logd("calling setProcessDefaultNetwork() for cell network");
                    ConnectivityManager.setProcessDefaultNetwork(network);
                }
                sendDataConnectionReady();
            } else {
                if (DBG) loge("CellCallback: ignoring onAvailable() after service quit");
            }
        }

        @Override
        public void onUnavailable() {
            if (mCurrentStartId != 0) {
                if (DBG) loge("CellCallback.onUnavailable() called (timeout)");
                mActiveCellNetwork = null;
                if (mActiveWiFiNetwork == null) {
                    if (DBG) logd("clearing setProcessDefaultNetwork() binding");
                    ConnectivityManager.setProcessDefaultNetwork(null);
                }
            } else {
                if (DBG) loge("CellCallback: ignoring onUnavailable() after service quit");
            }
        }
    }

    // Callback for WiFi connectivity.
    private class WiFiCallback extends ConnectivityManager.NetworkCallback {
        @Override
        public void onAvailable(Network network) {
            if (mCurrentStartId != 0) {
                if (DBG) logd("WiFiCallback.onAvailable() for network: " + network);
                mActiveWiFiNetwork = network;
                ConnectivityManager.setProcessDefaultNetwork(network);
                sendDataConnectionReady();
            } else {
                if (DBG) loge("WiFiCallback: ignoring onAvailable() after service quit");
            }
        }

        @Override
        public void onUnavailable() {
            if (mCurrentStartId != 0) {
                if (DBG) loge("WiFi.onUnavailable() called (timeout)");
                mActiveWiFiNetwork = null;
                if (mActiveCellNetwork == null) {
                    if (DBG) logd("clearing setProcessDefaultNetwork() binding");
                    ConnectivityManager.setProcessDefaultNetwork(null);
                    requestNetworks(false);   // request FOTA APN only
                }
            } else {
                if (DBG) loge("WiFiCallback: ignoring onUnavailable() after service quit");
            }
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private static void logd(String msg) {
        Log.d(TAG, msg);
    }

    private static void loge(String msg) {
        Log.e(TAG, msg);
    }
}
