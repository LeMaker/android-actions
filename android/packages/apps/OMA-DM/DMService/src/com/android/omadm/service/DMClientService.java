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

import android.app.IntentService;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.IBinder;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;

import com.android.omadm.plugin.DmtData;
import com.android.omadm.plugin.DmtException;
import com.android.omadm.plugin.IDMClientService;
import com.android.omadm.plugin.impl.DmtPluginManager;

import net.jcip.annotations.GuardedBy;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Map;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * This is the OMA DM client service as an IntentService.
 * FIXME: this should be rewritten as a regular Service with an associated StateMachine.
 */
public class DMClientService extends IntentService {
    private static final String TAG = "DMClientService";
    static final boolean DBG = false;    // STOPSHIP: change to false

    // flag "DM session in progress" used from DMIntentReceiver
    public static boolean sIsDMSessionInProgress;

    private boolean mInitGood;
    private WakeLock mWakeLock;

    /** Lock object for {@link #mSession} and {@link #mServiceID}. */
    private final Object mSessionLock = new Object();

    @GuardedBy("mSessionLock")
    private DMSession mSession;

    @GuardedBy("mSessionLock")
    private long mServiceID;

    @GuardedBy("mSessionTimeOutHandler")
    private final Handler mSessionTimeOutHandler = new Handler();

    /** AsyncTask to manage the settings SQLite database. */
    private DMConfigureTask mDMConfigureTask;

    /**
     * Helper class for DM session packages.
     */
    static final class DMSessionPkg {
        public DMSessionPkg(int type, long gId) {
            mType = type;
            mGlobalSID = gId;
            mobj = null;
        }

        public final int mType;
        public final long mGlobalSID;
        public Object mobj;
        public Object mobj2;
        public boolean mbvalue;
    }

    // Class for clients to access. Because we know this service always runs
    // in the same process as its clients, we don't need to deal with IPC.
    public class LocalBinder extends IDMClientService.Stub {
        @Override
        public DmtData getDMTree(String path, boolean recursive)
                throws RemoteException {
            try {
                if (DBG)
                    logd("getDMTree(\"" + path + "\", " + recursive + ") called");
                synchronized (mSessionLock) {
                    int nodeType = NativeDM.getNodeType(path);
                    String nodeValue = NativeDM.getNodeValue(path);
                    DmtData dmtData = new DmtData(nodeValue, nodeType);
                    if (nodeType == DmtData.NODE && recursive) {
                        addNodeChildren(path, dmtData);
                    }
                    return dmtData;
                }
            } catch (Exception e) {
                loge("caught exception", e);
                return new DmtData("", DmtData.STRING);
            }
        }

        private void addNodeChildren(String path, DmtData node) throws DmtException {
            for (Map.Entry<String, DmtData> child : node.getChildNodeMap().entrySet()) {
                String childPath = path + '/' + child.getKey();

                int nodeType = NativeDM.getNodeType(childPath);
                String nodeValue = NativeDM.getNodeValue(childPath);

                DmtData newChildNode = new DmtData(nodeValue, nodeType);

                node.addChildNode(child.getKey(), newChildNode);

                if (nodeType == DmtData.NODE) {
                    addNodeChildren(childPath, newChildNode);
                }
            }
        }

        @Override
        public int startClientSession(String path, String clientCert, String privateKey,
                                      String alertType, String redirectURI, String username, String password)
                throws RemoteException {
            if (DBG) logd("startClientSession(\"" + path + "\", \"" + clientCert
                    + "\", \"" + privateKey + "\", \"" + alertType + "\", \"" + redirectURI
                    + "\", \"" + username + "\", \"" + password + "\") called");
            return 0;
        }

        @Override
        public int notifyExecFinished(String path) throws RemoteException {
            if (DBG) logd("notifyExecFinished(\"" + path + "\") called");
            return 0;
        }

        @Override
        public int injectSoapPackage(String path, String command, String payload)
                throws RemoteException {
            if (DBG) logd("injectSoapPackage(\"" + path + "\", \"" + command
                    + "\", \"" + payload + "\") called");
            synchronized (mSessionLock) {
                //return processSerializedTree(serverId, path, command, payload);   // FIXME
            }
            return DMResult.SYNCML_DM_FAIL;
        }
    }

    /**
     * Create the IntentService, naming the worker thread DMClientService.
     */
    public DMClientService() {
        super(TAG);
    }

    @Override
    public void onCreate() {
        super.onCreate();

        logd("Enter onCreate tid=" + Thread.currentThread().getId());

        copyFilesFromAssets();      // wait for completion before continuing

        mInitGood = (NativeDM.initialize() == DMResult.SYNCML_DM_SUCCESS);
        DmtPluginManager.setContext(this);

        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        WakeLock lock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, getClass().getName());
        lock.setReferenceCounted(false);
        lock.acquire();
        logd("XXXXX mWakeLock.acquire() in DMClientService.onCreate() XXXXX");
        mWakeLock = lock;

        mDMConfigureTask = new DMConfigureTask();
        mDMConfigureTask.execute(this);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        logd("Enter onDestroy tid=" + Thread.currentThread().getId());

        mAbortSession = null;

        if (mInitGood) NativeDM.destroy();

        getConfigDB().closeDatabase();

        logd("XXXXX mWakeLock.release() in DMClientService.onDestroy() XXXXX");
        mWakeLock.release();

        synchronized (mSessionLock) {
            mSessionTimeOutHandler.removeCallbacks(mAbortSession);
        }

        if (DBG) logd("leave onDestroy");
    }

    /**
     * AsyncTask to create the DMConfigureDB object on a helper thread.
     */
    private static class DMConfigureTask extends
            AsyncTask<DMClientService, Void, DMConfigureDB> {
        DMConfigureTask() {}

        @Override
        protected DMConfigureDB doInBackground(DMClientService... params) {
            logd("creating new DMConfigureDB() on tid "
                    + Thread.currentThread().getId());
            return new DMConfigureDB(params[0]);
        }
    }

    /**
     * Process message on IntentService worker thread.
     * @param pkg the parameters to pass from the Intent
     */
    private void processMsg(DMSessionPkg pkg) {
        // wait for up to 70 seconds for config DB to initialize.
        if (getConfigDB() == null) {
            loge("processMsg: getConfigDB() failed. Aborting session");
            return;
        }
        logd("processMsg: received pkg type " + pkg.mType + "; getConfigDB() succeeded");

        sIsDMSessionInProgress = true;

        // check if DMT locked by DMSettingsProvider and wait. If DMT is
        // locked more then 1 minute (error case, means that something
        // wrong with DMSettingsProvider) we are continuing execution

        try {
            synchronized (mSessionLock) {
                mSession = new DMSession(this);
                mServiceID = pkg.mGlobalSID;
            }

            int timeOutSecond = 600 * 1000; /* 10 minutes */
            int ret = DMResult.SYNCML_DM_SESSION_PARAM_ERR;

            switch (pkg.mType) {
                case DMIntent.TYPE_PKG0_NOTIFICATION:
                    if (DBG) {
                        logd("Start pkg0 alert session");
                    }
                    startTimeOutTick(timeOutSecond);
                    synchronized (mSessionLock) {
                        ret = mSession.startPkg0AlertSession((byte[]) pkg.mobj);
                    }
                    break;

                case DMIntent.TYPE_FOTA_CLIENT_SESSION_REQUEST:
                    if (DBG) {
                        logd("Start fota client initialized session");
                    }
                    startTimeOutTick(timeOutSecond);
                    synchronized (mSessionLock) {
                        ret = mSession.startFotaClientSession(
                                (String) pkg.mobj, (String) pkg.mobj2);
                    }
                    break;

                case DMIntent.TYPE_FOTA_NOTIFY_SERVER:
                    if (DBG) {
                        logd("Start FOTA notify session");
                    }
                    startTimeOutTick(timeOutSecond);
                    synchronized (mSessionLock) {
                        ret = mSession.fotaNotifyDMServer((FotaNotifyContext) pkg.mobj);
                    }
                    break;

                case DMIntent.TYPE_CLIENT_SESSION_REQUEST:
                    if (DBG) {
                        logd("Start client initialized session:");
                    }
                    if (pkg.mobj != null) {
                        startTimeOutTick(timeOutSecond);
                        synchronized (mSessionLock) {
                            ret = mSession.startClientSession((String) pkg.mobj);
                        }
                    }
                    break;

                case DMIntent.TYPE_LAWMO_NOTIFY_SESSION:
                    if (DBG) {
                        logd("Start LAWMO notify session");
                    }
                    startTimeOutTick(timeOutSecond);
                    synchronized (mSessionLock) {
                        ret = mSession
                                .startLawmoNotifySession((FotaNotifyContext) pkg.mobj);
                    }
                    break;
            }

            logd("DM Session result code=" + ret);

            synchronized (mSessionLock) {
                mSession = null;
            }

            Intent intent = new Intent(DMIntent.DM_SERVICE_RESULT_INTENT);
            intent.putExtra(DMIntent.FIELD_DMRESULT, ret);
            intent.putExtra(DMIntent.FIELD_REQUEST_ID, pkg.mGlobalSID);
            sendBroadcast(intent);
        } finally {
            //set static flag "DM session in progress" to false. Used from DMIntentReceiver
            sIsDMSessionInProgress = false;
        }
    }

    void cancelSession(long requestID) {
        synchronized (mSessionLock) {
            if (requestID == 0 || mServiceID == requestID) {
                if (mSession != null) {
                    loge("Cancel session with serviceID: " + mServiceID);
                    mSession.cancelSession();
                }
            }
        }
    }

    /**
     * Called on worker thread with the Intent to handle. Calls DMSession directly.
     * @param intent The intent to handle
     */
    @Override
    protected void onHandleIntent(Intent intent) {
        long requestID = intent.getLongExtra(DMIntent.FIELD_REQUEST_ID, 0);
        int intentType = intent.getIntExtra(DMIntent.FIELD_TYPE, DMIntent.TYPE_UNKNOWN);

        logd("onStart intentType: " + intentType + " requestID: "
                + requestID);

        // wait for up to 70 seconds for config DB to initialize.
        if (getConfigDB() == null) {
            loge("WARNING! getConfigDB() failed. Aborting session");
            return;
        }
        if (DBG) logd("getConfigDB() succeeded");

        switch (intentType) {
            case DMIntent.TYPE_PKG0_NOTIFICATION: {
                if (DBG) logd("Pkg0 provision received.");

                byte[] pkg0data = intent.getByteArrayExtra(DMIntent.FIELD_PKG0);
                if (pkg0data == null) {
                    if (DBG) logd("Pkg0 provision received, but no pkg0 data.");
                    return;
                }
                DMSessionPkg pkg = new DMSessionPkg(intentType, requestID);
                pkg.mobj = intent.getByteArrayExtra(DMIntent.FIELD_PKG0);
                processMsg(pkg);
                break;
            }
            case DMIntent.TYPE_FOTA_CLIENT_SESSION_REQUEST: {
                if (DBG) logd("Client initiated dm session was received.");

                DMSessionPkg pkg = new DMSessionPkg(intentType, requestID);
                String serverID = intent.getStringExtra(DMIntent.FIELD_SERVERID);
                String alertStr = intent.getStringExtra(DMIntent.FIELD_ALERT_STR);

                if (TextUtils.isEmpty(serverID)) {
                    loge("missing server ID, returning");
                    return;
                }

                if (TextUtils.isEmpty(alertStr)) {
                    loge("missing alert string, returning");
                    return;
                }

                pkg.mobj = serverID;
                pkg.mobj2 = alertStr;
                processMsg(pkg);
                break;
            }
            case DMIntent.TYPE_FOTA_NOTIFY_SERVER: {
                String result = intent.getStringExtra(DMIntent.FIELD_FOTA_RESULT);
                String pkgURI = intent.getStringExtra(DMIntent.FIELD_PKGURI);
                String alertType = intent.getStringExtra(DMIntent.FIELD_ALERTTYPE);
                String serverID = intent.getStringExtra(DMIntent.FIELD_SERVERID);
                String correlator = intent.getStringExtra(DMIntent.FIELD_CORR);

                if (DBG) logd("FOTA_NOTIFY_SERVER_SESSION Input==>\n" + " Result="
                        + result + '\n' + " pkgURI=" + pkgURI + '\n'
                        + " alertType=" + alertType + '\n' + " serverID="
                        + serverID + '\n' + " correlator=" + correlator);

                DMSessionPkg pkg = new DMSessionPkg(intentType, requestID);
                pkg.mobj = new FotaNotifyContext(result, pkgURI, alertType,
                        serverID, correlator);
                processMsg(pkg);
                break;
            }
            case DMIntent.TYPE_CLIENT_SESSION_REQUEST: {
                if (DBG) logd("Client initiated dm session was received.");

                DMSessionPkg pkg = new DMSessionPkg(intentType, requestID);
                String serverID = intent.getStringExtra(DMIntent.FIELD_SERVERID);
                int timer = intent.getIntExtra(DMIntent.FIELD_TIMER, 0);

                // XXXXX FIXME this should not be here!
                synchronized (this) {
                    try {
                        if (DBG) logd("Timeout: " + timer);
                        if (timer > 0) {
                            wait(timer * 1000);
                        }
                    } catch (InterruptedException e) {
                        if (DBG) logd("Waiting has been interrupted.");
                    }
                }
                if (DBG) logd("Starting session.");

                if (serverID != null && !serverID.isEmpty()) {
                    pkg.mobj = serverID;
                    processMsg(pkg);
                }
                break;
            }
            case DMIntent.TYPE_CANCEL_DM_SESSION: {
                cancelSession(requestID);
                processMsg(new DMSessionPkg(DMIntent.TYPE_DO_NOTHING, requestID));
                break;
            }
            case DMIntent.TYPE_LAWMO_NOTIFY_SESSION: {
                if (DBG) logd("LAWMO Notify DM Session was received");

                DMSessionPkg pkg = new DMSessionPkg(intentType, requestID);

                String result = intent.getStringExtra(DMIntent.FIELD_LAWMO_RESULT);
                String pkgURI = intent.getStringExtra(DMIntent.FIELD_PKGURI);
                String alertType = intent.getStringExtra(DMIntent.FIELD_ALERTTYPE);
                String correlator = intent.getStringExtra(DMIntent.FIELD_CORR);

                pkg.mobj = new FotaNotifyContext(result, pkgURI, alertType, null, correlator);
                processMsg(pkg);
                break;
            }
        }
    }

    public int deleteNode(String node) {
        if (mInitGood) {
            return NativeDM.deleteNode(node);
        }
        return DMResult.SYNCML_DM_FAIL;
    }

    public int createInterior(String node) {
        if (mInitGood) {
            return NativeDM.createInterior(node);
        }
        return DMResult.SYNCML_DM_FAIL;
    }

    public int createLeaf(String node, String value) {
        if (mInitGood) {
            return NativeDM.createLeaf(node, value);
        }
        return DMResult.SYNCML_DM_FAIL;
    }

    public String getNodeInfoSP(String node) {
        if (mInitGood) {
            return NativeDM.getNodeInfo(node);
        }
        return null;
    }

    // This is the object that receives interactions from clients. See
    // RemoteService for a more complete example.
    private final IBinder mBinder = new LocalBinder();

    @Override
    public IBinder onBind(Intent arg0) {
        if (DBG) logd("entering onBind()");
        DMConfigureDB db = getConfigDB();   // wait for configure DB to initialize
        if (DBG) logd("returning mBinder");
        return mBinder;
    }

    /**
     * Get the {@code DMConfigureDB} object from the AsyncTask, waiting up to 70 seconds.
     * @return the {@code DMConfigureDB} object, or null if the AsyncTask failed
     */
    public DMConfigureDB getConfigDB() {
        try {
            return mDMConfigureTask.get(70, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            loge("onBind() got InterruptedException waiting for config DB", e);
        } catch (ExecutionException e) {
            loge("onBind() got ExecutionException waiting for config DB", e);
        } catch (TimeoutException e) {
            loge("onBind() got TimeoutException waiting for config DB", e);
        }
        return null;
    }

    String parseBootstrapServerId(byte[] data, boolean isWbxml) {
        String retServerId = NativeDM.parseBootstrapServerId(data, isWbxml);
        if (DBG) logd("parseBootstrapServerId retServerId=" + retServerId);

        if (DBG) {  // dump data for debug
            int logLevel = getConfigDB().getSyncMLLogLevel();
            if (logLevel > 0) {
                try {
                    // FIXME SECURITY: don't open file as world writeable, WTF!
                    FileOutputStream os = openFileOutput("syncml_" + System.currentTimeMillis()
                            + ".dump", MODE_WORLD_WRITEABLE);
                    os.write(data);
                    os.close();
                    logd("xml/wbxml file saved to "
                            + getApplication().getFilesDir().getAbsolutePath());

                    if (isWbxml && logLevel == 2) {
                        byte[] xml = NativeDM.nativeWbxmlToXml(data);
                        if (xml != null) {
                            // FIXME SECURITY: don't open file as world writeable, WTF!
                            FileOutputStream xmlos = openFileOutput("syncml_"
                                    + System.currentTimeMillis() + ".xml", MODE_WORLD_WRITEABLE);
                            xmlos.write(xml);
                            xmlos.close();
                            logd("wbxml2xml converted successful and saved to file");
                        }
                    }
                }
                catch (FileNotFoundException e) {
                    logd("unable to open file for wbxml, e=" + e.toString());
                }
                catch (IOException e) {
                    logd("unable to write to wbxml file, e=" + e.toString());
                }
                catch(Exception e) {
                    loge("Unexpected exception converting wbxml to xml, e=" + e.toString());
                }
            }
        }
        return retServerId;
    }

    private static int processBootstrapScript(byte[] data, boolean isWbxml, String serverId) {
        int retcode = NativeDM.processBootstrapScript(data, isWbxml, serverId);
        if (DBG) logd("processBootstrapScript retcode=" + retcode);
        return retcode;
    }

    private Runnable mAbortSession = new Runnable() {
        @Override
        public void run() {
            cancelSession(0);
        }
    };

    // FIXME: only used from SessionThread inner class
    private void startTimeOutTick(long delayTime) {
        synchronized (mSessionTimeOutHandler) {
            mSessionTimeOutHandler.removeCallbacks(mAbortSession);
            mSessionTimeOutHandler.postDelayed(mAbortSession, delayTime);
        }
    }

    private static boolean copyFile(InputStream in, File to) {
        try {
            if (!to.exists()) {
                to.createNewFile();
            }
            OutputStream out = new FileOutputStream(to);
            byte[] buf = new byte[1024];
            int len;
            while ((len = in.read(buf)) > 0) {
                out.write(buf, 0, len);
            }
            out.close();
        } catch (IOException e) {
            loge("Error: copyFile exception", e);
            return false;
        }
        return true;
    }

    /**
     * Copy files from assets folder.
     * @return true on success; false on any failure
     */
    private boolean copyFilesFromAssets() {
        // Check files in assets folder
        String strDes = getFilesDir().getAbsolutePath() + "/dm";
        logd("Directory is: " + strDes);
        File dirDes = new File(strDes);
        if (dirDes.exists() && dirDes.isDirectory()) {
            logd("Predefined files already created: " + strDes);
            return true;
        }
        logd("Predefined files not created: " + strDes);
        if (!dirDes.mkdir()) {
            logd("Failed to create dir: " + dirDes.getAbsolutePath());
            return false;
        }
        // Create log directory.
        File dirLog = new File(dirDes, "log");
        // FIXME: don't ignore return value
        dirLog.mkdir();
        if (DBG) logd("read assets");
        try {
            AssetManager am = getAssets();
            String[] arrRoot = am.list("dm");
            int cnt = arrRoot.length;
            if (DBG) logd("assets count: " + cnt);
            for (int i = 0; i < cnt; i++) {
                if (DBG) logd("Root No. " + i + ':' + arrRoot[i]);
                File dir2 = new File(dirDes, arrRoot[i]);
                if (!dir2.mkdir()) {
                    // FIXME: don't ignore return value
                    dirDes.delete();
                    return false;
                }
                String[] arrSub = am.list("dm/" + arrRoot[i]);
                int cntSub = arrSub.length;
                if (DBG) logd(arrRoot[i] + " has " + cntSub + " items");
                if (cntSub > 0) {
                    for (int j = 0; j < cntSub; j++) {
                        if (DBG) logd("Sub No. " + j + ':' + arrSub[j]);
                        File to2 = new File(dir2, arrSub[j]);
                        String strFrom = "dm/" + arrRoot[i] + '/' + arrSub[j];
                        InputStream in2 = am.open(strFrom);
                        if (!copyFile(in2, to2)) {
                            // FIXME: don't ignore return value
                            dirDes.delete();
                            return false;
                        }
                    }
                }
            }
        } catch (IOException e) {
            loge("error copying file from assets", e);
            return false;
        }
        return true;
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
