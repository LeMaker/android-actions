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

import android.content.ComponentName;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.os.IBinder;
import android.text.TextUtils;
import android.util.Log;

import com.android.omadm.plugin.IDmtPlugin;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;

public class DMConfigureDB {
    private static final String TAG = "DMConfigureDB";
    private static final boolean DBG = DMClientService.DBG;

    private final DMClientService mContext;

    private static final String DATABASE_NAME = "DMConfigure.db";

    private final SQLiteDatabase mdb;

    private IDmtPlugin mPluginConnection;

    private String isBoundTo = null;

    protected final Object mLock = new Object();

    private ServiceConnection mConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName className,
                                       IBinder service) {
            logd("onServiceConnected");
            synchronized (mLock) {
                // this gets an instance of the IRemoteInterface, which we can use to call on the service
                mPluginConnection = IDmtPlugin.Stub.asInterface(service);
                mLock.notifyAll();
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName arg0) {
            logd("onServiceDisconnected");
            synchronized (mLock) {
                mPluginConnection = null;
            }
        }
    };

    static final class AccountInfo {
        public String acctName;     // DM server account name
        public String serverID;     // DM server ID
        public String addr;         // DM server URL
        public String addrType;     // e.g. "URI"
        public String portNbr;      // DM server port, e.g. "443"
        public String conRef;       // e.g. ""
        public String serverName;   // DM server name
        public String authPref;     // e.g. "HMAC"
        public String serverPW;     // HMAC server password
        public String serverNonce;  // HMAC server nonce
        public String userName;     // DM username (e.g. IMSI or MEID)
        public String clientPW;     // HMAC client password
        public String clientNonce;  // HMAC client nonce
        public String proxyAddr;    // HTTP proxy address
        public String proxyPortNbr; // HTTP proxy port, e.g. "80"
    }

    public DMConfigureDB(DMClientService context) {
        mContext = context;
        mdb = mContext.openOrCreateDatabase(DATABASE_NAME, 0, null);
        Cursor cur = null;
        try {
            cur = mdb.rawQuery("PRAGMA table_info( dmAccounts )", null);
            int cnt = cur.getCount();
            if (DBG) logd("Cursor count for table dmAccounts is " + cnt);
            // TODO: we need a way for plugins to populate an empty table
            if (cnt == 0) {
                onCreate(mdb);
            }
            // NOTE: always update all the account info in DM tree
            loadDmConfig();
            loadDmAccount(mdb);

        } catch (Exception e) {
            loge("exception in DMConfigureDB", e);
        } finally {
            if (cur != null) {
                cur.close();
            }
        }
    }

    public void closeDatabase() {
        mdb.close();
        unbind();
    }

    void onCreate(SQLiteDatabase db) {
        logd("onCreate");

        db.execSQL("CREATE TABLE dmAccounts (" +
                "_id INTEGER PRIMARY KEY AUTOINCREMENT," +
                "ServerID TEXT UNIQUE ON CONFLICT REPLACE," +
                "AccName TEXT," +
                "Addr TEXT," +
                "AddrType TEXT," +
                "PortNbr TEXT," +
                "ConRef TEXT," +
                "Name TEXT," +
                "AuthPref TEXT," +
                "ServerPW TEXT," +
                "ServerNonce TEXT," +
                "UserName TEXT," +
                "ClientPW TEXT," +
                "ClientNonce TEXT," +
                "ProxyAddr TEXT," +
                "ProxyPortNbr TEXT" +
                ");");

        db.execSQL("CREATE TABLE dmFlexs (" +
                "_id INTEGER PRIMARY KEY AUTOINCREMENT," +
                "name TEXT UNIQUE ON CONFLICT REPLACE," +
                "value TEXT" +
                ");");
    }

    public String getFotaServerID() {
        String id = getConfigField("PreferredServerID");

        if (id != null && !id.isEmpty()) {
            return id;
        }

        return getConfigField("FOTAServerID");
    }

    public boolean isDmAlertEnabled() {
        String value = getConfigField("DmAlertEnabled");
        return null == value || "true".equalsIgnoreCase(value);
    }

    public boolean isDmNonceResyncEnabled() {
        String value = getConfigField("DmNonceResyncEnabled");
        return null != value && "true".equalsIgnoreCase(value);
    }

    /*
     * Returns SyncML logging level
     * 0 = do not log syncml messages
     * 1 = log messages received as is
     * 2 = convert wbxml to xml messages and log it
     */
    public int getSyncMLLogLevel() {
        return 1;

//        String value = getConfigField("DmSyncMLLogLevel");
//        if (null == value) {
//            return 0;
//        }
//        return Integer.parseInt(value);
    }

    private String getConfigField(String field) {
        String value = null;

        Cursor cr = mdb.query("dmFlexs", null, "name='" + field + '\'', null, null, null, null);

        if (cr != null) {
            if (cr.moveToFirst()) {
                int index = cr.getColumnIndex("value");
                value = cr.getString(index);
            }
            cr.close();
        }

        if (DBG) logd("get field '" + field + "'=" + value);
        return value;
    }

    public void setFotaServerID(String serverID) {
        ContentValues values = new ContentValues(1);

        //values.put("Name", "FOTAServerID");
        values.put("value", serverID);

        mdb.update("dmFlexs", values, "name='FOTAServerID'", null);
    }

    void setGsmImei(String imei) {
        ContentValues values = new ContentValues();
        values.put("name", "gsmImei");
        values.put("value", imei);
        mdb.insert("dmFlexs", null, values);

    }

    private void loadDmConfig() {
        //following statements just for debug print
        getConfigField("CarrierName");
        getConfigField("AdditionalCharge");
        getConfigField("PreferredServerID");

        // FIXME: this should be removed along with get/setGsmImei()
        if (DMSettingsHelper.isPhoneTypeLTE()) {
            SharedPreferences p = mContext.getSharedPreferences(DMHelper.IMEI_PREFERENCE_KEY, 0);
            String gsmImei = p.getString(DMHelper.IMEI_VALUE_KEY, null);
            logd("gsmImei in loadDmConfig is " + gsmImei);
            //ed.clear();
            if (null == gsmImei || gsmImei.isEmpty()) {
                // this is needed to avoid showing DMService app force close
                logd("set the imei value to zero");
                gsmImei = "0";
            } else if (gsmImei.length() > 15) {
                logd("imei length exceeding 15 digits so trim it to 15");
                gsmImei = gsmImei.substring(0, 15);
            }
            setGsmImei(gsmImei);
        }
        ///////////////////////////////////////////

    }

    private void loadDmAccount(SQLiteDatabase db) {
        AccountInfo ai = new AccountInfo();
        boolean isFirst = true;

        Cursor cr = db.rawQuery("SELECT * FROM dmAccounts", null);

        if (cr != null) {
            if (cr.moveToFirst()) {
                do {
                    ai.acctName = cr.getString(cr.getColumnIndex("AccName"));
                    logd("[Factory]account=" + ai.acctName);

                    ai.serverID = cr.getString(cr.getColumnIndex("ServerID"));
                    if (DBG) logd("[Factory]serverID=" + ai.serverID);

                    ai.addr = cr.getString(cr.getColumnIndex("Addr"));
                    if (DBG) logd("[Factory]addr=" + ai.addr);

                    ai.addrType = cr.getString(cr.getColumnIndex("AddrType"));
                    if (DBG) logd("[Factory]addrType=" + ai.addrType);

                    ai.portNbr = cr.getString(cr.getColumnIndex("PortNbr"));
                    if (DBG) logd("[Factory]portNbr=" + ai.portNbr);

                    ai.conRef = cr.getString(cr.getColumnIndex("ConRef"));
                    if (DBG) logd("[Factory]conRef=" + ai.conRef);

                    ai.serverName = cr.getString(cr.getColumnIndex("Name"));
                    if (DBG) logd("[Factory]serverName=" + ai.serverName);

                    ai.authPref = cr.getString(cr.getColumnIndex("AuthPref"));
                    if (DBG) logd("[Factory]authPref=" + ai.authPref);

                    ai.serverPW = cr.getString(cr.getColumnIndex("ServerPW"));
                    if (DBG) logd("[Factory]serverPW=" + ai.serverPW);

                    ai.serverNonce = cr.getString(cr.getColumnIndex("ServerNonce"));
                    if (DBG) logd("[Factory]serverNonce=" + ai.serverNonce);

                    ai.userName = cr.getString(cr.getColumnIndex("UserName"));
                    if (DBG) logd("[Factory]userName=" + ai.userName);

                    ai.clientPW = cr.getString(cr.getColumnIndex("ClientPW"));
                    if (DBG) logd("[Factory]clientPW=" + ai.clientPW);

                    ai.clientNonce = cr.getString(cr.getColumnIndex("ClientNonce"));
                    if (DBG) logd("[Factory]clientNonce=" + ai.clientNonce);

                    ai.proxyAddr = cr.getString(cr.getColumnIndex("ProxyAddr"));
                    if (DBG) logd("[Factory]proxyAddr=" + ai.proxyAddr);

                    ai.proxyPortNbr = cr.getString(cr.getColumnIndex("ProxyPortNbr"));
                    if (DBG) logd("[Factory]proxyPortNbr=" + ai.proxyPortNbr);

                    if (writeAccount2Dmt(ai) && isFirst) {
                        if (DBG) logd("[Factory]setFotaServerID: " + ai.serverID);

                        ContentValues values = new ContentValues();
                        values.put("name", "FOTAServerID");
                        values.put("value", ai.serverID);
                        db.insert("dmFlexs", null, values);

                        isFirst = false;
                    }

                } while (cr.moveToNext());

                cr.close();
                return;
            }
            cr.close();
        }

        try {
            XmlPullParser xpp = null;
            String[] accountInfo = null;
            int eventType = 0;
            int num_accounts = 0;
            InputStream in = getDMAccXmlInput();

            if (in != null) {
                XmlPullParserFactory factory = XmlPullParserFactory.newInstance();
                factory.setNamespaceAware(true);
                xpp = factory.newPullParser();
                xpp.setInput(in, null);
                eventType = xpp.getEventType();
            } else {
                logd("Reading dmAccounts from res");
                Resources res = mContext.getResources();
                accountInfo = res.getStringArray(R.array.dm_account_info);
                if (accountInfo == null) {
                    if (DBG) logd("accountInfo == null");
                    return;
                } else {
                    logd("Number of accounts = " + accountInfo.length);
                }
            }

            while ((in != null && eventType != XmlPullParser.END_DOCUMENT) ||
                    (in == null && num_accounts < accountInfo.length)) {
                if (in == null ||
                        (eventType == XmlPullParser.START_TAG && "Account".equals(xpp.getName()))) {
                    String[] account = null;
                    if (in == null) {
                        account = accountInfo[num_accounts].split(", ");
                        if (account == null) {
                            loge("account == null; invalid account info");
                        } else {
                            if (DBG) logd("Account # " + (num_accounts + 1) +
                                    "; number of fields = " + account.length);
                        }
                    }

                    ai.acctName = getAttributeValue(xpp, "AccName", account, 0);
                    logd("account=" + ai.acctName);

                    ai.serverID = getAttributeValue(xpp, "ServerID", account, 1);
                    if (DBG) logd("serverID=" + ai.serverID);

                    ai.addr = getRealString(getAttributeValue(xpp, "Addr", account, 2));
                    if (DBG) logd("addr=" + ai.addr);

                    ai.addrType = getAttributeValue(xpp, "AddrType", account, 3);
                    if (DBG) logd("addrType=" + ai.addrType);

                    ai.portNbr = getAttributeValue(xpp, "PortNbr", account, 4);
                    if (DBG) logd("portNbr=" + ai.portNbr);

                    ai.conRef = getAttributeValue(xpp, "ConRef", account, 5);
                    if (DBG) logd("conRef=" + ai.conRef);

                    ai.serverName = getAttributeValue(xpp, "ServerName", account, 6);
                    if (DBG) logd("serverName=" + ai.serverName);

                    ai.authPref = getAttributeValue(xpp, "AuthPref", account, 7);
                    if (DBG) logd("authPref=" + ai.authPref);

                    ai.serverPW = getAttributeValue(xpp, "ServerPW", account, 8);
                    if (DBG) logd("serverPW=" + ai.serverPW);

                    ai.serverNonce = getAttributeValue(xpp, "ServerNonce", account, 9);
                    if (DBG) logd("serverNonce=" + ai.serverNonce);

                    ai.userName = getAttributeValue(xpp, "UserName", account, 10);
                    if (DBG) logd("userName=" + ai.userName);

                    ai.clientPW = getAttributeValue(xpp, "ClientPW", account, 11);
                    if (DBG) logd("clientPW=" + ai.clientPW);

                    ai.clientNonce = getAttributeValue(xpp, "ClientNonce", account, 12);
                    if (DBG) logd("clientNonce=" + ai.clientNonce);

                    ai.proxyAddr = getRealString(getAttributeValue(xpp, "ProxyAddr", account, 13));
                    if (DBG) logd("proxyAddr=" + ai.proxyAddr);

                    ai.proxyPortNbr = getRealString(getAttributeValue(xpp, "ProxyPortNbr", account, 14));
                    if (DBG) logd("addr=" + ai.proxyPortNbr);

                    // FIXME: check should be on account name instead of isFirst
                    if (writeAccount2Dmt(ai) && isFirst) {
                        if (DBG) logd("setFotaServerID: " + ai.serverID);

                        ContentValues values = new ContentValues();
                        values.put("name", "FOTAServerID");
                        values.put("value", ai.serverID);
                        db.insert("dmFlexs", null, values);
                        isFirst = false;
                    }
                }

                if (in != null) {
                    eventType = xpp.next();
                } else {
                    num_accounts++;
                }
            }

            if (in != null) {
                in.close();
            }

        } catch (IOException e) {
            loge("IOException in loadDmAccount", e);
        } catch (XmlPullParserException e) {
            loge("XmlPullParserException in loadDmAccount", e);
        }
    }

    private String getAttributeValue(XmlPullParser xpp, String attribute, String[] account, int idx) {
        if (xpp != null) {
            return xpp.getAttributeValue(null, attribute);
        } else if (account != null && idx < account.length) {
            return account[idx];
        } else {
            return null;
        }
    }

    private boolean writeAccount2Dmt(AccountInfo ai) {
        if (DBG) logd("enter writeAccount2Dmt");

        String acctName = ai.serverID;     // e.g. "sprint"; this is also the value for ServerID
        String dmServerNodePath = "./DMAcc/" + acctName;

        logd("XXX DELETING old server node path: " + dmServerNodePath);
        NativeDM.deleteNode(dmServerNodePath);

        if (NativeDM.createInterior(dmServerNodePath) != DMResult.SYNCML_DM_SUCCESS) {
            loge("createInterior '" + acctName + "' Error");
            return false;
        }

        if (NativeDM.createLeaf(dmServerNodePath + "/ServerID", ai.serverID)
                != DMResult.SYNCML_DM_SUCCESS) {
            loge("createInterior '" + dmServerNodePath + "/ServerId' Error");
            return false;
        }

        if (NativeDM.createLeaf(dmServerNodePath + "/AppID", "w7") != DMResult.SYNCML_DM_SUCCESS) {
            loge("createInterior '" + dmServerNodePath + "/AppID' Error");
            return false;
        }

        if (NativeDM.createLeaf(dmServerNodePath + "/Name", ai.serverName)
                != DMResult.SYNCML_DM_SUCCESS) {
            loge("createLeaf '" + dmServerNodePath + "/Name' Error");
            return false;
        }

        if (NativeDM.createLeaf(dmServerNodePath + "/PrefConRef", ai.conRef)
                != DMResult.SYNCML_DM_SUCCESS) {
            loge("createLeaf '" + dmServerNodePath + "/ConRef' Error");
            return false;
        }

        if (NativeDM.createLeaf(dmServerNodePath + "/AAuthPref", ai.authPref)
                != DMResult.SYNCML_DM_SUCCESS) {
            loge("createLeaf '" + dmServerNodePath + "/AAuthPref' Error");
            return false;
        }

        dmServerNodePath += "/AppAddr";
        if (NativeDM.createInterior(dmServerNodePath) != DMResult.SYNCML_DM_SUCCESS) {
            loge("createInterior '" + dmServerNodePath + "' Error");
            return false;
        }

        dmServerNodePath += "/1";   // limited to one server address per account

        if (NativeDM.createInterior(dmServerNodePath) != DMResult.SYNCML_DM_SUCCESS) {
            loge("createInterior '" + dmServerNodePath + "' Error");
            return false;
        }

        if ("sprint".equalsIgnoreCase(ai.serverID)) {
            String address = DMHelper.getServerUrl(mContext);
            if (!TextUtils.isEmpty(address)) {
                logd("Overriding server URL to: " + address);
                ai.addr = address;
            }
        }

        if (NativeDM.createLeaf(dmServerNodePath + "/Addr", ai.addr)
                != DMResult.SYNCML_DM_SUCCESS) {
            loge("createLeaf '" + dmServerNodePath + "/Addr' Error");
            return false;
        }

        if (NativeDM.createLeaf(dmServerNodePath + "/AddrType", ai.addrType)
                != DMResult.SYNCML_DM_SUCCESS) {
            loge("createLeaf '" + dmServerNodePath + "/AddrType' Error");
            return false;
        }

        dmServerNodePath += "/Port";
        if (NativeDM.createInterior(dmServerNodePath) != DMResult.SYNCML_DM_SUCCESS) {
            loge("createInterior '" + dmServerNodePath + "' Error");
            return false;
        }

        dmServerNodePath += "/1";   // limited to one port number per server address

        if (NativeDM.createInterior(dmServerNodePath) != DMResult.SYNCML_DM_SUCCESS) {
            loge("createInterior '" + dmServerNodePath + "' Error");
            return false;
        }

        if (NativeDM.createLeaf(dmServerNodePath + "/PortNbr", ai.portNbr)
                != DMResult.SYNCML_DM_SUCCESS) {
            loge("createLeaf '" + dmServerNodePath + "/PortNbr' Error");
            return false;
        }

        // collection of authentication credentials
        dmServerNodePath = "./DMAcc/" + acctName + "/AppAuth";

        if (NativeDM.createInterior(dmServerNodePath) != DMResult.SYNCML_DM_SUCCESS) {
            loge("createInterior '" + dmServerNodePath + "' Error");
            return false;
        }

        // server credentials for authenticating the server from the OMA DM client
        dmServerNodePath += "/Server";

        if (NativeDM.createInterior(dmServerNodePath) != DMResult.SYNCML_DM_SUCCESS) {
            loge("createInterior '" + dmServerNodePath + "' Error");
            return false;
        }

        String authLevel = "SRVCRED";

        if (NativeDM.createLeaf(dmServerNodePath + "/AAuthLevel", authLevel)
                != DMResult.SYNCML_DM_SUCCESS) {
            loge("createLeaf '" + dmServerNodePath + "/AAuthLevel' Error");
            return false;
        }

        if (NativeDM.createLeaf(dmServerNodePath + "/AAuthType", ai.authPref)
                != DMResult.SYNCML_DM_SUCCESS) {
            loge("createLeaf '" + dmServerNodePath + "/AAuthType' Error");
            return false;
        }

        if (NativeDM.createLeaf(dmServerNodePath + "/AAuthName", acctName)
                != DMResult.SYNCML_DM_SUCCESS) {
            loge("createLeaf '" + dmServerNodePath + "/AAuthName' Error");
            return false;
        }

        if (ai.serverID.equalsIgnoreCase("sprint") ||
                ai.serverID.equalsIgnoreCase("com.vzwdmserver")) {
            String intentName;
            String serviceName;
            if (ai.serverID.equalsIgnoreCase("sprint")) {
                intentName = "com.android.sdm.plugins.sprintdm.SprintDMPlugin";
                serviceName = "SprintDMService";
            } else {
                intentName = "com.android.sdm.plugins.connmo.ConnmoPlugin";
                serviceName = "ConnmoService";
            }
            logd("ServerID is " + ai.serverID);
            if (isBoundTo == null || !isBoundTo.equalsIgnoreCase(ai.serverID)) {
                unbind();
                Intent intent = new Intent(intentName);
                if (mContext.bindService(intent, mConnection, Context.BIND_AUTO_CREATE)) {
                    isBoundTo = ai.serverID;
                    synchronized (mLock) {
                        if (mPluginConnection == null) {
                            logd("Waiting for binding to " + serviceName);
                            try {
                                mLock.wait();
                            } catch (Exception e) {
                                loge("Exception in writeAccount2Dmt->wait: ", e);
                            }
                        }
                    }
                }
            } else {
                logd("Already bound to " + serviceName);
            }
        } else {
            unbind();
        }

        boolean hasWriteServerPW = false;
        try {
            if (mPluginConnection != null) {
                String serverPW = mPluginConnection.getServerPW(ai.serverPW);
                if (serverPW != null) {
                    ai.serverPW = serverPW;
                    logd("ServerPW from plugin: " + serverPW);
                } else {
                    // This must be for vzw
                    byte[] svrPasswd = hexStringToBytes(ai.serverPW);
                    if (NativeDM.createLeaf(dmServerNodePath + "/AAuthSecret", svrPasswd)
                            != DMResult.SYNCML_DM_SUCCESS) {
                        loge("CreateLeaf '"+dmServerNodePath + "/AAuthSecret' Error");
                        return false;
                    }
                    hasWriteServerPW = true;
                }
            } else {
                logd("Using default ServerPW from dmAccounts: " + ai.serverPW);
            }
        } catch (Exception e) {
            loge("Exception in writeAccount2Dmt->getServerPW", e);
        }

        if (!hasWriteServerPW && NativeDM.createLeaf(dmServerNodePath + "/AAuthSecret", ai.serverPW)
                != DMResult.SYNCML_DM_SUCCESS) {
            loge("createLeaf '" + dmServerNodePath + "/AAuthSecret' Error");
            return false;
        }

        if (NativeDM.createLeaf(dmServerNodePath + "/AAuthData", ai.serverNonce)
                != DMResult.SYNCML_DM_SUCCESS) {
            loge("createLeaf '" + dmServerNodePath + "/AAuthData' Error");
            return false;
        }

        // client credentials for authenticating ourselves to the OMA DM server
        String dmClientNodePath = "./DMAcc/" + acctName + "/AppAuth/Client";

        if (NativeDM.createInterior(dmClientNodePath) != DMResult.SYNCML_DM_SUCCESS) {
            loge("createInterior '" + dmServerNodePath + "' Error");
            return false;
        }

        String clientAuthLevel = "CLCRED";

        if (NativeDM.createLeaf(dmClientNodePath + "/AAuthLevel", clientAuthLevel)
                != DMResult.SYNCML_DM_SUCCESS) {
            loge("createLeaf '" + dmClientNodePath + "/AAuthLevel' Error");
            return false;
        }

        String clientAuthType = ai.authPref;
        if (NativeDM.createLeaf(dmClientNodePath + "/AAuthType", clientAuthType)
                != DMResult.SYNCML_DM_SUCCESS) {
            loge("createLeaf '" + dmClientNodePath + "/AAuthType' Error");
            return false;
        }

        boolean hasWriteUserName = false;
        try {
            if (mPluginConnection != null) {
                String username = mPluginConnection.getUsername(ai.userName);
                if (username != null) {
                    ai.userName = username;
                    logd("Username from plugin: " + username);
                } else {
                    // This must be for vzw
                    byte[] clientName = hexStringToBytes(ai.userName);//"e0e5e7eaebeb");
                    if (NativeDM.createLeaf(dmClientNodePath + "/AAuthName", clientName)
                            != DMResult.SYNCML_DM_SUCCESS) {
                        loge("CreateLeaf '"+dmClientNodePath + "/AAuthName' Error");
                        return false;
                    }
                    hasWriteUserName = true;
                }
            } else {
                logd("Using default username from dmAccounts: " + ai.userName);
            }
        } catch (Exception e) {
            loge("Exception in writeAccount2Dmt->getUsername", e);
        }

        if (!hasWriteUserName && NativeDM.createLeaf(dmClientNodePath + "/AAuthName", ai.userName)
                != DMResult.SYNCML_DM_SUCCESS) {
            loge("createLeaf '" + dmClientNodePath + "/AAuthName' Error");
            return false;
        }

        boolean hasWriteClientPW = false;
        try {
            if (mPluginConnection != null) {
                String clientPW = mPluginConnection.getClientPW(ai.clientPW);
                if (clientPW != null) {
                    ai.clientPW = clientPW;
                    logd("ClientPW from plugin: " + clientPW);
                } else {
                    // This must be for vzw
                    byte[] clientPasswd=hexStringToBytes(ai.clientPW);//"ebe8efeeecec");
                    if (NativeDM.createLeaf(dmClientNodePath + "/AAuthSecret", clientPasswd) !=
                            DMResult.SYNCML_DM_SUCCESS) {
                        loge("CreateLeaf '" + dmClientNodePath + "/AAuthSecret' Error");
                        return false;
                    }
                    hasWriteClientPW = true;
                }
            } else  {
                logd("Using default ClientPW from dmAccounts: " + ai.clientPW);
            }
        } catch (Exception e) {
            loge("Exception in writeAccount2Dmt->getClientPW", e);
        }

        if (!hasWriteClientPW && NativeDM.createLeaf(dmClientNodePath + "/AAuthSecret", ai.clientPW)
                    != DMResult.SYNCML_DM_SUCCESS) {
            loge("createLeaf '" + dmClientNodePath + "/AAuthSecret' Error");
            return false;
        }

        String clientNonce = ai.clientNonce;
        if (NativeDM.createLeaf(dmClientNodePath + "/AAuthData", clientNonce)
                != DMResult.SYNCML_DM_SUCCESS) {
            loge("createLeaf '" + dmClientNodePath + "/AAuthData' Error");
            return false;
        }

        logd("leave writeAccount2Dmt: success");
        return true;
    }

    private InputStream getDMAccXmlInput() {
        try {
            File file = new File("/system/etc/", "dmAccounts.xml");
            if (file.exists()) {
                InputStream in = new BufferedInputStream(new FileInputStream(file));
                logd("Load config from /system/etc/dmAccounts.xml");
                return in;
            } else {
                return null;
            }
        } catch (IOException e) {
            loge("IOException in getDMAccXmlInput", e);
            return null;
        }
    }

    private String getRealString(String ins) {
        if (ins != null && !ins.isEmpty() && ins.charAt(0) == '$') {
            SharedPreferences prefs = mContext.getSharedPreferences("dmconfig", 0);
            String key = ins.substring(1);
            if (prefs.contains(key)) {
                return prefs.getString(key, "unknown");
            }
        }
        return ins;
    }

    private void unbind() {
        if (isBoundTo != null) {
            logd("Unbinding from " + isBoundTo);
            mContext.unbindService(mConnection);
            isBoundTo = null;
            synchronized (mLock) {
                mPluginConnection = null;
            }
        }
    }

    private static byte[] hexStringToBytes(String s) {
        byte[] ret;

        if (s == null) return null;

        int sz = s.length();

        ret = new byte[sz/2];

        for (int i=0 ; i <sz ; i+=2) {
            ret[i/2] = (byte) ((hexCharToInt(s.charAt(i)) << 4)
                    | hexCharToInt(s.charAt(i+1)));
        }

        return ret;
    }

    private static int hexCharToInt(char c) {
        if (c >= '0' && c <= '9') return (c - '0');
        if (c >= 'A' && c <= 'F') return (c - 'A' + 10);
        if (c >= 'a' && c <= 'f') return (c - 'a' + 10);

        throw new RuntimeException ("invalid hex char '" + c + "'");
    }

//    private static void testSprintHashGenerator() {
//        String equip = "A000001A2B3C4F";
//        String server = "sprint";
//        String secret = "foobar";
//
//        logd("XXXXX B64(MD5(\"foobar\") = " + sprintHashGenerator("foobar"));
//        logd("XXXXX f(equip,server,secret) = " + sprintHashGenerator(equip + server + secret));
//        logd("XXXXX f(server,equip,secret) = " + sprintHashGenerator(server + equip + secret));
//    }

    private static void logd(String msg) {
        Log.d(TAG, msg);
    }

    private static void loge(String msg) {
        Log.e(TAG, msg);
    }

    private static void loge(String msg, Throwable tr) {
        Log.d(TAG, msg, tr);
    }
}
