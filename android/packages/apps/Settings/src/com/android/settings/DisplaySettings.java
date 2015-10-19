/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.settings;

import com.android.internal.view.RotationPolicy;
import com.android.settings.notification.DropDownPreference;
import com.android.settings.notification.DropDownPreference.Callback;
import com.android.settings.search.BaseSearchIndexProvider;
import com.android.settings.search.Indexable;

import static android.provider.Settings.Secure.DOZE_ENABLED;
import static android.provider.Settings.Secure.WAKE_GESTURE_ENABLED;
import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE;
import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE_AUTOMATIC;
import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL;
import static android.provider.Settings.System.SCREEN_OFF_TIMEOUT;

import android.app.Activity;
import android.app.ActivityManagerNative;
import android.app.Dialog;
import android.app.AlertDialog.Builder;
import android.app.admin.DevicePolicyManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.os.Build;
import android.os.Bundle;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.content.Intent;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceScreen;
import android.widget.CompoundButton;
import android.widget.Checkable;
import android.widget.Switch;
import android.preference.SwitchPreference;
import android.provider.SearchIndexableResource;
import android.provider.Settings;
import android.view.View;
import android.util.AttributeSet;
import android.text.TextUtils;
import android.util.Log;
import android.content.pm.PackageManager;

import java.util.ArrayList;
import java.util.List;
import java.util.Arrays;
import android.content.Intent;
import com.android.settings.tvout.TvoutUtils;
import com.android.settings.tvout.TvoutScreenResizeActivity;

public class DisplaySettings extends SettingsPreferenceFragment implements
        Preference.OnPreferenceChangeListener, OnPreferenceClickListener, Indexable {
    private static final String TAG = "DisplaySettings";

    /** If there is no setting in the provider, use this. */
    private static final int FALLBACK_SCREEN_TIMEOUT_VALUE = 30000;

	/*** use this max screen time out time  */
	private static final int MAX_TIME_OUT = 2147483647;
	
    private static final String KEY_SCREEN_TIMEOUT = "screen_timeout";
    private static final String KEY_FONT_SIZE = "font_size";
    private static final String KEY_SCREEN_SAVER = "screensaver";
    private static final String KEY_LIFT_TO_WAKE = "lift_to_wake";
    private static final String KEY_DOZE = "doze";
    private static final String KEY_TVOUT_SETTINGS = "tvout_settings";
    private static final String KEY_CVBS_MODE_SELECTOR = "cvbs_mode_selector";
    private static final String KEY_HDMI_MODE_SELECTOR = "hdmi_mode_selector";
    private static final String KEY_TVOUT_SCREEN_RESIZE = "tvout_screen_resize";
    private static final String KEY_AUTO_BRIGHTNESS = "auto_brightness";
    private static final String KEY_AUTO_ROTATE = "auto_rotate";
    private static final String KEY_CALIBRATION = "accelerometer_calibration";
    private Preference mGsensorCalib;
    private static final String KEY_ENHANCED_COLOR_SYSTEM = "toggle_enhanced_color_system";
	private static final String KEY_DISABLE_APPS = "disable_apps";
	
    private static final int DLG_GLOBAL_CHANGE_WARNING = 1;
    public static final String TVOUT_STATUS_PROPERTY = "hw.config.hdmi_status"; 
    private static final String ENHANCED_COLOR_PROPERTY = "sys.image_enhanced_system";
    private WarnedListPreference mFontSizePref;

    private final Configuration mCurConfig = new Configuration();

    private ListPreference mScreenTimeoutPreference;
    private Preference mScreenSaverPreference;
    private CheckBoxPreference mEnhancedColor;
    private CheckBoxPreference disableapps;
    private PreferenceCategory mTvoutSettingsCategory;
    private ListPreference mCvbsModePref;
    private TvoutSettingsListPreference mHdmiModePref;
    private Preference mTvoutScreenResize;
    private boolean mSupportCvbs;
    private boolean mSupportHdmi;
    private TvoutUtils.CvbsUtils mCvbsUtils;
    private TvoutUtils.HdmiUtils mHdmiUtils;
    static DisplaySettings mDisplay;
    private SwitchPreference mLiftToWakePreference;
    private SwitchPreference mDozePreference;
    private SwitchPreference mAutoBrightnessPreference;

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
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final Activity activity = getActivity();
        //ActionsCode(phchen, new feature), 
        mSupportCvbs = false;
        mSupportHdmi = true;
        final ContentResolver resolver = activity.getContentResolver();

        addPreferencesFromResource(R.xml.display_settings);

        mScreenSaverPreference = findPreference(KEY_SCREEN_SAVER);
        if (mScreenSaverPreference != null
                && getResources().getBoolean(
                        com.android.internal.R.bool.config_dreamsSupported) == false) {
            getPreferenceScreen().removePreference(mScreenSaverPreference);
        }

        mScreenTimeoutPreference = (ListPreference) findPreference(KEY_SCREEN_TIMEOUT);
        final long currentTimeout = Settings.System.getLong(resolver, SCREEN_OFF_TIMEOUT,
                FALLBACK_SCREEN_TIMEOUT_VALUE);
        mScreenTimeoutPreference.setValue(String.valueOf(currentTimeout));
        mScreenTimeoutPreference.setOnPreferenceChangeListener(this);
        disableUnusableTimeouts(mScreenTimeoutPreference);
        updateTimeoutPreferenceDescription(currentTimeout);

        mFontSizePref = (WarnedListPreference) findPreference(KEY_FONT_SIZE);
        mFontSizePref.setOnPreferenceChangeListener(this);
        mFontSizePref.setOnPreferenceClickListener(this);

        if (isAutomaticBrightnessAvailable(getResources())) {
            mAutoBrightnessPreference = (SwitchPreference) findPreference(KEY_AUTO_BRIGHTNESS);
            mAutoBrightnessPreference.setOnPreferenceChangeListener(this);
        } else {
            removePreference(KEY_AUTO_BRIGHTNESS);
        }

        if (isLiftToWakeAvailable(activity)) {
            mLiftToWakePreference = (SwitchPreference) findPreference(KEY_LIFT_TO_WAKE);
            mLiftToWakePreference.setOnPreferenceChangeListener(this);
        } else {
            removePreference(KEY_LIFT_TO_WAKE);
        }

        if (isDozeAvailable(activity)) {
            mDozePreference = (SwitchPreference) findPreference(KEY_DOZE);
            mDozePreference.setOnPreferenceChangeListener(this);
        } else {
            removePreference(KEY_DOZE);
        }

        //ActionsCode(phchen, new feature,add sensor menu),
        mGsensorCalib = (Preference) findPreference(KEY_CALIBRATION);
        //hide Gsensor Calib for restricted users
        if(UserHandle.myUserId() != 0) {
            getPreferenceScreen().removePreference(mGsensorCalib);
        }
        
        //ActionsCode(phchen, NEW FEATURE: BUG00236195),
        mEnhancedColor = (CheckBoxPreference)findPreference(KEY_ENHANCED_COLOR_SYSTEM);
        //ActionsCode(phchen, NEW FEATURE: hidden google application),
        disableapps = (CheckBoxPreference)findPreference(KEY_DISABLE_APPS);
        if (SystemProperties.get("ro.hidden.google", "disable").equals("disable")){
        	getPreferenceScreen().removePreference(disableapps);
        } else {
        	disableapps.setChecked(getDisableApp(resolver));
        	if(getDisableApp(resolver)){
        		disableApps();        		
    		}else{
    			enableApps();
        	}
        }
        
        long memsize = getMemoryTotalSizeFromProp();
        if(memsize <= 512) {
            Log.d(TAG, "removePreference...because memoryTotalSize le 512;getMemoryTotalSizeFromProp="+memsize);
            getPreferenceScreen().removePreference(mEnhancedColor);
        }
        mEnhancedColor.setChecked(getEnhancedColorSystem(resolver));

        if (RotationPolicy.isRotationLockToggleVisible(activity)) {
            DropDownPreference rotatePreference =
                    (DropDownPreference) findPreference(KEY_AUTO_ROTATE);
            rotatePreference.addItem(activity.getString(R.string.display_auto_rotate_rotate),
                    false);
            int rotateLockedResourceId;
            // The following block sets the string used when rotation is locked.
            // If the device locks specifically to portrait or landscape (rather than current
            // rotation), then we use a different string to include this information.
            if (allowAllRotations(activity)) {
                rotateLockedResourceId = R.string.display_auto_rotate_stay_in_current;
            } else {
                if (RotationPolicy.getRotationLockOrientation(activity)
                        == Configuration.ORIENTATION_PORTRAIT) {
                    rotateLockedResourceId =
                            R.string.display_auto_rotate_stay_in_portrait;
                } else {
                    rotateLockedResourceId =
                            R.string.display_auto_rotate_stay_in_landscape;
                }
            }
            rotatePreference.addItem(activity.getString(rotateLockedResourceId), true);
            rotatePreference.setSelectedItem(RotationPolicy.isRotationLocked(activity) ?
                    1 : 0);
            rotatePreference.setCallback(new Callback() {
                @Override
                public boolean onItemSelected(int pos, Object value) {
                    RotationPolicy.setRotationLock(activity, (Boolean) value);
                    return true;
                }
            });
        } else {
            removePreference(KEY_AUTO_ROTATE);
        }


        //ActionsCode(phchen, new feature,add hdmi menu),
        mTvoutSettingsCategory = (PreferenceCategory)findPreference(KEY_TVOUT_SETTINGS);
        if(mSupportCvbs) {
        	mCvbsModePref = new TvoutSettingsListPreference(getActivity());
        	mCvbsModePref.setKey(KEY_CVBS_MODE_SELECTOR);    
        	mCvbsModePref.setTitle(getResources().getString(R.string.cvbs_mode));
        	mCvbsModePref.setPersistent(false);
        	mTvoutSettingsCategory.addPreference(mCvbsModePref);
        	mCvbsModePref.setDialogTitle(getResources().getString(R.string.cvbs_mode));
        	mCvbsModePref.setEntries(getResources().getStringArray(R.array.tvout_cvbs_entries));
        	mCvbsModePref.setEntryValues(getResources().getStringArray(R.array.tvout_cvbs_values));
        	mCvbsModePref.setOnPreferenceChangeListener(this);
        }
        if(mSupportHdmi) {
        	mHdmiModePref = new TvoutSettingsListPreference(getActivity());
        	mHdmiModePref.setLayoutResource(R.layout.preference_tvout);
        	mHdmiModePref.setKey(KEY_HDMI_MODE_SELECTOR);    
        	mHdmiModePref.setTitle(getResources().getString(R.string.hdmi_mode));
        	mHdmiModePref.setPersistent(true);
        	mTvoutSettingsCategory.addPreference(mHdmiModePref);
        	mHdmiModePref.setDialogTitle(getResources().getString(R.string.hdmi_mode));
        	mHdmiModePref.setOnPreferenceChangeListener(this);
        }
        if(mSupportHdmi || mSupportCvbs) {
        	mTvoutScreenResize = new Preference(getActivity());
        	mTvoutScreenResize.setTitle(getResources().getString(R.string.tvout_screen_resize));
        	mTvoutSettingsCategory.addPreference(mTvoutScreenResize);
        } else {
        	getPreferenceScreen().removePreference(mTvoutSettingsCategory);
        }
        mCvbsUtils = (TvoutUtils.CvbsUtils)TvoutUtils.getInstanceByName(TvoutUtils.TVOUT_CVBS);
        mHdmiUtils = (TvoutUtils.HdmiUtils)TvoutUtils.getInstanceByName(TvoutUtils.TVOUT_HDMI); 
    }
    
    /**
    *
    *
    ************************************
    *      
    *ActionsCode(phchen, new_method)
    */
	private boolean getDisableApp(ContentResolver resolver){
		int hide = SystemProperties.getInt("ro.hidden.google.enable", 0);
    	int disable = Settings.System.getInt(resolver, "actions_diable_app", hide) ;
    	return disable > 0;
    }
    private void setDisableApp(ContentResolver resolver, boolean disable) {
    	int disabled = disable ? 1:0;
    	Settings.System.putInt(resolver, "actions_diable_app", disabled) ;        
    }

	private void disableApps()
    {
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.android.vending", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
        try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.googlequicksearchbox", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.maps", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.plus", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.calendar", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.genie.geniewidget", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.android.contacts", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.docs.editors.docs", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.docs.editors.sheets", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.docs.editors.slides", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.docs", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.keep", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.videos", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.music", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.play.games", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.books", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.magazines", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.partnersetup", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.gms", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.inputmethod.latin", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.enterprise.dmagent", 3, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    }
    private void enableApps()
    {
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.android.vending", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
        try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.googlequicksearchbox", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.maps", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.plus", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.calendar", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.genie.geniewidget", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.android.contacts", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.docs.editors.docs", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.docs.editors.sheets", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.docs.editors.slides", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.docs", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.keep", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.videos", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.music", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.play.games", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.books", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.magazines", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.partnersetup", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.gms", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.inputmethod.latin", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    	try {
    		getPackageManager().setApplicationEnabledSetting("com.google.android.apps.enterprise.dmagent", 1, 0);
    	} catch (Exception e) {
    		//do nothing
    	}
    }

    /**
    *
    * get Memory Total Size for prop, because the EnhancedColor function 
    * do not fully test in >=1G environment.
    *
    ************************************
    *      
    *ActionsCode(phchen, new_method)
    */
    private long getMemoryTotalSizeFromProp() {
        return android.os.SystemProperties.getLong("system.ram.total", 512);
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
    public static boolean getEnhancedColorSystem(ContentResolver resolver) {
        int enhanced = Settings.System.getInt(resolver, "actions_enhanced_color_system", 0) ;
        String value = SystemProperties.get(ENHANCED_COLOR_PROPERTY, "-1");
        if(!value.equals(String.valueOf(enhanced))) {
        	SystemProperties.set(ENHANCED_COLOR_PROPERTY, String.valueOf(enhanced));
        }
        
        return enhanced > 0;
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
    public static void setEnhancedColorSystem(ContentResolver resolver, boolean bEnhanced) {
    	int enhanced = bEnhanced ? 1:0;
    	Settings.System.putInt(resolver, "actions_enhanced_color_system", enhanced) ;
        SystemProperties.set(ENHANCED_COLOR_PROPERTY, String.valueOf(enhanced));
    }
    
    private static boolean allowAllRotations(Context context) {
        return Resources.getSystem().getBoolean(
                com.android.internal.R.bool.config_allowAllRotations);
    }

    private static boolean isLiftToWakeAvailable(Context context) {
        SensorManager sensors = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
        return sensors != null && sensors.getDefaultSensor(Sensor.TYPE_WAKE_GESTURE) != null;
    }

    private static boolean isDozeAvailable(Context context) {
        String name = Build.IS_DEBUGGABLE ? SystemProperties.get("debug.doze.component") : null;
        if (TextUtils.isEmpty(name)) {
            name = context.getResources().getString(
                    com.android.internal.R.string.config_dozeComponent);
        }
        return !TextUtils.isEmpty(name);
    }

    private static boolean isAutomaticBrightnessAvailable(Resources res) {
        return res.getBoolean(com.android.internal.R.bool.config_automatic_brightness_available);
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
    private void updateTvoutMenuStatus(boolean isPlugIn) {
        boolean resetValueFlag = isPlugIn;
		
		Log.d(TAG, "run in updateTvoutMenuStatus");
		if(mSupportCvbs) {
			Log.d(TAG, "the cvbs select mode="+ mCvbsUtils.getLastSelectModeValue());
			mCvbsModePref.setValue(String.valueOf(mCvbsUtils.getLastSelectModeValue()));
		}
		
		if(!mSupportHdmi) {
			return;
		}
		/** if ListPreference' dialog is showing, may be need to update the 
		 * hdmi mode list,when hdmi line plug,here just dismiss*/
		if(mHdmiModePref.getDialog() != null && mHdmiModePref.getDialog().isShowing()) {
			mHdmiModePref.getDialog().dismiss();
			resetValueFlag = true;
        }
		
		String[] hdmiEntries = null;
		String[] hdmiValues = null;
		
        if(mHdmiUtils.isCablePlugIn()) {
            Log.d(TAG,"mHdmiUtils.isCablePlugIn true");
            hdmiEntries = hdmiValues = mHdmiUtils.getSupportedModesList();
            Log.d(TAG,"hdmiEntries " + Arrays.toString(hdmiValues) + " length: " + hdmiValues.length);
			if(hdmiEntries != null){
	            mHdmiModePref.setEntries(hdmiEntries);
	            mHdmiModePref.setEntryValues(hdmiValues);
	            String str = mHdmiUtils.getMode();
	            str.trim();
	            Log.d(TAG,"getMode: " + str);
	            mHdmiModePref.setValue(str);
	        }
        }

        if(hdmiEntries == null || hdmiValues == null){
	    	Log.d(TAG, "hdmiEntries == null || hdmiValues == null");
            hdmiEntries = getResources().getStringArray(R.array.tvout_hdmi_entries);
            hdmiValues = getResources().getStringArray(R.array.tvout_hdmi_entries);
            mHdmiModePref.setEntries(hdmiEntries);
            mHdmiModePref.setEntryValues(hdmiValues);
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
	public static void tryToUpdateTvoutMenuStatus(boolean isPlugIn) {
		if(mDisplay == null) {
			return;
		}
		try {
			mDisplay.updateTvoutMenuStatus(isPlugIn);
		} catch (Exception e) {
			Log.e(TAG, "can't update the tvout menu status!");
		}
    }

    private void updateTimeoutPreferenceDescription(long currentTimeout) {
        ListPreference preference = mScreenTimeoutPreference;
        String summary;
        if (currentTimeout < 0) {
            // Unsupported value
            summary = "";
        } else {
            final CharSequence[] entries = preference.getEntries();
            final CharSequence[] values = preference.getEntryValues();
            if (entries == null || entries.length == 0) {
                summary = "";
            } else if (currentTimeout == MAX_TIME_OUT){
            	summary = entries[0].toString();
            } else {
                int best = 0;
                for (int i = 1; i < values.length; i++) {
                    long timeout = Long.parseLong(values[i].toString());
                    if (currentTimeout >= timeout) {
                        best = i;
                    }
                }
                summary = preference.getContext().getString(R.string.screen_timeout_summary,
                        entries[best]);
            }
        }
        preference.setSummary(summary);
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
    private void disableUnusableTimeouts(ListPreference screenTimeoutPreference) {
        final DevicePolicyManager dpm =
                (DevicePolicyManager) getActivity().getSystemService(
                Context.DEVICE_POLICY_SERVICE);
        long maxTimeout = dpm != null ? dpm.getMaximumTimeToLock(null) : 0;
        if (maxTimeout == 0) {
            return; // policy not enforced
        }
        
        //ActionsCode(phchen, new feature: use my max time out), 
        if (maxTimeout < MAX_TIME_OUT) {
        	maxTimeout = MAX_TIME_OUT;
        }

        final CharSequence[] entries = screenTimeoutPreference.getEntries();
        final CharSequence[] values = screenTimeoutPreference.getEntryValues();
        ArrayList<CharSequence> revisedEntries = new ArrayList<CharSequence>();
        ArrayList<CharSequence> revisedValues = new ArrayList<CharSequence>();
        for (int i = 0; i < values.length; i++) {
            long timeout = Long.parseLong(values[i].toString());
            if (timeout <= maxTimeout) {
                revisedEntries.add(entries[i]);
                revisedValues.add(values[i]);
            }
        }
        if (revisedEntries.size() != entries.length || revisedValues.size() != values.length) {
            final int userPreference = Integer.parseInt(screenTimeoutPreference.getValue());
            screenTimeoutPreference.setEntries(
                    revisedEntries.toArray(new CharSequence[revisedEntries.size()]));
            screenTimeoutPreference.setEntryValues(
                    revisedValues.toArray(new CharSequence[revisedValues.size()]));
            if (userPreference <= maxTimeout) {
                screenTimeoutPreference.setValue(String.valueOf(userPreference));
            } else if (revisedValues.size() > 0
                    && Long.parseLong(revisedValues.get(revisedValues.size() - 1).toString())
                    == maxTimeout) {
                // If the last one happens to be the same as the max timeout, select that
                screenTimeoutPreference.setValue(String.valueOf(maxTimeout));
            } else {
                // There will be no highlighted selection since nothing in the list matches
                // maxTimeout. The user can still select anything less than maxTimeout.
                // TODO: maybe append maxTimeout to the list and mark selected.
            }
        }
        screenTimeoutPreference.setEnabled(revisedEntries.size() > 0);
    }

    int floatToIndex(float val) {
        String[] indices = getResources().getStringArray(R.array.entryvalues_font_size);
        float lastVal = Float.parseFloat(indices[0]);
        for (int i=1; i<indices.length; i++) {
            float thisVal = Float.parseFloat(indices[i]);
            if (val < (lastVal + (thisVal-lastVal)*.5f)) {
                return i-1;
            }
            lastVal = thisVal;
        }
        return indices.length-1;
    }

    public void readFontSizePreference(ListPreference pref) {
        try {
            mCurConfig.updateFrom(ActivityManagerNative.getDefault().getConfiguration());
        } catch (RemoteException e) {
            Log.w(TAG, "Unable to retrieve font size");
        }

        // mark the appropriate item in the preferences list
        int index = floatToIndex(mCurConfig.fontScale);
        pref.setValueIndex(index);

        // report the current size in the summary text
        final Resources res = getResources();
        String[] fontSizeNames = res.getStringArray(R.array.entries_font_size);
        pref.setSummary(String.format(res.getString(R.string.summary_font_size),
                fontSizeNames[index]));
    }

    @Override
    public void onResume() {
        super.onResume();
        updateState();
    }

    @Override
    public Dialog onCreateDialog(int dialogId) {
        if (dialogId == DLG_GLOBAL_CHANGE_WARNING) {
            return Utils.buildGlobalChangeWarningDialog(getActivity(),
                    R.string.global_font_change_title,
                    new Runnable() {
                        public void run() {
                            mFontSizePref.click();
                        }
                    });
        }
        return null;
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
    private void updateState() {
        readFontSizePreference(mFontSizePref);
        updateScreenSaverSummary();
        //ActionsCode(phchen, new feature), 
        updateTvoutMenuStatus(false);

        // Update auto brightness if it is available.
        if (mAutoBrightnessPreference != null) {
            int brightnessMode = Settings.System.getInt(getContentResolver(),
                    SCREEN_BRIGHTNESS_MODE, SCREEN_BRIGHTNESS_MODE_MANUAL);
            mAutoBrightnessPreference.setChecked(brightnessMode != SCREEN_BRIGHTNESS_MODE_MANUAL);
        }

        // Update lift-to-wake if it is available.
        if (mLiftToWakePreference != null) {
            int value = Settings.Secure.getInt(getContentResolver(), WAKE_GESTURE_ENABLED, 0);
            mLiftToWakePreference.setChecked(value != 0);
        }

        // Update doze if it is available.
        if (mDozePreference != null) {
            int value = Settings.Secure.getInt(getContentResolver(), DOZE_ENABLED, 1);
            mDozePreference.setChecked(value != 0);
        }
    }

    private void updateScreenSaverSummary() {
        if (mScreenSaverPreference != null) {
            mScreenSaverPreference.setSummary(
                    DreamSettings.getSummaryTextWithDreamName(getActivity()));
        }
    }

    public void writeFontSizePreference(Object objValue) {
        try {
            mCurConfig.fontScale = Float.parseFloat(objValue.toString());
            ActivityManagerNative.getDefault().updatePersistentConfiguration(mCurConfig);
        } catch (RemoteException e) {
            Log.w(TAG, "Unable to save font size");
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
        //ActionsCode(phchen, new feature,add sensor menu),
        if(preference == mGsensorCalib) {            
            final Intent intent = new Intent(Intent.ACTION_MAIN);            
            intent.setClassName("com.actions.sensor.calib", "com.actions.sensor.calib.SensorActivity");            
            startActivity(intent);        
        }
        else if(preference == mTvoutScreenResize) {
        	final Intent intent = new Intent(Intent.ACTION_MAIN);
            intent.setClass(getActivity(), TvoutScreenResizeActivity.class);
            startActivity(intent);
        //ActionsCode(phchen, NEW FEATURE: BUG00236195), 
        } else if(preference == mEnhancedColor) {
        	setEnhancedColorSystem(getContentResolver(), mEnhancedColor.isChecked());
		//ActionsCode(phchen, NEW FEATURE: hidden google application),
		} else if(preference == disableapps){
        	Log.d(TAG, "preference == disableapps");
        	if(disableapps.isChecked()){
        		disableApps();      		
        	}else{
        		enableApps();
        	}
        	setDisableApp(getContentResolver(), disableapps.isChecked()); 
		}
        return super.onPreferenceTreeClick(preferenceScreen, preference);
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
    public boolean onPreferenceChange(Preference preference, Object objValue) {
        final String key = preference.getKey();
        if (KEY_SCREEN_TIMEOUT.equals(key)) {
            try {
                int value = Integer.parseInt((String) objValue);
                Settings.System.putInt(getContentResolver(), SCREEN_OFF_TIMEOUT, value);
                updateTimeoutPreferenceDescription(value);
            } catch (NumberFormatException e) {
                Log.e(TAG, "could not persist screen timeout setting", e);
            }
        }
        if (KEY_FONT_SIZE.equals(key)) {
            writeFontSizePreference(objValue);
        }
        if (preference == mAutoBrightnessPreference) {
            boolean auto = (Boolean) objValue;
            Settings.System.putInt(getContentResolver(), SCREEN_BRIGHTNESS_MODE,
                    auto ? SCREEN_BRIGHTNESS_MODE_AUTOMATIC : SCREEN_BRIGHTNESS_MODE_MANUAL);
        }
        if (preference == mLiftToWakePreference) {
            boolean value = (Boolean) objValue;
            Settings.Secure.putInt(getContentResolver(), WAKE_GESTURE_ENABLED, value ? 1 : 0);
        }
        if (preference == mDozePreference) {
            boolean value = (Boolean) objValue;
            Settings.Secure.putInt(getContentResolver(), DOZE_ENABLED, value ? 1 : 0);
        }
        //ActionsCode(phchen, new feature), 
        if(KEY_CVBS_MODE_SELECTOR.equals(key)) {
        	String value = (String) objValue;        	     	
        	if(!value.equals(mHdmiUtils.getMode())){
                mCvbsUtils.switchToSelectModeByModeValue(value);
                mCvbsModePref.setValue(value);   
            }
        }

		//ActionsCode(phchen, BUGFIX: BUG00258039 )
        if(mHdmiUtils.isCablePlugIn()){
	        if(KEY_HDMI_MODE_SELECTOR.equals(key)) {
	        	String value = (String) objValue;        	     	
	        	if(!value.equals(mHdmiUtils.getMode())){
	                mHdmiUtils.switchToSelectModeByModeValue(value);
	                mHdmiModePref.setValue(value);   
	            }
	        }
    	}
    	
        return true;
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        if (preference == mFontSizePref) {
            if (Utils.hasMultipleUsers(getActivity())) {
                showDialog(DLG_GLOBAL_CHANGE_WARNING);
                return true;
            } else {
                mFontSizePref.click();
            }
        }
        return false;
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
    public class TvoutSettingsListPreference extends ListPreference implements CompoundButton.OnCheckedChangeListener {
    	private Switch mSwitchView = null;
    	
    	public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
    	 	Log.d(TAG, "isChecked="+isChecked);
    	 	if(getLastSwitchStatus() != isChecked) {
    	 		//setLastSwitchStatus(isChecked);
    	 		mHdmiUtils.setHdmiEnable(isChecked);
    	 		//if(!isChecked) {
    	 		//    mHdmiUtils.closeTvoutDisplay(); 
    	 		//}
    	 	}
    	}
        
        public TvoutSettingsListPreference(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        public TvoutSettingsListPreference(Context context) {
            super(context, null);
        }

        private boolean getLastSwitchStatus() {
			return mHdmiUtils.getHdmiEnable();
        }
        
        protected void onClick() {
            if (mSwitchView != null && !mSwitchView.isChecked()) {
                return;
            }

            if (getDialog() != null && getDialog().isShowing())
                return;

            showDialog(null);
        }
        @Override
        protected void onBindView(View view) {
            super.onBindView(view);

            View checkableView = view.findViewById(R.id.tvoutSwitch);
            Log.d(TAG, "checkableView=" + checkableView + ",isEnabled=" + this.isEnabled());
            if (checkableView != null && checkableView instanceof Checkable) {
                ((Checkable) checkableView).setChecked(getLastSwitchStatus());

                if (checkableView instanceof Switch) {
                    mSwitchView = (Switch) checkableView;
                    mSwitchView.setOnCheckedChangeListener(this);
                }
            }
            // syncSummaryView(view);
        }
        
        @Override
        protected void onPrepareDialogBuilder(Builder builder) {
            Log.d(TAG, "onPrepareDialogBuilder");
            updateTvoutMenuStatus(false);
            super.onPrepareDialogBuilder(builder);
        }
    }

    public static final Indexable.SearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider() {
                @Override
                public List<SearchIndexableResource> getXmlResourcesToIndex(Context context,
                        boolean enabled) {
                    ArrayList<SearchIndexableResource> result =
                            new ArrayList<SearchIndexableResource>();

                    SearchIndexableResource sir = new SearchIndexableResource(context);
                    sir.xmlResId = R.xml.display_settings;
                    result.add(sir);

                    return result;
                }

                @Override
                public List<String> getNonIndexableKeys(Context context) {
                    ArrayList<String> result = new ArrayList<String>();
                    if (!context.getResources().getBoolean(
                            com.android.internal.R.bool.config_dreamsSupported)) {
                        result.add(KEY_SCREEN_SAVER);
                    }
                    if (!isAutomaticBrightnessAvailable(context.getResources())) {
                        result.add(KEY_AUTO_BRIGHTNESS);
                    }
                    if (!isLiftToWakeAvailable(context)) {
                        result.add(KEY_LIFT_TO_WAKE);
                    }
                    if (!isDozeAvailable(context)) {
                        result.add(KEY_DOZE);
                    }
                    if (!RotationPolicy.isRotationLockToggleVisible(context)) {
                        result.add(KEY_AUTO_ROTATE);
                    }
                    return result;
                }
            };
}
