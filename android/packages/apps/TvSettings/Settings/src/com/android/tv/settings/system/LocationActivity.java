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

package com.android.tv.settings.system;

import com.android.tv.settings.R;

import com.android.tv.settings.ActionBehavior;
import com.android.tv.settings.ActionKey;
import com.android.tv.settings.BaseSettingsActivity;
import android.accounts.Account;
import android.accounts.AccountManager;
import com.android.tv.settings.util.SettingsHelper;
import com.android.tv.settings.dialog.old.Action;
import com.android.tv.settings.dialog.old.ActionAdapter;
import com.android.tv.settings.device.apps.AppManagementActivity;

import android.app.AppOpsManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.location.LocationManager;
import android.os.Bundle;
import android.preference.Preference;
import android.provider.Settings;
import android.util.Log;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/**
 * Controls location settings.
 */
public class LocationActivity extends BaseSettingsActivity implements ActionAdapter.Listener {

    private static final String TAG = "LocationActivity";
    private static final boolean DEBUG = false;

    /**
     * Stores a BatterySipper object and records whether the sipper has been
     * used.
     */
    private static final int RECENT_TIME_INTERVAL_MILLIS = 15 * 60 * 1000;
    /**
     * Package name of GmsCore
     */
    private static final String GMS_PACKAGE = "com.google.android.gms";
    /**
     * Class name of Google location settings
     */
    private static final String GOOGLE_LOCATION_SETTINGS_CLASS =
            "com.google.android.location.settings.GoogleLocationSettingsActivity";
    /**
     * Account type for google accounts
     */
    private static final String ACCOUNT_TYPE_GOOGLE = "com.google";

    /**
     * The extra key whose value specifies the name of account to be operated
     * on.
     */
    static final String EXTRA_KEY_ACCOUNT = "com.google.android.location.settings.extra.account";

    private SettingsHelper mHelper;
    private String mAccountName = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        mHelper = new SettingsHelper(getApplicationContext());

        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    protected Object getInitialState() {
        return ActionType.LOCATION_OVERVIEW;
    }

    @Override
    protected void refreshActionList() {
        mActions.clear();
        switch ((ActionType) mState) {
            case LOCATION_OVERVIEW:
                mActions.add(ActionType.LOCATION_STATUS.toAction(
                        mResources, mHelper.getStatusStringFromBoolean(isLocationEnabled())));
                if (isLocationEnabled()) {
                    mActions.add(ActionType.LOCATION_MODE.toAction(mResources,
                            getString(R.string.location_mode_wifi_description), false));
                    mActions.add(ActionType.LOCATION_RECENT_REQUESTS.toAction(mResources));
                }
                break;
            case LOCATION_STATUS:
                Action locationStatusOn = ActionType.ON.toAction(mResources);
                locationStatusOn.setChecked(isLocationEnabled());
                Action locationStatusOff = ActionType.OFF.toAction(mResources);
                locationStatusOff.setChecked(!isLocationEnabled());
                mActions.add(locationStatusOn);
                mActions.add(locationStatusOff);
                break;
            case LOCATION_RECENT_REQUESTS:
                mActions = getRecentRequestActions();
                if (mActions.size() == 0) {
                    mActions.add(ActionType.LOCATION_NO_RECENT_REQUESTS.toAction(
                            mResources, false));
                }
                break;
            case LOCATION_SERVICES:
                mActions.add(ActionType.LOCATION_SERVICES_GOOGLE.toAction(mResources));
                break;
            case LOCATION_SERVICES_GOOGLE:
                mActions = getAccountsActions();
                break;
            case LOCATION_SERVICES_GOOGLE_SETTINGS:
                // TODO add on and off
                mActions.add(ActionType.LOCATION_SERVICES_GOOGLE_REPORTING.toAction(mResources));
                mActions.add(ActionType.LOCATION_SERVICES_GOOGLE_HISTORY.toAction(mResources));
                break;
            case LOCATION_SERVICES_GOOGLE_REPORTING:
            case LOCATION_SERVICES_GOOGLE_HISTORY:
                // TODO set checked here
                mActions.add(ActionType.ON.toAction(mResources));
                mActions.add(ActionType.OFF.toAction(mResources));
                break;
            default:
        }
    }

    // TODO Remove this. Use our own UI
    private void startHoloGoogleLocationServicesSettings() {
        Intent i = new Intent();
        i.setClassName(GMS_PACKAGE, GOOGLE_LOCATION_SETTINGS_CLASS);
        startActivity(i);
    }

    private ArrayList<Action> getAccountsActions(){
        ArrayList<Action> result = new ArrayList<Action>();
        Action.Builder builder = new Action.Builder();
        Account[] googleAccounts = ((AccountManager) getSystemService(Context.ACCOUNT_SERVICE))
                .getAccountsByType(ACCOUNT_TYPE_GOOGLE);
        for (Account account : googleAccounts) {
            result.add(builder.key(account.name).title(account.name).build());
        }
        return result;
    }

    /**
     * Fills a list of applications which queried location recently within
     * specified time. TODO: add icons
     */
    private ArrayList<Action> getRecentRequestActions() {
        ArrayList<Action> result = new ArrayList<Action>();

        // Retrieve a location usage list from AppOps
        AppOpsManager aoManager = (AppOpsManager) getSystemService(Context.APP_OPS_SERVICE);
        List<AppOpsManager.PackageOps> appOps = aoManager.getPackagesForOps(
                new int[] {
                        AppOpsManager.OP_MONITOR_LOCATION,
                        AppOpsManager.OP_MONITOR_HIGH_POWER_LOCATION,
                });
        long now = System.currentTimeMillis();
        for (AppOpsManager.PackageOps ops : appOps) {
            Action action = getActionFromOps(now, ops);
            if (action != null) {
                result.add(action);
            }
        }

        return result;
    }

    private Action getActionFromOps(long now, AppOpsManager.PackageOps ops) {
        String packageName = ops.getPackageName();
        List<AppOpsManager.OpEntry> entries = ops.getOps();
        boolean highBattery = false;
        boolean normalBattery = false;

        // Earliest time for a location request to end and still be shown in
        // list.
        long recentLocationCutoffTime = now - RECENT_TIME_INTERVAL_MILLIS;
        for (AppOpsManager.OpEntry entry : entries) {
            if (entry.isRunning() || entry.getTime() >= recentLocationCutoffTime) {
                switch (entry.getOp()) {
                    case AppOpsManager.OP_MONITOR_LOCATION:
                        normalBattery = true;
                        break;
                    case AppOpsManager.OP_MONITOR_HIGH_POWER_LOCATION:
                        highBattery = true;
                        break;
                    default:
                        break;
                }
            }
        }

        if (!highBattery && !normalBattery) {
            if (DEBUG) {
                Log.v(TAG, packageName + " hadn't used location within the time interval.");
            }
            return null;
        }

        Action.Builder builder = new Action.Builder();
        // The package is fresh enough, continue.
        try {
            ApplicationInfo appInfo = getPackageManager().getApplicationInfo(
                    packageName, PackageManager.GET_META_DATA);
            if (appInfo.uid == ops.getUid()) {
                builder.key(packageName)
                        .title(getPackageManager().getApplicationLabel(appInfo).toString())
                        .description(highBattery ? getString(R.string.location_high_battery_use)
                                : getString(R.string.location_low_battery_use));
            } else if (DEBUG) {
                Log.v(TAG, "package " + packageName + " with Uid " + ops.getUid() +
                        " belongs to another inactive account, ignored.");
            }
        } catch (PackageManager.NameNotFoundException e) {
            Log.wtf(TAG, "Package not found: " + packageName);
        }
        return builder.build();
    }

    private boolean isLocationEnabled() {
        return Settings.Secure.getInt(getContentResolver(),
                Settings.Secure.LOCATION_MODE, Settings.Secure.LOCATION_MODE_OFF) !=
                Settings.Secure.LOCATION_MODE_OFF;
    }

    @Override
    protected void updateView() {
        refreshActionList();
        switch ((ActionType) mState) {
            case LOCATION_OVERVIEW:
                setView(R.string.system_location, R.string.settings_app_name,
                        R.string.system_desc_location, R.drawable.ic_settings_location);
                break;
            case LOCATION_SERVICES_GOOGLE_SETTINGS:
                setView(mAccountName, getPrevState() != null ?
                        ((ActionType) getPrevState()).getTitle(mResources) : null,
                        ((ActionType) mState).getDescription(mResources), 0);
                break;
            case ON:
            case OFF:
            case LOCATION_SERVICES_GOOGLE_REPORTING:
            case LOCATION_SERVICES_GOOGLE_HISTORY:
                break;
            default:
                setView(((ActionType) mState).getTitle(mResources), getPrevState() != null ?
                        ((ActionType) getPrevState()).getTitle(mResources) : null,
                        ((ActionType) mState).getDescription(mResources), 0);
                break;
        }
    }

    @Override
    public void onActionClicked(Action action) {
        // clicking on recent location access apps
        switch ((ActionType) mState) {
            case LOCATION_RECENT_REQUESTS:
                // TODO handle no recent apps
                String packageName = action.getKey();
                Intent i = AppManagementActivity.getLaunchIntent(packageName);
                startActivity(i);
                return;

            case LOCATION_SERVICES_GOOGLE:
                mAccountName = action.getTitle();
                setState(ActionType.LOCATION_SERVICES_GOOGLE_SETTINGS, true);
                return;
            default:
        }

        ActionKey<ActionType, ActionBehavior> actionKey = new ActionKey<ActionType, ActionBehavior>(
                ActionType.class, ActionBehavior.class, action.getKey());
        final ActionType type = actionKey.getType();
        switch (type) {
            case ON:
                setProperty(true);
                goBack();
                break;
            case OFF:
                setProperty(false);
                goBack();
                break;
            case LOCATION_SERVICES_GOOGLE_REPORTING:
                startLocationReportingSettings();
                break;
            case LOCATION_SERVICES_GOOGLE_HISTORY:
                startLocationHistorySettings();
                break;
            case LOCATION_SERVICES_GOOGLE: // TODO remove this here, it should
                                           // fall into the default once we
                                           // figure out how to now use the
                                           // settings in GmsCore
                startHoloGoogleLocationServicesSettings();
                break;
            default:
                setState(type, true);
                break;
        }
    }

    @Override
    protected void setProperty(boolean enable) {
        switch ((ActionType) mState) {
            case LOCATION_STATUS:
                setLocationMode(enable);
                break;
             default:
        }
    }

    // TODO Location history settings is currently in
    // com.google.android.location.settings.LocationHistorySettingsActivity and
    // is non exported
    private void startLocationHistorySettings() {
    }

    // TODO Location reporting settings is currently in
    // com.google.android.location.settings.LocationReportingSettingsActivity
    // and is non exported
    private void startLocationReportingSettings() {
    }

    private void setLocationMode(boolean enable) {
        if (enable) {
            // TODO
            // com.google.android.gms/com.google.android.location.network.ConfirmAlertActivity
            // pops up when we turn this on.
            Settings.Secure.putInt(getContentResolver(), Settings.Secure.LOCATION_MODE,
                    Settings.Secure.LOCATION_MODE_HIGH_ACCURACY);
        } else {
            Settings.Secure.putInt(getContentResolver(), Settings.Secure.LOCATION_MODE,
                    Settings.Secure.LOCATION_MODE_OFF);
        }
    }
}
