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

package com.android.managedprovisioning;

import android.os.Parcel;
import android.os.Parcelable;
import android.os.PersistableBundle;
import android.util.Base64;
import java.util.Locale;


/**
 * Provisioning Parameters for DeviceOwner Provisioning
 */
public class ProvisioningParams implements Parcelable {
    public static final long DEFAULT_LOCAL_TIME = -1;
    public static final boolean DEFAULT_WIFI_HIDDEN = false;
    public static final boolean DEFAULT_LEAVE_ALL_SYSTEM_APPS_ENABLED = false;
    public static final int DEFAULT_WIFI_PROXY_PORT = 0;

    public String mTimeZone;
    public long mLocalTime = DEFAULT_LOCAL_TIME;
    public Locale mLocale;

    public String mWifiSsid;
    public boolean mWifiHidden = DEFAULT_WIFI_HIDDEN;
    public String mWifiSecurityType;
    public String mWifiPassword;
    public String mWifiProxyHost;
    public int mWifiProxyPort = DEFAULT_WIFI_PROXY_PORT;
    public String mWifiProxyBypassHosts;
    public String mWifiPacUrl;

    public String mDeviceAdminPackageName; // Package name of the device admin package.

    public String mDeviceAdminPackageDownloadLocation; // Url of the device admin .apk
    public String mDeviceAdminPackageDownloadCookieHeader; // Cookie header for http request
    public byte[] mDeviceAdminPackageChecksum = new byte[0]; // SHA-1 sum of the .apk file.

    public PersistableBundle mAdminExtrasBundle;

    public boolean mStartedByNfc; // True iff provisioning flow was started by Nfc bump.

    public boolean mLeaveAllSystemAppsEnabled;

    public String getLocaleAsString() {
        if (mLocale != null) {
            return mLocale.getLanguage() + "_" + mLocale.getCountry();
        } else {
            return null;
        }
    }

    public String getDeviceAdminPackageChecksumAsString() {
        return Base64.encodeToString(mDeviceAdminPackageChecksum,
                Base64.URL_SAFE | Base64.NO_PADDING | Base64.NO_WRAP);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        out.writeString(mTimeZone);
        out.writeLong(mLocalTime);
        out.writeSerializable(mLocale);
        out.writeString(mWifiSsid);
        out.writeInt(mWifiHidden ? 1 : 0);
        out.writeString(mWifiSecurityType);
        out.writeString(mWifiPassword);
        out.writeString(mWifiProxyHost);
        out.writeInt(mWifiProxyPort);
        out.writeString(mWifiProxyBypassHosts);
        out.writeString(mDeviceAdminPackageName);
        out.writeString(mDeviceAdminPackageDownloadLocation);
        out.writeString(mDeviceAdminPackageDownloadCookieHeader);
        out.writeInt(mDeviceAdminPackageChecksum.length);
        out.writeByteArray(mDeviceAdminPackageChecksum);
        out.writeParcelable(mAdminExtrasBundle, 0 /* default */);
        out.writeInt(mStartedByNfc ? 1 : 0);
        out.writeInt(mLeaveAllSystemAppsEnabled ? 1 : 0);
    }

    public static final Parcelable.Creator<ProvisioningParams> CREATOR
        = new Parcelable.Creator<ProvisioningParams>() {
        @Override
        public ProvisioningParams createFromParcel(Parcel in) {
            ProvisioningParams params = new ProvisioningParams();
            params.mTimeZone = in.readString();
            params.mLocalTime = in.readLong();
            params.mLocale = (Locale) in.readSerializable();
            params.mWifiSsid = in.readString();
            params.mWifiHidden = in.readInt() == 1;
            params.mWifiSecurityType = in.readString();
            params.mWifiPassword = in.readString();
            params.mWifiProxyHost = in.readString();
            params.mWifiProxyPort = in.readInt();
            params.mWifiProxyBypassHosts = in.readString();
            params.mDeviceAdminPackageName = in.readString();
            params.mDeviceAdminPackageDownloadLocation = in.readString();
            params.mDeviceAdminPackageDownloadCookieHeader = in.readString();
            int checksumLength = in.readInt();
            params.mDeviceAdminPackageChecksum = new byte[checksumLength];
            in.readByteArray(params.mDeviceAdminPackageChecksum);
            params.mAdminExtrasBundle = in.readParcelable(null /* use default classloader */);
            params.mStartedByNfc = in.readInt() == 1;
            params.mLeaveAllSystemAppsEnabled = in.readInt() == 1;
            return params;
        }

        @Override
        public ProvisioningParams[] newArray(int size) {
            return new ProvisioningParams[size];
        }
    };
}
