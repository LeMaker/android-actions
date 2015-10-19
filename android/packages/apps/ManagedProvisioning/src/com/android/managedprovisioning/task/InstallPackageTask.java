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
import android.content.Context;
import android.content.pm.IPackageInstallObserver;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.provider.Settings.Global;
import android.text.TextUtils;
import android.Manifest.permission;

import com.android.managedprovisioning.ProvisionLogger;

import java.io.File;

/**
 * Installs a device owner package from a given path.
 * <p>
 * Before installing it is checked whether the file at the specified path contains the given package
 * and the given admin receiver.
 * </p>
 */
public class InstallPackageTask {
    public static final int ERROR_PACKAGE_INVALID = 0;
    public static final int ERROR_INSTALLATION_FAILED = 1;

    private final Context mContext;
    private final Callback mCallback;
    private final String mPackageName;

    private String mPackageLocation;
    private PackageManager mPm;
    private int mPackageVerifierEnable;

    public InstallPackageTask (Context context, String packageName,
            Callback callback) {
        mCallback = callback;
        mContext = context;
        mPackageLocation = null; // Initialized in run().
        mPackageName = packageName;
    }

    public void run(String packageLocation) {
        if (TextUtils.isEmpty(packageLocation)) {
            ProvisionLogger.loge("Package Location is empty.");
            mCallback.onError(ERROR_PACKAGE_INVALID);
            return;
        }
        mPackageLocation = packageLocation;

        PackageInstallObserver observer = new PackageInstallObserver();
        mPm = mContext.getPackageManager();

        if (packageContentIsCorrect()) {
            // Temporarily turn off package verification.
            mPackageVerifierEnable = Global.getInt(mContext.getContentResolver(),
                    Global.PACKAGE_VERIFIER_ENABLE, 1);
            Global.putInt(mContext.getContentResolver(), Global.PACKAGE_VERIFIER_ENABLE, 0);

            Uri packageUri = Uri.parse("file://" + mPackageLocation);

            // Allow for replacing an existing package.
            // Needed in case this task is performed multiple times.
            mPm.installPackage(packageUri, observer,
                    /* flags */ PackageManager.INSTALL_REPLACE_EXISTING, mContext.getPackageName());
        } else {
            // Error should have been reported in packageContentIsCorrect().
            return;
        }
    }

    private boolean packageContentIsCorrect() {
        PackageInfo pi = mPm.getPackageArchiveInfo(mPackageLocation,
                PackageManager.GET_RECEIVERS);
        if (pi == null) {
            ProvisionLogger.loge("Package could not be parsed successfully.");
            mCallback.onError(ERROR_PACKAGE_INVALID);
            return false;
        }
        if (!pi.packageName.equals(mPackageName)) {
            ProvisionLogger.loge("Package name in apk (" + pi.packageName
                    + ") does not match package name specified by programmer ("
                    + mPackageName + ").");
            mCallback.onError(ERROR_PACKAGE_INVALID);
            return false;
        }
        for (ActivityInfo ai : pi.receivers) {
            if (!TextUtils.isEmpty(ai.permission) &&
                    ai.permission.equals(android.Manifest.permission.BIND_DEVICE_ADMIN)) {
                return true;
            }
        }
        ProvisionLogger.loge("Installed package has no admin receiver.");
        mCallback.onError(ERROR_PACKAGE_INVALID);
        return false;
    }

    private class PackageInstallObserver extends IPackageInstallObserver.Stub {
        @Override
        public void packageInstalled(String packageName, int returnCode) {
            // Set package verification flag to its original value.
            Global.putInt(mContext.getContentResolver(), Global.PACKAGE_VERIFIER_ENABLE,
                    mPackageVerifierEnable);

            if (returnCode == PackageManager.INSTALL_SUCCEEDED
                    && mPackageName.equals(packageName)) {
                ProvisionLogger.logd("Package " + mPackageName + " is succesfully installed.");

                mCallback.onSuccess();
            } else {
                if (returnCode == PackageManager.INSTALL_FAILED_VERSION_DOWNGRADE) {
                    ProvisionLogger.logd("Current version of " + mPackageName
                            + " higher than the version to be installed.");
                    ProvisionLogger.logd("Package " + mPackageName + " was not reinstalled.");
                    mCallback.onSuccess();
                    return;
                }

                ProvisionLogger.logd("Installing package " + mPackageName + " failed.");
                ProvisionLogger.logd("Errorcode returned by IPackageInstallObserver = "
                        + returnCode);
                mCallback.onError(ERROR_INSTALLATION_FAILED);
            }
        }
    }

    public abstract static class Callback {
        public abstract void onSuccess();
        public abstract void onError(int errorCode);
    }
}