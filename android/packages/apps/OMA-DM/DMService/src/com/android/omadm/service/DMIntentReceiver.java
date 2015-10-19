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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.SystemProperties;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.TelephonyProperties;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

public class DMIntentReceiver extends BroadcastReceiver {
    private static final String TAG = "DMIntentReceiver";
    private static final boolean DBG = DMClientService.DBG;

    private int mUIMode = -1;

    private byte[] mData;

    private static final String ALERT_TYPE_DOWNLOADANDUPDATE
            = "org.openmobilealliance.dm.firmwareupdate.downloadandupdate";

    private static final String RP_OPERATIONS_FACTORYRESET
            = "./ManagedObjects/LAWMO/Operations/FactoryReset";

    private static final String RP_EXT_OPERATIONS_RESET
            = "./ManagedObjects/LAWMO/Ext/Operations/Reset";

    private static final String ACTION_NOTIFY_RESULT_TO_SERVER
            = "com.android.omadm.service.notify_result_to_server";

    private static final String ACTION_NOTIFY_START_UP_DMSERVICE
            = "com.android.omadm.service.start_up";

    private static final String DEV_DETAIL = "devdetail";

    private static final String WIFI_MAC_ADDR = "wifimacaddr";

    private static final String PRE_FW_VER = "prefwversion";

    private static final String CURR_FW_VER = "currfwversion";

    private static final String LAST_UPD_TIME = "lastupdatetime";

    private static boolean initialWapPending;

    @Override
    public void onReceive(Context context, Intent intent) {
        if (DMHelper.disableIfSecondaryUser(context)) {
            return;
        }

        String action = intent.getAction();

        logd("Received new intent: " + action);

        if (action.equals(DMIntent.ACTION_WAP_PUSH_RECEIVED_INTERNAL)) {
            handleWapPushIntent(context, intent);
        } else if (action.equals(DMIntent.ACTION_CLOSE_NOTIFICATION_INFO)) {
            DMHelper.cancelNotification(context, DMHelper.NOTIFICATION_INFORMATIVE_ID);
        } else if (action.equals(DMIntent.ACTION_USER_CONFIRMED_DM_SESSION)) {
            handleUserConfirmedSession(context);
        } else if (action.equals(DMIntent.ACTION_CLIENT_INITIATED_FOTA_SESSION)) {
            handleClientInitiatedFotaIntent(context, intent);
        } else if (action.equals(DMIntent.ACTION_TIMER_ALERT)) {
            handleTimeAlertIntent(context);
        } else if (action.equals(DMIntent.DM_SERVICE_RESULT_INTENT)) {
            handleDmServiceResult(context, intent);
        } else if (action.equals(ACTION_NOTIFY_RESULT_TO_SERVER)) {
            // FIXME old comment: change this to the DMIntent name
            handleNotifyResultToServer(context, intent);
        } else if (action.equals(DMIntent.ACTION_DATA_CONNECTION_READY)) {
            handleDataConnectionReady(context);
        } else if (action.equals(Intent.ACTION_BOOT_COMPLETED)) {
            logd("Ignoring Intent.ACTION_BOOT_COMPLETED");
            //if (!(isPhoneTypeLTE() || isPhoneTypeCDMA3G(context))) {
            //    saveDevDetail(context);
            //    handleBootCompletedIntent(context);
            //}
        } else if (action.equals(ACTION_NOTIFY_START_UP_DMSERVICE)) {
            if (isPhoneTypeLTE()) {
                saveDevDetail(context);
                SharedPreferences p = context.getSharedPreferences(DMHelper.IMEI_PREFERENCE_KEY, 0);
                String currGsmImei = p.getString(DMHelper.IMEI_VALUE_KEY, "");
                if (currGsmImei != null && currGsmImei.equals(intent.getStringExtra("gsmimei"))) {
                    logd("IMEI already stored, continuing");
                } else {
                    SharedPreferences.Editor ed = p.edit();
                    ed.putString(DMHelper.IMEI_VALUE_KEY, intent.getStringExtra("gsmimei"));
                    ed.apply();
                }
            } else if (isPhoneTypeCDMA3G(context)) {
                SharedPreferences p = context.getSharedPreferences(DMHelper.AKEY_PREFERENCE_KEY, 0);
                SharedPreferences.Editor ed = p.edit();
                ed.putString(DMHelper.AKEY_VALUE_KEY, intent.getStringExtra("akey"));
                ed.apply();
            }
            handleBootCompletedIntent(context);
        } else if (action.equals(DMIntent.ACTION_INJECT_PACKAGE_0_INTERNAL)) {
            String strServerID = intent.getStringExtra(DMIntent.FIELD_SERVERID);
            if (strServerID == null || strServerID.trim().isEmpty()) {
                logd("Error! Can't inject package0. The required extras parameter '" +
                        DMIntent.FIELD_SERVERID + "' is null or an empty string.");
                return;
            }

            Intent newIntent = new Intent(DMIntent.LAUNCH_INTENT);
            newIntent.putExtra(DMIntent.FIELD_REQUEST_ID, System.currentTimeMillis());
            newIntent.putExtra(DMIntent.FIELD_TYPE, DMIntent.TYPE_CLIENT_SESSION_REQUEST);
            newIntent.putExtra(DMIntent.FIELD_SERVERID, strServerID);
            logd("XXX received ACTION_INJECT_PACKAGE_0_INTERNAL, starting"
                    + " TYPE_CLIENT_SESSION_REQUEST with ID "
                    + newIntent.getLongExtra(DMIntent.FIELD_REQUEST_ID, 1234));
            newIntent.setClass(context, DMClientService.class);
            context.startService(newIntent);
        } else if (action.equals(DMIntent.ACTION_SET_SERVER_CONFIG)) {
            logd("ACTION_SET_SERVER_CONFIG received");
            String hostUrl = intent.getStringExtra(DMIntent.FIELD_SERVER_URL);
            String proxyAddress = intent.getStringExtra(DMIntent.FIELD_PROXY_ADDRESS);
            logd("server URL: " + hostUrl + " proxy address: " + proxyAddress);
            DMHelper.setServerUrl(context, hostUrl);
            DMHelper.setProxyHostname(context, proxyAddress);
        } else if (action.equals(DMIntent.ACTION_CANCEL_SESSION)) {
            // create intent and start DM service
            Intent newIntent = new Intent(DMIntent.LAUNCH_INTENT);
            newIntent.putExtra(DMIntent.FIELD_TYPE, DMIntent.TYPE_CANCEL_DM_SESSION);
            logd("cancelling DM Session");
            newIntent.setClass(context, DMClientService.class);
            context.startService(newIntent);
        }
    }

    // handle client-initiated FOTA intents
    private void handleClientInitiatedFotaIntent(Context context, Intent intent) {
        String strServerID = intent.getStringExtra(DMIntent.FIELD_SERVERID);
        if (TextUtils.isEmpty(strServerID)) {
            logd("Error! Can't start FOTA session: " +
                    DMIntent.FIELD_SERVERID + " is null or an empty string.");
            return;
        }

        String alertString = intent.getStringExtra(DMIntent.FIELD_ALERT_STR);
        if (TextUtils.isEmpty(alertString)) {
            logd("Error! Can't start FOTA session: " +
                    DMIntent.FIELD_ALERT_STR + " is null or an empty string.");
            return;
        }

        setState(context, DMHelper.STATE_APPROVED_BY_USER);
        long requestID = System.nanoTime();

        // Save pending info here
        SharedPreferences p = context.getSharedPreferences(DMHelper.DM_PREFERENCES_KEY, 0);
        SharedPreferences.Editor ed = p.edit();
        ed.putInt(DMHelper.DM_SESSION_TYPE_KEY, DMIntent.TYPE_FOTA_CLIENT_SESSION_REQUEST);
        ed.putLong(DMHelper.MESSAGE_TIMESTAMP_ID_KEY, requestID);
        ed.putString(DMHelper.FOTA_SERVER_ID_KEY, strServerID);
        ed.putString(DMHelper.FOTA_ALERT_STRING_KEY, alertString);
        ed.apply();

        if (isWifiConnected(context) || isDataNetworkAcceptable(context)) {
            if (!isWifiConnected(context) && isDataNetworkAcceptable(context) && isPhoneTypeLTE()) {
                logd("handleClientInitiatedFotaIntent, start apn monitoring service"
                        + " for requestID " + requestID);
                setFotaApnState(context, DMHelper.FOTA_APN_STATE_START_DM_SESSION);
                startDataConnectionService(context);
            } else {
                logd("handleClientInitiatedFotaIntent starting DM session");
                startDMSession(context);
            }
        } else {
            logd("handleClientInitiatedFotaIntent: start data/call state monitoring");
            startDataConnectionService(context);
        }
    }

    // handle SMS and WAP Push intents;
    private void handleWapPushIntent(Context context, Intent intent) {

        int currentState = getState(context);

        logd("handleWapPushIntent() current state: " + currentState);

        // if current state is already "session in progress" - ignore new message;
        // otherwise remove old message and process a new one.
        if (currentState == DMHelper.STATE_SESSION_IN_PROGRESS) {
            loge("current state is 'Session-in-Progress', ignoring new message.");
            return;
        }

        DMHelper.cleanAllResources(context);

        //clean fota apn resources and stop using fota apn
        if (isPhoneTypeLTE()) {
            int mgetFotaApnState = getFotaApnState(context);
            logd("handleWapPushIntent, check if necessary to stop using fota apn "
                    + mgetFotaApnState);
            if (mgetFotaApnState != DMHelper.FOTA_APN_STATE_INIT) {
                // resetting FOTA APN STATE
                logd("XXX resetting FOTA APN state");
                setFotaApnState(context, DMHelper.FOTA_APN_STATE_INIT);
                stopUsingFotaApn(context);
                DMHelper.cleanFotaApnResources(context);
            }
        }

        //parse & save message; get UI mode
        boolean result = parseAndSaveWapPushMessage(context, intent);

        if (!result) {
            loge("handleWapPushIntent(): error in parseAndSaveWapPushMessage()");
            DMHelper.cleanAllResources(context);
            return;
        }

        if (treeExist(context) || !isPhoneTypeLTE()) {
            //check UI mode and prepare and start process
            preprocess(context, currentState);
        } else {
            logd("WapPush arrived before tree initialization");
            initialWapPending = true;
            Intent intentConnmoInit = new Intent("com.android.omadm.service.wait_timer_alert");
            context.sendBroadcast(intentConnmoInit);
        }
    }

    private void handleUserConfirmedSession(Context context) {
        // check if message is not expired
        if (DMHelper.isMessageExpired(context)) {
            DMHelper.cleanAllResources(context);
            logd("handleUserConfirmedSession(): message is expired.");
            return;
        }

        startProcess(context);
    }

    // handle boot completed intent
    private void handleBootCompletedIntent(Context context) {
        // Check if DM tree already has been generated. Start service to generate tree
        // in case if required. It may happened only once during first boot.
        if (!treeExist(context)) {
            logd("Boot completed: there is no DM Tree. Start service to generate tree.");
            Intent intent = new Intent(DMIntent.LAUNCH_INTENT);
            intent.putExtra("NodePath", ".");
            intent.putExtra(DMIntent.FIELD_REQUEST_ID, -2L);
            intent.setClass(context, DMClientService.class);
            context.startService(intent);
            if (!initialWapPending) {
                logd("handleBootCompletedIntent, no initial WapPush pending.");
                DMHelper.cleanAllResources(context);
                return;
            } else {
                logd("handleBootCompletedIntent, initial WapPush pending.");
                initialWapPending = false;
                setState(context, DMHelper.STATE_PENDING_MESSAGE);
            }
        }

//        if (isPhoneTypeLTE()) {
//            int fotaApnState = getFotaApnState(context);
//            logd("handleBootCompletedIntent, check if need to stop using fota apn "
//                    + fotaApnState);
//            if (fotaApnState != DMHelper.FOTA_APN_STATE_INIT) {
                // resetting FOTA APN STATE
//                setFotaApnState(context, DMHelper.FOTA_APN_STATE_INIT);
                //stopUsingFotaApn();
//                DMHelper.cleanFotaApnResources(context);
//            }
            // stopUsingFotaApn();
//        }

        int currentState = getState(context);

        if (currentState == DMHelper.STATE_IDLE) {
            logd("Boot completed: there is no message to proceed.");
            return;
        }

        // check if message is not expired
        if (DMHelper.isMessageExpired(context)) {
            DMHelper.cleanAllResources(context);
            logd("handleBootCompletedIntent(): the message is expired.");
            return;
        }

        // initiate mUIMode and mData from preferences
        if (!initFromSharedPreferences(context)) {
            DMHelper.cleanAllResources(context);
            logd("handleBootCompletedIntent(): cannot init from shared preferences");
            return;
        }

        preprocess(context, currentState);
    }

    // handle time alert intent (all instances)
    private void handleTimeAlertIntent(Context context) {
        int currentState = getState(context);
        switch (currentState) {
            case DMHelper.STATE_IDLE:
                // nothing there
                DMHelper.cleanAllResources(context);
                logd("Time alert: there is no message to proceed.");
                break;
        }

        if (currentState == DMHelper.STATE_IDLE) {
            // nothing there
            DMHelper.cleanAllResources(context);
            logd("Time alert: there is no message to proceed.");
        } else if (DMHelper.isMessageExpired(context)) {
            // check if message is not expired
            DMHelper.cleanAllResources(context);
            logd("Warning from handleTimeAlertIntent(): the message is expired.");
        } else if (currentState == DMHelper.STATE_SESSION_IN_PROGRESS) {
            // session in progress; doing nothing.
            logd("Time alert: session in progress; doing nothing.");
            DMHelper.subscribeForTimeAlert(context,
                    DMHelper.TIME_CHECK_STATUS_AFTER_STARTING_DM_SERVICE);
        } else if (currentState == DMHelper.STATE_APPROVED_BY_USER) {
            // approved by user: try to start session or data/call monitoring service
            logd("Time alert: state 'approved by user'; starting process");
            startProcess(context);
        } else if (currentState == DMHelper.STATE_PENDING_MESSAGE) {
            // approved by user: try to start session or data/call monitoring service pending
            logd("Time alert: state 'pending message'; read from preferences starting preprocess");

            // initiate mUIMode and mData from preferences
            if (!initFromSharedPreferences(context)) {
                DMHelper.cleanAllResources(context);
                logd("Warning from handleTimeAlertIntent(): cannot init from shared preferences");
                return;
            }
            preprocess(context, currentState);
        } else {
            loge("Error from handleTimeAlertIntent(): unknown state " + currentState);
        }
    }

    private static void handleNotifyResultToServer(Context context, Intent intent) {
        logd("Inside handleNotifyResultToServer");

        // Save message
        SharedPreferences p = context.getSharedPreferences(DMHelper.FOTA_APN_PREFERENCE_KEY, 0);
        SharedPreferences.Editor ed = p.edit();

        ed.putString(DMHelper.LAWMO_RESULT_KEY, intent.getStringExtra(DMIntent.FIELD_LAWMO_RESULT));
        ed.putString(DMHelper.FOTA_RESULT_KEY, intent.getStringExtra(DMIntent.FIELD_FOTA_RESULT));
        ed.putString(DMHelper.PKG_URI_KEY, intent.getStringExtra(DMIntent.FIELD_PKGURI));
        ed.putString(DMHelper.ALERT_TYPE_KEY, intent.getStringExtra(DMIntent.FIELD_ALERTTYPE));
        ed.putString(DMHelper.CORRELATOR_KEY, intent.getStringExtra(DMIntent.FIELD_CORR));
        ed.putString(DMHelper.SERVER_ID_KEY, intent.getStringExtra(DMIntent.FIELD_SERVERID));

        ed.apply();

        if (isDataNetworkAcceptable(context) && !isWifiConnected(context) && isPhoneTypeLTE()) {
            int mgetFotaApnState = getFotaApnState(context);
            if (mgetFotaApnState != DMHelper.FOTA_APN_STATE_INIT) {
                logd("there must be a pending session, return");
                return;
            }
            // for LTE and eHRPD coverage , switch the apn before FDM
            logd("handleNotifyResultToServer starting FOTA APN");
            setFotaApnState(context, DMHelper.FOTA_APN_STATE_REPORT_DM_SESSION);
            startDataConnectionService(context);
        } else {
            sendNotifyIntent(context);
        }
    }

    // start session if we have network connectivity
    private void handleDataConnectionReady(Context context) {
        logd("Inside handleDataConnectionReady");
        int fotaApnState = getFotaApnState(context);
        logd("FOTA APN state is " + fotaApnState);

        if (fotaApnState == DMHelper.FOTA_APN_STATE_REPORT_DM_SESSION) {
            setFotaApnState(context, DMHelper.FOTA_APN_STATE_REPORT_DM_SESSION_RPTD);
            sendNotifyIntent(context);
        } else if (fotaApnState == DMHelper.FOTA_APN_STATE_START_DM_SESSION) {
            setFotaApnState(context, DMHelper.FOTA_APN_STATE_START_DM_SESSION_RPTD);
            // check if message is not expired
            if (DMHelper.isMessageExpired(context)) {
                DMHelper.cleanAllResources(context);
                logd("Warning from handleApnStateActive(): the message is expired.");
                return;
            }

            int currentState = getState(context);

            // nothing to do here
            if (currentState == DMHelper.STATE_IDLE) {
                DMHelper.cleanAllResources(context);
                logd("handleApnStateActive(): there is no message to proceed.");
                return;
            }

            if (currentState == DMHelper.STATE_SESSION_IN_PROGRESS) {
                logd("handleApnStateActive(): session in progress; doing nothing.");
                return;
            }

            startDMSession(context);
        } else {
            logd("handleApnStateActive: NO ACTION NEEDED");
        }
    }

    // check UI mode and prepare and start process
    private void preprocess(Context context, int currentState) {

        setState(context, currentState);

        logd("From preprocess().... Current state = " + currentState);

        // check UI mode. If updates has been replaced with the new one and user already
        // confirmed - we are skipping confirmation.
        if (mUIMode == DMHelper.UI_MODE_CONFIRMATION
                && currentState != DMHelper.STATE_APPROVED_BY_USER) {

            // user confirmation is required
            logd("User confirmation is required");
            DMHelper.postConfirmationNotification(context);
            setState(context, DMHelper.STATE_PENDING_MESSAGE);

            // check and repost notification in case user cancels it
            DMHelper.subscribeForTimeAlert(context,
                    DMHelper.TIME_CHECK_NOTIFICATION_AFTER_SUBSCRIPTION);

            return;
        }

        if (mUIMode == DMHelper.UI_MODE_INFORMATIVE) {
            // required notification, just inform the user
            logd("User notification is required");
            DMHelper.postInformativeNotification_message1(context);
        } else {
            logd("Silent DM session: silent mode or user already has approved.");
        }

        // try to start DM session or start Data and Call State Monitoring Service
        startProcess(context);
    }

    // parse data from intent; set UI mode; save required data.
    private boolean parseAndSaveWapPushMessage(Context context, Intent intent) {

        // Parse message
        Bundle bdl = intent.getExtras();
        byte[] data = bdl.getByteArray("data");
        mData = data;

        if (data == null || data.length < 25) {
            loge("parseAndSaveWapPushMessage: data[] is null or length < 25.");
            return false;
        }

        // first 16 bytes - digest
        int version = ((data[17] >> 6) & 0x3) | ((data[16]) << 2);
        int uiMode = (data[17] >> 4) & 0x3;
        int indicator = (data[17] >> 3) & 0x1;
        int sessionId = ((data[21] & 0xff) << 8) | data[22];
        int serverIdLength = data[23];    // must be equal to data.length-24

        if (serverIdLength <= 0) {
            loge("parseAndSaveWapPushMessage: serverIdLength is invalid: " + serverIdLength);
            return false;
        }

        String serverId = new String(data, 24, serverIdLength, StandardCharsets.UTF_8);
        mUIMode = uiMode;

        // fixme: treating invalid uimode as informative for now for Sprint
        if(mUIMode != DMHelper.UI_MODE_CONFIRMATION && mUIMode != DMHelper.UI_MODE_INFORMATIVE) {
            TelephonyManager tm = TelephonyManager.from(context);
            String simOperator = tm.getSimOperator();
            String imsi = tm.getSubscriberId();
            Log.d(TAG, "simOperator: " + simOperator + " IMSI: " + imsi);
            if ("310120".equals(simOperator) || (imsi != null && imsi.startsWith("310120"))) {
                loge("parseAndSaveWapPushMessage: UICC is sprint. Received uimode " + uiMode +
                        "; changing to informative");
                mUIMode = DMHelper.UI_MODE_INFORMATIVE;
            }
        }

        if (DBG) {
            Log.i(TAG, "Get Provision Package0"
                    + " version:" + version
                    + " uiMode:" + uiMode
                    + " indicator:" + indicator
                    + " sessionId:" + sessionId
                    + " serverId:" + serverId);
        }

        // Save message
        SharedPreferences p = context.getSharedPreferences(DMHelper.DM_PREFERENCES_KEY, 0);
        SharedPreferences.Editor ed = p.edit();
        //ed.putInt("type", DMIntent.TYPE_PKG0_NOTIFICATION);
        //ed.putLong(DMHelper.REQUEST_ID_KEY, System.currentTimeMillis());
        ed.putInt("length", data.length);
        ed.putInt(DMHelper.DM_SESSION_TYPE_KEY, DMIntent.TYPE_PKG0_NOTIFICATION);
        ed.putLong(DMHelper.MESSAGE_TIMESTAMP_ID_KEY, System.nanoTime());
        ed.putInt(DMHelper.DM_UI_MODE_KEY, mUIMode);

        ed.apply();

//        PendingResult pendingResult = goAsync();
//        DMParseSaveWapMsgRunnable dmParseSaveWapMsgRunnable
//                = new DMParseSaveWapMsgRunnable(pendingResult);
//        Thread dmParseSaveWapMsgThread = new Thread(dmParseSaveWapMsgRunnable);
//        dmParseSaveWapMsgThread.start();

        // TODO: move to worker thread
        try {
            FileOutputStream out = new FileOutputStream(DMHelper.POSTPONED_DATA_PATH);
            out.write(mData);
            out.close();
        } catch (IOException e) {
            loge("IOException while creating dmpostponed.dat", e);
        }

        return true;
    }

// TODO: remove or uncomment to use for saving file asynchronously
//    class DMParseSaveWapMsgRunnable implements Runnable {
//        /** Pending result to call finish() when thread returns. */
//        private final PendingResult mPendingResult;
//
//        DMParseSaveWapMsgRunnable(PendingResult pendingResult) {
//            mPendingResult = pendingResult;
//        }
//
//        @Override
//        public void run() {
//            logd("Enter dmParseSaveWapMsgThread tid=" + Thread.currentThread().getId());
//            try {
//                FileOutputStream out = new FileOutputStream(DMHelper.POSTPONED_DATA_PATH);
//                out.write(mData);
//                out.close();
//            } catch (IOException e) {
//                loge("IOException while creating dmpostponed.dat", e);
//            } finally {
//                mPendingResult.finish();
//            }
//        }
//    }

    //try to start DM session or starts Data and Call State Monitoring Service
    private void startProcess(Context context) {
        //wrj348 - VZW customization: reject the wap push if phone is in ECB mode or Roaming
        if (!allowDMSession()) {
            return;
        }
        setState(context, DMHelper.STATE_APPROVED_BY_USER);

        // Start DM session if wifi is available.
        if (isWifiConnected(context)) {
            startDMSession(context);
        } else {
            // request FOTA APN
            logd("startProcess(), start data connection service");
            setFotaApnState(context, DMHelper.FOTA_APN_STATE_START_DM_SESSION);
            startDataConnectionService(context);
        }
    }

    private static boolean allowDMSession() {
        if (isInECBMode()) {
            return false;
        }

        Log.i(TAG, "DMSession allowed - don't reject");
        return true;
    }

    /**
     * Returns whether phone is in emergency callback mode.
     * @return true if the phone is in ECB mode; false if not
     */
    private static boolean isInECBMode() {
        boolean ecbMode = SystemProperties.getBoolean(
                TelephonyProperties.PROPERTY_INECM_MODE, false);
        Log.i(TAG, "Phone ECB status: " + ecbMode);
        return ecbMode;
    }

    //start DM session.
    private void startDMSession(Context context) {
        logd("startDMSession");
        // get request ID from the shared preferences (the message time stamp used)
        SharedPreferences p = context.getSharedPreferences(DMHelper.DM_PREFERENCES_KEY, 0);

        long requestID = p.getLong(DMHelper.MESSAGE_TIMESTAMP_ID_KEY, -1);
        int type = p.getInt(DMHelper.DM_SESSION_TYPE_KEY, -1);

        // create intent and start DM service
        Intent intent = new Intent(DMIntent.LAUNCH_INTENT);
        intent.putExtra(DMIntent.FIELD_REQUEST_ID, requestID);
        intent.putExtra(DMIntent.FIELD_TYPE, type);

        if (type == DMIntent.TYPE_FOTA_CLIENT_SESSION_REQUEST) {
            String serverID = p.getString(DMHelper.FOTA_SERVER_ID_KEY, null);
            String alertString = p.getString(DMHelper.FOTA_ALERT_STRING_KEY, null);
            intent.putExtra(DMIntent.FIELD_TYPE, DMIntent.TYPE_FOTA_CLIENT_SESSION_REQUEST);
            intent.putExtra(DMIntent.FIELD_SERVERID, serverID);
            intent.putExtra(DMIntent.FIELD_ALERT_STR, alertString);
            logd("starting TYPE_FOTA_CLIENT_SESSION_REQUEST: serverID="
                    + serverID + " alertString=" + alertString
                    + " requestID=" + requestID);
        } else {
            // package 0 notification
            intent.putExtra(DMIntent.FIELD_TYPE, type);

            if (mData == null) { // session has not been started right away after receiving a message.
                mData = setDataFromFile(context);

                if (mData == null) {
                    logd("Error. Cannot read data from file dmpostponed.dat");
                    DMHelper.cleanAllResources(context);
                    return;
                }
            }

            intent.putExtra(DMIntent.FIELD_PKG0, mData);
        }

        increaseDMSessionAttempt(context);

        setState(context, DMHelper.STATE_SESSION_IN_PROGRESS);

        DMHelper.subscribeForTimeAlert(context,
                DMHelper.TIME_CHECK_STATUS_AFTER_STARTING_DM_SERVICE);

        intent.setClass(context, DMClientService.class);
        context.startService(intent);
    }

    // start data connection service
    private static void startDataConnectionService(Context context) {
        logd("Inside startDataConnectionService");
        DMHelper.subscribeForTimeAlert(context,
                DMHelper.TIME_CHECK_STATUS_AFTER_STARTING_MONITORING_SERVICE);
        Intent intent = new Intent(DMIntent.ACTION_START_DATA_CONNECTION_SERVICE);
        intent.setClass(context, DMDataConnectionService.class);
        context.startService(intent);
    }

    // stop data connection service
    private static void stopDataConnectionService(Context context) {
        logd("Inside stopDataConnectionService");
        Intent intent = new Intent(DMIntent.ACTION_START_DATA_CONNECTION_SERVICE);
        intent.setClass(context, DMDataConnectionService.class);
        context.stopService(intent);
    }

    // Verify session result: if result is successful, clean all resources.
    // Otherwise, try to resubmit session request.
    private void handleDmServiceResult(Context context, Intent intent) {
        // check if request ID from incoming intent match to the one which has been sent and saved
        // get request ID from the shared preferences (the message time stamp used)
        SharedPreferences p = context.getSharedPreferences(DMHelper.DM_PREFERENCES_KEY, 0);
        SharedPreferences.Editor ed = p.edit();
        long savedRequestId = p.getLong(DMHelper.MESSAGE_TIMESTAMP_ID_KEY, 0);
        long receivedRequestId = intent.getLongExtra(DMIntent.FIELD_REQUEST_ID, -1);

        if (receivedRequestId == -2) {
            logd("handleDmServiceResult, tree initialisation session.");
            return;
        }

        // clear fota apn resources and stop using fota apn
        if (isPhoneTypeLTE()) {
            int fotaApnState = getFotaApnState(context);
            logd("handleDmServiceResult, chk if need to stop using fota apn "
                    + fotaApnState);
            if (fotaApnState != DMHelper.FOTA_APN_STATE_INIT) {
                // resetting FOTA APN STATE
                setFotaApnState(context, DMHelper.FOTA_APN_STATE_INIT);
                stopUsingFotaApn(context);
                // removing shared prefs settings
                DMHelper.cleanFotaApnResources(context);
            }
            stopUsingFotaApn(context);
        }

        if (savedRequestId != receivedRequestId) {
            loge("request ID " + receivedRequestId + " from result intent doesn't "
                    + "match saved request ID " + savedRequestId + ", ignored");
//            return;
        }

        int sessionResult = intent.getIntExtra(DMIntent.FIELD_DMRESULT, -1);

        int uiMode = p.getInt(DMHelper.DM_UI_MODE_KEY, -1);
        mUIMode = uiMode;
        logd("mUIMode is: " + uiMode);
        if (uiMode == DMHelper.UI_MODE_INFORMATIVE) {
            if (sessionResult == DMResult.SYNCML_DM_SUCCESS) {
                logd("Displaying success notification message2");
                DMHelper.postInformativeNotification_message2_success(context);
            } else {
                logd("Displaying Fail notification message2");
                DMHelper.postInformativeNotification_message2_fail(context);
            }
            ed.putInt(DMHelper.DM_UI_MODE_KEY, -1);
            ed.apply();
        }

        if (sessionResult == DMResult.SYNCML_DM_SUCCESS) {
            DMHelper.cleanAllResources(context);
            logd("Finished success.");
            return;
        }

        if (!canRestartSession(context, p)) {
            DMHelper.cleanAllResources(context);
            return;
        }

        // update status in the preferences
        setState(context, DMHelper.STATE_APPROVED_BY_USER);

        //subscribe for the time alert to start DM session again after TIME_BETWEEN_SESSION_ATTEMPTS.
        DMHelper.subscribeForTimeAlert(context, DMHelper.TIME_BETWEEN_SESSION_ATTEMPTS);
    }

    // check if session request can be resubmitted (if message still valid and
    // number of tries doesn't exceed MAX)
    private static boolean canRestartSession(Context context, SharedPreferences p) {
        int numberOfSessionAttempts = p.getInt(DMHelper.DM_SESSION_ATTEMPTS_KEY, -1);

        // check if max number has not been exceeded
        if (numberOfSessionAttempts > DMHelper.MAX_SESSION_ATTEMPTS) {
            logd("Error. Number of attempts to start DM session exceed MAX.");
            return false;
        }

        // check if message is expired or not
        if (DMHelper.isMessageExpired(context)) {
            logd("Error from canRestartSession(): the message is expired.");
            return false;
        }

        return true;
    }

    // set current state
    private static void setState(Context context, int state) {
        SharedPreferences p = context.getSharedPreferences(DMHelper.DM_PREFERENCES_KEY, 0);
        SharedPreferences.Editor ed = p.edit();
        ed.putInt(DMHelper.STATE_KEY, state);
        ed.apply();
    }

    /**
     * Get current state from shared prefs. If state is "Session In Progress", verify that the DM
     * session didn't fail and also has the same status, otherwise current state will be changed
     * to "Approved by User" and will be ready to handle a request for a new DM session.
     *
     * @param context the context to use
     * @return the current state
     */
    private static int getState(Context context) {
        SharedPreferences p = context.getSharedPreferences(DMHelper.DM_PREFERENCES_KEY, 0);
        int currentState = p.getInt(DMHelper.STATE_KEY, 0);

        if (currentState == DMHelper.STATE_SESSION_IN_PROGRESS
                && !DMClientService.sIsDMSessionInProgress) {
            currentState = DMHelper.STATE_APPROVED_BY_USER;
            setState(context, currentState);
        }
        return currentState;
    }

    // increase attempt to start DM session
    private static void increaseDMSessionAttempt(Context context) {
        SharedPreferences p = context.getSharedPreferences(DMHelper.DM_PREFERENCES_KEY, 0);
        int numberOfSessionAttempts = p.getInt(DMHelper.DM_SESSION_ATTEMPTS_KEY, 0);
        SharedPreferences.Editor ed = p.edit();
        ed.putInt(DMHelper.DM_SESSION_ATTEMPTS_KEY, (numberOfSessionAttempts + 1));
        ed.apply();
    }

    // check and initialize variables from preferences
    private boolean initFromSharedPreferences(Context context) {
        SharedPreferences p = context.getSharedPreferences(DMHelper.DM_PREFERENCES_KEY, 0);
        long timestamp = p.getLong(DMHelper.MESSAGE_TIMESTAMP_ID_KEY, -1);
        mUIMode = p.getInt(DMHelper.DM_UI_MODE_KEY, -1);
        mData = setDataFromFile(context);
        boolean success = !(timestamp <= 0 || mUIMode < 0);
        if (DBG) logd("initFromSharedPreferences: " + (success ? "ok" : "fail"));
        return success;
    }

    private static byte[] setDataFromFile(Context context) {
        SharedPreferences p = context.getSharedPreferences(DMHelper.DM_PREFERENCES_KEY, 0);
        int length = p.getInt("length", -1);

        if (length <= 0) {
            //logd("Error. Invalid postponed data length.");
            return null;
        }

        byte[] data = new byte[length];

        try {
            FileInputStream in = new FileInputStream(DMHelper.POSTPONED_DATA_PATH);
            if (in.read(data) <= 0) {
                logd("Invalid postponed data.");
                in.close();
                return null;
            }
            in.close();
            return data;
        } catch (IOException e) {
            loge("IOException", e);
            return null;
        }
    }

    private static boolean treeExist(Context context) {
        if (context != null) {
            String strTreeHomeDir = context.getFilesDir().getAbsolutePath() + "/dm";
            File dirDes = new File(strTreeHomeDir);

            if (dirDes.exists() && dirDes.isDirectory()) {
                logd("DM Tree exists:" + strTreeHomeDir);
                return true;
            } else {
                logd("DM Tree NOT exists:" + strTreeHomeDir);
                return false;
            }
        } else {
            return false;
        }
    }

    // set current state
    private static void setFotaApnState(Context context, int state) {
        logd("setFotaApnState: " + state);
        SharedPreferences p = context.getSharedPreferences(DMHelper.FOTA_APN_PREFERENCE_KEY, 0);
        SharedPreferences.Editor ed = p.edit();
        ed.putInt(DMHelper.FOTA_APN_STATE_KEY, state);
        ed.apply();
    }

    // get current state.
    private static int getFotaApnState(Context context) {
        SharedPreferences p = context.getSharedPreferences(DMHelper.FOTA_APN_PREFERENCE_KEY, 0);
        return p.getInt(DMHelper.FOTA_APN_STATE_KEY, 0);
    }

    /**
     * Stop using the FOTA APN.
     * @param context the BroadcastReceiver context
     */
    private static void stopUsingFotaApn(Context context) {
        logd("stopUsingFotaApn");

        ConnectivityManager connMgr = (ConnectivityManager) context
                .getSystemService(Context.CONNECTIVITY_SERVICE);
        int result = connMgr.stopUsingNetworkFeature(ConnectivityManager.TYPE_MOBILE,
                Phone.FEATURE_ENABLE_FOTA);
        if (result != -1) {
            Log.w(TAG, "stopUsingNetworkFeature result=" + result);
        }
        stopDataConnectionService(context);
    }

    // Function which will send intents to start FDM
    private static void sendNotifyIntent(Context context) {
        logd("Inside sendNotifyIntent");

        SharedPreferences p = context.getSharedPreferences(DMHelper.FOTA_APN_PREFERENCE_KEY, 0);
        String lawmoResult = p.getString(DMHelper.LAWMO_RESULT_KEY, null);
        String fotaResult = p.getString(DMHelper.FOTA_RESULT_KEY, null);
        String pkgURI = p.getString(DMHelper.PKG_URI_KEY, null);
        String alertType = p.getString(DMHelper.ALERT_TYPE_KEY, null);
        String correlator = p.getString(DMHelper.CORRELATOR_KEY, null);
        String serverID = p.getString(DMHelper.SERVER_ID_KEY, null);

        logd("sendNotifyIntent Input==>\n" + " lawmoResult="
                + lawmoResult + '\n' + "fotaResult="
                + fotaResult + '\n' + " pkgURI="
                + pkgURI + '\n' + " alertType="
                + alertType + '\n' + " serverID="
                + serverID + '\n' + " correlator="
                + correlator);

        if (alertType.equals(ALERT_TYPE_DOWNLOADANDUPDATE)) {
            // Need to send an intent for doing a FOTA FDM session
            Intent fotafdmintent = new Intent(DMIntent.LAUNCH_INTENT);
            fotafdmintent.putExtra(DMIntent.FIELD_TYPE, DMIntent.TYPE_FOTA_NOTIFY_SERVER);
            fotafdmintent.putExtra(DMIntent.FIELD_FOTA_RESULT, fotaResult);
            fotafdmintent.putExtra(DMIntent.FIELD_PKGURI, pkgURI);
            fotafdmintent.putExtra(DMIntent.FIELD_ALERTTYPE, alertType);
            fotafdmintent.putExtra(DMIntent.FIELD_SERVERID, serverID);
            fotafdmintent.putExtra(DMIntent.FIELD_CORR, correlator);
            fotafdmintent.setClass(context, DMClientService.class);
            context.startService(fotafdmintent);
        } else if (pkgURI.equals(RP_OPERATIONS_FACTORYRESET) || pkgURI
                .equals(RP_EXT_OPERATIONS_RESET)) {
            // LAWMO FDM session
            Intent lawmofdmintent = new Intent(DMIntent.LAUNCH_INTENT);
            lawmofdmintent.putExtra(DMIntent.FIELD_TYPE, DMIntent.TYPE_LAWMO_NOTIFY_SESSION);
            lawmofdmintent.putExtra(DMIntent.FIELD_LAWMO_RESULT, lawmoResult);
            lawmofdmintent.putExtra(DMIntent.FIELD_PKGURI, pkgURI);
            lawmofdmintent.putExtra(DMIntent.FIELD_ALERTTYPE, "");
            lawmofdmintent.putExtra(DMIntent.FIELD_CORR, "");
            lawmofdmintent.setClass(context, DMClientService.class);
            context.startService(lawmofdmintent);
        } else {
            // just return for now
            logd("No Action, Just return for now");
        }
    }

    private static boolean isWifiConnected(Context context) {
        logd("Inside isWifiConnected");

        ConnectivityManager cm = (ConnectivityManager) context
                .getSystemService(Context.CONNECTIVITY_SERVICE);
        if (cm == null) {
            logd("can't get Connectivity Service");
            return false;
        }

        NetworkInfo ni = cm.getActiveNetworkInfo();
        if (ni == null) {
            logd("NetworkInfo is null");
            return false;
        }
        if (!ni.isConnected()) {
            logd("Network is not connected");
            return false;
        }
        if (ni.getType() != ConnectivityManager.TYPE_WIFI) {
            logd("network type is not wifi");
            return false;
        }

        // return true only when WiFi is connected
        return true;
    }


    private static boolean isPhoneTypeLTE() {
        return DMSettingsHelper.isPhoneTypeLTE();
    }

    private static boolean isPhoneTypeCDMA3G(Context context) {
        TelephonyManager tm = (TelephonyManager) context
                .getSystemService(Context.TELEPHONY_SERVICE);
        if ((tm.getCurrentPhoneType() == TelephonyManager.PHONE_TYPE_CDMA) && !isPhoneTypeLTE()) {
            logd("3G CDMA phone");
            return true;
        }
        logd("Non-CDMA or 4G Device");
        return false;
    }

    // check if we can set up a mobile data connection on this network type
    private static boolean isDataNetworkAcceptable(Context context) {
        TelephonyManager tm = (TelephonyManager) context
                .getSystemService(Context.TELEPHONY_SERVICE);

        int callState = tm.getCallState();
        if (callState != TelephonyManager.CALL_STATE_IDLE) {
            logd("Call state not idle: " + callState);
            return false;
        }

        int dataNetworkType = tm.getDataNetworkType();
        switch (dataNetworkType) {
            case TelephonyManager.NETWORK_TYPE_EVDO_0:
            case TelephonyManager.NETWORK_TYPE_EVDO_A:
            case TelephonyManager.NETWORK_TYPE_1xRTT:
            case TelephonyManager.NETWORK_TYPE_EVDO_B:
            case TelephonyManager.NETWORK_TYPE_LTE:
            case TelephonyManager.NETWORK_TYPE_EHRPD:
                logd("Data network type is acceptable: " + dataNetworkType);
                return true;

            default:
                logd("Data network type is not acceptable: " + dataNetworkType);
                return false;
        }
    }

    private static void saveDevDetail(Context context) {
        logd("Inside saveDevDetail");

        String swVer = SystemProperties.get("ro.build.version.full");
        if (TextUtils.isEmpty(swVer)) {
            swVer = "Unknown";
        }

        SharedPreferences p = context.getSharedPreferences(DEV_DETAIL, 0);
        String currFwV = p.getString(CURR_FW_VER, null);
        //String preFwV = p.getString(PRE_FW_VER, null);

        SharedPreferences.Editor ed = p.edit();
        if (TextUtils.isEmpty(currFwV)) {
            logd("First powerup or powerup after FDR, save current SwV");
            ed.putString(CURR_FW_VER, swVer);
        } else if (!(currFwV.equals(swVer))) {
            logd("System Update success, save previous FwV and LastUpdateTime");
            ed.putString(PRE_FW_VER, currFwV);
            ed.putString(CURR_FW_VER, swVer);
            SimpleDateFormat simpleDateFormat;
            if (isPhoneTypeLTE()) {
                simpleDateFormat = new SimpleDateFormat("MM:dd:yyyy:HH:mm", Locale.US);
                simpleDateFormat.setTimeZone(TimeZone.getTimeZone("UTC"));
            } else {
                simpleDateFormat = new SimpleDateFormat("MM:dd:yy:HH:mm:ss:z", Locale.US);
            }
            String currTime = simpleDateFormat.format(new Date(System.currentTimeMillis()));
            ed.putString(LAST_UPD_TIME, currTime);
        }
        WifiManager wm = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        WifiInfo wi = wm.getConnectionInfo();
        String wMacAddr = (wi == null) ? null : wi.getMacAddress();
        logd("WiFi Mac address " + wMacAddr);
        if (!TextUtils.isEmpty(wMacAddr)) {
            ed.putString(WIFI_MAC_ADDR, wMacAddr);
        }
        ed.apply();
    }

    private static void logd(String msg) {
        Log.d(TAG, msg);
    }

    private static void loge(String msg) {
        Log.e(TAG, msg);
    }

    private static void loge(String msg, Throwable tr) {
        Log.e(TAG, msg, tr);
    }
}
