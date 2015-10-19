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

import com.android.tv.settings.ActionBehavior;
import com.android.tv.settings.ActionKey;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsConstant;
import com.android.tv.settings.device.apps.ApplicationsState.AppEntry;
import com.android.tv.settings.dialog.old.Action;
import com.android.tv.settings.dialog.old.ActionAdapter;
import com.android.tv.settings.dialog.old.ActionFragment;
import com.android.tv.settings.dialog.old.ContentFragment;
import com.android.tv.settings.dialog.old.DialogActivity;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;

import java.lang.ref.WeakReference;
import java.util.ArrayList;

/**
 * Activity that manages an apps.
 */
public class AppManagementActivity extends DialogActivity implements ActionAdapter.Listener,
        ApplicationsState.Callbacks, DataClearer.Listener, CacheClearer.Listener,
        DefaultClearer.Listener {

    private static final String TAG = "AppManagementActivity";

    private static final String EXTRA_PACKAGE_NAME_KEY = SettingsConstant.PACKAGE
            + ".device.apps.PACKAGE_NAME";

    // Result code identifiers
    private static final int REQUEST_UNINSTALL = 1;
    private static final int REQUEST_MANAGE_SPACE = 2;

    private String mPackageName;
    private ApplicationsState mApplicationsState;
    private ApplicationsState.Session mSession;
    private AppInfo mAppInfo;
    private OpenManager mOpenManager;
    private ForceStopManager mForceStopManager;
    private UninstallManager mUninstallManager;
    private NotificationSetter mNotificationSetter;
    private DataClearer mDataClearer;
    private DefaultClearer mDefaultClearer;
    private CacheClearer mCacheClearer;
    private ActionFragment mActionFragment;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPackageName = getIntent().getStringExtra(EXTRA_PACKAGE_NAME_KEY);
        mApplicationsState = ApplicationsState.getInstance(getApplication());
        mSession = mApplicationsState.newSession(this);
        mSession.resume();
        mAppInfo = new AppInfo(this, mApplicationsState.getEntry(mPackageName));
        mOpenManager = new OpenManager(this, mAppInfo);
        mForceStopManager = new ForceStopManager(this, mAppInfo);
        mUninstallManager = new UninstallManager(this, mAppInfo);
        mNotificationSetter = new NotificationSetter(mAppInfo);
        mDataClearer = new DataClearer(this, mAppInfo);
        mDefaultClearer = new DefaultClearer(this, mAppInfo);
        mCacheClearer = new CacheClearer(this, mAppInfo);
        mActionFragment = ActionFragment.newInstance(getActions());

        setContentAndActionFragments(ContentFragment.newInstance(mAppInfo.getName(),
                getString(R.string.device_apps),
                getString(R.string.device_apps_app_management_version, mAppInfo.getVersion()),
                Uri.parse(AppsBrowseInfo.getAppIconUri(this, mAppInfo)),
                getResources().getColor(R.color.icon_background)), mActionFragment);
    }

    public static Intent getLaunchIntent(String packageName) {
        Intent i = new Intent();
        i.setComponent(new ComponentName(SettingsConstant.PACKAGE,
                SettingsConstant.PACKAGE + ".device.apps.AppManagementActivity"));
        i.putExtra(AppManagementActivity.EXTRA_PACKAGE_NAME_KEY, packageName);
        return i;
    }

    private void refreshUpdateActions() {
        mApplicationsState = ApplicationsState.getInstance(getApplication());
        mAppInfo = new AppInfo(this, mApplicationsState.getEntry(mPackageName));
        mUninstallManager = new UninstallManager(this, mAppInfo);
        updateActions();
    }

    static class DisableChanger extends AsyncTask<Void, Void, Void> {
        final PackageManager mPm;
        final WeakReference<AppManagementActivity> mActivity;
        final ApplicationInfo mInfo;
        final int mState;

        DisableChanger(AppManagementActivity activity, ApplicationInfo info, int state) {
            mPm = activity.getPackageManager();
            mActivity = new WeakReference<AppManagementActivity>(activity);
            mInfo = info;
            mState = state;
        }

        @Override
        protected Void doInBackground(Void... params) {
            mPm.setApplicationEnabledSetting(mInfo.packageName, mState, 0);
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            AppManagementActivity activity = mActivity.get();
            if (activity != null) {
                activity.refreshUpdateActions();
            }
        }
    }

    @Override
    public void onActionClicked(Action action) {
        ActionKey<ActionType, ActionBehavior> actionKey = new ActionKey<ActionType, ActionBehavior>(
                ActionType.class, ActionBehavior.class, action.getKey());
        ActionType actionType = actionKey.getType();

        switch (actionKey.getBehavior()) {
            case INIT:
                onInit(actionType, action);
                break;
            case OK:
                onOk(actionType);
                break;
            case CANCEL:
                onCancel(actionType);
                break;
            case ON:
                onOn(actionType);
                break;
            case OFF:
                onOff(actionType);
                break;
            default:
                break;
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        switch (requestCode) {
            case REQUEST_UNINSTALL:
                if (resultCode == RESULT_OK) {
                    mApplicationsState.removePackage(mPackageName);
                    goToAppSelectScreen();
                }
                break;
            case REQUEST_MANAGE_SPACE:
                mDataClearer.onActivityResult(resultCode);
                break;
        }
    }

    @Override
    public void onRunningStateChanged(boolean running) {
    }

    @Override
    public void onPackageListChanged() {
    }

    @Override
    public void onRebuildComplete() {
    }

    @Override
    public void onPackageIconChanged() {
    }

    @Override
    public void onPackageSizeChanged(String packageName) {
    }

    @Override
    public void onAllSizesComputed() {
        updateActions();
    }

    @Override
    public void dataCleared(boolean succeeded) {
        if (succeeded) {
            mApplicationsState.requestSize(mPackageName);
        } else {
            Log.w(TAG, "Failed to clear data!");
            updateActions();
        }
    }

    @Override
    public void defaultCleared() {
        updateActions();
    }

    @Override
    public void cacheCleared(boolean succeeded) {
        if (succeeded) {
            mApplicationsState.requestSize(mPackageName);
        } else {
            Log.w(TAG, "Failed to clear cache!");
            updateActions();
        }
    }

    private void onInit(ActionType actionType, Action action) {
        switch (actionType) {
            case OPEN:
                onOpen();
                break;
            case PERMISSIONS:
                setContentAndActionFragments(createContentFragment(actionType, action),
                        PermissionsFragment.newInstance(mPackageName));
                break;
            case NOTIFICATIONS:
                setContentAndActionFragments(createContentFragment(actionType,
                        action), ActionFragment.newInstance(actionType.toSelectableActions(
                        getResources(),
                        (mNotificationSetter.areNotificationsOn()) ? ActionBehavior.ON
                                : ActionBehavior.OFF)));
                break;
            default:
                setContentAndActionFragments(createContentFragment(actionType, action),
                        ActionFragment.newInstance(actionType.toActions(getResources())));
                break;
        }
    }

    private ContentFragment createContentFragment(ActionType actionType, Action action) {
        String description = actionType.getDesc(getResources());
        String description2 = actionType.getDesc2(getResources());
        String descriptionToUse = null;
        if (description != null) {
            if (description2 != null) {
                descriptionToUse = description + "\n" + description2;
            } else {
                descriptionToUse = description;
            }
        } else if (description2 != null) {
            descriptionToUse = description2;
        }
        return ContentFragment.newInstance(action.getTitle(), mAppInfo.getName(), descriptionToUse,
                Uri.parse(AppsBrowseInfo.getAppIconUri(this, mAppInfo)),
                getResources().getColor(R.color.icon_background));
    }

    private void onOk(ActionType actionType) {
        switch (actionType) {
            case CLEAR_CACHE:
                onClearCacheOk();
                break;
            case CLEAR_DATA:
                onClearDataOk();
                break;
            case CLEAR_DEFAULTS:
                onClearDefaultOk();
                break;
            case FORCE_STOP:
                onForceStopOk();
                break;
            case UNINSTALL:
                onUninstallOk();
                break;
            case DISABLE:
                onDisableOk();
                break;
            case ENABLE:
                onEnableOk();
                break;
            default:
                break;
        }
    }

    private void onCancel(ActionType actionType) {
        goToActionSelectScreen();
    }

    private void onOn(ActionType actionType) {
        switch (actionType) {
            case NOTIFICATIONS:
                onNotificationsOn();
                break;
            default:
                break;
        }
    }

    private void onOff(ActionType actionType) {
        switch (actionType) {
            case NOTIFICATIONS:
                onNotificationsOff();
                break;
            default:
                break;
        }
    }

    private void onUninstallOk() {
        mUninstallManager.uninstall(REQUEST_UNINSTALL);
    }

    private void onDisableOk() {
        new DisableChanger(this, mAppInfo.getApplicationInfo(),
                PackageManager.COMPONENT_ENABLED_STATE_DISABLED_USER).execute();
        goToActionSelectScreen();
    }

    private void onEnableOk() {
        new DisableChanger(this, mAppInfo.getApplicationInfo(),
                PackageManager.COMPONENT_ENABLED_STATE_DEFAULT).execute();
        goToActionSelectScreen();
    }

    private void onNotificationsOn() {
        if (!mNotificationSetter.enableNotifications()) {
            Log.w(TAG, "Failed to enable notifications!");
        }
        goToActionSelectScreen();
    }

    private void onNotificationsOff() {
        if (!mNotificationSetter.disableNotifications()) {
            Log.w(TAG, "Failed to disable notifications!");
        }
        goToActionSelectScreen();
    }

    private void onOpen() {
        mOpenManager.open(mApplicationsState);
        // TODO: figure out what to do here
    }

    private void onForceStopOk() {
        mForceStopManager.forceStop(mApplicationsState);
        goToActionSelectScreen();
    }

    private void onClearDataOk() {
        mDataClearer.clearData(this, REQUEST_MANAGE_SPACE);
        goToActionSelectScreen();
    }

    private void onClearDefaultOk() {
        mDefaultClearer.clearDefault(this);
        goToActionSelectScreen();
    }

    private void onClearCacheOk() {
        mCacheClearer.clearCache(this);
        goToActionSelectScreen();
    }

    private void goToActionSelectScreen() {
        updateActions();
        getFragmentManager().popBackStack(null, 0);
    }

    private void goToAppSelectScreen() {
        finish();
    }

    private ArrayList<Action> getActions() {
        ArrayList<Action> actions = new ArrayList<Action>();

        if (mOpenManager.canOpen()) {
            actions.add(ActionType.OPEN.toInitAction(getResources()));
        }
        if (mForceStopManager.canForceStop()) {
            actions.add(ActionType.FORCE_STOP.toInitAction(getResources()));
        }
        if (mUninstallManager.canUninstall()) {
            actions.add(ActionType.UNINSTALL.toInitAction(getResources()));
        } else {
            // App is on system partition.
            if (mUninstallManager.canDisable()) {
                if (mUninstallManager.isEnabled()) {
                    actions.add(ActionType.DISABLE.toInitAction(getResources()));
                } else {
                    actions.add(ActionType.ENABLE.toInitAction(getResources()));
                }
            }
        }
        actions.add(
                ActionType.CLEAR_DATA.toInitAction(getResources(), mDataClearer.getDataSize(this)));
        actions.add(ActionType.CLEAR_CACHE.toInitAction(
                getResources(), mCacheClearer.getCacheSize(this)));
        actions.add(ActionType.CLEAR_DEFAULTS.toInitAction(
                getResources(), mDefaultClearer.getDescription(this)));
        actions.add(ActionType.NOTIFICATIONS.toInitAction(getResources(),
                getString((mNotificationSetter.areNotificationsOn()) ? R.string.settings_on
                        : R.string.settings_off)));
        actions.add(ActionType.PERMISSIONS.toInitAction(getResources()));

        return actions;
    }

    private void updateActions() {
        ((ActionAdapter) mActionFragment.getAdapter()).setActions(getActions());
    }
}
