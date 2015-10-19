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

import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_TIME_ZONE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_LOCAL_TIME;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_LOCALE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_WIFI_SSID;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_WIFI_HIDDEN;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_WIFI_SECURITY_TYPE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_WIFI_PASSWORD;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_WIFI_PROXY_HOST;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_WIFI_PROXY_PORT;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_WIFI_PROXY_BYPASS;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_WIFI_PAC_URL;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_NAME;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_LOCATION;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_COOKIE_HEADER;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_CHECKSUM;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED;
import static android.app.admin.DevicePolicyManager.MIME_TYPE_PROVISIONING_NFC;
import static java.nio.charset.StandardCharsets.UTF_8;

import android.content.Intent;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.NfcAdapter;
import android.os.Bundle;
import android.os.Parcelable;
import android.os.PersistableBundle;
import android.text.TextUtils;
import android.util.Base64;

import java.io.IOException;
import java.io.StringReader;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.IllformedLocaleException;
import java.util.Locale;
import java.util.Properties;

/**
 * This class can initialize a {@link ProvisioningParams} object from an intent.
 * A {@link ProvisioningParams} object stores various parameters for the device owner provisioning.
 * There are two kinds of intents that can be parsed it into {@link ProvisioningParams}:
 *
 * <p>
 * Intent was received via Nfc.
 * The intent contains the extra {@link NfcAdapter.EXTRA_NDEF_MESSAGES}, which indicates that
 * provisioning was started via Nfc bump. This extra contains an NDEF message, which contains an
 * NfcRecord with mime type {@link MIME_TYPE_PROVISIONING_NFC}. This record stores a serialized
 * properties object, which contains the serialized extra's described in the next option.
 * A typical use case would be a programmer application that sends an Nfc bump to start Nfc
 * provisioning from a programmer device.
 *
 * <p>
 * Intent was received directly.
 * The intent contains the extra {@link #EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_NAME},
 * and may contain {@link #EXTRA_PROVISIONING_TIME_ZONE},
 * {@link #EXTRA_PROVISIONING_LOCAL_TIME}, and {@link #EXTRA_PROVISIONING_LOCALE}. A download
 * location may be specified in {@link #EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_LOCATION}
 * together with an optional http cookie header
 * {@link #EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_COOKIE_HEADER} accompanied by the SHA-1
 * sum of the target file {@link #EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_CHECKSUM}.
 * Additional information to send through to the device admin may be specified in
 * {@link #EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE}.
 * The boolean {@link #EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED} indicates wheter system
 * apps should not be disabled.
 * Furthermore a wifi network may be specified in {@link #EXTRA_PROVISIONING_WIFI_SSID}, and if
 * applicable {@link #EXTRA_PROVISIONING_WIFI_HIDDEN},
 * {@link #EXTRA_PROVISIONING_WIFI_SECURITY_TYPE}, {@link #EXTRA_PROVISIONING_WIFI_PASSWORD},
 * {@link #EXTRA_PROVISIONING_WIFI_PROXY_HOST}, {@link #EXTRA_PROVISIONING_WIFI_PROXY_PORT},
 * {@link #EXTRA_PROVISIONING_WIFI_PROXY_BYPASS}.
 * A typical use case would be the {@link BootReminder} sending the intent after device encryption
 * and reboot.
 *
 * <p>
 * Furthermore this class can construct the bundle of extras for the second kind of intent given a
 * {@link ProvisioningParams}, and it keeps track of the types of the extras in the
 * DEVICE_OWNER_x_EXTRAS, with x the appropriate type.
 */
public class MessageParser {
    private static final String EXTRA_PROVISIONING_STARTED_BY_NFC  =
            "com.android.managedprovisioning.extra.started_by_nfc";

    protected static final String[] DEVICE_OWNER_STRING_EXTRAS = {
        EXTRA_PROVISIONING_TIME_ZONE,
        EXTRA_PROVISIONING_LOCALE,
        EXTRA_PROVISIONING_WIFI_SSID,
        EXTRA_PROVISIONING_WIFI_SECURITY_TYPE,
        EXTRA_PROVISIONING_WIFI_PASSWORD,
        EXTRA_PROVISIONING_WIFI_PROXY_HOST,
        EXTRA_PROVISIONING_WIFI_PROXY_BYPASS,
        EXTRA_PROVISIONING_WIFI_PAC_URL,
        EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_NAME,
        EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_LOCATION,
        EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_COOKIE_HEADER,
        EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_CHECKSUM
    };

    protected static final String[] DEVICE_OWNER_LONG_EXTRAS = {
        EXTRA_PROVISIONING_LOCAL_TIME
    };

    protected static final String[] DEVICE_OWNER_INT_EXTRAS = {
        EXTRA_PROVISIONING_WIFI_PROXY_PORT
    };

    protected static final String[] DEVICE_OWNER_BOOLEAN_EXTRAS = {
        EXTRA_PROVISIONING_WIFI_HIDDEN,
        EXTRA_PROVISIONING_STARTED_BY_NFC,
        EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED
    };

    protected static final String[] DEVICE_OWNER_PERSISTABLE_BUNDLE_EXTRAS = {
        EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE
    };

    public void addProvisioningParamsToBundle(Bundle bundle, ProvisioningParams params) {
        bundle.putString(EXTRA_PROVISIONING_TIME_ZONE, params.mTimeZone);
        bundle.putString(EXTRA_PROVISIONING_LOCALE, params.getLocaleAsString());
        bundle.putString(EXTRA_PROVISIONING_WIFI_SSID, params.mWifiSsid);
        bundle.putString(EXTRA_PROVISIONING_WIFI_SECURITY_TYPE, params.mWifiSecurityType);
        bundle.putString(EXTRA_PROVISIONING_WIFI_PASSWORD, params.mWifiPassword);
        bundle.putString(EXTRA_PROVISIONING_WIFI_PROXY_HOST, params.mWifiProxyHost);
        bundle.putString(EXTRA_PROVISIONING_WIFI_PROXY_BYPASS, params.mWifiProxyBypassHosts);
        bundle.putString(EXTRA_PROVISIONING_WIFI_PAC_URL, params.mWifiPacUrl);
        bundle.putString(EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_NAME,
                params.mDeviceAdminPackageName);
        bundle.putString(EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_LOCATION,
                params.mDeviceAdminPackageDownloadLocation);
        bundle.putString(EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_COOKIE_HEADER,
                params.mDeviceAdminPackageDownloadCookieHeader);
        bundle.putString(EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_CHECKSUM,
                params.getDeviceAdminPackageChecksumAsString());

        bundle.putLong(EXTRA_PROVISIONING_LOCAL_TIME, params.mLocalTime);

        bundle.putInt(EXTRA_PROVISIONING_WIFI_PROXY_PORT, params.mWifiProxyPort);

        bundle.putBoolean(EXTRA_PROVISIONING_WIFI_HIDDEN, params.mWifiHidden);
        bundle.putBoolean(EXTRA_PROVISIONING_STARTED_BY_NFC, params.mStartedByNfc);
        bundle.putBoolean(EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED,
                params.mLeaveAllSystemAppsEnabled);

        bundle.putParcelable(EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE, params.mAdminExtrasBundle);
    }

    public ProvisioningParams parseIntent(Intent intent) throws ParseException {
        ProvisionLogger.logi("Processing intent.");
        if (intent.hasExtra(NfcAdapter.EXTRA_NDEF_MESSAGES)) {
            return parseNfcIntent(intent);
        } else {
            return parseNonNfcIntent(intent);
        }
    }

    public ProvisioningParams parseNfcIntent(Intent nfcIntent)
        throws ParseException {
        ProvisionLogger.logi("Processing Nfc Payload.");
        // Only one first message with NFC_MIME_TYPE is used.
        for (Parcelable rawMsg : nfcIntent
                     .getParcelableArrayExtra(NfcAdapter.EXTRA_NDEF_MESSAGES)) {
            NdefMessage msg = (NdefMessage) rawMsg;

            // Assume only first record of message is used.
            NdefRecord firstRecord = msg.getRecords()[0];
            String mimeType = new String(firstRecord.getType(), UTF_8);

            if (MIME_TYPE_PROVISIONING_NFC.equals(mimeType)) {
                ProvisioningParams params = parseProperties(new String(firstRecord.getPayload()
                                , UTF_8));
                params.mStartedByNfc = true;
                return params;
            }
        }
        throw new ParseException(
                "Intent does not contain NfcRecord with the correct MIME type.",
                R.string.device_owner_error_general);
    }

    // Note: You can't include the EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE in the properties that is
    // send over Nfc, because there is no publicly documented conversion from PersistableBundle to
    // String.
    private ProvisioningParams parseProperties(String data)
            throws ParseException {
        ProvisioningParams params = new ProvisioningParams();
        try {
            Properties props = new Properties();
            props.load(new StringReader(data));

            String s; // Used for parsing non-Strings.
            params.mTimeZone
                    = props.getProperty(EXTRA_PROVISIONING_TIME_ZONE);
            if ((s = props.getProperty(EXTRA_PROVISIONING_LOCALE)) != null) {
                params.mLocale = stringToLocale(s);
            }
            params.mWifiSsid = props.getProperty(EXTRA_PROVISIONING_WIFI_SSID);
            params.mWifiSecurityType = props.getProperty(EXTRA_PROVISIONING_WIFI_SECURITY_TYPE);
            params.mWifiPassword = props.getProperty(EXTRA_PROVISIONING_WIFI_PASSWORD);
            params.mWifiProxyHost = props.getProperty(EXTRA_PROVISIONING_WIFI_PROXY_HOST);
            params.mWifiProxyBypassHosts = props.getProperty(EXTRA_PROVISIONING_WIFI_PROXY_BYPASS);
            params.mWifiPacUrl = props.getProperty(EXTRA_PROVISIONING_WIFI_PAC_URL);
            params.mDeviceAdminPackageName
                    = props.getProperty(EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_NAME);
            params.mDeviceAdminPackageDownloadLocation
                    = props.getProperty(EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_LOCATION);
            params.mDeviceAdminPackageDownloadCookieHeader = props.getProperty(
                    EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_COOKIE_HEADER);
            if ((s = props.getProperty(EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_CHECKSUM)) != null) {
                params.mDeviceAdminPackageChecksum = stringToByteArray(s);
            }

            if ((s = props.getProperty(EXTRA_PROVISIONING_LOCAL_TIME)) != null) {
                params.mLocalTime = Long.parseLong(s);
            }

            if ((s = props.getProperty(EXTRA_PROVISIONING_WIFI_PROXY_PORT)) != null) {
                params.mWifiProxyPort = Integer.parseInt(s);
            }

            if ((s = props.getProperty(EXTRA_PROVISIONING_WIFI_HIDDEN)) != null) {
                params.mWifiHidden = Boolean.parseBoolean(s);
            }

            if ((s = props.getProperty(EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED)) != null) {
                params.mLeaveAllSystemAppsEnabled = Boolean.parseBoolean(s);
            }

            checkValidityOfProvisioningParams(params);
            return params;
        } catch (IOException e) {
            throw new ParseException("Couldn't load payload",
                    R.string.device_owner_error_general, e);
        } catch (NumberFormatException e) {
            throw new ParseException("Incorrect numberformat.",
                    R.string.device_owner_error_general, e);
        } catch (IllformedLocaleException e) {
            throw new ParseException("Invalid locale.",
                    R.string.device_owner_error_general, e);
        }
    }

    public ProvisioningParams parseNonNfcIntent(Intent intent)
        throws ParseException {
        ProvisionLogger.logi("Processing intent.");
        ProvisioningParams params = new ProvisioningParams();

        params.mTimeZone = intent.getStringExtra(EXTRA_PROVISIONING_TIME_ZONE);
        String localeString = intent.getStringExtra(EXTRA_PROVISIONING_LOCALE);
        if (localeString != null) {
            params.mLocale = stringToLocale(localeString);
        }
        params.mWifiSsid = intent.getStringExtra(EXTRA_PROVISIONING_WIFI_SSID);
        params.mWifiSecurityType = intent.getStringExtra(EXTRA_PROVISIONING_WIFI_SECURITY_TYPE);
        params.mWifiPassword = intent.getStringExtra(EXTRA_PROVISIONING_WIFI_PASSWORD);
        params.mWifiProxyHost = intent.getStringExtra(EXTRA_PROVISIONING_WIFI_PROXY_HOST);
        params.mWifiProxyBypassHosts = intent.getStringExtra(EXTRA_PROVISIONING_WIFI_PROXY_BYPASS);
        params.mWifiPacUrl = intent.getStringExtra(EXTRA_PROVISIONING_WIFI_PAC_URL);
        params.mDeviceAdminPackageName
                = intent.getStringExtra(EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_NAME);
        params.mDeviceAdminPackageDownloadLocation
                = intent.getStringExtra(EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_LOCATION);
        params.mDeviceAdminPackageDownloadCookieHeader = intent.getStringExtra(
                EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_COOKIE_HEADER);
        String hashString = intent.getStringExtra(EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_CHECKSUM);
        if (hashString != null) {
            params.mDeviceAdminPackageChecksum = stringToByteArray(hashString);
        }

        params.mLocalTime = intent.getLongExtra(EXTRA_PROVISIONING_LOCAL_TIME,
                ProvisioningParams.DEFAULT_LOCAL_TIME);

        params.mWifiProxyPort = intent.getIntExtra(EXTRA_PROVISIONING_WIFI_PROXY_PORT,
                ProvisioningParams.DEFAULT_WIFI_PROXY_PORT);

        params.mWifiHidden = intent.getBooleanExtra(EXTRA_PROVISIONING_WIFI_HIDDEN,
                ProvisioningParams.DEFAULT_WIFI_HIDDEN);
        params.mStartedByNfc = intent.getBooleanExtra(EXTRA_PROVISIONING_STARTED_BY_NFC,
                false);
        params.mLeaveAllSystemAppsEnabled = intent.getBooleanExtra(
                EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED,
                ProvisioningParams.DEFAULT_LEAVE_ALL_SYSTEM_APPS_ENABLED);

        try {
            params.mAdminExtrasBundle = (PersistableBundle) intent.getParcelableExtra(
                    EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE);
        } catch (ClassCastException e) {
            throw new ParseException("Extra " + EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE
                    + " must be of type PersistableBundle.",
                    R.string.device_owner_error_general, e);
        }

        checkValidityOfProvisioningParams(params);
        return params;
    }

    /**
     * Check whether necessary fields are set.
     */
    private void checkValidityOfProvisioningParams(ProvisioningParams params)
        throws ParseException  {
        if (TextUtils.isEmpty(params.mDeviceAdminPackageName)) {
            throw new ParseException("Must provide the name of the device admin package.",
                    R.string.device_owner_error_general);
        }
        if (!TextUtils.isEmpty(params.mDeviceAdminPackageDownloadLocation)) {
            if (params.mDeviceAdminPackageChecksum == null ||
                    params.mDeviceAdminPackageChecksum.length == 0) {
                throw new ParseException("Checksum of installer file is required for downloading " +
                        "device admin file, but not provided.",
                        R.string.device_owner_error_general);
            }
        }
    }

    /**
     * Exception thrown when the ProvisioningParams initialization failed completely.
     *
     * Note: We're using a custom exception to avoid catching subsequent exceptions that might be
     * significant.
     */
    public static class ParseException extends Exception {
        private int mErrorMessageId;

        public ParseException(String message, int errorMessageId) {
            super(message);
            mErrorMessageId = errorMessageId;
        }

        public ParseException(String message, int errorMessageId, Throwable t) {
            super(message, t);
            mErrorMessageId = errorMessageId;
        }

        public int getErrorMessageId() {
            return mErrorMessageId;
        }
    }

    public static byte[] stringToByteArray(String s)
        throws NumberFormatException {
        try {
            return Base64.decode(s, Base64.URL_SAFE);
        } catch (IllegalArgumentException e) {
            throw new NumberFormatException("Incorrect checksum format.");
        }
    }

    public static Locale stringToLocale(String s)
        throws IllformedLocaleException {
        return new Locale.Builder().setLanguageTag(s.replace("_", "-")).build();
    }
}
