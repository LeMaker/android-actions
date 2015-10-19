/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.android.settings.accessibility;

import android.accessibilityservice.AccessibilityServiceInfo;
import android.app.ActivityManagerNative;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.os.UserHandle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceScreen;
import android.preference.SwitchPreference;
import android.provider.SearchIndexableResource;
import android.provider.Settings;
import android.text.TextUtils;
import android.text.TextUtils.SimpleStringSplitter;
import android.os.SystemProperties;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.View;
import android.view.accessibility.AccessibilityManager;
import android.widget.TextView;
import android.util.Log;
import java.io.File;
import android.os.FileUtils;
import java.io.IOException;
import android.util.Log;
import android.os.ServiceManager;

import com.android.internal.content.PackageMonitor;
import com.android.internal.view.RotationPolicy;
import com.android.internal.view.RotationPolicy.RotationPolicyListener;
import com.android.settings.DialogCreatable;
import com.android.settings.R;
import com.android.settings.SettingsPreferenceFragment;
import com.android.settings.Utils;
import com.android.settings.search.BaseSearchIndexProvider;
import com.android.settings.search.Indexable;
import com.android.settings.search.SearchIndexableRaw;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Activity with the accessibility settings.
 */
public class AccessibilitySettings extends SettingsPreferenceFragment implements DialogCreatable,
        Preference.OnPreferenceChangeListener, Indexable {
    private static final String LOG_TAG = "AccessibilitySettings";

    private static final float LARGE_FONT_SCALE = 1.3f;

    static final char ENABLED_ACCESSIBILITY_SERVICES_SEPARATOR = ':';
	
    // Preference categories
    private static final String SERVICES_CATEGORY = "services_category";
    private static final String SYSTEM_CATEGORY = "system_category";

    // Preferences
    private static final String PURE_BACKGROUND_TEXT_PREFERENCE =
            "pure_background_text_preference";
    private static final String TOGGLE_LARGE_TEXT_PREFERENCE =
            "toggle_large_text_preference";
    private static final String TOGGLE_HIGH_TEXT_CONTRAST_PREFERENCE =
            "toggle_high_text_contrast_preference";
    private static final String TOGGLE_INVERSION_PREFERENCE =
            "toggle_inversion_preference";
    private static final String TOGGLE_POWER_BUTTON_ENDS_CALL_PREFERENCE =
            "toggle_power_button_ends_call_preference";
    private static final String TOGGLE_IMPROVED_COMPATIBILITY =
        "toggle_improved_compatibility";
    private static final String TOGGLE_LOCK_SCREEN_ROTATION_PREFERENCE =
            "toggle_lock_screen_rotation_preference";
    private static final String TOGGLE_SPEAK_PASSWORD_PREFERENCE =
            "toggle_speak_password_preference";
    private static final String SELECT_LONG_PRESS_TIMEOUT_PREFERENCE =
            "select_long_press_timeout_preference";
    private static final String ENABLE_ACCESSIBILITY_GESTURE_PREFERENCE_SCREEN =
            "enable_global_gesture_preference_screen";
    private static final String CAPTIONING_PREFERENCE_SCREEN =
            "captioning_preference_screen";
    private static final String DISPLAY_MAGNIFICATION_PREFERENCE_SCREEN =
            "screen_magnification_preference_screen";
    private static final String DISPLAY_DALTONIZER_PREFERENCE_SCREEN =
            "daltonizer_preference_screen";
    private static final String TOGGLE_LOGO = 
            "toggle_logo";


    private static final String SWITCH_LOGO_FILE =
            "/sys/miscinfo/infos/bf";
    
    private static final String BOOT_PIC1 =
            "/misc/boot_logo.bmp.gz";
    private static final String BOOT_PIC2 =
            "/misc/boot_logo_1.bmp.gz";
    		
    // Extras passed to sub-fragments.
    static final String EXTRA_PREFERENCE_KEY = "preference_key";
    static final String EXTRA_CHECKED = "checked";
    static final String EXTRA_TITLE = "title";
    static final String EXTRA_SUMMARY = "summary";
    static final String EXTRA_SETTINGS_TITLE = "settings_title";
    static final String EXTRA_COMPONENT_NAME = "component_name";
    static final String EXTRA_SETTINGS_COMPONENT_NAME = "settings_component_name";

    // Timeout before we update the services if packages are added/removed
    // since the AccessibilityManagerService has to do that processing first
    // to generate the AccessibilityServiceInfo we need for proper
    // presentation.
    private static final long DELAY_UPDATE_SERVICES_MILLIS = 1000;

    // Auxiliary members.
    final static SimpleStringSplitter sStringColonSplitter =
            new SimpleStringSplitter(ENABLED_ACCESSIBILITY_SERVICES_SEPARATOR);

    static final Set<ComponentName> sInstalledServices = new HashSet<ComponentName>();

    private final Map<String, String> mLongPressTimeoutValuetoTitleMap =
            new HashMap<String, String>();

    private final Configuration mCurConfig = new Configuration();

    private final Handler mHandler = new Handler();

    private final Runnable mUpdateRunnable = new Runnable() {
        @Override
        public void run() {
            loadInstalledServices();
            updateServicesPreferences();
        }
    };

    private final PackageMonitor mSettingsPackageMonitor = new PackageMonitor() {
        @Override
        public void onPackageAdded(String packageName, int uid) {
            sendUpdate();
        }

        @Override
        public void onPackageAppeared(String packageName, int reason) {
            sendUpdate();
        }

        @Override
        public void onPackageDisappeared(String packageName, int reason) {
            sendUpdate();
        }

        @Override
        public void onPackageRemoved(String packageName, int uid) {
            sendUpdate();
        }

        private void sendUpdate() {
            mHandler.postDelayed(mUpdateRunnable, DELAY_UPDATE_SERVICES_MILLIS);
        }
    };

    private final SettingsContentObserver mSettingsContentObserver =
            new SettingsContentObserver(mHandler) {
                @Override
                public void onChange(boolean selfChange, Uri uri) {
                    loadInstalledServices();
                    updateServicesPreferences();
                }
            };

    private final RotationPolicyListener mRotationPolicyListener = new RotationPolicyListener() {
        @Override
        public void onChange() {
            updateLockScreenRotationCheckbox();
        }
    };

    // Preference controls.
    private PreferenceCategory mServicesCategory;
    private PreferenceCategory mSystemsCategory;

	//ActionsCode(phchen, new feature),
	private SwitchPreference mPureBackgroundPreference;

    private SwitchPreference mToggleLargeTextPreference;
    private SwitchPreference mToggleHighTextContrastPreference;
    private SwitchPreference mTogglePowerButtonEndsCallPreference;
    private SwitchPreference mToggleLockScreenRotationPreference;
    private SwitchPreference mToggleImprovedCompatibilityPreference;
    private SwitchPreference mToggleSpeakPasswordPreference;
    private ListPreference mSelectLongPressTimeoutPreference;
    private Preference mNoServicesMessagePreference;
    private PreferenceScreen mCaptioningPreferenceScreen;
    private PreferenceScreen mDisplayMagnificationPreferenceScreen;
    private PreferenceScreen mGlobalGesturePreferenceScreen;
    private PreferenceScreen mDisplayDaltonizerPreferenceScreen;
    private SwitchPreference mToggleInversionPreference;
    private SwitchPreference mToggleLogo;
    private int mLongPressTimeoutDefault;

    private DevicePolicyManager mDpm;
    IBackgroundControl mBgControl;
    
    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        addPreferencesFromResource(R.xml.accessibility_settings);
        initializeAllPreferences();
        mDpm = (DevicePolicyManager) (getActivity()
                .getSystemService(Context.DEVICE_POLICY_SERVICE));
    }

    @Override
    public void onResume() {
        super.onResume();
        loadInstalledServices();
        updateAllPreferences();

        mSettingsPackageMonitor.register(getActivity(), getActivity().getMainLooper(), false);
        mSettingsContentObserver.register(getContentResolver());
        if (RotationPolicy.isRotationSupported(getActivity())) {
            RotationPolicy.registerRotationPolicyListener(getActivity(),
                    mRotationPolicyListener);
        }
    }

	/**
    *
    *xxxxx
    *xxxxx
    *
    ************************************
    *      
    *ActionsCode(author:phchen, change_code)
    */
    @Override
    public void onPause() {
        mSettingsPackageMonitor.unregister();
        mSettingsContentObserver.unregister(getContentResolver());
        if (RotationPolicy.isRotationSupported(getActivity())) {
            RotationPolicy.unregisterRotationPolicyListener(getActivity(),
                    mRotationPolicyListener);
        }
        //ActionsCode(phchen, bugfix BUG00110370), 
        mHandler.removeMessages(0);            
        super.onPause();
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (mSelectLongPressTimeoutPreference == preference) {
            handleLongPressTimeoutPreferenceChange((String) newValue);
            return true;
        } else if (mToggleInversionPreference == preference) {
            handleToggleInversionPreferenceChange((Boolean) newValue);
            return true;
        }
        return false;
    }

    private void handleLongPressTimeoutPreferenceChange(String stringValue) {
        Settings.Secure.putInt(getContentResolver(),
                Settings.Secure.LONG_PRESS_TIMEOUT, Integer.parseInt(stringValue));
        mSelectLongPressTimeoutPreference.setSummary(
                mLongPressTimeoutValuetoTitleMap.get(stringValue));
    }

    private void handleToggleInversionPreferenceChange(boolean checked) {
        Settings.Secure.putInt(getContentResolver(),
                Settings.Secure.ACCESSIBILITY_DISPLAY_INVERSION_ENABLED, (checked ? 1 : 0));
    }


     //ActionsCode(author:phchen, pure background)
    private void initBgSvc(){
    try {
	mBgControl = IBackgroundControl.Stub.asInterface(
       ServiceManager.getService("bgcontrol"));
        }catch(Exception e){
	    System.out.println("initBgSvc fail" + e);
	   
	}
    }


     //ActionsCode(author:phchen, pure background)
    private boolean isBgForbidden(){
        if(mBgControl == null)
            initBgSvc();
	try {
		return mBgControl.isForbiddenAll() > 0;
	}catch(Exception e){
	    System.out.println("isBgForbidden fail" + e);
	    return false;
	}
    }
     //ActionsCode(author:phchen, pure background)
    private void forbiddenBg(int forbidden){
        if(mBgControl == null)
            initBgSvc();
        try {
		mBgControl.forbiddenAll(forbidden);
	}catch(Exception e){
	        System.out.println("isBgForbidden fail" + e);
	 }
    }

    /**
    *
    *xxxxx
    *xxxxx
    *
    ************************************
    *      
    *ActionsCode(author:phchen, change_code)
    */
    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
        if (mPureBackgroundPreference == preference) {
        	handlePureBackgroundPreferenceClick();
            return true;
        } else if (mToggleLargeTextPreference == preference) {
            handleToggleLargeTextPreferenceClick();
            return true;
        } else if (mToggleHighTextContrastPreference == preference) {
            handleToggleTextContrastPreferenceClick();
            return true;
        } else if (mTogglePowerButtonEndsCallPreference == preference) {
            handleTogglePowerButtonEndsCallPreferenceClick();
            return true;
        } else if (mToggleLockScreenRotationPreference == preference) {
            handleLockScreenRotationPreferenceClick();
            return true;
        //ActionsCode(phchen, new feature : Improved cts test compatibility), 
        } else if (mToggleImprovedCompatibilityPreference == preference) {
        	handleImprovedCompatibilityPreferenceClick();
            return true;
        } else if (mToggleSpeakPasswordPreference == preference) {
            handleToggleSpeakPasswordPreferenceClick();
            return true;
        } else if (mGlobalGesturePreferenceScreen == preference) {
            handleToggleEnableAccessibilityGesturePreferenceClick();
            return true;
        } else if (mDisplayMagnificationPreferenceScreen == preference) {
            handleDisplayMagnificationPreferenceScreenClick();
            return true;
        //ActionsCode(phchen, new feature), 
        } else if(mToggleLogo == preference) {
        	handleToggleLogo();
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    //ActionsCode(author:phchen, pure background)
    private void handlePureBackgroundPreferenceClick() {
        int forbidden = mPureBackgroundPreference.isChecked() ? 1: 0;
        forbiddenBg(forbidden);
    }
    
    private void handleToggleLargeTextPreferenceClick() {
        try {
            mCurConfig.fontScale = mToggleLargeTextPreference.isChecked() ? LARGE_FONT_SCALE : 1;
            ActivityManagerNative.getDefault().updatePersistentConfiguration(mCurConfig);
        } catch (RemoteException re) {
            /* ignore */
        }
    }

    private void handleToggleTextContrastPreferenceClick() {
        Settings.Secure.putInt(getContentResolver(),
                Settings.Secure.ACCESSIBILITY_HIGH_TEXT_CONTRAST_ENABLED,
                (mToggleHighTextContrastPreference.isChecked() ? 1 : 0));
    }

    private void handleTogglePowerButtonEndsCallPreferenceClick() {
        Settings.Secure.putInt(getContentResolver(),
                Settings.Secure.INCALL_POWER_BUTTON_BEHAVIOR,
                (mTogglePowerButtonEndsCallPreference.isChecked()
                        ? Settings.Secure.INCALL_POWER_BUTTON_BEHAVIOR_HANGUP
                        : Settings.Secure.INCALL_POWER_BUTTON_BEHAVIOR_SCREEN_OFF));
    }

    private void handleLockScreenRotationPreferenceClick() {
        RotationPolicy.setRotationLockForAccessibility(getActivity(),
                !mToggleLockScreenRotationPreference.isChecked());
    }

    /**
    *
    * Improved cts test compatibility
    *xxxxx
    *
    ************************************
    *      
    *ActionsCode(phchen, new_method)
    */
    private void handleImprovedCompatibilityPreferenceClick() {
    	SystemProperties.set("persist.sys.extra_features", mToggleImprovedCompatibilityPreference.isChecked() ? "1" : "0");
    }
    
    private void handleToggleSpeakPasswordPreferenceClick() {
        Settings.Secure.putInt(getContentResolver(),
                Settings.Secure.ACCESSIBILITY_SPEAK_PASSWORD,
                mToggleSpeakPasswordPreference.isChecked() ? 1 : 0);
    }

    private void handleToggleEnableAccessibilityGesturePreferenceClick() {
        Bundle extras = mGlobalGesturePreferenceScreen.getExtras();
        extras.putString(EXTRA_TITLE, getString(
                R.string.accessibility_global_gesture_preference_title));
        extras.putString(EXTRA_SUMMARY, getString(
                R.string.accessibility_global_gesture_preference_description));
        extras.putBoolean(EXTRA_CHECKED, Settings.Global.getInt(getContentResolver(),
                Settings.Global.ENABLE_ACCESSIBILITY_GLOBAL_GESTURE_ENABLED, 0) == 1);
        super.onPreferenceTreeClick(mGlobalGesturePreferenceScreen,
                mGlobalGesturePreferenceScreen);
    }

    private void handleDisplayMagnificationPreferenceScreenClick() {
        Bundle extras = mDisplayMagnificationPreferenceScreen.getExtras();
        extras.putString(EXTRA_TITLE, getString(
                R.string.accessibility_screen_magnification_title));
        extras.putCharSequence(EXTRA_SUMMARY, getActivity().getResources().getText(
                R.string.accessibility_screen_magnification_summary));
        extras.putBoolean(EXTRA_CHECKED, Settings.Secure.getInt(getContentResolver(),
                Settings.Secure.ACCESSIBILITY_DISPLAY_MAGNIFICATION_ENABLED, 0) == 1);
        super.onPreferenceTreeClick(mDisplayMagnificationPreferenceScreen,
                mDisplayMagnificationPreferenceScreen);
    }

    
    /**
    *
    *xxxxx
    *xxxxx
    *
    ************************************
    *      
    *ActionsCode(phchen, new_method)
    */
    private void handleToggleLogo() {
        setSwitchLogoValue(mToggleLogo.isChecked() ? 1 : 0);
        mToggleLogo.setChecked((getSwitchLogoValue() == 1));
    }

    /**
    *
    *xxxxx
    *xxxxx
    *
    ************************************
    *      
    *ActionsCode(author:phchen, change_code)
    */
    private void initializeAllPreferences() {
        mServicesCategory = (PreferenceCategory) findPreference(SERVICES_CATEGORY);
        mSystemsCategory = (PreferenceCategory) findPreference(SYSTEM_CATEGORY);
		
		//ActionsCode(phchen, new feature : pure background text.), 
        mPureBackgroundPreference =
                (SwitchPreference) findPreference(PURE_BACKGROUND_TEXT_PREFERENCE);
                
        // Large text.
        mToggleLargeTextPreference =
                (SwitchPreference) findPreference(TOGGLE_LARGE_TEXT_PREFERENCE);

        // Text contrast.
        mToggleHighTextContrastPreference =
                (SwitchPreference) findPreference(TOGGLE_HIGH_TEXT_CONTRAST_PREFERENCE);

        // Display inversion.
        mToggleInversionPreference = (SwitchPreference) findPreference(TOGGLE_INVERSION_PREFERENCE);
        mToggleInversionPreference.setOnPreferenceChangeListener(this);

        // Power button ends calls.
        mTogglePowerButtonEndsCallPreference =
                (SwitchPreference) findPreference(TOGGLE_POWER_BUTTON_ENDS_CALL_PREFERENCE);
        if (!KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_POWER)
                || !Utils.isVoiceCapable(getActivity())) {
            mSystemsCategory.removePreference(mTogglePowerButtonEndsCallPreference);
        }

        // Lock screen rotation.
        mToggleLockScreenRotationPreference =
                (SwitchPreference) findPreference(TOGGLE_LOCK_SCREEN_ROTATION_PREFERENCE);
        if (!RotationPolicy.isRotationSupported(getActivity())) {
            mSystemsCategory.removePreference(mToggleLockScreenRotationPreference);
        }

        //ActionsCode(phchen, new feature : Improved cts test compatibility), 
        mToggleImprovedCompatibilityPreference =
            (SwitchPreference) findPreference(TOGGLE_IMPROVED_COMPATIBILITY);
        boolean build_mode = SystemProperties.getBoolean("ro.settings.compatibility", false);
        String extra_features = SystemProperties.get("persist.sys.extra_features","1");
        mToggleImprovedCompatibilityPreference.setChecked(extra_features.trim().equals("1"));
        if (build_mode == false) {
        	mSystemsCategory.removePreference(mToggleImprovedCompatibilityPreference);
		}

        // Speak passwords.
        mToggleSpeakPasswordPreference =
                (SwitchPreference) findPreference(TOGGLE_SPEAK_PASSWORD_PREFERENCE);
        //ActionsCode(phchen, new feature),
        mSystemsCategory.removePreference(mToggleSpeakPasswordPreference);


        // Long press timeout.
        mSelectLongPressTimeoutPreference =
                (ListPreference) findPreference(SELECT_LONG_PRESS_TIMEOUT_PREFERENCE);
        mSelectLongPressTimeoutPreference.setOnPreferenceChangeListener(this);
        if (mLongPressTimeoutValuetoTitleMap.size() == 0) {
            String[] timeoutValues = getResources().getStringArray(
                    R.array.long_press_timeout_selector_values);
            mLongPressTimeoutDefault = Integer.parseInt(timeoutValues[0]);
            String[] timeoutTitles = getResources().getStringArray(
                    R.array.long_press_timeout_selector_titles);
            final int timeoutValueCount = timeoutValues.length;
            for (int i = 0; i < timeoutValueCount; i++) {
                mLongPressTimeoutValuetoTitleMap.put(timeoutValues[i], timeoutTitles[i]);
            }
        }

        // Captioning.
        mCaptioningPreferenceScreen = (PreferenceScreen) findPreference(
                CAPTIONING_PREFERENCE_SCREEN);

        // Display magnification.
        mDisplayMagnificationPreferenceScreen = (PreferenceScreen) findPreference(
                DISPLAY_MAGNIFICATION_PREFERENCE_SCREEN);

        // Display color adjustments.
        mDisplayDaltonizerPreferenceScreen = (PreferenceScreen) findPreference(
                DISPLAY_DALTONIZER_PREFERENCE_SCREEN);

        // Global gesture.
        mGlobalGesturePreferenceScreen =
                (PreferenceScreen) findPreference(ENABLE_ACCESSIBILITY_GESTURE_PREFERENCE_SCREEN);
        final int longPressOnPowerBehavior = getActivity().getResources().getInteger(
                com.android.internal.R.integer.config_longPressOnPowerBehavior);
        final int LONG_PRESS_POWER_GLOBAL_ACTIONS = 1;
        if (!KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_POWER)
                || longPressOnPowerBehavior != LONG_PRESS_POWER_GLOBAL_ACTIONS) {
            // Remove accessibility shortcut if power key is not present
            // nor long press power does not show global actions menu.
            mSystemsCategory.removePreference(mGlobalGesturePreferenceScreen);
        }
        //ActionsCode(phchen, new feature),
        mToggleLogo = (SwitchPreference)findPreference(TOGGLE_LOGO);
        if(!switchLogoEnabled()){
            mSystemsCategory.removePreference(mToggleLogo);
        }

    }



    /**
    *
    *xxxxx
    *xxxxx
    *
    ************************************
    *      
    *ActionsCode(phchen, new_method)
    */
    public static int getSwitchLogoValue(){
        String info = "0";
        int value = 0;

        try {
            info = FileUtils.readTextFile(new File(SWITCH_LOGO_FILE), 0, null).trim();
        } catch (IOException e) {
            Log.e(LOG_TAG, "failed to read " + SWITCH_LOGO_FILE);
        }

        if(info.charAt(0) == '1'){
            value = 1;
        }
        return value;
    }    
    /**
    *
    *xxxxx
    *xxxxx
    *
    ************************************
    *      
    *ActionsCode(phchen, new_method)
    */
    public static void setSwitchLogoValue(int value){
        String info = "0";
        if(value == 1){
            info = "1";
        }

        try {
            FileUtils.stringToFile(SWITCH_LOGO_FILE, info);
        } catch (IOException e) {
            Log.e(LOG_TAG, "failed to write to" + SWITCH_LOGO_FILE);
        }
    }    

    /**
    *
    *xxxxx
    *xxxxx
    *
    ************************************
    *      
    *ActionsCode(phchen, new_method)
    */
    public boolean switchLogoEnabled(){
        int supportduallogo = SystemProperties.getInt("ro.boot.supportduallogo", 0);
        int openDuallogo = supportduallogo & 0x0f;
        supportduallogo = supportduallogo & 0x0f0;
        if ((openDuallogo > 0) && ((supportduallogo == 16) || (supportduallogo == 48)) 
        && (new File(BOOT_PIC1).exists() || new File(BOOT_PIC2).exists())) {
            return true;
        }
        return false;
    }
    
    private void updateAllPreferences() {
        updateServicesPreferences();
        updateSystemPreferences();
    }

    private void updateServicesPreferences() {
        // Since services category is auto generated we have to do a pass
        // to generate it since services can come and go and then based on
        // the global accessibility state to decided whether it is enabled.

        // Generate.
        mServicesCategory.removeAll();

        AccessibilityManager accessibilityManager = AccessibilityManager.getInstance(getActivity());

        List<AccessibilityServiceInfo> installedServices =
                accessibilityManager.getInstalledAccessibilityServiceList();
        Set<ComponentName> enabledServices = AccessibilityUtils.getEnabledServicesFromSettings(
                getActivity());
        List<String> permittedServices = mDpm.getPermittedAccessibilityServices(
                UserHandle.myUserId());
        final boolean accessibilityEnabled = Settings.Secure.getInt(getContentResolver(),
                Settings.Secure.ACCESSIBILITY_ENABLED, 0) == 1;

        for (int i = 0, count = installedServices.size(); i < count; ++i) {
            AccessibilityServiceInfo info = installedServices.get(i);

            PreferenceScreen preference = getPreferenceManager().createPreferenceScreen(
                    getActivity());
            String title = info.getResolveInfo().loadLabel(getPackageManager()).toString();

            ServiceInfo serviceInfo = info.getResolveInfo().serviceInfo;
            ComponentName componentName = new ComponentName(serviceInfo.packageName,
                    serviceInfo.name);

            preference.setKey(componentName.flattenToString());

            preference.setTitle(title);
            final boolean serviceEnabled = accessibilityEnabled
                    && enabledServices.contains(componentName);
            String serviceEnabledString;
            if (serviceEnabled) {
                serviceEnabledString = getString(R.string.accessibility_feature_state_on);
            } else {
                serviceEnabledString = getString(R.string.accessibility_feature_state_off);
            }

            // Disable all accessibility services that are not permitted.
            String packageName = serviceInfo.packageName;
            boolean serviceAllowed =
                    permittedServices == null || permittedServices.contains(packageName);
            preference.setEnabled(serviceAllowed || serviceEnabled);

            String summaryString;
            if (serviceAllowed) {
                summaryString = serviceEnabledString;
            } else  {
                summaryString = getString(R.string.accessibility_feature_or_input_method_not_allowed);
            }
            preference.setSummary(summaryString);

            preference.setOrder(i);
            preference.setFragment(ToggleAccessibilityServicePreferenceFragment.class.getName());
            preference.setPersistent(true);

            Bundle extras = preference.getExtras();
            extras.putString(EXTRA_PREFERENCE_KEY, preference.getKey());
            extras.putBoolean(EXTRA_CHECKED, serviceEnabled);
            extras.putString(EXTRA_TITLE, title);

            String description = info.loadDescription(getPackageManager());
            if (TextUtils.isEmpty(description)) {
                description = getString(R.string.accessibility_service_default_description);
            }
            extras.putString(EXTRA_SUMMARY, description);

            String settingsClassName = info.getSettingsActivityName();
            if (!TextUtils.isEmpty(settingsClassName)) {
                extras.putString(EXTRA_SETTINGS_TITLE,
                        getString(R.string.accessibility_menu_item_settings));
                extras.putString(EXTRA_SETTINGS_COMPONENT_NAME,
                        new ComponentName(info.getResolveInfo().serviceInfo.packageName,
                                settingsClassName).flattenToString());
            }

            extras.putParcelable(EXTRA_COMPONENT_NAME, componentName);

            mServicesCategory.addPreference(preference);
        }

        if (mServicesCategory.getPreferenceCount() == 0) {
            if (mNoServicesMessagePreference == null) {
                mNoServicesMessagePreference = new Preference(getActivity());
                mNoServicesMessagePreference.setPersistent(false);
                mNoServicesMessagePreference.setLayoutResource(
                        R.layout.text_description_preference);
                mNoServicesMessagePreference.setSelectable(false);
                mNoServicesMessagePreference.setSummary(
                        getString(R.string.accessibility_no_services_installed));
            }
            mServicesCategory.addPreference(mNoServicesMessagePreference);
        }
    }

    private void updateSystemPreferences() {
    	
    	//ActionsCode(phchen, new feature: pure background),
    	boolean forbidden = isBgForbidden();
    	mPureBackgroundPreference.setChecked(forbidden);
    	
        // Large text.
        try {
            mCurConfig.updateFrom(ActivityManagerNative.getDefault().getConfiguration());
        } catch (RemoteException re) {
            /* ignore */
        }
        mToggleLargeTextPreference.setChecked(mCurConfig.fontScale == LARGE_FONT_SCALE);

        mToggleHighTextContrastPreference.setChecked(
                Settings.Secure.getInt(getContentResolver(),
                        Settings.Secure.ACCESSIBILITY_HIGH_TEXT_CONTRAST_ENABLED, 0) == 1);

        // If the quick setting is enabled, the preference MUST be enabled.
        mToggleInversionPreference.setChecked(Settings.Secure.getInt(getContentResolver(),
                Settings.Secure.ACCESSIBILITY_DISPLAY_INVERSION_ENABLED, 0) == 1);

        // Power button ends calls.
        if (KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_POWER)
                && Utils.isVoiceCapable(getActivity())) {
            final int incallPowerBehavior = Settings.Secure.getInt(getContentResolver(),
                    Settings.Secure.INCALL_POWER_BUTTON_BEHAVIOR,
                    Settings.Secure.INCALL_POWER_BUTTON_BEHAVIOR_DEFAULT);
            final boolean powerButtonEndsCall =
                    (incallPowerBehavior == Settings.Secure.INCALL_POWER_BUTTON_BEHAVIOR_HANGUP);
            mTogglePowerButtonEndsCallPreference.setChecked(powerButtonEndsCall);
        }

        // Auto-rotate screen
        updateLockScreenRotationCheckbox();

        // Speak passwords.
        final boolean speakPasswordEnabled = Settings.Secure.getInt(getContentResolver(),
                Settings.Secure.ACCESSIBILITY_SPEAK_PASSWORD, 0) != 0;
        mToggleSpeakPasswordPreference.setChecked(speakPasswordEnabled);

        // Long press timeout.
        final int longPressTimeout = Settings.Secure.getInt(getContentResolver(),
                Settings.Secure.LONG_PRESS_TIMEOUT, mLongPressTimeoutDefault);
        String value = String.valueOf(longPressTimeout);
        mSelectLongPressTimeoutPreference.setValue(value);
        mSelectLongPressTimeoutPreference.setSummary(mLongPressTimeoutValuetoTitleMap.get(value));

        updateFeatureSummary(Settings.Secure.ACCESSIBILITY_CAPTIONING_ENABLED,
                mCaptioningPreferenceScreen);
        updateFeatureSummary(Settings.Secure.ACCESSIBILITY_DISPLAY_MAGNIFICATION_ENABLED,
                mDisplayMagnificationPreferenceScreen);
        updateFeatureSummary(Settings.Secure.ACCESSIBILITY_DISPLAY_DALTONIZER_ENABLED,
                mDisplayDaltonizerPreferenceScreen);

        // Global gesture
        final boolean globalGestureEnabled = Settings.Global.getInt(getContentResolver(),
                Settings.Global.ENABLE_ACCESSIBILITY_GLOBAL_GESTURE_ENABLED, 0) == 1;
        if (globalGestureEnabled) {
            mGlobalGesturePreferenceScreen.setSummary(
                    R.string.accessibility_global_gesture_preference_summary_on);
        } else {
            mGlobalGesturePreferenceScreen.setSummary(
                    R.string.accessibility_global_gesture_preference_summary_off);
        }

        final boolean toggleLogo = (getSwitchLogoValue() == 1);
        mToggleLogo.setChecked(toggleLogo);
    }

    private void updateFeatureSummary(String prefKey, Preference pref) {
        final boolean enabled = Settings.Secure.getInt(getContentResolver(), prefKey, 0) == 1;
        pref.setSummary(enabled ? R.string.accessibility_feature_state_on
                : R.string.accessibility_feature_state_off);
    }

    private void updateLockScreenRotationCheckbox() {
        Context context = getActivity();
        if (context != null) {
            mToggleLockScreenRotationPreference.setChecked(
                    !RotationPolicy.isRotationLocked(context));
        }
    }

    private void loadInstalledServices() {
        Set<ComponentName> installedServices = sInstalledServices;
        installedServices.clear();

        List<AccessibilityServiceInfo> installedServiceInfos =
                AccessibilityManager.getInstance(getActivity())
                        .getInstalledAccessibilityServiceList();
        if (installedServiceInfos == null) {
            return;
        }

        final int installedServiceInfoCount = installedServiceInfos.size();
        for (int i = 0; i < installedServiceInfoCount; i++) {
            ResolveInfo resolveInfo = installedServiceInfos.get(i).getResolveInfo();
            ComponentName installedService = new ComponentName(
                    resolveInfo.serviceInfo.packageName,
                    resolveInfo.serviceInfo.name);
            installedServices.add(installedService);
        }
    }

    public static final SearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider() {
        @Override
        public List<SearchIndexableRaw> getRawDataToIndex(Context context, boolean enabled) {
            List<SearchIndexableRaw> indexables = new ArrayList<SearchIndexableRaw>();

            PackageManager packageManager = context.getPackageManager();
            AccessibilityManager accessibilityManager = (AccessibilityManager)
                    context.getSystemService(Context.ACCESSIBILITY_SERVICE);

            String screenTitle = context.getResources().getString(
                    R.string.accessibility_services_title);

            // Indexing all services, regardless if enabled.
            List<AccessibilityServiceInfo> services = accessibilityManager
                    .getInstalledAccessibilityServiceList();
            final int serviceCount = services.size();
            for (int i = 0; i < serviceCount; i++) {
                AccessibilityServiceInfo service = services.get(i);
                if (service == null || service.getResolveInfo() == null) {
                    continue;
                }

                ServiceInfo serviceInfo = service.getResolveInfo().serviceInfo;
                ComponentName componentName = new ComponentName(serviceInfo.packageName,
                        serviceInfo.name);

                SearchIndexableRaw indexable = new SearchIndexableRaw(context);
                indexable.key = componentName.flattenToString();
                indexable.title = service.getResolveInfo().loadLabel(packageManager).toString();
                indexable.summaryOn = context.getString(R.string.accessibility_feature_state_on);
                indexable.summaryOff = context.getString(R.string.accessibility_feature_state_off);
                indexable.screenTitle = screenTitle;
                indexables.add(indexable);
            }

            return indexables;
        }

        @Override
        public List<SearchIndexableResource> getXmlResourcesToIndex(Context context,
               boolean enabled) {
            List<SearchIndexableResource> indexables = new ArrayList<SearchIndexableResource>();
            SearchIndexableResource indexable = new SearchIndexableResource(context);
            indexable.xmlResId = R.xml.accessibility_settings;
            indexables.add(indexable);
            return indexables;
        }
    };
}
