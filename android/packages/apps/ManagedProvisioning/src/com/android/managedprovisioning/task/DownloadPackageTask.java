/*
 * Copyright 2014, The Android Open Source Project
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
package com.android.managedprovisioning.task;

import android.app.DownloadManager;
import android.app.DownloadManager.Query;
import android.app.DownloadManager.Request;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Base64;

import com.android.managedprovisioning.ProvisionLogger;

import java.io.InputStream;
import java.io.IOException;
import java.io.FileInputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;

/**
 * Downloads a given file and checks whether its hash matches a given hash to verify that the
 * intended file was downloaded.
 */
public class DownloadPackageTask {
    public static final int ERROR_HASH_MISMATCH = 0;
    public static final int ERROR_DOWNLOAD_FAILED = 1;
    public static final int ERROR_OTHER = 2;

    private static final String HASH_TYPE = "SHA-1";

    private final Context mContext;
    private final String mDownloadLocationFrom;
    private final Callback mCallback;
    private final byte[] mHash;
    private final String mHttpCookieHeader;

    private boolean mDoneDownloading;
    private String mDownloadLocationTo;
    private long mDownloadId;
    private BroadcastReceiver mReceiver;

    public DownloadPackageTask (Context context, String downloadLocation, byte[] hash,
            String httpCookieHeader, Callback callback) {
        mCallback = callback;
        mContext = context;
        mDownloadLocationFrom = downloadLocation;
        mHash = hash;
        mHttpCookieHeader = httpCookieHeader;
        mDoneDownloading = false;
    }

    public boolean downloadLocationWasProvided() {
        return !TextUtils.isEmpty(mDownloadLocationFrom);
    }

    public void run() {
        mReceiver = createDownloadReceiver();
        mContext.registerReceiver(mReceiver,
                new IntentFilter(DownloadManager.ACTION_DOWNLOAD_COMPLETE));

        ProvisionLogger.logd("Starting download from " + mDownloadLocationFrom);
        DownloadManager dm = (DownloadManager) mContext
                .getSystemService(Context.DOWNLOAD_SERVICE);
        Request request = new Request(Uri.parse(mDownloadLocationFrom));
        if (mHttpCookieHeader != null) {
            request.addRequestHeader("Cookie", mHttpCookieHeader);
            ProvisionLogger.logd("Downloading with http cookie header: " + mHttpCookieHeader);
        }
        mDownloadId = dm.enqueue(request);
    }

    private BroadcastReceiver createDownloadReceiver() {
        return new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (DownloadManager.ACTION_DOWNLOAD_COMPLETE.equals(intent.getAction())) {
                    Query q = new Query();
                    q.setFilterById(mDownloadId);
                    DownloadManager dm = (DownloadManager) mContext
                            .getSystemService(Context.DOWNLOAD_SERVICE);
                    Cursor c = dm.query(q);
                    if (c.moveToFirst()) {
                        int columnIndex = c.getColumnIndex(DownloadManager.COLUMN_STATUS);
                        if (DownloadManager.STATUS_SUCCESSFUL == c.getInt(columnIndex)) {
                            String location = c.getString(
                                    c.getColumnIndex(DownloadManager.COLUMN_LOCAL_FILENAME));
                            c.close();
                            onDownloadSuccess(location);
                        } else if (DownloadManager.STATUS_FAILED == c.getInt(columnIndex)){
                            int reason = c.getColumnIndex(DownloadManager.COLUMN_REASON);
                            c.close();
                            onDownloadFail(reason);
                        }
                    }
                }
            }
        };
    }

    private void onDownloadSuccess(String location) {
        if (mDoneDownloading) {
            // DownloadManager can send success more than once. Only act first time.
            return;
        } else {
            mDoneDownloading = true;
        }

        ProvisionLogger.logd("Downloaded succesfully to: " + location);

        // Check whether hash of downloaded file matches hash given in constructor.
        byte[] hash = computeHash(location);
        if (hash == null) {

            // Error should have been reported in computeHash().
            return;
        }

        if (Arrays.equals(mHash, hash)) {
            ProvisionLogger.logd(HASH_TYPE + "-hashes matched, both are "
                    + byteArrayToString(hash));
            mDownloadLocationTo = location;
            mCallback.onSuccess();
        } else {
            ProvisionLogger.loge(HASH_TYPE + "-hash of downloaded file does not match given hash.");
            ProvisionLogger.loge(HASH_TYPE + "-hash of downloaded file: "
                    + byteArrayToString(hash));
            ProvisionLogger.loge(HASH_TYPE + "-hash provided by programmer: "
                    + byteArrayToString(mHash));

            mCallback.onError(ERROR_HASH_MISMATCH);
        }
    }

    private void onDownloadFail(int errorCode) {
        ProvisionLogger.loge("Downloading package failed.");
        ProvisionLogger.loge("COLUMN_REASON in DownloadManager response has value: "
                + errorCode);
        mCallback.onError(ERROR_DOWNLOAD_FAILED);
    }

    private byte[] computeHash(String fileLocation) {
        InputStream fis = null;
        MessageDigest md;
        byte hash[] = null;
        try {
            md = MessageDigest.getInstance(HASH_TYPE);
        } catch (NoSuchAlgorithmException e) {
            ProvisionLogger.loge("Hashing algorithm " + HASH_TYPE + " not supported.", e);
            mCallback.onError(ERROR_OTHER);
            return null;
        }
        try {
            fis = new FileInputStream(fileLocation);

            byte[] buffer = new byte[256];
            int n = 0;
            while (n != -1) {
                n = fis.read(buffer);
                if (n > 0) {
                    md.update(buffer, 0, n);
                }
            }
            hash = md.digest();
        } catch (IOException e) {
            ProvisionLogger.loge("IO error.", e);
            mCallback.onError(ERROR_OTHER);
        } finally {
            // Close input stream quietly.
            try {
                if (fis != null) {
                    fis.close();
                }
            } catch (IOException e) {
                // Ignore.
            }
        }
        return hash;
    }

    public String getDownloadedPackageLocation() {
        return mDownloadLocationTo;
    }

    public void cleanUp() {
        if (mReceiver != null) {
            //Unregister receiver.
            mContext.unregisterReceiver(mReceiver);
            mReceiver = null;
        }

        //Remove download.
        DownloadManager dm = (DownloadManager) mContext
                .getSystemService(Context.DOWNLOAD_SERVICE);
        boolean removeSuccess = dm.remove(mDownloadId) == 1;
        if (removeSuccess) {
            ProvisionLogger.logd("Successfully removed the device owner installer file.");
        } else {
            ProvisionLogger.loge("Could not remove the device owner installer file.");
            // Ignore this error. Failing cleanup should not stop provisioning flow.
        }
    }

    // For logging purposes only.
    String byteArrayToString(byte[] ba) {
        return Base64.encodeToString(ba, Base64.URL_SAFE | Base64.NO_PADDING | Base64.NO_WRAP);
    }

    public abstract static class Callback {
        public abstract void onSuccess();
        public abstract void onError(int errorCode);
    }
}
