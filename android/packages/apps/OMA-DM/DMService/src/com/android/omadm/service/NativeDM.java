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

final class NativeDM {
    private static final String TAG = "NativeDM";

    private NativeDM() {}

    static {
        try {
            logd("loading libdmengine.so");
            System.loadLibrary("dmengine");
            logd("loaded libdmengine.so");
        } catch (UnsatisfiedLinkError ule) {
            loge("error: Could not load libdmengine.so", ule);
            throw ule;
        }
    }

    /**
     * Initialize the native DM library.
     * @return either {@link DMResult#SYNCML_DM_SUCCESS} or {@link DMResult#SYNCML_DM_FAIL}
     */
    public static native int initialize();

    /**
     * Initialize the native DM library.
     * @return either {@link DMResult#SYNCML_DM_SUCCESS} or {@link DMResult#SYNCML_DM_FAIL}
     */
    public static native int destroy();

    /**
     * Start a DM client session.
     * @param serverID the server ID to use
     * @param sessionAgent the {@link DMSession} object to use
     * @return either {@link DMResult#SYNCML_DM_SUCCESS} or a {@link DMResult} error code
     */
    public static native int startClientSession(String serverID, DMSession sessionAgent);

    /**
     * Start a FOTA client session. The alert URI is {@code "./DevDetail/Ext/SystemUpdate"}.
     * @param serverID the server ID to use
     * @param alertStr the alert string to use
     * @param sessionAgent the {@link DMSession} object to use
     * @return either {@link DMResult#SYNCML_DM_SUCCESS} or a {@link DMResult} error code
     */
    public static native int startFotaClientSession(String serverID, String alertStr,
            DMSession sessionAgent);

    /**
     * Start a FOTA server session.
     * @param serverID the server ID to use
     * @param sessionID the session ID to use
     * @param sessionAgent the {@link DMSession} object to use
     * @return either {@link DMResult#SYNCML_DM_SUCCESS} or a {@link DMResult} error code
     */
    public static native int startFotaServerSession(String serverID, int sessionID,
            DMSession sessionAgent);

    /**
     * Start a FOTA notification session.
     * @param result
     * @param pkgURI
     * @param alertType
     * @param serverID
     * @param correlator
     * @param sessionAgent
     * @return either {@link DMResult#SYNCML_DM_SUCCESS} or a {@link DMResult} error code
     */
    public static native int startFotaNotifySession(String result, String pkgURI,
            String alertType, String serverID, String correlator, DMSession sessionAgent);

    /**
     *
     * @return
     */
    public static native int cancelSession();

    /**
     *
     * @param pkg0
     * @param dmtNotification
     * @return
     */
    public static native int parsePkg0(byte[] pkg0, DMPkg0Notification dmtNotification);

    /**
     *
     * @param node
     * @return
     */
    public static native int createInterior(String node);

    /**
     *
     * @param node
     * @param value
     * @return
     */
    public static native int createLeaf(String node, String value);

    /**
     *
     * @param node
     * @param value
     * @return
     */
    public static native int createLeaf(String node, byte[] value);

    /**
     *
     * @param node
     * @return
     */
    public static native int deleteNode(String node);

    /**
     *
     * @param node
     * @param value
     * @return
     */
    public static native String setStringNode(String node, String value);

    /**
     *
     * @param node
     * @param value
     * @return
     */
    public static native String createLeafInteger(String node, String value);

    /**
     *
     * @param node
     * @return
     */
    public static native String getNodeInfo(String node);

    /**
     *
     * @param node
     * @param data
     * @return
     */
    public static native String executePlugin(String node, String data);

    /**
     *
     * @param node
     * @return
     */
    public static native String dumpTree(String node);

    /**
     *
     * @param wbxmlBuf
     * @return
     */
    public static native byte[] nativeWbxmlToXml(byte[] wbxmlBuf);

    /**
     *
     * @param serverId
     * @param fileName
     * @param bBinary
     * @param retCode
     * @param session
     * @return
     */
    public static native byte[] processScript(String serverId, String fileName, boolean bBinary,
            int retCode, DMSession session);

    /**
     *
     * @param msgData
     * @param bIsWbxml
     * @param serverId
     * @return
     */
    public static native int processBootstrapScript(byte[] msgData, boolean bIsWbxml,
            String serverId);

    /**
     *
     * @param msgData
     * @param bIsWbxml
     * @return
     */
    public static native String parseBootstrapServerId(byte[] msgData, boolean bIsWbxml);

    /**
     * Get the node type for a DM node.
     * @param path the OMA DM path to use
     * @return the DM node type
     */
    public static native int getNodeType(String path);

    /**
     * Get the node value for a DM node.
     * @param path the OMA DM path to use
     * @return the DM node value, as a String
     */
    public static native String getNodeValue(String path);

    private static void logd(String msg) {
        Log.d(TAG, msg);
    }

    private static void loge(String msg, Throwable tr) {
        Log.e(TAG, msg, tr);
    }
}
