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

import android.util.Log;

import java.io.File;
import java.io.FilenameFilter;

class DMSession {
    private static final String TAG = "DMSession";
    private static final boolean DBG = DMClientService.DBG;

    private final String mLogPath;

    private int mResultCode;

    private DMHttpConnector mHttpConnector;

    private final DMAlert mDMAlert;

    private final DMClientService mDMClientService;

    private String mLogName;

    private String mServerID;

    public String getLogFileName() {
        return mLogName;
    }

    DMSession(DMClientService context) {
        mLogName = null;
        mDMClientService = context;
        mDMAlert = new DMAlert(context);
        mLogPath = context.getFilesDir().getPath() + "/dm/log/";
        if (DBG) logd("XXX DMSession mLogPath dir = " + mLogPath);
    }

    public int startClientSession(String serverID) {
        prepareLogFile();

        try {
            mServerID = serverID;
            mHttpConnector = new DMHttpConnector(this);

            if (DBG) logd("Start client session with server: " + mServerID);
            mResultCode = NativeDM.startClientSession(mServerID, this);

            logd("startDmSession resultCode: " + mResultCode);
        } catch (RuntimeException e) {
            loge("Exception caught starting DM session", e);
        }

        mHttpConnector.closeSession();
        mHttpConnector = null;
        mLogName = null;

        return mResultCode;
    }

    public int startFotaClientSession(String serverID, String alertStr) {
        prepareLogFile();

        try {
            mServerID = serverID;
            mHttpConnector = new DMHttpConnector(this);

            if (DBG) logd("Start DM session with server: " + mServerID
                    + " alert string: " + alertStr);
            mResultCode = NativeDM.startFotaClientSession(serverID, alertStr, this);

            logd("startDmSession resultCode: " + mResultCode);
        } catch (Exception e) {
            loge("Exception caught starting DM session", e);
        }

        mHttpConnector.closeSession();
        mHttpConnector = null;
        mLogName = null;

        return mResultCode;
    }

    public int startLawmoNotifySession(FotaNotifyContext LawmoContext) {
        prepareLogFile();

        try {
            mServerID = mDMClientService.getConfigDB().getFotaServerID();
            mHttpConnector = new DMHttpConnector(this);

            if (DBG) logd("Start LAWMONotify session with server: " + mServerID
                    + " PkgURI: " + LawmoContext.mPkgURI
                    + " AlertType: " + LawmoContext.mAlertType
                    + " Result: " + LawmoContext.mResult);
            mResultCode = NativeDM.startFotaNotifySession(
                    LawmoContext.mResult,
                    LawmoContext.mPkgURI,
                    LawmoContext.mAlertType,
                    mServerID,
                    LawmoContext.mCorrelator,
                    this);
            logd("LAWMONotifySession resultCode: " + mResultCode);
        } catch (Exception e) {
            loge("Exception caught starting DM session", e);
        }

        mHttpConnector.closeSession();
        mHttpConnector = null;
        mLogName = null;

        return mResultCode;
    }

    public int startPkg0AlertSession(byte[] data) {
        DMPkg0Notification notification = new DMPkg0Notification();
        int ret = NativeDM.parsePkg0(data, notification);
        if (ret != DMResult.SYNCML_DM_SUCCESS) {
            if (ret != DMResult.SYNCML_DM_SESSION_AUTH_FAIL) {
                logd("parsePkg0 return:" + ret);
                return ret;
            }
            if (mDMClientService.getConfigDB().isDmNonceResyncEnabled()) {
                logd("Nonce resync enabled, ignore as per OMA DM spec.");
                return ret;
            }
        }

        if (DBG) logd("NativeDM.parsePkg0 result: uiMode: " + notification.getUIMode()
                + " initiator: " + notification.getInitiator()
                + " sessionId: " + notification.getSessionID()
                + " serverId: " + notification.getServerID());

        if (notification.getServerID() == null) {
            return DMResult.SYNCML_DM_SESSION_PARAM_ERR;
        }

        if (notification.getUIMode() == 3 || notification.getUIMode() == 0) {
            mDMAlert.setUIMode(mDMClientService.getConfigDB().isDmAlertEnabled());
        } else {
            mDMAlert.setUIMode(false);
        }
        prepareLogFile();

        mServerID = notification.getServerID();
        mHttpConnector = new DMHttpConnector(this);

        try {
            if (notification.getInitiator() == 1) {
                logd("Start server initiated session");
                mResultCode = NativeDM.startFotaServerSession(notification.getServerID(),
                        notification.getSessionID(),
                        this);
            } else {
                logd("Start client initiated session");
                String alertStr = "org.openmobilealliance.dm.firmwareupdate.userrequest";
                mResultCode = NativeDM.startFotaClientSession(notification.getServerID(), alertStr,
                        this);
            }

            logd("startDmSession resultCode: " + mResultCode);

            if (mResultCode == DMResult.SYNCML_DM_SUCCESS) {
                mDMClientService.getConfigDB().setFotaServerID(notification.getServerID());
            }
        } catch (Exception e) {
            loge("Exception caught starting DM session", e);
        }

        mHttpConnector.closeSession();
        mHttpConnector = null;
        mLogName = null;

        return mResultCode;
    }

    public int fotaNotifyDMServer(FotaNotifyContext fotaContext) {
        prepareLogFile();

        try {
            mServerID = fotaContext.mServerID;
            mHttpConnector = new DMHttpConnector(this);

            mResultCode = NativeDM.startFotaNotifySession(
                    fotaContext.mResult,
                    fotaContext.mPkgURI,
                    fotaContext.mAlertType,
                    fotaContext.mServerID,
                    fotaContext.mCorrelator,
                    this);
            if (DBG) logd("fotaNotifyDMServer resultCode: " + mResultCode);
        } catch (Exception e) {
            loge("Exception caught starting DM session", e);
        }

        mHttpConnector.closeSession();
        mHttpConnector = null;
        mLogName = null;

        return mResultCode;
    }

    public int cancelSession() {
        NativeDM.cancelSession();       // Just set cancel flag.

        mHttpConnector.closeSession();
        mHttpConnector = null;

        mDMAlert.cancelSession();
        return 0;
    }

    private void prepareLogFile() {
        File dirs = new File(mLogPath);
        FilenameFilter filter = new SyncMLFilenameFilter();

        int maxNum = 0;
        String[] files = dirs.list(filter);

        if(files != null) {
            for (String f : files) {
                String digitalStr = f.substring(7, 8);
                try {
                    int number = Integer.parseInt(digitalStr);
                    if (number > maxNum) {
                        maxNum = number;
                    }

                } catch (NumberFormatException e) {
                    loge("NumberFormatException in prepareLogFile()", e);
                }
            }
            maxNum++;
        }

        if (files == null || maxNum >= 10) {
            // FIXME: check return value
            dirs.delete();
            // FIXME: check return value
            dirs.mkdirs();
            maxNum = 1;
        }

        String num = Integer.toString(maxNum);
        String logName = "SyncML_" + num + ".txt";
        mLogName = mLogPath + logName;
        if (DBG) logd("Log File: " + mLogName);
    }

    public DMClientService getServiceContext() {
        return mDMClientService;
    }

    /**
     * Get the {@link DMHttpConnector} object for this session. Called from JNI code.
     *
     * @return the DMHttpConnector object for this session
     */
    public DMHttpConnector getNetConnector() {
        return mHttpConnector;
    }

    /**
     * Get the {@link DMAlert} object for this session. Called from JNI code.
     *
     * @return the DMAlert object for this session
     */
    public DMAlert getDMAlert() {
        return mDMAlert;
    }

    // FIXME: remove if unused?!
    public String getServerID() {
        return mServerID;
    }

    private static class SyncMLFilenameFilter implements FilenameFilter {

        SyncMLFilenameFilter() {
        }

        @Override
        public boolean accept(File dir, String name) {
            return name.endsWith(".txt") && name.startsWith("SyncML_");
        }
    }

    private static void logd(String msg) {
        Log.d(TAG, msg);
    }

    private static void loge(String msg, Throwable tr) {
        Log.e(TAG, msg, tr);
    }
}
