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

package com.android.tv.settings.device.apps;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.drawable.Drawable;
import android.text.format.Formatter;

/**
 * Contains all the info necessary to manage an application.
 */
class AppInfo {

    private final Object mLock = new Object();
    private final Context mContext;
    private ApplicationsState.AppEntry mEntry;

    AppInfo(Context context, ApplicationsState.AppEntry entry) {
        mContext = context;
        mEntry = entry;
    }

    void setEntry(ApplicationsState.AppEntry entry) {
        synchronized (mLock) {
            mEntry = entry;
        }
    }

    String getName() {
        synchronized (mLock) {
            mEntry.ensureLabel(mContext);
            return mEntry.label;
        }
    }

    String getSize() {
        synchronized (mLock) {
            return mEntry.sizeStr;
        }
    }

    int getIconResource() {
        synchronized (mLock) {
            return mEntry.info.icon;
        }
    }

    String getPackageName() {
        synchronized (mLock) {
            return mEntry.info.packageName;
        }
    }

    ApplicationInfo getApplicationInfo() {
        synchronized (mLock) {
            return mEntry.info;
        }
    }

    boolean isStopped() {
        synchronized (mLock) {
            return (mEntry.info.flags & ApplicationInfo.FLAG_STOPPED) != 0;
        }
    }

    boolean isInstalled() {
        synchronized (mLock) {
            return (mEntry.info.flags & ApplicationInfo.FLAG_INSTALLED) != 0;
        }
    }

    boolean isUpdatedSystemApp() {
        synchronized (mLock) {
            return (mEntry.info.flags & ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) != 0;
        }
    }

    boolean isEnabled() {
        synchronized (mLock) {
            return mEntry.info.enabled;
        }
    }

    boolean isSystemApp() {
        synchronized (mLock) {
            return (mEntry.info.flags & ApplicationInfo.FLAG_SYSTEM) != 0;
        }
    }

    String getCacheSize() {
        synchronized (mLock) {
            return Formatter.formatFileSize(mContext, mEntry.cacheSize + mEntry.externalCacheSize);
        }
    }

    String getDataSize() {
        synchronized (mLock) {
            return Formatter.formatFileSize(mContext, mEntry.dataSize + mEntry.externalDataSize);
        }
    }

    String getSpaceManagerActivityName() {
        synchronized (mLock) {
            return mEntry.info.manageSpaceActivityName;
        }
    }

    int getUid() {
        synchronized (mLock) {
            return mEntry.info.uid;
        }
    }

    String getVersion() {
        synchronized (mLock) {
            return mEntry.getVersion(mContext);
        }
    }

    @Override
    public String toString() {
        return getName();
    }
}
