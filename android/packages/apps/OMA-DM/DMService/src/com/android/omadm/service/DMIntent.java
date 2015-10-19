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

public interface DMIntent {

    String LAUNCH_INTENT = "com.android.omadm.service.Launch";

    String DM_SERVICE_RESULT_INTENT = "com.android.omadm.service.Result";

    String SHOW_PKG0_ALERT_DLG = "com.android.omadm.service.pkg0_alert_dlg";

    String SHOW_PKG0_INFO_DLG = "com.android.omadm.service.pkg0_info_dlg";

    String SHOW_UPDATE_CANCEL_DLG = "com.android.omadm.service.update_cancel_dlg";

    String SHOW_PKG0_ALERT_DLG_CLOSE = "com.android.omadm.service.pkg0_alert_dlg_close";

    String SHOW_DISPLAY_ALERT_DLG = "com.android.omadm.service.display_alert_dlg";

    String SHOW_CONFIRM_ALERT_DLG = "com.android.omadm.service.confirm_alert_dlg";

    String SHOW_TEXTINPUT_ALERT_DLG = "com.android.omadm.service.textinput_alert_dlg";

    String SHOW_SINGLECHOICE_ALERT_DLG = "com.android.omadm.service.singlechoice_alert_dlg";

    String SHOW_MULTICHOICE_ALERT_DLG = "com.android.omadm.service.multichoice_alert_dlg";

    String SHOW_PROGRESS_ALERT_DLG = "com.android.omadm.service.show_progress_alert_dlg";

    String HIDE_PROGRESS_ALERT_DLG = "com.android.omadm.service.hide_progress_alert_dlg";

    String CANCEL_ALERT_DLG = "com.android.omadm.service.cancel_alert_dlg";

    String DM_ALERT_DLG_CLOSED = "com.android.omadm.service.dm_alert_dlg_closed";

    String ACTION_TIMER_ALERT = "com.android.omadm.service.pending_notification";

    // for UI mode Informative management action
    String ACTION_CLOSE_NOTIFICATION_INFO = "com.android.omadm.service.close_notification_info";

    // for waiting for Wi-Fi or waiting for mobile data and then bringing up the FOTA APN
    String ACTION_START_DATA_CONNECTION_SERVICE = "com.android.omadm.service.StartDataConnection";

    // data connection was successfully started
    String ACTION_DATA_CONNECTION_READY = "com.android.omadm.service.DataConnectionReady";

    // user from UI confirmed starting DM session
    String ACTION_USER_CONFIRMED_DM_SESSION
            = "com.android.omadm.service.user_confirmed_dm_session";

    // inject package0 from command line
    String ACTION_INJECT_PACKAGE_0_INTERNAL = "com.android.omadm.service.InjectPackage0_Internal";

    // internal wap push intent
    String ACTION_WAP_PUSH_RECEIVED_INTERNAL
            = "com.android.omadm.service.WAP_PUSH_RECEIVED_INTERNAL";

    // start client initiated provisioning request
    String ACTION_CLIENT_INITIATED_FOTA_SESSION
            = "com.android.omadm.service.client_initiated_fota";

    // set server hostname info
    String ACTION_SET_SERVER_CONFIG = "com.android.omadm.service.set_server_config";

    // cancel DM session
    String ACTION_CANCEL_SESSION = "com.android.omadm.service.cancel_dm_session";

    int TYPE_UNKNOWN = 0;

    int TYPE_PKG0_NOTIFICATION = 1;

    int TYPE_FOTA_CLIENT_SESSION_REQUEST = 2;

    int TYPE_FOTA_NOTIFY_SERVER = 3;

    int TYPE_CANCEL_DM_SESSION = 4;

    int TYPE_CLIENT_SESSION_REQUEST = 6;

    int TYPE_LAWMO_NOTIFY_SESSION = 15;

    int TYPE_DO_NOTHING = 100;


    String FIELD_TYPE = "Type";

    String FIELD_PKG0 = "Pkg0";

    String FIELD_REQUEST_ID = "RequestID";

    String FIELD_ALERT_STR = "AlertStr";

    String FIELD_DMRESULT = "DMResult";

    String FIELD_FOTA_RESULT = "fotaResult";

    String FIELD_PKGURI = "PkgURI";

    String FIELD_ALERTTYPE = "AlertType";

    String FIELD_SERVERID = "ServerID";

    String FIELD_SERVER_URL = "ServerURL";

    String FIELD_PROXY_ADDRESS = "ProxyAddress";

    String FIELD_TIMER = "Timer";

    String FIELD_CORR = "Correlator";

    String FIELD_DM_UNITEST_RESULT = "UnitestResult";

    String FIELD_FILENAME = "FileName";

    String FIELD_IS_BINARY = "IsBinary";

    String FIELD_BOOTSTRAP_MSG = "BootstrapMsg";

    String FIELD_LAWMO_RESULT = "LawmoResult";

}
