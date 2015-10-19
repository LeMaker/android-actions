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

class DMPkg0Notification {
    private static final String TAG = "DMPkg0Notification";
    private static final boolean DBG = DMClientService.DBG;

    private int mUIMode;

    private int mInitiator;

    private int mSessionID;

    private String mServerID;

    private boolean mAuthFlag;

    public DMPkg0Notification() {
        mSessionID = 0;
        mUIMode = 0;
        mInitiator = 0;
        mAuthFlag = false;
        mServerID = null;
    }

    public int getSessionID() {
        return mSessionID;
    }

    public int getUIMode() {
        return mUIMode;
    }

    public int getInitiator() {
        return mInitiator;
    }

    public boolean getAuthFlag() {
        return mAuthFlag;
    }

    public String getServerID() {
        return mServerID;
    }

    /**
     * Set the session ID.
     * Called from JNI code.
     *
     * @param sessionID
     */
    public void setSessionID(int sessionID) {
        if (DBG) logd("setSessionID: " + sessionID);
        mSessionID = sessionID;
    }

    /**
     * Set the UI mode.
     * Called from JNI code.
     *
     * @param uiMode
     */
    public void setUIMode(int uiMode) {
        if (DBG) logd("setUIMode: " + uiMode);
        mUIMode = uiMode;
    }

    /**
     * Set the initiator.
     * Called from JNI code.
     *
     * @param initiator
     */
    public void setInitiator(int initiator) {
        if (DBG) logd("setInitiator: " + initiator);
        mInitiator = initiator;
    }

    /**
     * Set the auth flag.
     * Called from JNI code.
     *
     * @param authFlag
     */
    public void setAuthFlag(int authFlag) {
        if (DBG) logd("setAuthFlag: " + authFlag);
        mAuthFlag = authFlag == 1;
    }

    /**
     * Set the server ID.
     * Called from JNI code.
     *
     * @param serverID
     */
    public void setServerID(String serverID) {
        if (DBG) logd("setServerID: \"" + serverID + '"');
        mServerID = serverID;
    }

    private static void logd(String msg) {
        Log.d(TAG, msg);
    }
}
