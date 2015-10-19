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

package com.android.tv.settings.util;

import com.android.tv.settings.R;
import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Resources;
import android.os.AsyncTask;
import android.os.IBinder;
import android.os.Parcel;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.util.Log;
import android.view.IWindowManager;
import android.view.View;

import java.util.ArrayList;

public class SettingsHelper {

    private static final String TAG = "SettingsHelper";
    private static final boolean DEBUG = false;

    private Context mContext;
    private ContentResolver mContentResolver;
    private Resources mResources;

    public SettingsHelper(Context context) {
        mContext = context;
        mContentResolver = context.getContentResolver();
        mResources = context.getResources();
    }

    /**
     * Returns a human readable Status description of the setting's value.
     */
    public String getSecureStatusIntSetting(String setting) {
        try {
            return getStatusStringFromInt(Settings.Secure.getInt(mContentResolver, setting));
        } catch (SettingNotFoundException e) {
            Log.d(TAG, "setting: " + setting + " not found");
            // TODO: show error message
        }
        return null;
    }

    /**
     * Returns a string representation of the Integer setting's value.
     */
    public String getSecureIntSetting(String setting, String def) {
        return Integer.toString(
                Settings.Secure.getInt(mContentResolver, setting, Integer.parseInt(def)));
    }

    /**
     * Returns the int as a boolean, for use in determining "on|off".
     */
    public boolean getSecureIntValueSettingToBoolean(String setting) {
        try {
            return Settings.Secure.getInt(mContentResolver, setting) == 1;
        } catch (SettingNotFoundException e) {
            return false;
        }
    }

    public void setSecureIntSetting(String setting, boolean value) {
        int settingValue = value ? 1 : 0;
        Settings.Secure.putInt(mContentResolver, setting, settingValue);
    }

    public void setSecureIntValueSetting(String setting, Object value) {
        int settingValue = Integer.parseInt((String) value);
        Settings.Secure.putInt(mContentResolver, setting, settingValue);
    }

    /**
     * Returns a human readable description of the setting's value.
     */
    public String getSystemIntSetting(String setting) {
        try {
            return getStatusStringFromInt(Settings.System.getInt(mContentResolver, setting));
        } catch (SettingNotFoundException e) {
            Log.d(TAG, "setting: " + setting + " not found");
            // TODO: show error message
        }
        return null;
    }

    public boolean getSystemIntSettingToBoolean(String setting) {
        try {
            return Settings.System.getInt(mContentResolver, setting) == 1;
        } catch (SettingNotFoundException e) {
            return false;
        }
    }

    public void setSystemIntSetting(String setting, boolean value) {
        int settingValue = value ? 1 : 0;
        Settings.System.putInt(mContentResolver, setting, settingValue);
    }

    public void setSystemProperties(String setting, String value) {
        SystemProperties.set(setting, value);
        pokeSystemProperties();
    }

    public String getSystemProperties(String setting) {
        return SystemProperties.get(setting);
    }

    /**
     * Returns a human readable description of the setting's value.
     */
    public String getSystemBooleanProperties(String setting) {
        return getStatusStringFromBoolean(SystemProperties.getBoolean(setting, false));
    }
    private void pokeSystemProperties() {
        (new SystemPropPoker()).execute();
    }

    static class SystemPropPoker extends AsyncTask<Void, Void, Void> {
        @Override
        protected Void doInBackground(Void... params) {
            String[] services;
            try {
                services = ServiceManager.listServices();
            } catch (RemoteException e) {
                return null;
            }
            for (String service : services) {
                IBinder obj = ServiceManager.checkService(service);
                if (obj != null) {
                    Parcel data = Parcel.obtain();
                    try {
                        obj.transact(IBinder.SYSPROPS_TRANSACTION, data, null, 0);
                    } catch (RemoteException e) {
                    } catch (Exception e) {
                        Log.i(TAG, "Somone wrote a bad service '" + service
                                + "' that doesn't like to be poked: " + e);
                    }
                    data.recycle();
                }
            }
            return null;
        }
    }

    public String getGlobalIntSetting(String setting) {
        try {
            return getStatusStringFromInt(Settings.Global.getInt(mContentResolver, setting));
        } catch (SettingNotFoundException e) {
            Log.d(TAG, "setting: " + setting + " not found");
            // TODO: show error message
        }
        // Default to OFF if not found.
        return mResources.getString(R.string.action_off_description);
    }

    public int getGlobalIntSettingToInt(String setting) {
        try {
            return Settings.Global.getInt(mContentResolver, setting);
        } catch (SettingNotFoundException e) {
            Log.d(TAG, "setting: " + setting + " not found");
            // TODO: show error message
        }
        return 0;
    }

    public boolean getGlobalIntSettingAsBoolean(String setting) {
        return Settings.Global.getInt(mContentResolver, setting, 0) != 0;
    }

    public void setGlobalIntSetting(String setting, boolean value) {
        int settingValue = value ? 1 : 0;
        Settings.Global.putInt(mContentResolver, setting, settingValue);
    }

    public String getStatusStringFromBoolean(boolean status) {
        int descResId = status ? R.string.action_on_description : R.string.action_off_description;
        return mResources.getString(descResId);
    }

    public String getStatusStringFromInt(int status) {
        int descResId = status > 0 ? R.string.action_on_description :
                R.string.action_off_description;
        return mResources.getString(descResId);
    }

    /**
     * Translates a status string into a boolean value.
     *
     * @param status Status string ("ON"/"OFF")
     * @return true if the provided status string equals the localized
     *         R.string.action_on_description; false otherwise.
     */
    public boolean getStatusFromString(String status) {
        if (status == null) {
            return false;
        }
        return status.equalsIgnoreCase(mResources.getString(R.string.action_on_description));
    }

}
