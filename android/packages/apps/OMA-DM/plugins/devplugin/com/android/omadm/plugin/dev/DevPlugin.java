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

package com.android.omadm.plugin.dev;

import android.content.Context;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;

import com.android.omadm.plugin.DmtBasePlugin;
import com.android.omadm.plugin.DmtData;
import com.android.omadm.plugin.DmtException;
import com.android.omadm.plugin.DmtPluginNode;
import com.android.omadm.plugin.ErrorCodes;
import com.android.omadm.service.DMSettingsHelper;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

public class DevPlugin extends DmtBasePlugin {

    static final String TAG = "DM_DevPlugin";

    private String mRootPath;

    private Context mContext;

    public static final String DEV_DETAIL = "devdetail";

    public static final String WIFI_MAC_ADDR = "wifimacaddr";

    public static final String PRE_FW_VER = "prefwversion";

    public static final String LAST_UPD_TIME = "lastupdatetime";

//    public static final Uri DMFLEXS_CONTENT_URI = Uri
//            .parse("content://com.android.omadm.service/flexs");

    public DevPlugin(Context ctx) {
        mContext = ctx;
    }

    @Override
    @SuppressWarnings({"unchecked", "rawtypes"})
    public boolean init(String rootPath, Map parameters) {
        logd("Enter DevPlugin.init(\"" + rootPath + "\", " + parameters + "\")");
        mRootPath = rootPath;
        return true;
    }

    @Override
    public DmtPluginNode getNode(String path) {
        logd("getNode(\"" + path + "\")");
        DmtPluginNode node = new DmtPluginNode("", new DmtData("abc"));
        setOperationResult(node == null ?
                ErrorCodes.SYNCML_DM_FAIL :
                ErrorCodes.SYNCML_DM_SUCCESS);
        return node;
    }

    @Override
    public DmtData getNodeValue(String path) {
        logd("getNodeValue: rootPath=" + mRootPath + " path=" + path);

        //mContext.enforceCallingPermission("com.android.permission.READ_OMADM_SETTINGS", "Insufficient Permissions");
        DmtData data = null;

        if (path.equals("./DevDetail/SwV") || path.equals("./DevDetail/FwV")) {
            String SwV;
            try {
                if (!isSprint() && path.equals("./DevDetail/SwV")) {
                    SwV = "Android " + SystemProperties.get("ro.build.version.release");
                } else {
                    SwV = SystemProperties.get("ro.build.version.full");
                    if (null == SwV || SwV.equals("")) {
                        SwV = SystemProperties.get("ro.build.id", null) + "~"
                                + SystemProperties.get("ro.build.config.version", null) + "~"
                                + SystemProperties.get("gsm.version.baseband", null) + "~"
                                + SystemProperties.get("ro.gsm.flexversion", null);
                    }
                }
            } catch (RuntimeException e) {
                SwV = "Unknown";
            }
            data = new DmtData(SwV);
        } else if ("./DevDetail/HwV".equals(path)) {
            String HwV;
            try {
                HwV = SystemProperties.get("ro.hardware", "Unknown")
                        + "." + SystemProperties.get("ro.revision", "Unknown");
            } catch (RuntimeException e) {
                HwV = "Unknown";
            }
            logd("get ./DevDetail/HwV = " + HwV);
            data = new DmtData(HwV);
        } else if ("./DevDetail/DevTyp".equals(path)) {
            String DevTyp = getDeviceType();
            logd("get ./DevDetail/DevTyp = " + DevTyp);
            data = new DmtData(DevTyp);
        } else if ("./DevInfo/DevId".equals(path)) {
            TelephonyManager tm = (TelephonyManager) mContext
                    .getSystemService(Context.TELEPHONY_SERVICE);
            String simOperator = tm.getSimOperator();
            String imsi = tm.getSubscriberId();
            logd("simOperator: " + simOperator + " IMSI: " + imsi);
            /* Use MEID for sprint */
            if ("310120".equals(simOperator) || (imsi != null && imsi.startsWith("310120"))) {
                /* MEID is 14 digits. If IMEI is returned as DevId, MEID can be extracted by taking
                 * first 14 characters. This is not always true but should be the case for sprint */
                String strDevId = tm.getDeviceId();
                strDevId = strDevId.toUpperCase(Locale.US);
                logd("DeviceId from telemgr: " + strDevId);
                if (strDevId != null && strDevId.length() >= 14) {
                    strDevId = strDevId.substring(0, 14);
                    logd("MEID (from DeviceId): " + strDevId);
                    data = new DmtData("MEID:" + strDevId);
                } else {
                    loge("MEID cannot be extracted from DeviceId " + strDevId);
                }
            } else {
                String strDevId;
                if (isPhoneTypeLTE()) {
                    strDevId = tm.getImei();
                } else {
                    strDevId = tm.getDeviceId();
                }
                strDevId = strDevId.toUpperCase(Locale.US);
                logd("DevId from telemgr: " + strDevId);

                if (isPhoneTypeLTE()) {
                    data = new DmtData("IMEI:" + strDevId);
                } else {
                    data = new DmtData("MEID:" + strDevId.substring(0, 14));
                }
            }
        } else if (path.equals("./DevInfo/DmV")) {
            data = new DmtData("1.2");
        } else if (path.equals("./DevInfo/Lang")) {
            String strLang = readValueFromFile("Lang");
            logd("Language from shared file:" + strLang);
            if (strLang == null) {
                strLang = Locale.getDefault().toString();
                logd("Language from system is:" + strLang);
            }
            data = new DmtData(strLang);
        } else if (path.equals("./DevInfo/Man")) {
            String strMan = readValueFromFile("Man");
            logd("Manufacturer got from shared file:" + strMan);
            if (strMan == null) {
                strMan = SystemProperties.get("ro.product.manufacturer", "unknown");
                logd("Manufacturer from system properties:" + strMan);
            }
            data = new DmtData(strMan.toLowerCase(Locale.US));
        } else if (path.equals("./DevInfo/Mod")) {
            String strMod = readValueFromFile("Mod");
            logd("Mod got from shared file:" + strMod);
            if (strMod == null) {
                strMod = SystemProperties.get("ro.product.model", "generic");
                logd("Mod got from system properties:" + strMod);
            }
            data = new DmtData(strMod);
        } else if (path.equals("./DevDetail/Bearer/GSM")) {
            data = new DmtData("GSM_1900");
        } else if (path.equals("./DevDetail/Bearer/CDMA")) {
            data = new DmtData("CDMA_2000");
        } else if (path.equals("./DevDetail/Ext/SystemSettings/AllowUnknownSources")) {
            if (!isThisForATT()) {
                logd("Not Supported for non ATT carriers");
                data = new DmtData("Not Supported");
            } else {
                boolean mUnknownSources = false;

                try {
                    if (Settings.Global.getInt(mContext.getContentResolver(),
                            Settings.Global.INSTALL_NON_MARKET_APPS) > 0) {
                        logd("install from non-market -- allowed");
                        mUnknownSources = true;
                        data = new DmtData(mUnknownSources);
                    } else {
                        logd("install from non-market -- not allowed");
                        mUnknownSources = false;
                        data = new DmtData(mUnknownSources);
                    }
                } catch (SettingNotFoundException e) {
                    loge("Exception while reading Settings.Global.INSTALL_NON_MARKET_APPS", e);
                }
            }
        } else if (path.equals("./DevDetail/Ext/WLANMacAddr")) {
            if (!isThisForATT()) {
                logd("Not Supported for non ATT carriers");
                data = new DmtData("Not Supported");
            } else {
                String wMac = "";
                SharedPreferences p = mContext.getSharedPreferences(DEV_DETAIL, 0);

                if (p.contains(WIFI_MAC_ADDR)) {
                    wMac = p.getString(DevPlugin.WIFI_MAC_ADDR, null);
                    logd("Read WiFi Mac address from shared file: " + wMac);
                    data = new DmtData(wMac);
                } else {
                    WifiManager wm = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
                    WifiInfo wi = wm.getConnectionInfo();
                    wMac = wi == null ? null : wi.getMacAddress();
                    logd("WiFi Mac address" + wMac);
                    if (!TextUtils.isEmpty(wMac)) {
                        SharedPreferences.Editor ed = p.edit();
                        ed.putString(DevPlugin.WIFI_MAC_ADDR, wMac);
                        ed.apply();

                        data = new DmtData(wMac);
                    } else {
                        data = new DmtData("Unavailable");
                    }
                }
            }
        } else if (path.equals("./DevDetail/Ext/PreFwV")) {
            if (!isThisForATT()) {
                logd("Not Supported for non ATT carriers");
                data = new DmtData("Not Supported");
            } else {
                String preFwV = "";
                SharedPreferences p = mContext.getSharedPreferences(DEV_DETAIL, 0);

                if (p.contains(PRE_FW_VER)) {
                    preFwV = p.getString(DevPlugin.PRE_FW_VER, null);
                    logd("Read Previous Firmware Version from shared file" + preFwV);
                    data = new DmtData(preFwV);
                } else {
                    String SwV;
                    try {
                        SwV = SystemProperties.get("ro.build.version.full");
                    } catch (Exception e) {
                        SwV = "Unknown";
                    }
                    data = new DmtData(SwV);
                }
            }
        } else if (path.equals("./DevDetail/Ext/LastUpdateTime")) {
            if (!isThisForATT()) {
                logd("Not Supported for non ATT carriers");
                data = new DmtData("Not Supported");
            } else {
                String lastUpd = "";
                SharedPreferences p = mContext.getSharedPreferences(DevPlugin.DEV_DETAIL, 0);

                if (p.contains(LAST_UPD_TIME)) {
                    lastUpd = p.getString(DevPlugin.LAST_UPD_TIME, null);
                    logd("Read date stamp for the last update from shared file: " + lastUpd);
                    data = new DmtData(lastUpd);
                } else {
                    data = new DmtData("No Known Update");
                }
            }
        } else if (path.equals("./DevDetail/Ext/DateTime/Date")) {
            if (!isPhoneTypeLTE()) {
                logd("Not Supported for non VZW carriers");
                data = new DmtData("Not Supported");
            } else {
                String updateDateTime = "";
                SharedPreferences p = mContext.getSharedPreferences(DevPlugin.DEV_DETAIL, 0);

                if (p.contains(LAST_UPD_TIME)) {
                    updateDateTime = p.getString(DevPlugin.LAST_UPD_TIME, null);
                    logd("Read date and time stamp for the last update from shared file: "
                            + updateDateTime);
                    String[] updateDate = updateDateTime.split("\\:");
                    try {
                        data = new DmtData(
                                updateDate[0] + ":" + updateDate[1] + ":" + updateDate[2]);
                    } catch (Exception e) {
                        data = new DmtData("No Known Update");
                        e.printStackTrace();
                    }
                } else {
                    data = new DmtData("No Known Update");
                }
            }
        } else if (path.equals("./DevDetail/Ext/DateTime/TimeUTC")) {
            if (!isPhoneTypeLTE()) {
                logd("Not Supported for non VZW carriers");
                data = new DmtData("Not Supported");
            } else {
                String updateDateTime = "";
                SharedPreferences p = mContext.getSharedPreferences(DevPlugin.DEV_DETAIL, 0);

                if (p.contains(LAST_UPD_TIME)) {
                    updateDateTime = p.getString(DevPlugin.LAST_UPD_TIME, null);
                    logd("Read date and time stamp for the last update from shared file: "
                            + updateDateTime);
                    String[] updateTime = updateDateTime.split("\\:");
                    try {
                        data = new DmtData(updateTime[3] + ":" + updateTime[4]);
                    } catch (Exception e) {
                        data = new DmtData("No Known Update");
                        e.printStackTrace();
                    }
                } else {
                    data = new DmtData("No Known Update");
                }
            }
        } else if (path.equals("./DevDetail/Ext/DateTime/DLS")) {
            setOperationResult(ErrorCodes.SYNCML_DM_UNSUPPORTED_OPERATION);
        }

        setOperationResult(data == null ?
                ErrorCodes.SYNCML_DM_UNSUPPORTED_OPERATION :
                ErrorCodes.SYNCML_DM_SUCCESS);

        return data;
    }

    @Override
    public int updateLeafNode(String path, DmtData newValue) throws RemoteException {
        logd("updateLeafNode: rootPath=" + mRootPath + " path=" + path + " newValue=" + newValue);
        //mContext.enforceCallingPermission("com.android.permission.WRITE_OMADM_SETTINGS", "Insufficient Permissions");

        if (path.equals("./DevDetail/Ext/SystemSettings/AllowUnknownSources")) {
            if (!isThisForATT()) {
                logd("Not Supported for non ATT carriers");
                return setOperationResult(ErrorCodes.SYNCML_DM_UNSUPPORTED_OPERATION);
            } else {
                // enable/disable install from non market
                try {
                    Settings.Global.putInt(mContext.getContentResolver(),
                            Settings.Global.INSTALL_NON_MARKET_APPS,
                            ((newValue.getBoolean()) ? 1 : 0));
                    if ((Settings.Global.getInt(mContext.getContentResolver(),
                            Settings.Global.INSTALL_NON_MARKET_APPS))
                            == (newValue.getBoolean() ? 1 : 0)) {
                        logd("Update to settings.db for install_non_market_apps -- success");
                        return setOperationResult(ErrorCodes.SYNCML_DM_SUCCESS);
                    } else {
                        logd("Update to settings.db for install_non_market_apps -- failed");
                        return setOperationResult(ErrorCodes.SYNCML_DM_FAIL);
                    }
                } catch (DmtException e) {
                    loge("Exception during parsing the newValue.getBoolean()", e);
                    return setOperationResult(ErrorCodes.SYNCML_DM_FAIL);
                } catch (SettingNotFoundException e) {
                    loge("Exception retrieving Settings.Global.INSTALL_NON_MARKET_APPS", e);
                    return setOperationResult(ErrorCodes.SYNCML_DM_FAIL);
                }
            }
        } else {
            return setOperationResult(ErrorCodes.SYNCML_DM_FAIL);
        }
    }

    @SuppressWarnings("unchecked")
    @Override
    public Map getNodes(String rootPath) {
        logd("Enter DevPlugin.getNodes(\"" + rootPath + "\")");
        Map<String, DmtPluginNode> hMap = new HashMap<String, DmtPluginNode>();
        DmtPluginNode node1;
        node1 = new DmtPluginNode("", new DmtData("abc"));
        hMap.put("", node1);
        logd("Leave DevPlugin::getNodes()");
        return hMap;
    }

    public boolean release() {
        return true;
    }

    private String readValueFromFile(String propName) {
        String ret = null;
        // use preference instead of the system property
        SharedPreferences prefs = mContext.getSharedPreferences("dmconfig", 0);
        if (prefs.contains(propName)) {
            ret = prefs.getString(propName, "");
            if (ret.length() == 0) {
                ret = null;
            }
        }
        return ret;
    }

    private boolean isThisForATT() {
        boolean carrierATT = false;
        Cursor cr = null;
        try {
            String DATABASE_NAME = "DmConfigure.db";
            SQLiteDatabase mdb = mContext.openOrCreateDatabase(DATABASE_NAME, 0, null);
            cr = mdb.query("dmFlexs", null, "name='CarrierName'", null, null, null, null);
            String value = null;

            if (cr != null) {
                if (cr.moveToFirst() == true) {
                    int index = cr.getColumnIndex("value");
                    value = cr.getString(index);
                    logd("CarrierName = " + value);
                }
                if ("ATT".equals(value) || "Att".equals(value) || "att".equals(value)) {
                    carrierATT = true;
                }
            }

        } catch (Exception e) {
            loge("Not able to get CarrierName from database, return false", e);
        } finally {
            if (cr != null) {
                cr.close();
            }
        }
        return carrierATT;
    }

    private boolean isPhoneTypeLTE() {
        return DMSettingsHelper.isPhoneTypeLTE();
    }

    private String getDeviceType() {
        String devicetype = SystemProperties.get("ro.build.characteristics");
        if (((!TextUtils.isEmpty(devicetype)) && (devicetype.equals("tablet")))) {
            logd("Device Type is Tablet");
        } else {
            devicetype = "phone";
            logd("Device Type is Phone");
        }
        return devicetype;
    }

    private static void logd(String s) {
        Log.d(TAG, s);
    }

    private static void loge(String s) {
        Log.e(TAG, s);
    }

    private static void loge(String s, Throwable tr) {
        Log.e(TAG, s, tr);
    }

    private boolean isSprint() {
        TelephonyManager tm = (TelephonyManager) mContext
            .getSystemService(Context.TELEPHONY_SERVICE);
        String simOperator = tm.getSimOperator();
        String imsi = tm.getSubscriberId();
        logd("simOperator: " + simOperator + " IMSI: " + imsi);
        /* Use MEID for sprint */
        if ("310120".equals(simOperator) || (imsi != null && imsi.startsWith("310120"))) {
            return true;
        } else {
            return false;
        }
    }
}
