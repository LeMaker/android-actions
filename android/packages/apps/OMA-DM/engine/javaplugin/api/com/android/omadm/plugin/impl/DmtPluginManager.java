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

package com.android.omadm.plugin.impl;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.ResolveInfo;
import android.os.IBinder;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;

import com.android.omadm.plugin.DmtData;
import com.android.omadm.plugin.DmtException;
import com.android.omadm.plugin.DmtPluginNode;
import com.android.omadm.plugin.ErrorCodes;
import com.android.omadm.plugin.IDmtPlugin;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * This class does not manage DMT plugins. It is a proxy between native plugin and java plugins.
 * An instance of the class is created for each plugin.
 */
public final class DmtPluginManager {

    private static final String TAG = "DM_DmtPluginManager";
    private static final boolean DBG = false;

    private static Context sContext;

    /* Parameters of the plug-in */
    private String mPath;
    private String mUid;
    private String mServerID;   // FIXME: mServerID is never set, remove?
    private Map<String, String> mParameters;

    private final class DmtServiceConnection implements ServiceConnection {

        DmtServiceConnection() {
        }

        @Override
        public synchronized void onServiceConnected(ComponentName className, IBinder service) {
            logd("Enter onServiceConnected...");
            logd("Class:" + className + " service:" + service);

            mPluginConnection = IDmtPlugin.Stub.asInterface(service);

            try {
                notifyAll();
            } catch (Exception e) {
                loge("onServiceConnected(): exception", e);
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            logd("Plug-in service disconnected. className:" + className);
            mPluginConnection = null;
        }

        public synchronized void waitForConnection() {
            try {
                Intent intent = new Intent(mUid);
                intent.putExtra("rootPath", mPath);
                boolean currentPackageMatch = false;
                List<ResolveInfo> intentServices = sContext.getPackageManager()
                        .queryIntentServices(intent, 0);
                if (intentServices != null) {
                    for (ResolveInfo resolveInfo : intentServices) {
                        logd("getPackageName is: " + sContext.getPackageName());
                        logd("resolveInfo.serviceInfo.packageName is: "
                                + resolveInfo.serviceInfo.packageName);
                        if(resolveInfo.serviceInfo.packageName.equals(sContext.getPackageName())) {
                            try {
                                Class.forName(mUid);
                            } catch (ClassNotFoundException e1) {
                                loge("ClassNotFoundException in waitForConnection", e1);
                                currentPackageMatch = true;
                            }
                        }
                    }
                }

                if (currentPackageMatch) {
                    return;
                }

                if (DBG) logd("Calling bindService: uid=\"" + mUid + "\" path=\"" + mPath + '"');
                sContext.bindService(intent, mConnector,Context.BIND_AUTO_CREATE);

                wait(10000);        // FIXME: wait not in loop!
                if (DBG) logd("Waiting is finished");
            } catch (Exception e) {
                loge("exception in waitForConnection", e);
            }
        }
    }

    private final DmtServiceConnection mConnector = new DmtServiceConnection();

    private IDmtPlugin mPluginConnection;

    public DmtPluginManager() {
        logd("DmtPluginManager.java constructor...");
    }

    public static void setContext(Context context) {
        if (DBG) logd("Enter setContext(" + context + ')');

        if (context == null) {
            logd("Context is null!");
            return;
        }

        Context appContext = context.getApplicationContext();

        if (DBG) logd("app context is: " + appContext);

        sContext = appContext;
    }

    /**
     * Initialize Java plugin. Called from JNI in:
     *  engine/javaplugin/nativelib/src/DmtJavaPluginManager.cc
     *
     * @param path the root path of the plug-in.
     * @param parameters initial parameters of the plug-in, as an array of strings.
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} on success, error code on failure.
     */
    public boolean initJavaPlugin(String path, String[] parameters) {
        if (DBG) logd("Enter initJavaPlugin: path = " + path);
        if (DBG) logd("parameters are: " + Arrays.toString(parameters));

        if (TextUtils.isEmpty(path)) {
            loge("Invalid path!....");
            return false;
        }

        if (mPath != null) {
            loge("Plugin already loaded!....");
            return false;
        }

        if (DBG) logd("Parameters count is " + parameters.length);
        if (parameters.length % 2 != 0) {
            loge("Parameter count is not right.");
            return false;
        }

        // FIXME: replace HashTable with HashMap
        Map<String, String> params = new HashMap<String, String>();
        for (int i = 0; i < parameters.length; i += 2 ) {
            params.put(parameters[i], parameters[i + 1]);

            if ("_uid".equals(parameters[i])) {
                mUid = parameters[i + 1];
            }
        }

        if (TextUtils.isEmpty(mUid)) {
            loge("uid is empty...");
            return false;
        }

        mParameters = params;
        mPath       = path;

        return bindPluginService();
    }

    /**
     * Performs "Execute" command on a plug-in node.
     * Called from JNI.
     *
     * @param args exec plug-in arguments.
     * @param correlator correlator.
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} on success, error code on failure.
     */
    public int executeNode(String args, String correlator) {
        if (DBG) logd("Enter executeNode(\"" + args + "\", \"" + correlator + "\")");

        if (mPluginConnection == null) {
            loge("There is no bound plug-in");
            return ErrorCodes.SYNCML_DM_FAIL;
        }

        try {
            return mPluginConnection.exec(mPath, args, correlator);
        } catch (Exception e) {
            loge("Exception in executeNode", e);
            return ErrorCodes.SYNCML_DM_FAIL;
        }
    }

    /**
     * Performs commit operation under current DMT.
     * Called from JNI.
     *
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} on success, error code on failure.
     */
    public int commit() {
        loge("Enter commit... " + mPath);

        if (mPluginConnection == null) {
            loge("There is no bound plug-in");
            return ErrorCodes.SYNCML_DM_FAIL;
        }

        try {
            return mPluginConnection.commit();
        } catch (Exception e) {
            loge("Exception in commit", e);
            return ErrorCodes.SYNCML_DM_FAIL;
        }
    }

    /**
     * Sets Server ID of the plug-in.
     * Called from JNI.
     *
     * @param serverID service ID.
     */
    public void setServerID(String serverID) {
        if (DBG) logd("Enter setServerID(\"" + serverID + "\")");

        if (mPluginConnection == null) {
            loge("There is no bound plug-in");
            return;
        }

        try {
            mPluginConnection.setServerID(serverID);
        } catch (Exception e) {
            loge("Exception in setServerID", e);
       }
    }

    /**
     * Creates an interior node in the tree for the specified path.
     * Called from JNI.
     *
     * @param path full path to the node.
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} on success, error code on failure.
     */
    public int createInteriorNode(String path) {
        if (DBG) logd("Enter createInteriorNode(\"" + path + "\")");

        if (mPluginConnection == null) {
            loge("There is no bound plug-in");
            return ErrorCodes.SYNCML_DM_FAIL;
        }

        try {
            return mPluginConnection.createInteriorNode(getFullPath(path));
        } catch (Exception e) {
            loge("Exception in createInteriorNode", e);
            return ErrorCodes.SYNCML_DM_FAIL;
        }
    }

    /**
     * Creates a leaf node in the tree by given path.
     * Called from JNI.
     *
     * @param path full path to the node.
     * @param type type of node, defined as constants in {@link DmtData}.
     * @param value the new value to set.
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} on success, error code on failure.
     */
    public int createLeafNode(String path, int type, String value) {
        if (DBG) logd("Enter createLeafNode(\"" + path + "\", " + type + ", \"" + value + "\")");

        if (mPluginConnection == null) {
            loge("There is no bound plug-in");
            return ErrorCodes.SYNCML_DM_FAIL;
        }

        if (type < DmtData.NULL || type >= DmtData.NODE) {
            loge("Invalid data type: " + type);
            return ErrorCodes.SYNCML_DM_FAIL;
        }

        DmtData data = new DmtData(value, type);

        try {
            return mPluginConnection.createLeafNode(getFullPath(path), data);
        } catch (Exception e) {
            loge("Exception in createLeafNode", e);
            return ErrorCodes.SYNCML_DM_FAIL;
        }
    }

    /**
     * Renames a node.
     * Called from JNI.
     *
     * NOTE: This is an optional command of the OMA DM protocol.
     *       Currently java plug-ins do not support the command.
     *
     * @param path full path to node to be renamed.
     * @param newNodeName new node name.
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} on success, error code on failure.
     */
    public int renameNode(String path, String newNodeName) {
        if (DBG) logd("Enter renameNode(\"" + path + "\", \"" + newNodeName + "\")");

        if (mPluginConnection == null) {
            loge("There is no bound plug-in");
            return ErrorCodes.SYNCML_DM_FAIL;
        }

        if (TextUtils.isEmpty(newNodeName)) {
            loge("Invalid new node name!");
            return ErrorCodes.SYNCML_DM_FAIL;
        }

        try {
            return mPluginConnection.renameNode(getFullPath(path), newNodeName);
        } catch (Exception e) {
            loge("Exception in renameNode", e);
            return ErrorCodes.SYNCML_DM_FAIL;
        }
    }

    /**
     * Deletes a node with the specified path.
     * Called from JNI.
     *
     * @param path full path to the node.
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} on success, error code on failure.
     */
    public int deleteNode(String path) {
        if (DBG) logd("Enter deleteNode(\"" + path + "\")");

        if (mPluginConnection == null) {
            loge("There is no bound plug-in");
            return ErrorCodes.SYNCML_DM_FAIL;
        }

        try {
            return mPluginConnection.deleteNode(getFullPath(path));
        } catch (Exception e) {
            loge("Exception in deleteNode", e);
            return ErrorCodes.SYNCML_DM_FAIL;
        }
    }

    /**
     * Set new value for the specified node.
     * Called from JNI.
     *
     * @param path full path to the node.
     * @param type type of node, defined as constants in {@link DmtData}.
     * @param value the new value to set.
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} on success, error code on failure.
     */
    public int setNodeValue(String path, int type, String value) {
        if (DBG) logd("Enter setNodeValue(\"" + path + "\", " + type + ", \"" + value + "\")");

        if (mPluginConnection == null) {
            loge("There is no bound plug-in");
            return ErrorCodes.SYNCML_DM_FAIL;
        }

        if (type < DmtData.NULL || type >= DmtData.NODE) {
            loge("Invalid data type: " + type);
            return ErrorCodes.SYNCML_DM_FAIL;
        }

        DmtData data = new DmtData(value, type);

        try {
            logd("Update leaf node: path = " + path + ", data = " + data.getString());
            return mPluginConnection.updateLeafNode(getFullPath(path), data);
        } catch (Exception e) {
            loge("Exception in setNodeValue", e);
            return ErrorCodes.SYNCML_DM_FAIL;
        }
    }

    /**
     * Returns value of leaf node for the specified path.
     * Called from JNI.
     *
     * @param path path to the leaf node.
     * @return String array where the first element is value type and the second is the value.
     * @throws DmtException in case of error.
     */
    public String[] getNodeValue(String path) throws DmtException {
        if (DBG) logd("Enter getNodeValue(\"" + path + "\")");

        if (mPluginConnection == null) {
            loge("There is no bound plug-in");
            throw new DmtException("There is no bound plug-in");
        }

        DmtData data;
        try {
            data = mPluginConnection.getNodeValue(getFullPath(path));
        } catch (Exception e) {
            loge("Exception in getNodeValue", e);
            throw new DmtException(e.getMessage());
        }

        if (data == null) {
            int retcode;
            try {
                retcode = mPluginConnection.getOperationResult();
            } catch (Exception e) {
                loge("Exception in getNodeValue", e);
                throw new DmtException(e.getMessage());
            }

            if (retcode == ErrorCodes.SYNCML_DM_SUCCESS) {
                loge("Invalid plug-in implementation!");
                throw new DmtException("Invalid plug-in implementation!");
            }
            /*
             * Added this block to return Error code to DM Engine since
             * throwing an exception is causing a VM error and aborting
             * the app. 
             */
            else if (retcode == ErrorCodes.SYNCML_DM_UNSUPPORTED_OPERATION) {
                loge("Get feature not implemented on this node");
                String[] errString = new String[1];
                errString[0] = Integer.toString(retcode);
                return errString;
            }
            
            else {
                loge("Error occurred while doing a get on this node");
                String[] errString = new String[1];
                errString[0] = Integer.toString(retcode);
                return errString;
            }

            //throw new DmtException(retcode, "Value is not set");
        }

        int dataType = data.getType();

        switch (dataType) {
            case DmtData.NULL:
            case DmtData.STRING:
            case DmtData.INT:
            case DmtData.BOOL:
            case DmtData.BIN:
            case DmtData.DATE:
            case DmtData.TIME:
            case DmtData.FLOAT:
                String value = data.getString();
                String[] resStrArr = new String[2];
                resStrArr[0] = Integer.toString(dataType);
                resStrArr[1] = TextUtils.isEmpty(value) ? "" : value;
                return resStrArr;

            case DmtData.NODE:
                throw new DmtException("Operation not allowed for interior node!");

            default:
                throw new DmtException("Invalid node type");
        }
    }

    /**
     * Gets a set of nodes for the specified path.
     * Called from JNI.
     *
     * If the plug-in cannot return the set, null value shall be returned by
     * the method and DM engine will use the getOperationResult() method
     * to get result of the operation.
     *
     * @return a String array containing triples of (key, type, value) as Strings
     * @throws DmtException on any exception
     */
    public String[] getNodes() throws DmtException {
        if (DBG) logd("Enter getNodes...");

        if (mPluginConnection == null) {
            loge("There is no bound plug-in");
            throw new DmtException("There is no bound plug-in");
        }

        Map<String, DmtPluginNode> pluginNodes;
        try {
            pluginNodes = (Map<String, DmtPluginNode>) mPluginConnection.getNodes(mPath);
        } catch (RemoteException e) {
            loge("RemoteException in getNodes", e);
            throw new DmtException(e.getMessage());
        }

        if (pluginNodes == null) {
            int retcode;
            try {
                retcode = mPluginConnection.getOperationResult();
            } catch (Exception e) {
                loge("Exception in getNodes", e);
                throw new DmtException(e.getMessage());
            }

            if (retcode == ErrorCodes.SYNCML_DM_SUCCESS) {
                loge("Invalid plug-in implementation!");
                throw new DmtException("Invalid plug-in implementation!");
            }

            throw new DmtException(retcode, "Value is not set");
        }

        if (pluginNodes.isEmpty()) {
            // FIXME: zero-length array constructed
            return new String[0];
        }

        if (DBG) logd("Data plugin has " + pluginNodes.size() + " nodes.");
        String[] resStrArr = new String[pluginNodes.size() * 3];

        int i = 0;
        for (String key : pluginNodes.keySet()) {
            if (DBG) logd("No." + i + " : " + key);
            DmtPluginNode tmpNode = pluginNodes.get(key);
            if (tmpNode == null) {
                throw new DmtException("Invalid map of all nodes");
            }
            resStrArr[i] = getRelativePath(key);
            resStrArr[i + 1] = Integer.toString(tmpNode.getType());
            switch (tmpNode.getType()) {
                case DmtData.NODE:
                    StringBuilder tmpSB = new StringBuilder("");
                    DmtData data = tmpNode.getValue();
                    if (data != null) {
                        Map<String, DmtData> childNodes = data.getChildNodeMap();
                        for (String subNodeName : childNodes.keySet()) {
                            if (TextUtils.isEmpty(subNodeName)) {
                                loge("invalid interior node value for " + subNodeName + "!!!");
                                throw new DmtException("Invalid interior node value");
                            } else {
                                tmpSB.append(subNodeName).append('\n');
                            }
                        }
                    }
                    resStrArr[i + 2] = tmpSB.toString();
                    break;

                case DmtData.NULL:
                case DmtData.STRING:
                case DmtData.INT:
                case DmtData.BOOL:
                case DmtData.BIN:
                case DmtData.DATE:
                case DmtData.TIME:
                case DmtData.FLOAT:
                    resStrArr[i + 2] = "";
                    break;

                default:
                    loge("invalid node type " + tmpNode.getType() + "!!!!");
                    throw new DmtException("Invalid node type");
            }
            i += 3;
        }
        return resStrArr;
    }

    public void release() {
        if (DBG) logd("Enter release... " + mPath);

        if (mPluginConnection == null) {
            return;
        }

        try {
            mPluginConnection.release();
        } catch (Exception e) {
            loge("exception releasing plugin", e);
        }

        mPluginConnection = null;

        try {
            sContext.unbindService(mConnector);
        } catch (Exception e) {
            loge("unbindService exception in release", e);
        }
    }

    private boolean bindPluginService() {
        if (DBG) logd("Enter bindPluginService...");

        if (sContext == null) {
            loge("Undefined context!");
            return false;
        }

        if (mPluginConnection != null) {
            loge("Already bound!");
            return true;
        }

        try {
            logd("uid      = " + mUid);
            logd("rootPath = " + mPath);

            mConnector.waitForConnection();

            if (mPluginConnection == null) {
                loge("Impossible to bind to plug-in!...");
                sContext.unbindService(mConnector);
                return false;
            }

            if (DBG) logd("context  = " + sContext.toString());

            // FIXME: mServerID is never set, remove this?
            if (mServerID != null) {
                mPluginConnection.setServerID(mServerID);
            }

            return mPluginConnection.init(mPath, mParameters);
        } catch (Exception e) {
            loge("bindPluginService: Unable to get service " + mUid, e);
            return false;
        }
    }

    private String getRelativePath(String path) {
        if (TextUtils.isEmpty(path) || path.equals(mPath)) {
            return "";
        }

        if (path.startsWith(mPath + '/')) {
            return path.substring(mPath.length() + 1);
        }

        return path;
    }

    private String getFullPath(String path) {
        if (TextUtils.isEmpty(path)) {
            return mPath;
        }

        if (path.startsWith("./")) {
            return path;
        }

        return mPath + '/' + path;
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
