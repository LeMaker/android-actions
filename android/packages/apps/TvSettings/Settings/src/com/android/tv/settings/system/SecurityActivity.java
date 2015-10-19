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

import com.android.tv.settings.ActionBehavior;
import com.android.tv.settings.ActionKey;
import com.android.tv.settings.BaseSettingsActivity;
import com.android.tv.settings.R;
import com.android.tv.settings.users.RestrictedProfileActivity;
import com.android.tv.settings.util.SettingsHelper;
import com.android.tv.settings.dialog.old.Action;
import com.android.tv.settings.dialog.old.ActionAdapter;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Bundle;
import android.os.UserManager;
import android.provider.Settings;

import java.util.List;

/**
 * Manages app security preferences.
 * TODO: get a better icon from UX
 * TODO: implement Notification listener settings
 */
public class SecurityActivity extends BaseSettingsActivity implements ActionAdapter.Listener {

    private static final String PACKAGE_MIME_TYPE = "application/vnd.android.package-archive";
    private static final String ACTION_RESTRICTED_PROFILE = "action_restricted_profile";

    private SettingsHelper mHelper;
    private boolean mVerifierInstalled;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        mHelper = new SettingsHelper(getApplicationContext());
        mVerifierInstalled = isVerifierInstalled();
        // Do this after setting up what's needed.
        super.onCreate(savedInstanceState);
    }

    @Override
    protected Object getInitialState() {
        return ActionType.SECURITY_OVERVIEW;
    }

    @Override
    protected void updateView() {
        refreshActionList();
        switch ((ActionType) mState) {
            case SECURITY_OVERVIEW:
                setView(R.string.system_security, R.string.header_category_personal, 0,
                        R.drawable.ic_settings_security);
                break;
            case SECURITY_UNKNOWN_SOURCES:
                setView(R.string.security_unknown_sources_title, R.string.system_security,
                        R.string.security_unknown_sources_desc, 0);
                break;
            case SECURITY_UNKNOWN_SOURCES_CONFIRM:
                setView(R.string.security_unknown_sources_title, R.string.system_security,
                        R.string.security_unknown_sources_confirm_desc, 0);
                break;
            case SECURITY_VERIFY_APPS:
                setView(R.string.security_verify_apps_title, R.string.system_security,
                        R.string.security_verify_apps_desc, 0);
                break;
            default:
        }
    }

    @Override
    protected void refreshActionList() {
        mActions.clear();
        boolean isNonMarketAppsAllowed = isNonMarketAppsAllowed();
        switch ((ActionType) mState) {
            case SECURITY_OVERVIEW:
                mActions.add(ActionType.SECURITY_UNKNOWN_SOURCES.toAction(mResources,
                        mHelper.getStatusStringFromBoolean(isNonMarketAppsAllowed())));
                if (showVerifierSetting()) {
                    Action verifierAction = ActionType.SECURITY_VERIFY_APPS.toAction(mResources,
                            mHelper.getStatusStringFromBoolean(isVerifyAppsEnabled()
                                    && mVerifierInstalled));
                    verifierAction.setEnabled(mVerifierInstalled);
                    mActions.add(verifierAction);
                }
                mActions.add(new Action.Builder().key(ACTION_RESTRICTED_PROFILE)
                        .title(getString(R.string.launcher_restricted_profile_app_name))
                        .description(RestrictedProfileActivity.getActionDescription(this))
                        .intent(new Intent(this, RestrictedProfileActivity.class)).build());
                break;
            case SECURITY_UNKNOWN_SOURCES:
                mActions.add(ActionBehavior.ON.toAction(ActionBehavior.getOnKey(
                        ActionType.SECURITY_UNKNOWN_SOURCES.name()), mResources,
                        isNonMarketAppsAllowed));
                mActions.add(ActionBehavior.OFF.toAction(ActionBehavior.getOffKey(
                        ActionType.SECURITY_UNKNOWN_SOURCES.name()), mResources,
                        !isNonMarketAppsAllowed));
                break;
            case SECURITY_UNKNOWN_SOURCES_CONFIRM:
                mActions.add(ActionBehavior.OK.toAction(ActionBehavior.getOnKey(
                        ActionType.SECURITY_UNKNOWN_SOURCES_CONFIRM.name()), mResources));
                mActions.add(ActionBehavior.CANCEL.toAction(ActionBehavior.getOffKey(
                        ActionType.SECURITY_UNKNOWN_SOURCES_CONFIRM.name()), mResources));
                break;
            case SECURITY_VERIFY_APPS:
                boolean isVerifyAppsEnabled = mVerifierInstalled && isVerifyAppsEnabled();
                mActions.add(ActionBehavior.ON.toAction(ActionBehavior.getOnKey(
                        ActionType.SECURITY_VERIFY_APPS.name()), mResources,
                        isVerifyAppsEnabled));
                mActions.add(ActionBehavior.OFF.toAction(ActionBehavior.getOffKey(
                        ActionType.SECURITY_VERIFY_APPS.name()), mResources,
                        !isVerifyAppsEnabled));
                break;
            default:
        }
    }

    @Override
    public void onActionClicked(Action action) {
        if (ACTION_RESTRICTED_PROFILE.equals(action.getKey())) {
            startActivity(action.getIntent());
            return;
        }
        String key = action.getKey();
        ActionKey<ActionType, ActionBehavior> actionKey = new ActionKey<ActionType, ActionBehavior>(
                ActionType.class, ActionBehavior.class, key);
        final ActionType type = actionKey.getType();
        final ActionBehavior behavior = actionKey.getBehavior();
        switch (behavior) {
            case INIT:
                setState(type, true);
                break;
            case ON:
            case OK:
                if (ActionType.SECURITY_UNKNOWN_SOURCES == type) {
                    setState(ActionType.SECURITY_UNKNOWN_SOURCES_CONFIRM, false);
                } else {
                    setProperty(type, true);
                    goBack();
                }
                break;
            case OFF:
                setProperty(type, false);
                goBack();
                break;
            case CANCEL:
                goBack();
            default:
        }
    }

    @Override
    protected void setProperty(boolean enable) {
    }

    private void setProperty(ActionType type, boolean enable) {
        if (type == ActionType.SECURITY_UNKNOWN_SOURCES_CONFIRM && enable) {
            setNonMarketAppsAllowed(true);
        } else if (type == ActionType.SECURITY_UNKNOWN_SOURCES && !enable) {
            setNonMarketAppsAllowed(false);
        } else if (type == ActionType.SECURITY_VERIFY_APPS) {
            setVerifyAppsEnabled(enable);
        }
    }

    private boolean isNonMarketAppsAllowed() {
        return Settings.Global.getInt(getContentResolver(),
                                      Settings.Global.INSTALL_NON_MARKET_APPS, 0) > 0;
    }

    private void setNonMarketAppsAllowed(boolean enabled) {
        final UserManager um = (UserManager) getSystemService(Context.USER_SERVICE);
        if (um.hasUserRestriction(UserManager.DISALLOW_INSTALL_UNKNOWN_SOURCES)) {
            return;
        }
        // Change the system setting
        Settings.Global.putInt(getContentResolver(), Settings.Global.INSTALL_NON_MARKET_APPS,
                                enabled ? 1 : 0);
    }

    private boolean isVerifyAppsEnabled() {
        return Settings.Global.getInt(getContentResolver(),
                                      Settings.Global.PACKAGE_VERIFIER_ENABLE, 1) > 0;
    }

    private void setVerifyAppsEnabled(boolean enable) {
        Settings.Global.putInt(getContentResolver(), Settings.Global.PACKAGE_VERIFIER_ENABLE,
                enable ? 1 : 0);
    }

    private boolean isVerifierInstalled() {
        final PackageManager pm = getPackageManager();
        final Intent verification = new Intent(Intent.ACTION_PACKAGE_NEEDS_VERIFICATION);
        verification.setType(PACKAGE_MIME_TYPE);
        verification.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        final List<ResolveInfo> receivers = pm.queryBroadcastReceivers(verification, 0);
        return (receivers.size() > 0) ? true : false;
    }

    private boolean showVerifierSetting() {
        return Settings.Global.getInt(getContentResolver(),
                Settings.Global.PACKAGE_VERIFIER_SETTING_VISIBLE, 1) > 0;
    }

}
