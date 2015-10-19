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

package com.android.omadm.plugin.diagmon;

import android.content.Context;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.android.omadm.plugin.DmtBasePlugin;
import com.android.omadm.plugin.DmtData;
import com.android.omadm.plugin.DmtPathUtils;
import com.android.omadm.plugin.DmtPluginNode;
import com.android.omadm.plugin.ErrorCodes;

import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;

public class DiagmonPlugin extends DmtBasePlugin {
    private static final String TAG = "DiagmonPlugin";
    private static final boolean DBG = true;

    private String mRootPath;

    private Context mContext;

    private Map<String, DmtData> nodeValues;

    private boolean mHomeRoam = false;

    private RFPhoneStateListener mRFPhoneStateListener;

    public TelephonyManager mTelephonyManager;

    public static final String DIAGMON_ROOT = DmtPathUtils.ROOTNODE;

    public static final String DIAGMON_PATH = "./ManagedObjects/DiagMon";


    public DiagmonPlugin(Context ctx) {
        mContext = ctx;

        mTelephonyManager = (TelephonyManager) mContext
                .getSystemService(Context.TELEPHONY_SERVICE);
        mRFPhoneStateListener = new RFPhoneStateListener();
        mTelephonyManager.listen(mRFPhoneStateListener,
                PhoneStateListener.LISTEN_SIGNAL_STRENGTHS);
        mTelephonyManager.listen(mRFPhoneStateListener,
                PhoneStateListener.LISTEN_SERVICE_STATE);
    }

    @SuppressWarnings("unchecked")
    public boolean init(String rootPath, Map parameters) {
        if (DiagmonPlugin.DBG) {
            Log.i(TAG, "Enter DiagmonPlugin.init");
        }
        mRootPath = rootPath;
        nodeValues = new HashMap<String, DmtData>();
        String tmpPath = DIAGMON_ROOT;
        DmtData tmpNodeData = new DmtData("RF", DmtData.NODE);
        nodeValues.put(tmpPath, tmpNodeData);
        tmpPath = "RF";
        tmpNodeData = new DmtData("CurrentSystem|HomeRoam", DmtData.NODE);
        nodeValues.put(tmpPath, tmpNodeData);
        tmpPath = "RF/CurrentSystem";
        tmpNodeData = new DmtData("Data|Voice", DmtData.NODE);
        nodeValues.put(tmpPath, tmpNodeData);
        tmpPath = "RF/CurrentSystem/Data";
        tmpNodeData = new DmtData(null, DmtData.STRING);
        nodeValues.put(tmpPath, tmpNodeData);
        tmpPath = "RF/CurrentSystem/Voice";
        tmpNodeData = new DmtData(null, DmtData.STRING);
        nodeValues.put(tmpPath, tmpNodeData);
        tmpPath = "RF/HomeRoam";
        tmpNodeData = new DmtData(null, DmtData.STRING);
        nodeValues.put(tmpPath, tmpNodeData);

        return true;
    }

    //@Override
    public DmtPluginNode getNode(String path) {
        DmtPluginNode node = new DmtPluginNode("", new DmtData("abc"));
        setOperationResult(ErrorCodes.SYNCML_DM_SUCCESS);
        return node;
    }

    //@Override
    public DmtData getNodeValue(String path) {
        if (DiagmonPlugin.DBG) {
            Log.i(TAG, "rootPath=" + mRootPath);
        }
        if (DiagmonPlugin.DBG) {
            Log.i(TAG, "path=" + path);
        }
        //mContext.enforceCallingPermission("com.android.permission.READ_OMADM_SETTINGS", "Insufficient Permissions");

        DmtData data = null;

        if (path.equals("./ManagedObjects/DiagMon/RF/CurrentSystem/Data")) {
            if (DiagmonPlugin.DBG) {
                Log.d(TAG, "Get on CurrentSystem Value");
            }
            String mCurrentSystem;
            try {
                mCurrentSystem = getCurrentSystem();
                if (DiagmonPlugin.DBG) {
                    Log.d(TAG, "Current System Value: " + mCurrentSystem);
                }
            } catch (Exception e) {
                mCurrentSystem = "Unknown";
                if (DiagmonPlugin.DBG) {
                    Log.d(TAG, "Get on Current System Value failed");
                }
                e.printStackTrace();
            }
            data = new DmtData(mCurrentSystem);
            setOperationResult(ErrorCodes.SYNCML_DM_SUCCESS);
        } else if (path.equals("./ManagedObjects/DiagMon/RF/CurrentSystem/Voice")) {
            if (DiagmonPlugin.DBG) {
                Log.d(TAG, "Get on CurrentSystem Voice Value");
            }
            String mCurrentSystemVoice;
            if (isVoiceCapable(mContext)) {
                mCurrentSystemVoice = "1xRTT"; //As per VZW (36953) requirement value is 1xRTT
            } else {
                mCurrentSystemVoice = "Voice not available";
            }
            if (DiagmonPlugin.DBG) {
                Log.d(TAG, "Current System Voice Value: " + mCurrentSystemVoice);
            }
            data = new DmtData(mCurrentSystemVoice);
            setOperationResult(ErrorCodes.SYNCML_DM_SUCCESS);
        } else if (path.equals("./ManagedObjects/DiagMon/RF/HomeRoam")) {
            if (DiagmonPlugin.DBG) {
                Log.d(TAG, "Get on HomeRoam Value");
            }
            String sHomeRoam;
            try {
                sHomeRoam = getHomeRoam();
                if (DiagmonPlugin.DBG) {
                    Log.d(TAG, "HomeRoam Value: " + sHomeRoam);
                }
            } catch (Exception e) {
                sHomeRoam = "Unknown";
                if (DiagmonPlugin.DBG) {
                    Log.d(TAG, "Get on HomeRoam Value failed");
                }
                e.printStackTrace();
            }
            data = new DmtData(sHomeRoam);
            setOperationResult(ErrorCodes.SYNCML_DM_SUCCESS);
        } else {
            setOperationResult(ErrorCodes.SYNCML_DM_FAIL);
        }

        return data;
    }

    @SuppressWarnings("unchecked")
    //@Override
    public Map getNodes(String rootPath) {
        if (DiagmonPlugin.DBG) {
            Log.d(TAG, "Enter DiagmonPlugin::getNodes()");
        }
        Map<String, DmtPluginNode> hMap = new HashMap<String, DmtPluginNode>();
        DmtPluginNode node;

        Map<String, DmtData> nodes = getNodeMap(DmtPathUtils.toRelativePath(DIAGMON_PATH,
                mRootPath));

        for (String diagPath : nodes.keySet()) {
            DmtData data = nodes.get(diagPath);
            if (diagPath.equals(DIAGMON_ROOT)) {
                diagPath = "";
            }
            if (DiagmonPlugin.DBG) {
                Log.i(TAG, "put node = '" + diagPath + "'");
            }
            node = new DmtPluginNode(diagPath, data);
            hMap.put(diagPath, node);
        }

        if (DiagmonPlugin.DBG) {
            Log.i(TAG, "created the nodes.");
        }
        if (DiagmonPlugin.DBG) {
            Log.d(TAG, "Leave DiagmonPlugin::getNodes()");
        }
        return hMap;
    }

    public Map<String, DmtData> getNodeMap(String rootPath) {

        Map<String, DmtData> nodeMap = new HashMap<String, DmtData>();
        if (rootPath.equals(DIAGMON_ROOT)) {
            nodeMap.putAll(nodeValues);
        } else {
            for (String key : nodeValues.keySet()) {
                if (key.startsWith(rootPath)) {
                    if ((key.substring(rootPath.length(), (rootPath.length() + 1))).equals("/")) {
                        nodeMap.put(key, nodeValues.get(key));
                    }
                }
            }
        }
        return nodeMap;
    }

    public boolean release() {
        mTelephonyManager.listen(mRFPhoneStateListener, PhoneStateListener.LISTEN_NONE);
        return true;
    }

    private final class RFPhoneStateListener extends PhoneStateListener {

        @Override
        public void onServiceStateChanged(ServiceState serviceState) {
            mHomeRoam = serviceState.getRoaming();
            if (DiagmonPlugin.DBG) {
                Log.v(TAG, "Service State intent received: " + mHomeRoam);
            }
        }
    }

    private String getCurrentSystem() {
        int mNetworkType = mTelephonyManager.getNetworkType();
        if (DiagmonPlugin.DBG) {
            Log.d(TAG, "Network Type value : " + mNetworkType);
        }
        if (mNetworkType == TelephonyManager.NETWORK_TYPE_UNKNOWN) {
            return ("Unknown");
        } else if (mNetworkType == TelephonyManager.NETWORK_TYPE_1xRTT) {
            return ("1xRTT");
        } else if (mNetworkType == TelephonyManager.NETWORK_TYPE_EVDO_0
                || mNetworkType == TelephonyManager.NETWORK_TYPE_EVDO_A) {
            return ("EVDO");
        } else if (mNetworkType == TelephonyManager.NETWORK_TYPE_LTE) {
            return ("LTE");
        } else if (mNetworkType == TelephonyManager.NETWORK_TYPE_EHRPD) {
            return ("eHRPD");
        } else {
            return ("Unknown");
        }
    }

    private String getHomeRoam() {
        if (mHomeRoam) {
            return ("Roam");
        } else {
            return ("Home");
        }
    }

    private static boolean isVoiceCapable(Context context) {
        TelephonyManager telephonyManager =
                (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
        if (telephonyManager == null) {
            return false;
        }
        try {
            Class partypes[] = new Class[0];
            Method sIsVoiceCapable = TelephonyManager.class.getMethod("isVoiceCapable", partypes);
            Object arglist[] = new Object[0];
            Object retobj = sIsVoiceCapable.invoke(telephonyManager, arglist);
            return (Boolean) retobj;
        } catch (java.lang.reflect.InvocationTargetException e) {
            // Failure, must be another device.
            // Assume that it is voice capable.
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            // Failure, must be an other device.
            // Assume that it is voice capable.
            e.printStackTrace();
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        }
        return true;
    }
}
