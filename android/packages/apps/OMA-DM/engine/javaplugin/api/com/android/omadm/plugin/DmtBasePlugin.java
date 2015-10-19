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

package com.android.omadm.plugin;

import android.os.RemoteException;

import java.util.Map;

public abstract class DmtBasePlugin extends IDmtPlugin.Stub {

    private String mServerId;
    protected static final boolean DEBUG = false;

    private int mOperationResult;   // = ErrorCodes.SYNCML_DM_SUCCESS;

    protected int setOperationResult(int result) {
        mOperationResult = result;
        return result;
    }

    @Override
    public int getOperationResult() {
        return mOperationResult;
    }

    @Override
    public void setServerID(String serverID) {
        mServerId = serverID;
    }

    @Override
    public String getServerID() {
        return mServerId;
    }

    @Override
    public DmtPluginNode getNode(String arg0) throws RemoteException {
        setOperationResult(ErrorCodes.SYNCML_DM_UNSUPPORTED_OPERATION);
        return null;
    }

    @Override
    public Map getNodes(String arg0) throws RemoteException {
        setOperationResult(ErrorCodes.SYNCML_DM_UNSUPPORTED_OPERATION);
        return null;
    }

    @Override
    public DmtData getNodeValue(String path) throws RemoteException {
        setOperationResult(ErrorCodes.SYNCML_DM_UNSUPPORTED_OPERATION);
        return null;
    }

    @Override
    public int createInteriorNode(String arg0) throws RemoteException {
        return setOperationResult(ErrorCodes.SYNCML_DM_UNSUPPORTED_OPERATION);
    }

    @Override
    public int createLeafNode(String arg0, DmtData arg1) throws RemoteException {
        return setOperationResult(ErrorCodes.SYNCML_DM_UNSUPPORTED_OPERATION);
    }

    @Override
    public int updateLeafNode(String path, DmtData newValue) throws RemoteException {
        return setOperationResult(ErrorCodes.SYNCML_DM_UNSUPPORTED_OPERATION);
    }

    @Override
    public int deleteNode(String arg0) throws RemoteException {
        return setOperationResult(ErrorCodes.SYNCML_DM_UNSUPPORTED_OPERATION);
    }

    @Override
    public int clone(String arg0, String arg1) throws RemoteException {
        return setOperationResult(ErrorCodes.SYNCML_DM_UNSUPPORTED_OPERATION);
    }

    @Override
    public int renameNode(String arg0, String arg1) throws RemoteException {
        return setOperationResult(ErrorCodes.SYNCML_DM_UNSUPPORTED_OPERATION);
    }

    @Override
    public int exec(String path, String args, String correlator) throws RemoteException {
        return setOperationResult(ErrorCodes.SYNCML_DM_UNSUPPORTED_OPERATION);
    }

    @Override
    public int commit() throws RemoteException {
        return setOperationResult(ErrorCodes.SYNCML_DM_UNSUPPORTED_OPERATION);
    }

    @Override
    public String getServerPW(String aiServerPW) {
        return null;
    }

    @Override
    public String getClientPW(String aiClientPW) {
        return null;
    }

    @Override
    public String getUsername(String aiUsername) {
        return null;
    }
}
