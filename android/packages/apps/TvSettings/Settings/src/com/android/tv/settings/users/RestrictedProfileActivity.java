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

package com.android.tv.settings.users;

import com.android.tv.settings.R;
import com.android.tv.settings.dialog.DialogFragment;
import com.android.tv.settings.dialog.DialogFragment.Action;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.app.Activity;
import android.app.ActivityManagerNative;
import android.app.Fragment;
import android.app.admin.DevicePolicyManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.IPackageManager;
import android.content.pm.UserInfo;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.os.UserManager;
import android.preference.PreferenceManager;
import android.provider.Settings.Secure;
import android.util.Log;
import android.view.inputmethod.InputMethodInfo;
import android.view.inputmethod.InputMethodManager;

import com.android.internal.widget.ILockSettings;
import com.android.internal.widget.LockPatternUtils;
import com.android.tv.dialog.PinDialogFragment;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Activity that allows the configuration of a user's restricted profile.
 */
public class RestrictedProfileActivity extends Activity implements Action.Listener,
        AppLoadingTask.Listener {

    public static class RestrictedProfilePinDialogFragment extends PinDialogFragment {

        private static final String PREF_DISABLE_PIN_UNTIL =
                "RestrictedProfileActivity$RestrictedProfilePinDialogFragment.disable_pin_until";

        /**
         * Returns the time until we should disable the PIN dialog (because the user input wrong
         * PINs repeatedly).
         */
        public static final long getDisablePinUntil(Context context) {
            return PreferenceManager.getDefaultSharedPreferences(context).getLong(
                    PREF_DISABLE_PIN_UNTIL, 0);
        }

        /**
         * Saves the time until we should disable the PIN dialog (because the user input wrong PINs
         * repeatedly).
         */
        public static final void setDisablePinUntil(Context context, long timeMillis) {
            PreferenceManager.getDefaultSharedPreferences(context).edit().putLong(
                    PREF_DISABLE_PIN_UNTIL, timeMillis).apply();
        }

        private final LockPatternUtils mLpu;
        private final ILockSettings mILockSettings;

        public RestrictedProfilePinDialogFragment(int type, ResultListener listener,
                LockPatternUtils lpu, ILockSettings iLockSettings) {
            super(type, listener);
            mLpu = lpu;
            mILockSettings = iLockSettings;
        }

        @Override
        public long getPinDisabledUntil() {
            return getDisablePinUntil(getActivity());
        }

        @Override
        public void setPinDisabledUntil(long retryDisableTimeout) {
            setDisablePinUntil(getActivity(), retryDisableTimeout);
        }

        @Override
        public void setPin(String pin) {
            mLpu.saveLockPassword(pin, DevicePolicyManager.PASSWORD_QUALITY_SOMETHING);
        }

        @Override
        public boolean isPinCorrect(String pin) {
            try {
                if (mILockSettings.checkPassword(pin, UserHandle.USER_OWNER)) {
                    return true;
                }
            } catch (RemoteException re) {
                // Do nothing
            }
            return false;
        }

        @Override
        public boolean isPinSet() {
            return UserHandle.myUserId() != UserHandle.USER_OWNER || hasLockscreenSecurity(mLpu);
        }
    }

    private static final boolean DEBUG = false;
    private static final String TAG = "RestrictedProfile";

    private static final String
            ACTION_RESTRICTED_PROFILE_SETUP_LOCKSCREEN = "restricted_setup_locakscreen";
    private static final String ACTION_RESTRICTED_PROFILE_CREATE = "restricted_profile_create";
    private static final String
            ACTION_RESTRICTED_PROFILE_SWITCH_TO = "restricted_profile_switch_to";
    private static final String
            ACTION_RESTRICTED_PROFILE_SWITCH_OUT = "restricted_profile_switch_out";
    private static final String ACTION_RESTRICTED_PROFILE_CONFIG = "restricted_profile_config";
    private static final String ACTION_RESTRICTED_PROFILE_CONFIG_APPS = "restricted_profile_config_apps";
    private static final String ACTION_RESTRICTED_PROFILE_CHANGE_PASSWORD = "restricted_profile_change_password";
    private static final String ACTION_RESTRICTED_PROFILE_DELETE = "restricted_profile_delete";
    private static final String
            ACTION_RESTRICTED_PROFILE_DELETE_CONFIRM = "restricted_profile_delete_confirm";
    private static final String
            ACTION_RESTRICTED_PROFILE_DELETE_CANCEL = "restricted_profile_delete_cancel";

    /**
     * The description string that should be used for an action that launches the restricted profile
     * activity.
     *
     * @param context used to get the appropriate string.
     * @return the description string that should be used for an action that launches the restricted
     *         profile activity.
     */
    public static String getActionDescription(Context context) {
        return context.getString(isRestrictedProfileInEffect(context) ? R.string.on : R.string.off);
    }

    public static boolean isRestrictedProfileInEffect(Context context) {
        UserManager userManager = (UserManager) context.getSystemService(Context.USER_SERVICE);
        UserInfo restrictedUserInfo = findRestrictedUser(userManager);
        boolean isOwner = UserHandle.myUserId() == UserHandle.USER_OWNER;
        boolean isRestrictedProfileOn = restrictedUserInfo != null && !isOwner;
        return isRestrictedProfileOn;
    }

    static void switchUserNow(int userId) {
        try {
            ActivityManagerNative.getDefault().switchUser(userId);
        } catch (RemoteException re) {
            Log.e(TAG, "Caught exception while switching user! " + re);
        }
    }

    static int getIconResource() {
        return R.drawable.ic_settings_restricted_profile;
    }

    static UserInfo findRestrictedUser(UserManager userManager) {
        for (UserInfo userInfo : userManager.getUsers()) {
            if (userInfo.isRestricted()) {
                return userInfo;
            }
        }
        return null;
    }

    private final HashMap<String, Boolean> mSelectedPackages = new HashMap<String, Boolean>();
    private final boolean mIsOwner = UserHandle.myUserId() == UserHandle.USER_OWNER;
    private final AsyncTask<Void, Void, UserInfo>
            mAddUserAsyncTask = new AsyncTask<Void, Void, UserInfo>() {
        @Override
        protected UserInfo doInBackground(Void... params) {
            UserInfo restrictedUserInfo = mUserManager.createUser(
                    RestrictedProfileActivity.this.getString(R.string.user_new_profile_name),
                    UserInfo.FLAG_RESTRICTED);
            if (restrictedUserInfo == null) {
                Log.wtf(TAG, "Got back a null user handle!");
                return null;
            }
            int userId = restrictedUserInfo.id;
            UserHandle user = new UserHandle(userId);
            mUserManager.setUserRestriction(UserManager.DISALLOW_MODIFY_ACCOUNTS, true, user);
            Secure.putIntForUser(getContentResolver(), Secure.LOCATION_MODE,
                    Secure.LOCATION_MODE_OFF, userId);
            mUserManager.setUserRestriction(UserManager.DISALLOW_SHARE_LOCATION, true, user);
            Bitmap bitmap = createBitmapFromDrawable(R.drawable.ic_avatar_default);
            mUserManager.setUserIcon(userId, bitmap);
            // Add shared accounts
            AccountManager am = AccountManager.get(RestrictedProfileActivity.this);
            Account[] accounts = am.getAccounts();
            if (accounts != null) {
                for (Account account : accounts) {
                    am.addSharedAccount(account, user);
                }
            }
            return restrictedUserInfo;
        }

        @Override
        protected void onPostExecute(UserInfo result) {
            if (result == null) {
                return;
            }
            mRestrictedUserInfo = result;
            UserSwitchListenerService.updateLaunchPoint(RestrictedProfileActivity.this, true);
            int userId = result.id;
            if (result.isRestricted() && mIsOwner) {
                DialogFragment dialogFragment = UserAppRestrictionsDialogFragment.newInstance(
                        RestrictedProfileActivity.this, userId, true);
                DialogFragment.add(getFragmentManager(), dialogFragment);
                mMainMenuDialogFragment.setActions(getMainMenuActions());
            }
        }
    };

    private UserManager mUserManager;
    private UserInfo mRestrictedUserInfo;
    private DialogFragment mMainMenuDialogFragment;
    private ILockSettings mLockSettingsService;
    private Handler mHandler;
    private IPackageManager mIPm;
    private AppLoadingTask mAppLoadingTask;
    private Action mConfigAppsAction;
    private DialogFragment mConfigDialogFragment;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mHandler = new Handler();
        mIPm = IPackageManager.Stub.asInterface(ServiceManager.getService("package"));
        mUserManager = (UserManager) getSystemService(Context.USER_SERVICE);
        mRestrictedUserInfo = findRestrictedUser(mUserManager);
        mConfigAppsAction = createConfigAppsAction(-1);
        mMainMenuDialogFragment = new DialogFragment.Builder()
                .title(getString(R.string.launcher_restricted_profile_app_name))
                .description(getString(R.string.user_add_profile_item_summary))
                .iconResourceId(getIconResource())
                .iconBackgroundColor(getResources().getColor(R.color.icon_background))
                .actions(getMainMenuActions()).build();
        DialogFragment.add(getFragmentManager(), mMainMenuDialogFragment);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (mRestrictedUserInfo != null && (mAppLoadingTask == null
                || mAppLoadingTask.getStatus() == AsyncTask.Status.FINISHED)) {
            mAppLoadingTask = new AppLoadingTask(this, mRestrictedUserInfo.id, false, mIPm, this);
            mAppLoadingTask.execute((Void[]) null);
        }
    }

    @Override
    public void onPackageEnableChanged(String packageName, boolean enabled) {
    }

    @Override
    public void onActionsLoaded(ArrayList<Action> actions) {
        int allowedApps = 0;
        for(Action action : actions) {
            if(action.isChecked()) {
                allowedApps++;
            }
        }
        mConfigAppsAction = createConfigAppsAction(allowedApps);
        if (mConfigDialogFragment != null) {
            mConfigDialogFragment.setActions(getConfigActions());
        }
    }

    @Override
    public void onActionClicked(Action action) {
        if (ACTION_RESTRICTED_PROFILE_SWITCH_TO.equals(action.getKey())) {
            switchUserNow(mRestrictedUserInfo.id);
            finish();
        } else if (ACTION_RESTRICTED_PROFILE_SWITCH_OUT.equals(action.getKey())) {
            if (getFragmentManager().findFragmentByTag(PinDialogFragment.DIALOG_TAG) != null) {
                return;
            }
            new RestrictedProfilePinDialogFragment(PinDialogFragment.PIN_DIALOG_TYPE_ENTER_PIN,
                    new PinDialogFragment.ResultListener() {
                        @Override
                        public void done(boolean success) {
                            if (success) {
                                switchUserNow(UserHandle.USER_OWNER);
                                finish();
                            }
                        }
                    }, new LockPatternUtils(this), getLockSettings()).show(getFragmentManager(),
                    PinDialogFragment.DIALOG_TAG);
        } else if (ACTION_RESTRICTED_PROFILE_CHANGE_PASSWORD.equals(action.getKey())) {
            if (getFragmentManager().findFragmentByTag(PinDialogFragment.DIALOG_TAG) != null) {
                return;
            }
            new RestrictedProfilePinDialogFragment(PinDialogFragment.PIN_DIALOG_TYPE_NEW_PIN,
                    new PinDialogFragment.ResultListener() {
                        @Override
                        public void done(boolean success) {
                            // do nothing
                        }
                    }, new LockPatternUtils(this), getLockSettings()).show(getFragmentManager(),
                    PinDialogFragment.DIALOG_TAG);
        } else if (ACTION_RESTRICTED_PROFILE_CONFIG.equals(action.getKey())) {
            mConfigDialogFragment = new DialogFragment.Builder()
                    .title(getString(R.string.restricted_profile_configure_title))
                    .iconResourceId(getIconResource())
                    .iconBackgroundColor(getResources().getColor(R.color.icon_background))
                    .actions(getConfigActions()).build();
            DialogFragment.add(getFragmentManager(), mConfigDialogFragment);
        } else if (ACTION_RESTRICTED_PROFILE_CONFIG_APPS.equals(action.getKey())) {
            DialogFragment dialogFragment = UserAppRestrictionsDialogFragment.newInstance(
                    RestrictedProfileActivity.this, mRestrictedUserInfo.id, false);
            DialogFragment.add(getFragmentManager(), dialogFragment);
        } else if (ACTION_RESTRICTED_PROFILE_DELETE.equals(action.getKey())) {
            if (getFragmentManager().findFragmentByTag(PinDialogFragment.DIALOG_TAG) != null) {
                return;
            }
            new RestrictedProfilePinDialogFragment(PinDialogFragment.PIN_DIALOG_TYPE_ENTER_PIN,
                    new PinDialogFragment.ResultListener() {
                        @Override
                        public void done(boolean success) {
                            if (success) {
                                removeRestrictedUser();
                                LockPatternUtils lpu = new LockPatternUtils(
                                        RestrictedProfileActivity.this);
                                lpu.clearLock(false);
                            }
                        }
                    }, new LockPatternUtils(this), getLockSettings()).show(getFragmentManager(),
                    PinDialogFragment.DIALOG_TAG);
        } else if (ACTION_RESTRICTED_PROFILE_DELETE_CONFIRM.equals(action.getKey())) {
            // TODO remove once we confirm it's not needed
            removeRestrictedUser();
            LockPatternUtils lpu = new LockPatternUtils(this);
            lpu.clearLock(false);
        } else if (ACTION_RESTRICTED_PROFILE_DELETE_CANCEL.equals(action.getKey())) {
            // TODO remove once we confirm it's not needed
            onBackPressed();
        } else if (ACTION_RESTRICTED_PROFILE_CREATE.equals(action.getKey())) {
            if (hasLockscreenSecurity(new LockPatternUtils(this))) {
                addRestrictedUser();
            } else {
                launchChooseLockscreen();
            }
        }
    }

    private ILockSettings getLockSettings() {
        if (mLockSettingsService == null) {
            mLockSettingsService = ILockSettings.Stub.asInterface(
                    ServiceManager.getService("lock_settings"));
        }
        return mLockSettingsService;
    }

    private ArrayList<Action> getMainMenuActions() {
        ArrayList<Action> actions = new ArrayList<Action>();
        if (mRestrictedUserInfo != null) {
            if (mIsOwner) {
                actions.add(new Action.Builder()
                        .key(ACTION_RESTRICTED_PROFILE_SWITCH_TO)
                        .title(getString(R.string.restricted_profile_switch_to))
                        .build());
                actions.add(new Action.Builder()
                        .key(ACTION_RESTRICTED_PROFILE_CONFIG)
                        .title(getString(R.string.restricted_profile_configure_title))
                        .build());
                actions.add(new Action.Builder()
                        .key(ACTION_RESTRICTED_PROFILE_DELETE)
                        .title(getString(R.string.restricted_profile_delete_title))
                        .build());
            } else {
                actions.add(new Action.Builder()
                        .key(ACTION_RESTRICTED_PROFILE_SWITCH_OUT)
                        .title(getString(R.string.restricted_profile_switch_out))
                        .build());
            }
        } else {
            actions.add(new Action.Builder()
                    .key(ACTION_RESTRICTED_PROFILE_CREATE)
                        .title(getString(R.string.restricted_profile_configure_title))
                    .build());
        }
        return actions;
    }

    private ArrayList<Action> getConfigActions() {
        ArrayList<Action> actions = new ArrayList<Action>();
        actions.add(new Action.Builder()
                .key(ACTION_RESTRICTED_PROFILE_CHANGE_PASSWORD)
                .title(getString(R.string.restricted_profile_change_password_title))
                .build());
        actions.add(mConfigAppsAction);
        return actions;
    }

    private Action createConfigAppsAction(int allowedApps) {
        String description = allowedApps >= 0 ? getResources().getQuantityString(
                R.plurals.restricted_profile_configure_apps_description, allowedApps, allowedApps)
                : getString(R.string.restricted_profile_configure_apps_description_loading);
        return new Action.Builder()
                .key(ACTION_RESTRICTED_PROFILE_CONFIG_APPS)
                .title(getString(R.string.restricted_profile_configure_apps_title))
                .description(description)
                .build();
    }

    private static boolean hasLockscreenSecurity(LockPatternUtils lpu) {
        return lpu.isLockPasswordEnabled() || lpu.isLockPatternEnabled();
    }

    private void launchChooseLockscreen() {
        if (getFragmentManager().findFragmentByTag(PinDialogFragment.DIALOG_TAG) != null) {
            return;
        }
        new RestrictedProfilePinDialogFragment(PinDialogFragment.PIN_DIALOG_TYPE_NEW_PIN,
                new PinDialogFragment.ResultListener() {
                    @Override
                    public void done(boolean success) {
                        if (success) {
                            addRestrictedUser();
                        }
                    }
                }, new LockPatternUtils(this), getLockSettings()).show(getFragmentManager(),
                PinDialogFragment.DIALOG_TAG);
    }

    private void removeRestrictedUser() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mUserManager.removeUser(mRestrictedUserInfo.id);
                // pop confirm dialog
                mRestrictedUserInfo = null;
                UserSwitchListenerService.updateLaunchPoint(RestrictedProfileActivity.this, false);
                mMainMenuDialogFragment.setActions(getMainMenuActions());
                getFragmentManager().popBackStack();
            }
        });
    }

    private Bitmap createBitmapFromDrawable(int resId) {
        Drawable icon = getResources().getDrawable(resId);
        icon.setBounds(0, 0, icon.getIntrinsicWidth(), icon.getIntrinsicHeight());
        Bitmap bitmap = Bitmap.createBitmap(icon.getIntrinsicWidth(), icon.getIntrinsicHeight(),
                Bitmap.Config.ARGB_8888);
        icon.draw(new Canvas(bitmap));
        return bitmap;
    }

    private void addRestrictedUser() {
        if (AsyncTask.Status.PENDING == mAddUserAsyncTask.getStatus()) {
            mAddUserAsyncTask.execute((Void[]) null);
        }
    }
}
