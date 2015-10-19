/*
 * Copyright 2014, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.managedprovisioning.task;

import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.text.TextUtils;

import com.android.managedprovisioning.ProvisionLogger;

public class SetDevicePolicyTask {
    public static final int ERROR_PACKAGE_NOT_INSTALLED = 0;
    public static final int ERROR_NO_RECEIVER = 1;
    public static final int ERROR_OTHER = 2;

    private final Callback mCallback;
    private final Context mContext;
    private final String mPackageName;
    private final String mOwner;

    private String mAdminReceiver;
    private PackageManager mPackageManager;
    private DevicePolicyManager mDevicePolicyManager;

    public SetDevicePolicyTask(Context context, String packageName, String owner,
            Callback callback) {
        mCallback = callback;
        mContext = context;
        mPackageName = packageName;
        mOwner = owner;
        mPackageManager = mContext.getPackageManager();
        mDevicePolicyManager = (DevicePolicyManager) mContext.
                getSystemService(Context.DEVICE_POLICY_SERVICE);
    }

    public void run() {
        // Check whether package is installed and find the admin receiver.
        if (isPackageInstalled()) {
            enableDevicePolicyApp();
            setActiveAdmin();
            setDeviceOwner();
            mCallback.onSuccess();
        }
    }

    private boolean isPackageInstalled() {
        try {
            PackageInfo pi = mPackageManager.getPackageInfo(mPackageName,
                    PackageManager.GET_RECEIVERS);
            for (ActivityInfo ai : pi.receivers) {
                if (!TextUtils.isEmpty(ai.permission) &&
                        ai.permission.equals(android.Manifest.permission.BIND_DEVICE_ADMIN)) {
                    mAdminReceiver = ai.name;
                    return true;
                }
            }
            mCallback.onError(ERROR_NO_RECEIVER);
            return false;
        } catch (NameNotFoundException e) {
            mCallback.onError(ERROR_PACKAGE_NOT_INSTALLED);
            return false;
        }
    }

    private void enableDevicePolicyApp() {
        int enabledSetting = mPackageManager
                .getApplicationEnabledSetting(mPackageName);
        if (enabledSetting != PackageManager.COMPONENT_ENABLED_STATE_DEFAULT) {
            mPackageManager.setApplicationEnabledSetting(mPackageName,
                    PackageManager.COMPONENT_ENABLED_STATE_DEFAULT, 0);
        }
    }

    public void setActiveAdmin() {
        ProvisionLogger.logd("Setting " + mPackageName + " as active admin.");
        ComponentName component = new ComponentName(mPackageName, mAdminReceiver);
        mDevicePolicyManager.setActiveAdmin(component, true);
    }

    public void setDeviceOwner() {
        ProvisionLogger.logd("Setting " + mPackageName + " as device owner " + mOwner + ".");
        if (!mDevicePolicyManager.isDeviceOwner(mPackageName)) {
            mDevicePolicyManager.setDeviceOwner(mPackageName, mOwner);
        }
    }

    public abstract static class Callback {
        public abstract void onSuccess();
        public abstract void onError(int errorCode);
    }
}
