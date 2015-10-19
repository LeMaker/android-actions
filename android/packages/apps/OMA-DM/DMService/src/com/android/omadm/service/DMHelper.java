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

import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Process;
import android.content.pm.PackageManager;
import android.util.Log;

import java.io.File;

final class DMHelper {
    private static final String TAG = "DMHelper";
    private static final boolean DBG = DMClientService.DBG;

    public static final String POSTPONED_DATA_PATH
            = "/data/data/com.android.omadm.service/shared_prefs/dmpostponed.dat";

    private static final String FOTA_APN_FILE_PATH
            = "/data/data/com.android.omadm.service/shared_prefs/fotaapnprefs.xml";

    public static final int UI_MODE_INFORMATIVE = 2;

    public static final int UI_MODE_CONFIRMATION = 3;

    public static final int NOTIFICATION_INFORMATIVE_ID = 0xcb01fa64;

    public static final int NOTIFICATION_CONFIRMATION_ID = 0xadc19b91;


    // -------  Constant VALUES -----------//
    // message lifetime (24 hours) in milliseconds
    private static final long MESSAGE_LIFETIME = 24 * 60 * 60 * 1000000000L; //nanoseconds

    // maximum number failures to attempt starting DM session
    public static final long MAX_SESSION_ATTEMPTS = 3;

    // time before attempt start DM session after a failure (in seconds)
    public static final int TIME_BETWEEN_SESSION_ATTEMPTS = 30 * 60; //seconds

    // time to check status after starting DM service; try to restart if service will die (in seconds)
    public static final int TIME_CHECK_STATUS_AFTER_STARTING_DM_SERVICE = 30 * 60; //seconds

    // time to check and repost notification in case if user cancel it; subscribe during posting notification (in seconds)
    public static final int TIME_CHECK_NOTIFICATION_AFTER_SUBSCRIPTION = 30 * 60; //seconds

    // time to check status after starting call and data monitoring service; try to restart if service will die (in seconds)
    public static final int TIME_CHECK_STATUS_AFTER_STARTING_MONITORING_SERVICE = 20 * 60; //seconds


    // -----  States ----//
    public static final String STATE_KEY = "dmmsgstate";

    public static final int STATE_IDLE = 0;                 // nothing there

    public static final int STATE_PENDING_MESSAGE = 1;      // message received and saved

    public static final int STATE_APPROVED_BY_USER = 2;     // message approved or silent

    public static final int STATE_SESSION_IN_PROGRESS = 3;  // DM session has been started


    // -----  Keys  ----//
    // shared properties name
    public static final String DM_PREFERENCES_KEY = "dmpostponed";

    // key to keep timestamp for the message in the shared properties. Also used as a request ID
    public static final String MESSAGE_TIMESTAMP_ID_KEY = "dm_msg_time_init";

    public static final String MESSAGE_SERVER_URL_KEY = "dm_server_url";

    public static final String MESSAGE_PROXY_HOSTNAME_KEY = "dm_proxy_hostname";

    // key to keep number attempts to start DM session
    public static final String DM_SESSION_ATTEMPTS_KEY = "dm_session_attempts";

    // key to keep uiMode
    public static final String DM_UI_MODE_KEY = "dm_ui_mode";

    // key for pending session type
    public static final String DM_SESSION_TYPE_KEY = "type";

    // ---- Key and States used for fota apn ----//
    // shared property name
    public static final String FOTA_APN_PREFERENCE_KEY = "fotaapnprefs";

    public static final String FOTA_APN_STATE_KEY = "fotapnstate";

    public static final String FOTA_ALERT_STRING_KEY = "fota_alert_string";

    public static final String FOTA_SERVER_ID_KEY = "fota_server_id";

    // initial APN monitor state
    public static final int FOTA_APN_STATE_INIT = 0; // no action needed for this

    // apn switch needed for NI session
    public static final int FOTA_APN_STATE_START_DM_SESSION = 1;

    // apn switch needed for FDM session
    public static final int FOTA_APN_STATE_REPORT_DM_SESSION = 2;

    // RPTD states used to decide when to stop using fota apn
    public static final int FOTA_APN_STATE_START_DM_SESSION_RPTD = 3;
    public static final int FOTA_APN_STATE_REPORT_DM_SESSION_RPTD = 4;

    // ---- Keys used for storing FDM MSG Values ----//
    public static final String LAWMO_RESULT_KEY = "lawmoResult";

    public static final String FOTA_RESULT_KEY = "fotaResult";

    public static final String PKG_URI_KEY = "pkgURI";

    public static final String ALERT_TYPE_KEY = "alertType";

    public static final String CORRELATOR_KEY = "correlator";

    public static final String SERVER_ID_KEY = "serverID";

    // --- Key used for storing hostname override for Sprint ---//
    public static final String SERVER_HOSTNAME_OVERRIDE_KEY = "serverHostname";

    //--- Keys used for storing imei ---//
    public static final String IMEI_PREFERENCE_KEY = "imeivalue";

    public static final String IMEI_VALUE_KEY = "imei";

    //--- Keys used for storing AKEY ---//
    public static final String AKEY_PREFERENCE_KEY = "akeyvalue";

    public static final String AKEY_VALUE_KEY = "akey";

    private static boolean sfirstTriggerReceived = false;

    // private constructor
    private DMHelper() {}

    // Subscribing for the Timer Alert
    public static void subscribeForTimeAlert(Context context, int seconds) {
        cancelTimeAlert(context);

        logd("subscribeForTimeAlert ...");

        Intent intent = new Intent(context, DMIntentReceiver.class);
        intent.setAction(DMIntent.ACTION_TIMER_ALERT);
        PendingIntent sender = PendingIntent.getBroadcast(context, 0, intent, 0);

        long wakeupTime = System.currentTimeMillis() + (seconds * 1000L);

        // Schedule the alarm
        AlarmManager am = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        am.set(AlarmManager.RTC_WAKEUP, wakeupTime, sender);
        logd("subscribeForTimeAlert for " + seconds + " seconds done!");
    }

    // Canceling subscription for time alert
    private static void cancelTimeAlert(Context context) {
        logd("cancelTimeAlarm ...");

        Intent intent = new Intent(context, DMIntentReceiver.class);
        intent.setAction(DMIntent.ACTION_TIMER_ALERT);
        PendingIntent sender = PendingIntent.getBroadcast(context, 0, intent, 0);

        AlarmManager am = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        am.cancel(sender);
        logd("cancelTimeAlarm done!");
    }

    // post confirmation notification when UI mode required user interaction
    public static void postConfirmationNotification(Context context) {
        NotificationManager mNotificationManager = (NotificationManager) context
                .getSystemService(Context.NOTIFICATION_SERVICE);

        CharSequence text = context.getText(
                R.string.dm_session_confirmation_notification_message)
                .toString();

        Intent provisioning = new Intent(context, DMSessionConfirmAlertActivity.class);

        PendingIntent contentIntent = PendingIntent.getActivity(context, 0, provisioning, 0);

        Notification notification = new Notification.Builder(context)
                .setSmallIcon(R.drawable.alert_dialog_icon)
                .setTicker(text)
                .setWhen(System.currentTimeMillis())
                .setContentTitle(
                        context.getText(R.string.dm_session_confirmation_notification_label))
                .setContentText(text)
                .setContentIntent(contentIntent)
                .setColor(context.getResources().getColor(
                        com.android.internal.R.color.system_notification_accent_color))
                .build();

        mNotificationManager.notify(NOTIFICATION_CONFIRMATION_ID,
                notification);
    }

    // post informative notification when UI mode required to inform users
    private static void postInformativeNotification(Context context, int titleId, int textId) {
        NotificationManager mNotificationManager = (NotificationManager) context
                .getSystemService(Context.NOTIFICATION_SERVICE);

        CharSequence text = context.getText(textId);

        Intent provisioning = new Intent(context, DMIntentReceiver.class);
        provisioning.setAction(DMIntent.ACTION_CLOSE_NOTIFICATION_INFO);

        PendingIntent contentIntent = PendingIntent.getBroadcast(context, 0, provisioning, 0);

        Notification notification = new Notification.Builder(context)
                .setSmallIcon(R.drawable.alert_dialog_icon)
                .setTicker(text)
                .setWhen(System.currentTimeMillis())
                .setContentTitle(context.getText(titleId))
                .setContentText(text)
                .setContentIntent(contentIntent)
                .setColor(context.getResources().getColor(
                        com.android.internal.R.color.system_notification_accent_color))
                .build();

        mNotificationManager.notify(NOTIFICATION_INFORMATIVE_ID, notification);
    }

    // post informative notification when UI mode required to inform users
    public static void postInformativeNotification_message1(Context context) {
        postInformativeNotification(context,
                R.string.dm_session_information_notification_label,
                R.string.dm_session_information_notification_message1);
    }

    //post informative notification when UI mode required to inform users
    public static void postInformativeNotification_message2_success(Context context) {
        postInformativeNotification(context,
                R.string.dm_session_information_notification_label,
                R.string.dm_session_information_notification_message2_success);
    }

    //post informative notification when UI mode required to inform users
    public static void postInformativeNotification_message2_fail(Context context) {
        postInformativeNotification(context,
                R.string.dm_session_information_notification_label,
                R.string.dm_session_information_notification_message2_fail);
    }

    //clear notification
    public static void cancelNotification(Context context, int notificationId) {
        NotificationManager nm = (NotificationManager) context
                .getSystemService(Context.NOTIFICATION_SERVICE);
        nm.cancel(notificationId);
    }


    //remove message and all from shared properties
    private static void clearSharedPreferences(Context context) {
        SharedPreferences p = context.getSharedPreferences(DM_PREFERENCES_KEY, 0);
        SharedPreferences.Editor ed = p.edit();
        ed.clear();
        ed.apply();

        //remove file with message
        File file = new File(POSTPONED_DATA_PATH);
        if (file.exists()) {
            file.delete();
        }
    }

    // set Sprint server URL
    public static void setServerUrl(Context context, String url) {
        logd("setServerUrl: " + url);
        SharedPreferences p = context.getSharedPreferences(SERVER_HOSTNAME_OVERRIDE_KEY, 0);
        p.edit().putString(MESSAGE_SERVER_URL_KEY, url).apply();
    }

    // set Sprint proxy hostname
    public static void setProxyHostname(Context context, String hostname) {
        logd("setProxyHostname: " + hostname);
        SharedPreferences p = context.getSharedPreferences(SERVER_HOSTNAME_OVERRIDE_KEY, 0);
        p.edit().putString(MESSAGE_PROXY_HOSTNAME_KEY, hostname).apply();
    }

    // get Sprint server URL
    public static String getServerUrl(Context context) {
        SharedPreferences p = context.getSharedPreferences(SERVER_HOSTNAME_OVERRIDE_KEY, 0);
        String url = p.getString(MESSAGE_SERVER_URL_KEY, null);
        logd("getServerUrl: " + url);
        return url;
    }

    // get Sprint proxy hostname
    public static String getProxyHostname(Context context) {
        SharedPreferences p = context.getSharedPreferences(SERVER_HOSTNAME_OVERRIDE_KEY, 0);
        String hostname = p.getString(MESSAGE_PROXY_HOSTNAME_KEY, null);
        logd("getProxyHostname: " + hostname);
        return hostname;
    }

    // check if message is expired; compares current time with the timestamp
    // for the message and its lifetime
    public static boolean isMessageExpired(Context context) {
        // get message timestamp from preferences
        SharedPreferences p = context.getSharedPreferences(DM_PREFERENCES_KEY, 0);
        long messageTimestamp = p.getLong(MESSAGE_TIMESTAMP_ID_KEY, -1);
        logd("isMessageExpired: messageTimestamp is " + messageTimestamp);

        return messageTimestamp == -1 || System.nanoTime() > (messageTimestamp
                + MESSAGE_LIFETIME);
    }

    // clean all...  shared preferences, notifications, time alerts....
    public static void cleanAllResources(Context context) {
        logd("Inside cleanAllResources");
        clearSharedPreferences(context);
        // Not canceling informative notification since otherwise it disappears very quickly (as
        // soon as DM session ends which is just a few seconds)
        // cancelNotification(context, NOTIFICATION_INFORMATIVE_ID);
        cancelNotification(context, NOTIFICATION_CONFIRMATION_ID);
        cancelTimeAlert(context);
    }

    // remove message and all from shared properties for fota apn
    private static void clearFotaApnSharedPreferences(Context context) {
        logd("Inside clearFotaApnSharedPreferences");
        SharedPreferences p = context.getSharedPreferences(
                FOTA_APN_PREFERENCE_KEY, 0);
        SharedPreferences.Editor ed = p.edit();
        ed.clear();
        ed.apply();

        //remove file with message
        File file = new File(FOTA_APN_FILE_PATH);
        if (file.exists()) {
            logd("fotaapnprefs.xml file deleted");
            // FIXME: don't ignore result of delete
            file.delete();
        }
    }

    public static boolean isRunningAsOwner() {
        return Process.myUserHandle().isOwner();
    }

    public static void cleanFotaApnResources(Context context) {
        if (DBG) logd("Inside cleanFotaApnResources");
        clearFotaApnSharedPreferences(context);
    }

    public static boolean disableIfSecondaryUser(Context context) {
        if (sfirstTriggerReceived == false) {
            sfirstTriggerReceived = true;
            if (!isRunningAsOwner()) {
                PackageManager pm = context.getPackageManager();
                logd("Disabling com.android.omadm.service for secondary user");
                pm.setApplicationEnabledSetting("com.android.omadm.service",
                        PackageManager.COMPONENT_ENABLED_STATE_DISABLED, 0 );
                return true;
            }
        }
        return false;
    }

    private static void logd(String msg) {
        Log.d(TAG, msg);
    }
}
