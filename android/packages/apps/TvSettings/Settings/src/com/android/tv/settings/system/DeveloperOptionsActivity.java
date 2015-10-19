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
import com.android.tv.settings.util.SettingsHelper;
import com.android.tv.settings.dialog.old.Action;
import com.android.tv.settings.dialog.old.ActionAdapter;
import com.android.tv.settings.dialog.old.ActionFragment;
import com.android.tv.settings.dialog.old.ContentFragment;
import com.android.tv.settings.dialog.old.EditTextFragment;

import android.app.ActivityManagerNative;
import android.app.ActivityThread;
import android.bluetooth.BluetoothAdapter;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Parcel;
import android.os.PowerManager;
import android.os.Process;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.StrictMode;
import android.os.SystemProperties;
import android.provider.Settings;
import android.util.Log;
import android.view.HardwareRenderer;
import android.view.IWindowManager;
import android.view.View;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.text.Collator;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class DeveloperOptionsActivity extends BaseSettingsActivity
        implements ActionAdapter.Listener {

    private static final String TAG = "DeveloperOptionsActivity";
    private static final boolean DEBUG = false;

    private static final int INDEX_WINDOW_ANIMATION_SCALE = 0;
    private static final int INDEX_TRANSITION_ANIMATION_SCALE = 1;
    private static final int INDEX_ANIMATOR_DURATION_SCALE = 2;
    private static final String KEY_SCALE = "scale_value";
    private static final String OPENGL_TRACES_PROPERTY = "debug.egl.trace";
    private static final String HDCP_CHECKING_PROPERTY = "persist.sys.hdcp_checking";

    private static final String HDMI_OPTIMIZATION_PROPERTY = "persist.sys.hdmi.resolution";

    private static SettingsHelper mHelper;
    private ContentResolver mContentResolver;
    private IWindowManager mWindowManager;
    private final List<MyApplicationInfo> mPackageInfoList = new ArrayList<MyApplicationInfo>();
    private WifiManager mWifiManager;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mHelper = new SettingsHelper(this);
        mContentResolver = getContentResolver();
        mWindowManager = IWindowManager.Stub.asInterface(ServiceManager.getService("window"));

        mWifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);


        super.onCreate(savedInstanceState);
    }

    @Override
    protected void refreshActionList() {
        mActions.clear();
        switch ((ActionType) mState) {
            case DEVELOPER_OVERVIEW:
                mActions.add(ActionType.DEVELOPER_GENERAL.toAction(mResources));
                mActions.add(ActionType.DEVELOPER_DEBUGGING.toAction(mResources));
                mActions.add(ActionType.DEVELOPER_INPUT.toAction(mResources));
                mActions.add(ActionType.DEVELOPER_DRAWING.toAction(mResources));
                mActions.add(ActionType.DEVELOPER_MONITORING.toAction(mResources));
                mActions.add(ActionType.DEVELOPER_APPS.toAction(mResources));
                break;
            case DEVELOPER_GENERAL:
                mActions.add(ActionType.DEVELOPER_GENERAL_STAY_AWAKE.toAction(mResources,
                        mHelper.getGlobalIntSetting(Settings.Global.STAY_ON_WHILE_PLUGGED_IN)));
                if (!"user".equals(Build.TYPE)) {
                    mActions.add(ActionType.DEVELOPER_GENERAL_HDCP_CHECKING.toAction(mResources,
                            getHdcpStatus(mHelper.getSystemProperties(HDCP_CHECKING_PROPERTY))));
                }
                mActions.add(ActionType.DEVELOPER_GENERAL_HDMI_OPTIMIZATION.toAction(mResources,
                        getHdmiOptimizationStatus(mHelper.getSystemProperties(
                                HDMI_OPTIMIZATION_PROPERTY))));
                mActions.add(ActionType.DEVELOPER_GENERAL_BT_HCI_LOG.toAction(mResources,
                        mHelper.getStatusStringFromBoolean(
                                mHelper.getSecureIntValueSettingToBoolean(
                                        Settings.Secure.BLUETOOTH_HCI_LOG))));
                break;
            case EMAIL_ADDRESS:
                break;
            case DEVELOPER_DEBUGGING:
                mActions.add(ActionType.DEVELOPER_DEBUGGING_USB_DEBUGGING.toAction(
                        mResources, mHelper.getGlobalIntSetting(Settings.Global.ADB_ENABLED)));
                mActions.add(ActionType.DEVELOPER_DEBUGGING_ALLOW_MOCK_LOCATIONS.toAction(
                        mResources,
                        mHelper.getSecureStatusIntSetting(Settings.Secure.ALLOW_MOCK_LOCATION)));
                mActions.add(ActionType.DEVELOPER_DEBUGGING_SELECT_DEBUG_APP.toAction(
                        mResources, getDebugApp()));
                mActions.add(ActionType.DEVELOPER_DEBUGGING_WAIT_FOR_DEBUGGER.toAction(
                        mResources,
                        mHelper.getGlobalIntSetting(Settings.Global.WAIT_FOR_DEBUGGER)));
                mActions.add(
                        ActionType.DEVELOPER_DEBUGGING_VERIFY_APPS_OVER_USB.toAction(mResources,
                                mHelper.getGlobalIntSetting(
                                        Settings.Global.PACKAGE_VERIFIER_INCLUDE_ADB)));
                mActions.add(
                        ActionType.DEVELOPER_DEBUGGING_WIFI_VERBOSE_LOGGING.toAction(mResources,
                                mWifiManager.getVerboseLoggingLevel() > 0 ? "On" : "Off"));
                break;
            case DEVELOPER_INPUT:
                mActions.add(ActionType.DEVELOPER_INPUT_SHOW_TOUCHES.toAction(
                        mResources, mHelper.getSystemIntSetting(Settings.System.SHOW_TOUCHES)));
                mActions.add(ActionType.DEVELOPER_INPUT_POINTER_LOCATION.toAction(
                        mResources, mHelper.getSystemIntSetting(Settings.System.POINTER_LOCATION)));
                break;
            case DEVELOPER_GENERAL_REBOOT:
                mActions.add(ActionType.OK.toAction(mResources));
                mActions.add(ActionType.CANCEL.toAction(mResources));
                break;
            case DEVELOPER_GENERAL_STAY_AWAKE:
            case DEVELOPER_DEBUGGING_USB_DEBUGGING:
            case DEVELOPER_GENERAL_BT_HCI_LOG:
            case DEVELOPER_DEBUGGING_VERIFY_APPS_OVER_USB:
            case DEVELOPER_DEBUGGING_WIFI_VERBOSE_LOGGING:
            case DEVELOPER_DEBUGGING_ALLOW_MOCK_LOCATIONS:
            case DEVELOPER_DEBUGGING_WAIT_FOR_DEBUGGER:
            case DEVELOPER_INPUT_SHOW_TOUCHES:
            case DEVELOPER_INPUT_POINTER_LOCATION:
            case DEVELOPER_DRAWING_SHOW_GPU_VIEW_UPDATES:
            case DEVELOPER_DRAWING_SHOW_HARDWARE_LAYER:
            case DEVELOPER_DRAWING_SHOW_LAYOUT_BOUNDS:
            case DEVELOPER_DRAWING_SHOW_SURFACE_UPDATES:
            case DEVELOPER_APPS_DONT_KEEP_ACTIVITIES:
            case DEVELOPER_APPS_SHOW_ALL_ANRS:
            case DEVELOPER_MONITORING_SHOW_CPU_USAGE:
            case DEVELOPER_MONITORING_STRICT_MODE_ENABLED:
                mActions = getEnableActions(((ActionType) mState).name(), getProperty());
                break;
            case DEVELOPER_DRAWING:
                mActions.add(ActionType.DEVELOPER_DRAWING_SHOW_LAYOUT_BOUNDS.toAction(
                        mResources,
                        mHelper.getSystemBooleanProperties(View.DEBUG_LAYOUT_PROPERTY)));
                mActions.add(
                        ActionType.DEVELOPER_DRAWING_SHOW_GPU_VIEW_UPDATES.toAction(
                                mResources, mHelper.getSystemBooleanProperties(
                                        HardwareRenderer.DEBUG_DIRTY_REGIONS_PROPERTY)));
                mActions.add(ActionType.DEVELOPER_DRAWING_SHOW_GPU_OVERDRAW.toAction(
                        mResources, getGpuOverdrawLabel()));
                mActions.add(ActionType.DEVELOPER_DRAWING_SHOW_HARDWARE_LAYER.toAction(
                        mResources, mHelper.getSystemBooleanProperties(
                                HardwareRenderer.DEBUG_SHOW_LAYERS_UPDATES_PROPERTY)));
                mActions.add(
                        ActionType.DEVELOPER_DRAWING_SHOW_SURFACE_UPDATES.toAction(
                                mResources, mHelper.getStatusStringFromBoolean(
                                        getShowUpdatesOption())));
                mActions.add(
                        ActionType.DEVELOPER_DRAWING_WINDOW_ANIMATION_SCALE.toAction(mResources,
                                getAnimationScaleValue(INDEX_WINDOW_ANIMATION_SCALE) + ""));
                mActions.add(ActionType.DEVELOPER_DRAWING_TRANSITION_ANIMATION_SCALE.toAction(
                        mResources, getAnimationScaleValue(INDEX_TRANSITION_ANIMATION_SCALE) + ""));
                mActions.add(
                        ActionType.DEVELOPER_DRAWING_ANIMATOR_DURATION_SCALE.toAction(mResources,
                                getAnimationScaleValue(INDEX_ANIMATOR_DURATION_SCALE) + ""));
                break;
            case DEVELOPER_DEBUGGING_SELECT_DEBUG_APP:
                mActions = getApps();
                break;
            case DEVELOPER_GENERAL_HDCP_CHECKING:
                mActions = Action.createActionsFromArrays(
                        mResources.getStringArray(R.array.hdcp_checking_values),
                        mResources.getStringArray(R.array.hdcp_checking_summaries),
                        1 /* non zero check set ID */,
                        mHelper.getSystemProperties(HDCP_CHECKING_PROPERTY));
                break;
            case DEVELOPER_GENERAL_HDMI_OPTIMIZATION:
                mActions = Action.createActionsFromArrays(
                        mResources.getStringArray(R.array.hdmi_optimization_values),
                        mResources.getStringArray(R.array.hdmi_optimization_entries),
                        1 /* non zero check set ID */,
                        mHelper.getSystemProperties(HDMI_OPTIMIZATION_PROPERTY));
                break;
            case DEVELOPER_DRAWING_ANIMATOR_DURATION_SCALE:
                mActions = getAnimationScaleActions(INDEX_ANIMATOR_DURATION_SCALE);
                break;
            case DEVELOPER_DRAWING_TRANSITION_ANIMATION_SCALE:
                mActions = getAnimationScaleActions(INDEX_TRANSITION_ANIMATION_SCALE);
                break;
            case DEVELOPER_DRAWING_WINDOW_ANIMATION_SCALE:
                mActions = getAnimationScaleActions(INDEX_WINDOW_ANIMATION_SCALE);
                break;
            case DEVELOPER_MONITORING_PROFILE_GPU_RENDERING:
                mActions = Action.createActionsFromArrays(
                        mResources.getStringArray(R.array.track_frame_time_values),
                        mResources.getStringArray(R.array.track_frame_time_entries));
                break;
            case DEVELOPER_DRAWING_SHOW_GPU_OVERDRAW:
                mActions = Action.createActionsFromArrays(
                        mResources.getStringArray(R.array.debug_hw_overdraw_values),
                        mResources.getStringArray(R.array.debug_hw_overdraw_entries), 1,
                        getGpuOverdrawValue());
                break;
            case DEVELOPER_MONITORING_ENABLE_TRACES:
                mActions = Action.createActionsFromArrays(
                        mResources.getStringArray(R.array.enable_opengl_traces_values),
                        mResources.getStringArray(R.array.enable_opengl_traces_entries));
                break;
            case DEVELOPER_APPS_BACKGROUND_PROCESS_LIMIT:
                mActions = Action.createActionsFromArrays(
                        mResources.getStringArray(R.array.app_process_limit_values),
                        mResources.getStringArray(R.array.app_process_limit_entries));
                break;
            case DEVELOPER_MONITORING:
                mActions.add(
                        ActionType.DEVELOPER_MONITORING_STRICT_MODE_ENABLED.toAction(
                                mResources, mHelper.getStatusStringFromBoolean(
                                        SystemProperties.getBoolean(StrictMode.VISUAL_PROPERTY,
                                                false))));
                mActions.add(ActionType.DEVELOPER_MONITORING_SHOW_CPU_USAGE.toAction(
                        mResources, mHelper.getGlobalIntSetting(Settings.Global.SHOW_PROCESSES)));
                mActions.add(
                        ActionType.DEVELOPER_MONITORING_PROFILE_GPU_RENDERING.toAction(mResources,
                                SystemProperties.get(HardwareRenderer.PROFILE_PROPERTY)));
                mActions.add(ActionType.DEVELOPER_MONITORING_ENABLE_TRACES.toAction(
                        mResources, SystemProperties.get(OPENGL_TRACES_PROPERTY)));
                break;
            case DEVELOPER_APPS:
                mActions.add(ActionType.DEVELOPER_APPS_DONT_KEEP_ACTIVITIES.toAction(
                        mResources, mHelper.getGlobalIntSetting(
                                Settings.Global.ALWAYS_FINISH_ACTIVITIES)));
                mActions.add(
                        ActionType.DEVELOPER_APPS_BACKGROUND_PROCESS_LIMIT.toAction(
                                mResources, getAppProcessLimit() + ""));
                mActions.add(ActionType.DEVELOPER_APPS_SHOW_ALL_ANRS.toAction(
                        mResources,
                        mHelper.getSecureStatusIntSetting(Settings.Secure.ANR_SHOW_BACKGROUND)));
                break;
            default:
                break;
        }
    }

    @Override
    protected void updateView() {
        refreshActionList();

        switch ((ActionType) mState) {
            case DEVELOPER_OVERVIEW:
                setView(R.string.system_developer_options, R.string.settings_app_name, 0,
                        R.drawable.ic_settings_developeroptions);
                break;
            case DEVELOPER_GENERAL:
                setView(R.string.system_general, R.string.system_developer_options, 0, 0);
                break;
            case EMAIL_ADDRESS:
                mContentFragment = EditTextFragment.newInstance(
                        mResources.getString(R.string.system_email_address));
                mActionFragment = ActionFragment.newInstance(mActions);
                setContentAndActionFragments(mContentFragment, mActionFragment);
                break;
            case DEVELOPER_DEBUGGING:
                setView(R.string.system_debugging, R.string.system_developer_options, 0, 0);
                break;
            case DEVELOPER_INPUT:
                setView(R.string.system_input, R.string.system_developer_options, 0, 0);
                break;
            case DEVELOPER_GENERAL_STAY_AWAKE:
            case DEVELOPER_GENERAL_REBOOT:
            case DEVELOPER_DEBUGGING_USB_DEBUGGING:
            case DEVELOPER_GENERAL_BT_HCI_LOG:
            case DEVELOPER_DEBUGGING_VERIFY_APPS_OVER_USB:
            case DEVELOPER_DEBUGGING_WIFI_VERBOSE_LOGGING:
            case DEVELOPER_DEBUGGING_ALLOW_MOCK_LOCATIONS:
            case DEVELOPER_DEBUGGING_WAIT_FOR_DEBUGGER:
            case DEVELOPER_INPUT_SHOW_TOUCHES:
            case DEVELOPER_INPUT_POINTER_LOCATION:
            case DEVELOPER_DRAWING_SHOW_GPU_VIEW_UPDATES:
            case DEVELOPER_DRAWING_SHOW_GPU_OVERDRAW:
            case DEVELOPER_DRAWING_SHOW_HARDWARE_LAYER:
            case DEVELOPER_DRAWING_SHOW_LAYOUT_BOUNDS:
            case DEVELOPER_DRAWING_SHOW_SURFACE_UPDATES:
            case DEVELOPER_APPS_DONT_KEEP_ACTIVITIES:
            case DEVELOPER_APPS_SHOW_ALL_ANRS:
            case DEVELOPER_MONITORING_SHOW_CPU_USAGE:
            case DEVELOPER_MONITORING_STRICT_MODE_ENABLED:
                setView(((ActionType) mState).getTitle(mResources), getPrevState() != null ?
                        ((ActionType) getPrevState()).getTitle(mResources) : null,
                        ((ActionType) mState).getDescription(mResources), 0);
                break;
            case DEVELOPER_DRAWING:
                setView(R.string.system_drawing, R.string.system_developer_options, 0, 0);
                break;
            case DEVELOPER_DEBUGGING_SELECT_DEBUG_APP:
                setView(R.string.system_select_debug_app, R.string.system_debugging, 0, 0);
                break;
            case DEVELOPER_GENERAL_HDCP_CHECKING:
                setView(R.string.system_hdcp_checking, R.string.system_general, 0, 0);
                break;
            case DEVELOPER_GENERAL_HDMI_OPTIMIZATION:
                setView(R.string.system_hdmi_optimization, R.string.system_general,
                        R.string.system_desc_hdmi_optimization, 0);
                break;
            case DEVELOPER_DRAWING_ANIMATOR_DURATION_SCALE:
                setView(R.string.system_animator_duration_scale, R.string.system_drawing, 0, 0);
                break;
            case DEVELOPER_DRAWING_TRANSITION_ANIMATION_SCALE:
                setView(R.string.system_transition_animation_scale, R.string.system_drawing, 0, 0);
                break;
            case DEVELOPER_DRAWING_WINDOW_ANIMATION_SCALE:
                setView(R.string.system_window_animation_scale, R.string.system_drawing, 0, 0);
                break;
            case DEVELOPER_MONITORING_PROFILE_GPU_RENDERING:
                setView(R.string.system_profile_gpu_rendering, R.string.system_monitoring, 0, 0);
                break;
            case DEVELOPER_MONITORING_ENABLE_TRACES:
                setView(R.string.system_enable_traces, R.string.system_developer_options, 0, 0);
                break;
            case DEVELOPER_APPS_BACKGROUND_PROCESS_LIMIT:
                setView(R.string.system_background_process_limit, R.string.system_apps, 0, 0);
                break;
            case DEVELOPER_MONITORING:
                setView(R.string.system_monitoring, R.string.system_developer_options, 0, 0);
                break;
            case DEVELOPER_APPS:
                setView(R.string.system_apps, R.string.system_developer_options, 0, 0);
                break;
            default:
                break;
        }
    }


    @Override
    public void onActionClicked(Action action) {
        /*
         * For list preferences
         */
        final String key = action.getKey();
        switch ((ActionType) mState) {
            case DEVELOPER_DEBUGGING_SELECT_DEBUG_APP:
                setDebugApp(key);
                goBack();
                return;
            case DEVELOPER_GENERAL_HDCP_CHECKING:
                mHelper.setSystemProperties(HDCP_CHECKING_PROPERTY, key);
                goBack();
                return;
            case DEVELOPER_GENERAL_HDMI_OPTIMIZATION:
                String currentValue = mHelper.getSystemProperties(HDMI_OPTIMIZATION_PROPERTY);
                if (!key.equals(currentValue)) {
                    mHelper.setSystemProperties(HDMI_OPTIMIZATION_PROPERTY, key);
                    setState(ActionType.DEVELOPER_GENERAL_REBOOT, true);
                } else {
                    goBack();
                }
                return;
            case DEVELOPER_DRAWING_ANIMATOR_DURATION_SCALE:
                setAnimationScaleOption(INDEX_ANIMATOR_DURATION_SCALE, action);
                goBack();
                return;
            case DEVELOPER_DRAWING_TRANSITION_ANIMATION_SCALE:
                setAnimationScaleOption(INDEX_TRANSITION_ANIMATION_SCALE, action);
                goBack();
                return;
            case DEVELOPER_DRAWING_WINDOW_ANIMATION_SCALE:
                setAnimationScaleOption(INDEX_WINDOW_ANIMATION_SCALE, action);
                goBack();
                return;
            case DEVELOPER_DRAWING_SHOW_GPU_OVERDRAW:
                mHelper.setSystemProperties(HardwareRenderer.DEBUG_OVERDRAW_PROPERTY,
                        action.getKey());
                goBack();
                return;
            case DEVELOPER_MONITORING_PROFILE_GPU_RENDERING:
                mHelper.setSystemProperties(HardwareRenderer.PROFILE_PROPERTY, key);
                goBack();
                return;
            case DEVELOPER_MONITORING_ENABLE_TRACES:
                mHelper.setSystemProperties(OPENGL_TRACES_PROPERTY, key);
                goBack();
                return;
            case DEVELOPER_APPS_BACKGROUND_PROCESS_LIMIT:
                setAppProcessLimit(key);
                goBack();
                return;
            case DEVELOPER_GENERAL_REBOOT:
                if (ActionType.OK.toAction(mResources).getKey().equals(action.getKey())) {
                    PowerManager manager = (PowerManager) getSystemService(POWER_SERVICE);
                    manager.reboot(null);
                } else {
                    goBack();
                }
                return;
            default:
                break;
        }

        /*
         * For regular states
         */
        ActionKey<ActionType, ActionBehavior> actionKey = new ActionKey<ActionType, ActionBehavior>(
                ActionType.class, ActionBehavior.class, action.getKey());
        final ActionType type = actionKey.getType();
        final ActionBehavior behavior = actionKey.getBehavior();
        if (behavior == null) {
            Log.w(TAG, "Could not find behavior for " + action.getKey());
            return;
        }
        switch (behavior) {
            case ON:
                setProperty(true);
                break;
            case OFF:
                setProperty(false);
                break;
            case INIT:
                setState(type, true);
                break;
            default:
        }
    }

    @Override
    protected void setProperty(boolean enable) {
        switch ((ActionType) mState) {
            case DEVELOPER_GENERAL_STAY_AWAKE:
                mHelper.setGlobalIntSetting(Settings.Global.STAY_ON_WHILE_PLUGGED_IN, enable);
                break;
            case DEVELOPER_GENERAL_BT_HCI_LOG:
                mHelper.setSecureIntSetting(Settings.Secure.BLUETOOTH_HCI_LOG, enable);
                BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
                adapter.configHciSnoopLog(enable);
                break;
            case DEVELOPER_DEBUGGING_USB_DEBUGGING:
                mHelper.setGlobalIntSetting(Settings.Global.ADB_ENABLED, enable);
                break;
            case DEVELOPER_DEBUGGING_ALLOW_MOCK_LOCATIONS:
                mHelper.setSecureIntSetting(Settings.Secure.ALLOW_MOCK_LOCATION, enable);
                break;
            case DEVELOPER_DEBUGGING_WAIT_FOR_DEBUGGER:
                mHelper.setGlobalIntSetting(Settings.Global.WAIT_FOR_DEBUGGER, enable);
                break;
            case DEVELOPER_DEBUGGING_VERIFY_APPS_OVER_USB:
                mHelper.setGlobalIntSetting(Settings.Global.PACKAGE_VERIFIER_INCLUDE_ADB, enable);
                break;
            case DEVELOPER_DEBUGGING_WIFI_VERBOSE_LOGGING:
                mWifiManager.enableVerboseLogging(enable ? 1 : 0);
                break;
            case DEVELOPER_INPUT_SHOW_TOUCHES:
                mHelper.setSystemIntSetting(Settings.System.SHOW_TOUCHES, enable);
                break;
            case DEVELOPER_INPUT_POINTER_LOCATION:
                mHelper.setSystemIntSetting(Settings.System.POINTER_LOCATION, enable);
                break;
            case DEVELOPER_DRAWING_SHOW_HARDWARE_LAYER:
                mHelper.setSystemProperties(HardwareRenderer.DEBUG_SHOW_LAYERS_UPDATES_PROPERTY,
                        Boolean.toString(enable));
                break;
            case DEVELOPER_DRAWING_SHOW_LAYOUT_BOUNDS:
                mHelper.setSystemProperties(View.DEBUG_LAYOUT_PROPERTY, Boolean.toString(enable));
                break;
            case DEVELOPER_DRAWING_SHOW_SURFACE_UPDATES:
                setShowUpdatesOption(enable);
                break;
            case DEVELOPER_DRAWING_SHOW_GPU_VIEW_UPDATES:
                mHelper.setSystemProperties(
                        HardwareRenderer.DEBUG_DIRTY_REGIONS_PROPERTY, Boolean.toString(enable));
                break;
            case DEVELOPER_MONITORING_SHOW_CPU_USAGE:
                mHelper.setGlobalIntSetting(Settings.Global.SHOW_PROCESSES, enable);
                Intent service = (new Intent()).setClassName("com.android.systemui",
                        "com.android.systemui.LoadAverageService");
                if (enable) {
                    startService(service);
                } else {
                    stopService(service);
                }
                break;
            case DEVELOPER_MONITORING_STRICT_MODE_ENABLED:
                setStrictModeVisualOptions(enable);
                break;
            case DEVELOPER_APPS_DONT_KEEP_ACTIVITIES:
                try {
                    ActivityManagerNative.getDefault().setAlwaysFinish(enable);
                } catch (RemoteException ex) {
                }
                break;
            case DEVELOPER_APPS_SHOW_ALL_ANRS:
                mHelper.setSecureIntSetting(Settings.Secure.ANR_SHOW_BACKGROUND, enable);
                break;
            default:
                break;
        }
        goBack();
    }

    private boolean getProperty() {
        switch ((ActionType) mState) {
            case DEVELOPER_GENERAL_STAY_AWAKE:
                return mHelper.getGlobalIntSettingToInt(Settings.Global.STAY_ON_WHILE_PLUGGED_IN) ==
                    1;
            case DEVELOPER_GENERAL_BT_HCI_LOG:
                return mHelper.getSecureIntValueSettingToBoolean(Settings.Secure.BLUETOOTH_HCI_LOG);
            case DEVELOPER_DEBUGGING_USB_DEBUGGING:
                return mHelper.getGlobalIntSettingToInt(Settings.Global.ADB_ENABLED) == 1;
            case DEVELOPER_DEBUGGING_ALLOW_MOCK_LOCATIONS:
                return mHelper.getSecureIntValueSettingToBoolean(
                        Settings.Secure.ALLOW_MOCK_LOCATION);
            case DEVELOPER_DEBUGGING_WAIT_FOR_DEBUGGER:
                return mHelper.getGlobalIntSettingToInt(Settings.Global.WAIT_FOR_DEBUGGER) == 1;
            case DEVELOPER_DEBUGGING_VERIFY_APPS_OVER_USB:
                return mHelper.getGlobalIntSettingToInt(
                        Settings.Global.PACKAGE_VERIFIER_INCLUDE_ADB) == 1;
            case DEVELOPER_DEBUGGING_WIFI_VERBOSE_LOGGING:
                return mWifiManager.getVerboseLoggingLevel() > 0;
            case DEVELOPER_INPUT_SHOW_TOUCHES:
                return mHelper.getSystemIntSettingToBoolean(Settings.System.SHOW_TOUCHES);
            case DEVELOPER_INPUT_POINTER_LOCATION:
                return mHelper.getSystemIntSettingToBoolean(Settings.System.POINTER_LOCATION);
            case DEVELOPER_DRAWING_SHOW_HARDWARE_LAYER:
                return SystemProperties.getBoolean(
                        HardwareRenderer.DEBUG_SHOW_LAYERS_UPDATES_PROPERTY, false);
            case DEVELOPER_DRAWING_SHOW_LAYOUT_BOUNDS:
                return SystemProperties.getBoolean(View.DEBUG_LAYOUT_PROPERTY, false);
            case DEVELOPER_DRAWING_SHOW_SURFACE_UPDATES:
                return getShowUpdatesOption();
            case DEVELOPER_DRAWING_SHOW_GPU_VIEW_UPDATES:
                return SystemProperties.getBoolean(
                        HardwareRenderer.DEBUG_DIRTY_REGIONS_PROPERTY, false);
            case DEVELOPER_MONITORING_SHOW_CPU_USAGE:
                return mHelper.getGlobalIntSettingToInt(Settings.Global.SHOW_PROCESSES) == 1;
            case DEVELOPER_MONITORING_STRICT_MODE_ENABLED:
                return SystemProperties.getBoolean(StrictMode.VISUAL_PROPERTY, false);
            case DEVELOPER_APPS_DONT_KEEP_ACTIVITIES:
                return mHelper.getGlobalIntSettingToInt(Settings.Global.ALWAYS_FINISH_ACTIVITIES) ==
                        1;
            case DEVELOPER_APPS_SHOW_ALL_ANRS:
                return mHelper.getSecureIntValueSettingToBoolean(
                        Settings.Secure.ANR_SHOW_BACKGROUND);
            default:
        }
        return false;
    }

    private ArrayList<Action> getEnableActions(String type, boolean enabled) {
        ArrayList<Action> actions = new ArrayList<Action>();
        actions.add(ActionBehavior.ON.toAction(ActionBehavior.getOnKey(type), mResources, enabled));
        actions.add(ActionBehavior.OFF.toAction(ActionBehavior.getOffKey(type), mResources,
                !enabled));
        return actions;
    }

    class MyApplicationInfo {
        ApplicationInfo info;
        CharSequence label;
    }

    private ArrayList<Action> getApps() {
        mPackageInfoList.clear();
        List<ApplicationInfo> pkgs = getPackageManager().getInstalledApplications(0);
        for (int i = 0; i < pkgs.size(); i++) {
            ApplicationInfo ai = pkgs.get(i);
            if (ai.uid == Process.SYSTEM_UID) {
                continue;
            }
            // On a user build, we only allow debugging of apps that
            // are marked as debuggable. Otherwise (for platform development)
            // we allow all apps.
            if ((ai.flags & ApplicationInfo.FLAG_DEBUGGABLE) == 0
                    && "user".equals(Build.TYPE)) {
                continue;
            }
            MyApplicationInfo info = new MyApplicationInfo();
            info.info = ai;
            info.label = info.info.loadLabel(getPackageManager()).toString();
            mPackageInfoList.add(info);
        }
        Collections.sort(mPackageInfoList, sDisplayNameComparator);
        MyApplicationInfo info = new MyApplicationInfo();
        info.label = mResources.getString(R.string.no_application);
        mPackageInfoList.add(0, info);

        ArrayList<Action> actions = new ArrayList<Action>();
        int totalApps = mPackageInfoList.size();
        for (int i = 0; i < totalApps; i++) {
            MyApplicationInfo app = mPackageInfoList.get(i);
            if (app.info != null) {
                actions.add(new Action.Builder()
                        .key(app.info.packageName)
                        .title(app.label.toString())
                        .description(app.info.packageName)
                        .build());
            } else {
                actions.add(new Action.Builder()
                        .key("")
                        .title(app.label.toString())
                        .description("")
                        .build());
            }
        }

        return actions;
    }

    private final static Comparator<MyApplicationInfo> sDisplayNameComparator
            = new Comparator<MyApplicationInfo>() {
                    @Override
                public final int
                        compare(MyApplicationInfo a, MyApplicationInfo b) {
                    return collator.compare(a.label, b.label);
                }

                private final Collator collator = Collator.getInstance();
            };



    private String getDebugApp() {
        return Settings.Global.getString(mContentResolver, Settings.Global.DEBUG_APP);
    }

    private void setDebugApp(String debugApp){
        boolean waitForDebugger =
                mHelper.getGlobalIntSettingToInt(Settings.Global.WAIT_FOR_DEBUGGER) > 0 ? true
                : false;
        try {
            ActivityManagerNative.getDefault().setDebugApp(debugApp, waitForDebugger, true);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    private void setShowUpdatesOption(boolean enable) {
        try {
            IBinder flinger = ServiceManager.getService("SurfaceFlinger");
            if (flinger != null) {
                Parcel data = Parcel.obtain();
                data.writeInterfaceToken("android.ui.ISurfaceComposer");
                final int showUpdates = enable ? 1 : 0;
                data.writeInt(showUpdates);
                flinger.transact(1002, data, null, 0);
                data.recycle();
            }
        } catch (RemoteException ex) {
        }
    }

    private boolean getShowUpdatesOption() {
        // magic communication with surface flinger.
        int showUpdates = 0;
        try {
            IBinder flinger = ServiceManager.getService("SurfaceFlinger");
            if (flinger != null) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken("android.ui.ISurfaceComposer");
                flinger.transact(1010, data, reply, 0);
                @SuppressWarnings("unused")
                int showCpu = reply.readInt();
                @SuppressWarnings("unused")
                int enableGL = reply.readInt();
                showUpdates = reply.readInt();
                reply.recycle();
                data.recycle();
            }
        } catch (RemoteException ex) {
        }
        return showUpdates > 0;
    }

    private void setStrictModeVisualOptions(boolean enable) {
        try {
            mWindowManager.setStrictModeVisualIndicatorPreference(enable
                    ? "1" : "");
        } catch (RemoteException e) {
        }
    }

    private ArrayList<Action> getAnimationScaleActions(int index) {
        String[] keys = null;
        String[] titles = null;
        float scaleValue = getAnimationScaleValue(index);
        switch (index) {
            case INDEX_ANIMATOR_DURATION_SCALE:
                keys = getResources().getStringArray(R.array.animator_duration_scale_values);
                titles = getResources().getStringArray(R.array.animator_duration_scale_entries);
                break;
            case INDEX_TRANSITION_ANIMATION_SCALE:
                keys = getResources().getStringArray(R.array.transition_animation_scale_values);
                titles = getResources().getStringArray(R.array.transition_animation_scale_entries);
                break;
            case INDEX_WINDOW_ANIMATION_SCALE:
                keys = getResources().getStringArray(R.array.window_animation_scale_values);
                titles = getResources().getStringArray(R.array.window_animation_scale_entries);
                break;
            default:
                return null;
        }

        ArrayList<Action> actions = new ArrayList<Action>();
        for (int i = 0; i < keys.length; i++) {
            Action.Builder builder = new Action.Builder();
            float keyScaleValue = Float.parseFloat(keys[i]);
            builder.key(keys[i])
                    .title(titles[i])
                    .checkSetId(1)
                    .intent(new Intent().putExtra(KEY_SCALE, keyScaleValue))
                    .checked(keyScaleValue == scaleValue);
            actions.add(builder.build());
        }
        return actions;
    }

    private void setAnimationScaleOption(int which, Action action) {
        try {
            float scale = action != null ? action.getIntent().getFloatExtra(KEY_SCALE, 1.0f) : 1;
            mWindowManager.setAnimationScale(which, scale);
        } catch (RemoteException e) {
        }
    }

    private float getAnimationScaleValue(int which) {
        float scale = 0;
        try {
            scale = mWindowManager.getAnimationScale(which);
        } catch (RemoteException e) {
        }
        return scale;
    }

    private String getGpuOverdrawValue() {
        String value = SystemProperties.get(HardwareRenderer.DEBUG_OVERDRAW_PROPERTY);
        if (value == null) {
            value = "false"; // default value.
        }
        return value;
    }

    private String getGpuOverdrawLabel() {
        // This is a little ugly, but this shouldn't be called much.
        ArrayList<Action> actions = Action.createActionsFromArrays(
                mResources.getStringArray(R.array.debug_hw_overdraw_values),
                mResources.getStringArray(R.array.debug_hw_overdraw_entries), 1,
                getGpuOverdrawValue());

        for (Action action : actions) {
            if (action.isChecked()) {
                return action.getTitle();
            }
        }
        return actions.get(0).getTitle();
    }

    private int getAppProcessLimit() {
        try {
            return ActivityManagerNative.getDefault().getProcessLimit();
        } catch (RemoteException e) {
        }
        return 0;
    }

    private void setAppProcessLimit(Object newValue) {
        try {
            int limit = newValue != null ? Integer.parseInt(newValue.toString()) : -1;
            ActivityManagerNative.getDefault().setProcessLimit(limit);
        } catch (RemoteException e) {
        }
    }

    @Override
    protected Object getInitialState() {
        return ActionType.DEVELOPER_OVERVIEW;
    }

    /**
     * Gets the HDCP status based on string value.
     */
    private String getHdcpStatus(String value) {
        // Defaults to drm-only. Needs to match with R.array.hdcp_checking_values
        // This is matches phone DevelopmentSettings.
        int index = 1;
        String[] keys = getResources().getStringArray(R.array.hdcp_checking_values);
        String[] summaries = getResources().getStringArray(R.array.hdcp_checking_summaries);
        for (int keyIndex = 0; keyIndex < keys.length; ++keyIndex) {
            if (keys[keyIndex].equals(value)) {
                index = keyIndex;
                break;
            }
        }
        return summaries[index];
    }

    /**
     * Gets HDMI optimization status based on string value.
     */
    private String getHdmiOptimizationStatus(String value) {
        int index = 0;
        String[] keys = getResources().getStringArray(R.array.hdmi_optimization_values);
        String[] summaries = getResources().getStringArray(R.array.hdmi_optimization_entries);
        for (int keyIndex = 0; keyIndex < keys.length; ++keyIndex) {
            if (keys[keyIndex].equals(value)) {
                index = keyIndex;
                break;
            }
        }
        return summaries[index];
    }
}
