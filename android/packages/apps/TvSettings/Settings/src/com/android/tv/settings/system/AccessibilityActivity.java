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

import com.android.internal.view.RotationPolicy;
import com.android.tv.settings.ActionBehavior;
import com.android.tv.settings.BaseSettingsActivity;

import static android.provider.Settings.Secure.TTS_DEFAULT_RATE;
import static android.provider.Settings.Secure.TTS_DEFAULT_SYNTH;
import com.android.tv.settings.ActionKey;
import com.android.tv.settings.R;
import com.android.tv.settings.system.DeveloperOptionsActivity.MyApplicationInfo;
import com.android.tv.settings.util.SettingsHelper;
import com.android.tv.settings.dialog.old.Action;
import com.android.tv.settings.dialog.old.ActionAdapter;
import com.android.tv.settings.dialog.old.ContentFragment;
import com.android.tv.settings.util.SettingsHelper;

import android.accessibilityservice.AccessibilityServiceInfo;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.view.accessibility.AccessibilityManager;
import android.widget.TextView;
import android.content.Intent;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.os.Bundle;
import android.provider.Settings;
import android.speech.tts.TextToSpeech;
import android.text.TextUtils.SimpleStringSplitter;
import android.speech.tts.TtsEngines;
import android.util.Log;
import android.util.Pair;
import android.speech.tts.TextToSpeech.EngineInfo;
import android.speech.tts.UtteranceProgressListener;
import android.text.TextUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;

public class AccessibilityActivity extends BaseSettingsActivity implements ActionAdapter.Listener {

    private static final String TAG = "AccessibilityActivity";
    private static final boolean DEBUG = false;

    private static final int GET_SAMPLE_TEXT = 1983;
    private static final int VOICE_DATA_INTEGRITY_CHECK = 1977;

    private static final char ENABLED_ACCESSIBILITY_SERVICES_SEPARATOR = ':';

    private SettingsHelper mHelper;

    private boolean mTtsSettingsEnabled;
    private TextToSpeech mTts = null;
    private TtsEngines mEnginesHelper = null;
    private String mCurrentEngine;
    private String mPreviousEngine;
    private Intent mVoiceCheckData;

    private String mServiceSettingTitle;
    private String mSelectedServiceComponent;
    private String mSelectedServiceSettings;
    private boolean mSelectedServiceEnabled;

    /**
     * The initialization listener used when we are initalizing the settings
     * screen for the first time (as opposed to when a user changes his choice
     * of engine).
     */
    private final TextToSpeech.OnInitListener mInitListener = new TextToSpeech.OnInitListener() {
            @Override
        public void onInit(int status) {
            onInitEngine(status);
        }
    };

    /**
     * The initialization listener used when the user changes his choice of
     * engine (as opposed to when then screen is being initialized for the first
     * time).
     */
    private final TextToSpeech.OnInitListener mUpdateListener = new TextToSpeech.OnInitListener() {
        @Override
        public void onInit(int status) {
            onUpdateEngine(status);
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mResources = getResources();
        mHelper = new SettingsHelper(this);
        mActions = new ArrayList<Action>();

        mTts = new TextToSpeech(getApplicationContext(), mInitListener);
        mEnginesHelper = new TtsEngines(getApplicationContext());
        initSettings();

        super.onCreate(savedInstanceState);
    }

    @Override
    protected void refreshActionList() {
        mActions.clear();
        switch ((ActionType) mState) {
            case ACCESSIBILITY_OVERVIEW:
                mActions.add(ActionType.ACCESSIBILITY_CAPTIONS.toAction(mResources));
                mActions.add(ActionType.ACCESSIBILITY_SERVICES.toAction(mResources));
                // TODO b/18007521
                // uncomment when Talkback is able to support not speaking passwords aloud
                //mActions.add(ActionType.ACCESSIBILITY_SPEAK_PASSWORDS.toAction(mResources,
                //        mHelper.getSecureStatusIntSetting(
                //                Settings.Secure.ACCESSIBILITY_SPEAK_PASSWORD)));
                mActions.add(ActionType.ACCESSIBILITY_TTS_OUTPUT.toAction(mResources,
                                getDisplayNameForEngine(mTts.getCurrentEngine())));
                break;
            case ACCESSIBILITY_SERVICES:
                mActions = getInstalledServicesActions();
                break;
            case ACCESSIBILITY_SERVICES_SETTINGS:
                mActions.add(ActionType.ACCESSIBILITY_SERVICES_STATUS.toAction(mResources,
                        mHelper.getStatusStringFromBoolean(mSelectedServiceEnabled)));
                if (mSelectedServiceSettings != null) {
                    mActions.add(ActionType.ACCESSIBILITY_SERVICE_CONFIG.toAction(mResources));
                }
                break;
            case ACCESSIBILITY_SERVICES_STATUS:
                mActions = getEnableActions(mServiceSettingTitle, mSelectedServiceEnabled);
                break;
            case ACCESSIBILITY_SERVICES_CONFIRM_ON:
                mActions.add(ActionType.AGREE.toAction(mResources));
                mActions.add(ActionType.DISAGREE.toAction(mResources));
                break;
            case ACCESSIBILITY_SERVICES_CONFIRM_OFF:
                mActions.add(ActionType.OK.toAction(mResources));
                mActions.add(ActionType.CANCEL.toAction(mResources));
                break;
            case ACCESSIBILITY_SPEAK_PASSWORDS:
                mActions = getEnableActions(((ActionType) mState).name(), getProperty());
                break;
            case ACCESSIBILITY_TTS_OUTPUT:
                mActions.add(ActionType.ACCESSIBILITY_PREFERRED_ENGINE.toAction(
                        mResources, getDisplayNameForEngine(mTts.getCurrentEngine())));
                if (mTtsSettingsEnabled) {
                    if (mTts.getLanguage() != null) {
                        mActions.add(ActionType.ACCESSIBILITY_LANGUAGE.toAction(
                                mResources, mTts.getLanguage().getDisplayName()));
                    } else {
                        mActions.add(ActionType.ACCESSIBILITY_LANGUAGE.toAction(
                                mResources, " "));
                    }
                }
                mActions.add(ActionType.ACCESSIBILITY_INSTALL_VOICE_DATA.toAction(mResources));
                mActions.add(ActionType.ACCESSIBILITY_SPEECH_RATE.toAction(
                        mResources, getTtsRate(mHelper.getSecureIntSetting(
                                TTS_DEFAULT_RATE, getString(R.string.tts_rate_default_value)))));
                if (mTtsSettingsEnabled) {
                    mActions.add(ActionType.ACCESSIBILITY_PLAY_SAMPLE.toAction(mResources));
                }
                break;
            case ACCESSIBILITY_PREFERRED_ENGINE:
                mActions = getEngines();
                break;
            case ACCESSIBILITY_LANGUAGE:
                final ArrayList<String> available = mVoiceCheckData.getStringArrayListExtra(
                        TextToSpeech.Engine.EXTRA_AVAILABLE_VOICES);
                if (available != null && available.size() > 0) {
                    mActions = updateDefaultLocalePref(available);
                    if (mTts.getLanguage() != null) {
                        String currLang = getLanguageString(mTts.getLanguage());
                        checkSelectedAction(mActions, currLang);
                    }
                }
                break;
            case ACCESSIBILITY_SPEECH_RATE:
                mActions = Action.createActionsFromArrays(
                        mResources.getStringArray(R.array.tts_rate_values),
                        mResources.getStringArray(R.array.tts_rate_entries));
                checkSelectedAction(mActions, mHelper.getSecureIntSetting(
                        TTS_DEFAULT_RATE, getString(R.string.tts_rate_default_value)));
                break;
            default:
                break;
        }
    }

    @Override
    protected void updateView() {
        refreshActionList();
        switch ((ActionType) mState) {
            case ACCESSIBILITY_SERVICES_SETTINGS:
                setView(mServiceSettingTitle,
                        ((ActionType) getPrevState()).getTitle(mResources),
                        null, R.drawable.ic_settings_accessibility);
                break;
            case ACCESSIBILITY_SERVICES_STATUS:
                setView(((ActionType) mState).getTitle(mResources), mServiceSettingTitle,
                        null,
                        R.drawable.ic_settings_accessibility);
                return;
            case ACCESSIBILITY_SERVICES_CONFIRM_ON:
                String onTitle = getString(
                        R.string.system_accessibility_service_on_confirm_title,
                        mServiceSettingTitle);
                setView(onTitle, mServiceSettingTitle,
                        getString(R.string.system_accessibility_service_on_confirm_desc,
                                mServiceSettingTitle), R.drawable.ic_settings_accessibility);
                return;
            case ACCESSIBILITY_SERVICES_CONFIRM_OFF:
                String offTitle = getString(
                        R.string.system_accessibility_service_off_confirm_title,
                        mServiceSettingTitle);
                setView(offTitle, mServiceSettingTitle,
                        getString(R.string.system_accessibility_service_off_confirm_desc,
                                mServiceSettingTitle), R.drawable.ic_settings_accessibility);
                return;
            default:
                setView(((ActionType) mState).getTitle(mResources),
                        getPrevState() != null ?
                        ((ActionType) getPrevState()).getTitle(mResources) :
                        getString(R.string.settings_app_name),
                        ((ActionType) mState).getDescription(mResources),
                        R.drawable.ic_settings_accessibility);
        }
    }

    private String getTtsRate(String value) {
        String[] values = mResources.getStringArray(R.array.tts_rate_values);
        String[] entries = mResources.getStringArray(R.array.tts_rate_entries);
        for (int index = 0; index < values.length; ++index) {
            if (values[index].equals(value)) {
                return entries[index];
            }
        }
        return "";
    }

    private ArrayList<Action> getInstalledServicesActions() {
        ArrayList<Action> actions = new ArrayList<Action>();
        final List<AccessibilityServiceInfo> installedServiceInfos = AccessibilityManager
                .getInstance(this).getInstalledAccessibilityServiceList();

        Set<ComponentName> enabledServices = getEnabledServicesFromSettings(this);

        final boolean accessibilityEnabled = Settings.Secure.getInt(getContentResolver(),
                Settings.Secure.ACCESSIBILITY_ENABLED, 0) == 1;

        for (AccessibilityServiceInfo accInfo : installedServiceInfos) {
            ServiceInfo serviceInfo = accInfo.getResolveInfo().serviceInfo;
            ComponentName componentName = new ComponentName(serviceInfo.packageName,
                    serviceInfo.name);
            final boolean serviceEnabled = accessibilityEnabled
                    && enabledServices.contains(componentName);
            String title = accInfo.getResolveInfo().loadLabel(getPackageManager()).toString();
            actions.add(new Action.Builder()
                    .key(componentName.flattenToString())
                    .title(title)
                    .description(mHelper.getStatusStringFromBoolean(serviceEnabled))
                    .build());
        }
        return actions;
    }

    private String getSettingsForService(String serviceComponentName) {
        final List<AccessibilityServiceInfo> installedServiceInfos = AccessibilityManager
                .getInstance(this).getInstalledAccessibilityServiceList();
        ComponentName comp = ComponentName.unflattenFromString(serviceComponentName);

        if (comp != null) {
            for (AccessibilityServiceInfo accInfo : installedServiceInfos) {
                ServiceInfo serviceInfo = accInfo.getResolveInfo().serviceInfo;
                if (serviceInfo.packageName.equals(comp.getPackageName()) &&
                        serviceInfo.name.equals(comp.getClassName())) {
                    String settingsClassName = accInfo.getSettingsActivityName();
                    if (!TextUtils.isEmpty(settingsClassName)) {
                        ComponentName settingsComponent =
                                new ComponentName(comp.getPackageName(), settingsClassName);
                        return settingsComponent.flattenToString();
                    } else {
                        return null;
                    }
                }
            }
        }
        return null;
    }

    private static Set<ComponentName> getEnabledServicesFromSettings(Context context) {
        String enabledServicesSetting = Settings.Secure.getString(context.getContentResolver(),
                Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES);
        if (enabledServicesSetting == null) {
            enabledServicesSetting = "";
        }
        Set<ComponentName> enabledServices = new HashSet<ComponentName>();
        SimpleStringSplitter colonSplitter = new SimpleStringSplitter(
                ENABLED_ACCESSIBILITY_SERVICES_SEPARATOR);
        colonSplitter.setString(enabledServicesSetting);
        while (colonSplitter.hasNext()) {
            String componentNameString = colonSplitter.next();
            ComponentName enabledService = ComponentName.unflattenFromString(
                    componentNameString);
            if (enabledService != null) {
                enabledServices.add(enabledService);
            }
        }
        return enabledServices;
    }

    private ArrayList<Action> getEnableActions(String type, boolean enabled) {
        ArrayList<Action> actions = new ArrayList<Action>();
        actions.add(ActionBehavior.ON.toAction(ActionBehavior.getOnKey(type), mResources, enabled));
        actions.add(ActionBehavior.OFF.toAction(ActionBehavior.getOffKey(type), mResources,
                !enabled));
        return actions;
    }

    private void checkSelectedAction(ArrayList<Action> actions, String selectedKey) {
        for (Action action : actions) {
            if (action.getKey().equalsIgnoreCase(selectedKey)) {
                action.setChecked(true);
                break;
            }
        }
    }

    private String getLanguageString(Locale lang) {
        if (lang.getLanguage().isEmpty())
            return "";

        StringBuilder builder = new StringBuilder();
        builder.append(lang.getLanguage());

        if (!lang.getCountry().isEmpty()) {
            builder.append('-');
            builder.append(lang.getCountry());
        }

        if (!lang.getVariant().isEmpty()) {
            builder.append('-');
            builder.append(lang.getVariant());
        }

        return builder.toString();
    }

    private void updateDefaultEngine(String engine) {
        if (DEBUG) {
            Log.d(TAG, "Updating default synth to : " + engine);
        }

        // TODO Disable the "play sample text" preference and the speech
        // rate preference while the engine is being swapped.

        // Keep track of the previous engine that was being used. So that
        // we can reuse the previous engine.
        //
        // Note that if TextToSpeech#getCurrentEngine is not null, it means at
        // the very least that we successfully bound to the engine service.
        mPreviousEngine = mTts.getCurrentEngine();

        // Step 1: Shut down the existing TTS engine.
        if (mTts != null) {
            try {
                mTts.shutdown();
                mTts = null;
            } catch (Exception e) {
                Log.e(TAG, "Error shutting down TTS engine" + e);
            }
        }

        // Step 2: Connect to the new TTS engine.
        // Step 3 is continued on #onUpdateEngine (below) which is called when
        // the app binds successfully to the engine.
        if (DEBUG) {
            Log.d(TAG, "Updating engine : Attempting to connect to engine: " + engine);
        }
        mTts = new TextToSpeech(getApplicationContext(), mUpdateListener, engine);
        setTtsUtteranceProgressListener();
    }

    @Override
    public void onActionClicked(Action action) {
        /*
         * For list preferences
         */
        final String key = action.getKey();
        final String title = action.getTitle();
        switch((ActionType)mState){
            case ACCESSIBILITY_SERVICES:
                mServiceSettingTitle = action.getTitle();
                mSelectedServiceComponent = action.getKey();
                mSelectedServiceEnabled = mHelper.getStatusFromString(action.getDescription());
                mSelectedServiceSettings = getSettingsForService(mSelectedServiceComponent);
                if (mSelectedServiceSettings != null) {
                    // Service provides a settings component, so go to the Status/Settings screen
                    setState(ActionType.ACCESSIBILITY_SERVICES_SETTINGS, true);
                } else {
                    // Service does not provide Settings, so go straight to Enable/Disable
                    setState(ActionType.ACCESSIBILITY_SERVICES_STATUS, true);
                }
                return;
            case ACCESSIBILITY_PREFERRED_ENGINE:
                mCurrentEngine = key;
                updateDefaultEngine(mCurrentEngine);
                // Delay the goBack here until we are done binding to the service, so we have
                // the Language data available for the previous screen.
                return;
            case ACCESSIBILITY_LANGUAGE:
                updateLanguageTo(
                        !TextUtils.isEmpty(key) ? mEnginesHelper.parseLocaleString(key) : null);
                goBack();
                return;
            case ACCESSIBILITY_SPEECH_RATE:
                mHelper.setSecureIntValueSetting(TTS_DEFAULT_RATE, key);
                goBack();
                return;
        }

        /*
         * For regular states
         */
        ActionKey<ActionType, ActionBehavior> actionKey = new ActionKey<ActionType, ActionBehavior>(
                ActionType.class, ActionBehavior.class, action.getKey());
        final ActionType type = actionKey.getType();
        if (type != null) {
            switch (type) {
                case ACCESSIBILITY_PLAY_SAMPLE:
                    getSampleText();
                    return;
                case ACCESSIBILITY_INSTALL_VOICE_DATA:
                    installVoiceData();
                    return;
                case ACCESSIBILITY_SERVICE_CONFIG: {
                    ComponentName comp = ComponentName.unflattenFromString(
                            mSelectedServiceSettings);
                    Intent settingsIntent = new Intent(Intent.ACTION_MAIN).setComponent(comp);
                    startActivity(settingsIntent);
                    return;
                }
                case ACCESSIBILITY_CAPTIONS: {
                    ComponentName comp = new ComponentName(this, CaptionSetupActivity.class);
                    Intent captionsIntent = new Intent(Intent.ACTION_MAIN).setComponent(comp);
                    startActivity(captionsIntent);
                    return;
                }
                case AGREE:
                    setProperty(true); // Agreed to turn ON service
                    return;
                case DISAGREE:
                    setProperty(false); // Disagreed to turn service ON
                    return;
                case OK:
                    setProperty(false); // ok to STOP Service
                    return;
                case CANCEL:
                    goBack(); // Cancelled request to STOP service
                    return;
                default:
            }
        }

        final ActionBehavior behavior = actionKey.getBehavior();
        switch (behavior) {
            case ON:
                setProperty(true);
                return;
            case OFF:
                setProperty(false);
                return;
            default:
        }
        setState(type, true);
    }

    @Override
    protected void setProperty(boolean enable) {
        switch ((ActionType) mState) {
            case ACCESSIBILITY_SPEAK_PASSWORDS:
                mHelper.setSecureIntSetting(Settings.Secure.ACCESSIBILITY_SPEAK_PASSWORD, enable);
                break;
            case ACCESSIBILITY_SERVICES_STATUS:
                // Accessibility Service ON/OFF requires an extra confirmation screen.
                if (enable) {
                    setState(ActionType.ACCESSIBILITY_SERVICES_CONFIRM_ON, true);
                } else {
                    if (mSelectedServiceEnabled) {
                        setState(ActionType.ACCESSIBILITY_SERVICES_CONFIRM_OFF, true);
                    } else {
                        goBack();
                    }
                }
                return;
            case ACCESSIBILITY_SERVICES_CONFIRM_ON:
                setAccessibilityServiceState(mSelectedServiceComponent, enable);
                mSelectedServiceEnabled = enable;
                // go back twice: Remove the ON/OFF screen from the stack
                goBack();
                break;
            case ACCESSIBILITY_SERVICES_CONFIRM_OFF:
                setAccessibilityServiceState(mSelectedServiceComponent, enable);
                mSelectedServiceEnabled = enable;
                // go back twice: Remove the ON/OFF screen from the stack
                goBack();
                break;
        }
        goBack();
    }

    private boolean getProperty() {
        if ((ActionType) mState == ActionType.ACCESSIBILITY_SPEAK_PASSWORDS) {
            return mHelper.getSecureIntValueSettingToBoolean(
                    Settings.Secure.ACCESSIBILITY_SPEAK_PASSWORD);
        }
        return false;
    }

    private Set<ComponentName> getInstalledServices() {
        Set<ComponentName> installedServices = new HashSet<ComponentName>();
        installedServices.clear();

        List<AccessibilityServiceInfo> installedServiceInfos =
                AccessibilityManager.getInstance(this)
                        .getInstalledAccessibilityServiceList();
        if (installedServiceInfos == null) {
            return installedServices;
        }

        final int installedServiceInfoCount = installedServiceInfos.size();
        for (int i = 0; i < installedServiceInfoCount; i++) {
            ResolveInfo resolveInfo = installedServiceInfos.get(i).getResolveInfo();
            ComponentName installedService = new ComponentName(
                    resolveInfo.serviceInfo.packageName,
                    resolveInfo.serviceInfo.name);
            installedServices.add(installedService);
        }
        return installedServices;
    }

    public void setAccessibilityServiceState(String preferenceKey, boolean enabled) {
        // Parse the enabled services.
        Set<ComponentName> enabledServices = getEnabledServicesFromSettings(this);

        // Determine enabled services and accessibility state.
        ComponentName toggledService = ComponentName.unflattenFromString(preferenceKey);
        boolean accessibilityEnabled = false;
        if (enabled) {
            enabledServices.add(toggledService);
            // Enabling at least one service enables accessibility.
            accessibilityEnabled = true;
        } else {
            enabledServices.remove(toggledService);
            // Check how many enabled and installed services are present.
            Set<ComponentName> installedServices = getInstalledServices();
            for (ComponentName enabledService : enabledServices) {
                if (installedServices.contains(enabledService)) {
                    // Disabling the last service disables accessibility.
                    accessibilityEnabled = true;
                    break;
                }
            }
        }

        // Update the enabled services setting.
        StringBuilder enabledServicesBuilder = new StringBuilder();
        // Keep the enabled services even if they are not installed since we
        // have no way to know whether the application restore process has
        // completed. In general the system should be responsible for the
        // clean up not settings.
        for (ComponentName enabledService : enabledServices) {
            enabledServicesBuilder.append(enabledService.flattenToString());
            enabledServicesBuilder.append(ENABLED_ACCESSIBILITY_SERVICES_SEPARATOR);
        }
        final int enabledServicesBuilderLength = enabledServicesBuilder.length();
        if (enabledServicesBuilderLength > 0) {
            enabledServicesBuilder.deleteCharAt(enabledServicesBuilderLength - 1);
        }
        Settings.Secure.putString(getContentResolver(),
                Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES,
                enabledServicesBuilder.toString());

        // Update accessibility enabled.
        Settings.Secure.putInt(getContentResolver(),
                Settings.Secure.ACCESSIBILITY_ENABLED, accessibilityEnabled ? 1 : 0);
    }

    private ArrayList<Action> getEngines() {
        ArrayList<Action> actions = new ArrayList<Action>();
        List<EngineInfo> engines = mEnginesHelper.getEngines();
        int totalEngine = engines.size();
        for (int i = 0; i < totalEngine; i++) {
            Action action = new Action.Builder()
                    .key(engines.get(i).name)
                    .title(engines.get(i).label)
                    .build();
            actions.add(action);
        }
        mCurrentEngine = mTts.getCurrentEngine();
        checkVoiceData(mCurrentEngine);
        return actions;
    }

    private String getDisplayNameForEngine(String enginePackageName) {
        List<EngineInfo> engines = mEnginesHelper.getEngines();
        int totalEngine = engines.size();
        for (int i = 0; i < totalEngine; i++) {
            if (engines.get(i).name.equals(enginePackageName)) {
                return engines.get(i).label;
            }
        }
        // Not found, return package name then
        return enginePackageName;
    }

    /*
     * Check whether the voice data for the engine is ok.
     */
    private void checkVoiceData(String engine) {
        Intent intent = new Intent(TextToSpeech.Engine.ACTION_CHECK_TTS_DATA);
        intent.setPackage(engine);
        try {
            if (DEBUG) {
                Log.d(TAG, "Updating engine: Checking voice data: " + intent.toUri(0));
            }
            startActivityForResult(intent, VOICE_DATA_INTEGRITY_CHECK);
        } catch (ActivityNotFoundException ex) {
            Log.e(TAG, "Failed to check TTS data, no activity found for " + intent + ")");
        }
    }

    /**
     * Called when the TTS engine is initialized.
     */
    public void onInitEngine(int status) {
        if (status == TextToSpeech.SUCCESS) {
            if (DEBUG) {
                Log.d(TAG, "TTS engine for settings screen initialized.");
            }
        } else {
            if (DEBUG) {
                Log.d(TAG, "TTS engine for settings screen failed to initialize successfully.");
            }
        }
    }

    private void initSettings() {
        mCurrentEngine = mTts.getCurrentEngine();

        checkVoiceData(mCurrentEngine);
    }

    /*
     * Step 3: We have now bound to the TTS engine the user requested. We will
     * attempt to check voice data for the engine if we successfully bound to it,
     * or revert to the previous engine if we didn't.
     */
    public void onUpdateEngine(int status) {
        if (status == TextToSpeech.SUCCESS) {
            if (DEBUG) {
                Log.d(TAG, "Updating engine: Successfully bound to the engine: " +
                        mTts.getCurrentEngine());
            }
            checkVoiceData(mTts.getCurrentEngine());
        } else {
            if (DEBUG) {
                Log.d(TAG, "Updating engine: Failed to bind to engine, reverting.");
            }
            if (mPreviousEngine != null) {
                // This is guaranteed to at least bind, since mPreviousEngine
                // would be
                // null if the previous bind to this engine failed.
                mTts = new TextToSpeech(getApplicationContext(), mInitListener,
                        mPreviousEngine);
                setTtsUtteranceProgressListener();
            }
            mPreviousEngine = null;
        }
        goBack();
    }

    private void setTtsUtteranceProgressListener() {
        if (mTts == null) {
            return;
        }
        mTts.setOnUtteranceProgressListener(new UtteranceProgressListener() {
                @Override
            public void onStart(String utteranceId) {
            }

                @Override
            public void onDone(String utteranceId) {
            }

                @Override
            public void onError(String utteranceId) {
                Log.e(TAG, "Error while trying to synthesize sample text");
            }
        });
    }

    private ArrayList<Action> updateDefaultLocalePref(ArrayList<String> availableLangs) {
        ArrayList<Action> actions = new ArrayList<Action>();
        Locale currentLocale = mEnginesHelper.getLocalePrefForEngine(mCurrentEngine);

        ArrayList<Pair<String, Locale>> entryPairs =
                new ArrayList<Pair<String, Locale>>(availableLangs.size());
        for (int i = 0; i < availableLangs.size(); i++) {
            Locale locale = mEnginesHelper.parseLocaleString(availableLangs.get(i));
            if (locale != null) {
                entryPairs.add(new Pair<String, Locale>(
                        locale.getDisplayName(), locale));
            }
        }

        // Sort it
        Collections.sort(entryPairs, new Comparator<Pair<String, Locale>>() {
                @Override
            public int compare(Pair<String, Locale> lhs, Pair<String, Locale> rhs) {
                return lhs.first.compareToIgnoreCase(rhs.first);
            }
        });

        // Get two arrays out of one of pairs
        int selectedLanguageIndex = -1;
        int i = 0;
        for (Pair<String, Locale> entry : entryPairs) {
            if (entry.second.equals(currentLocale)) {
                selectedLanguageIndex = i;
            }
            Action action = new Action.Builder()
                    .key(entry.second.toString())
                    .title(entry.first).build();
            actions.add(action);
        }
        return actions;
    }

    private void updateLanguageTo(Locale locale) {
        mEnginesHelper.updateLocalePrefForEngine(mCurrentEngine, locale);
        if (mCurrentEngine.equals(mTts.getCurrentEngine())) {
            // Null locale means "use system default"
            mTts.setLanguage((locale != null) ? locale : Locale.getDefault());
        }
    }

    /**
     * Called when voice data integrity check returns
     */
    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == GET_SAMPLE_TEXT) {
            onSampleTextReceived(resultCode, data);
        } else if (requestCode == VOICE_DATA_INTEGRITY_CHECK) {
            onVoiceDataIntegrityCheckDone(data);
        }
    }

    /**
     * Ask the current default engine to return a string of sample text to be
     * spoken to the user.
     */
    private void getSampleText() {
        String currentEngine = mTts.getCurrentEngine();

        if (TextUtils.isEmpty(currentEngine))
            currentEngine = mTts.getDefaultEngine();

        Locale defaultLocale = mTts.getDefaultLanguage();
        if (defaultLocale == null) {
            Log.e(TAG, "Failed to get default language from engine " + currentEngine);
            return;
        }
        mTts.setLanguage(defaultLocale);

        // TODO: This is currently a hidden private API. The intent extras
        // and the intent action should be made public if we intend to make this
        // a public API. We fall back to using a canned set of strings if this
        // doesn't work.
        Intent intent = new Intent(TextToSpeech.Engine.ACTION_GET_SAMPLE_TEXT);

        intent.putExtra("language", defaultLocale.getLanguage());
        intent.putExtra("country", defaultLocale.getCountry());
        intent.putExtra("variant", defaultLocale.getVariant());
        intent.setPackage(currentEngine);

        try {
            if (DEBUG) {
                Log.d(TAG, "Getting sample text: " + intent.toUri(0));
            }
            startActivityForResult(intent, GET_SAMPLE_TEXT);
        } catch (ActivityNotFoundException ex) {
            Log.e(TAG, "Failed to get sample text, no activity found for " + intent + ")");
        }
    }

    private String getDefaultSampleString() {
        if (mTts != null && mTts.getLanguage() != null) {
            final String currentLang = mTts.getLanguage().getISO3Language();
            String[] strings = mResources.getStringArray(R.array.tts_demo_strings);
            String[] langs = mResources.getStringArray(R.array.tts_demo_string_langs);

            for (int i = 0; i < strings.length; ++i) {
                if (langs[i].equals(currentLang)) {
                    return strings[i];
                }
            }
        }
        return null;
    }

    private void onSampleTextReceived(int resultCode, Intent data) {
        String sample = getDefaultSampleString();

        if (resultCode == TextToSpeech.LANG_AVAILABLE && data != null) {
            if (data != null && data.getStringExtra("sampleText") != null) {
                sample = data.getStringExtra("sampleText");
            }
            if (DEBUG) {
                Log.d(TAG, "Got sample text: " + sample);
            }
        } else {
            if (DEBUG) {
                Log.d(TAG, "Using default sample text :" + sample);
            }
        }

        if (sample != null && mTts != null) {
            // The engine is guaranteed to have been initialized here
            // because this preference is not enabled otherwise.

            final boolean networkRequired = isNetworkRequiredForSynthesis();
            if (!networkRequired || networkRequired &&
                    (mTts.isLanguageAvailable(mTts.getLanguage()) >= TextToSpeech.LANG_AVAILABLE)) {
                HashMap<String, String> params = new HashMap<String, String>();
                params.put(TextToSpeech.Engine.KEY_PARAM_UTTERANCE_ID, "Sample");

                mTts.speak(sample, TextToSpeech.QUEUE_FLUSH, params);
            } else {
                Log.w(TAG, "Network required for sample synthesis for requested language");
                // TODO displayNetworkAlert();
            }
        } else {
            // TODO: Display an error here to the user.
            Log.e(TAG, "Did not have a sample string for the requested language");
        }
    }

    private boolean isNetworkRequiredForSynthesis() {
        Set<String> features = mTts.getFeatures(mTts.getLanguage());
        return features.contains(TextToSpeech.Engine.KEY_FEATURE_NETWORK_SYNTHESIS) &&
                !features.contains(TextToSpeech.Engine.KEY_FEATURE_EMBEDDED_SYNTHESIS);
    }

    /*
     * Step 5: The voice data check is complete.
     */
    private void onVoiceDataIntegrityCheckDone(Intent data) {
        final String engine = mTts.getCurrentEngine();

        if (engine == null) {
            Log.e(TAG, "Voice data check complete, but no engine bound");
            return;
        }

        if (data == null) {
            Log.e(TAG, "Engine failed voice data integrity check (null return)" +
                    mTts.getCurrentEngine());
            return;
        }

        Settings.Secure.putString(getContentResolver(), TTS_DEFAULT_SYNTH, engine);

        setVoiceDataDetails(data);
    }

    public void setVoiceDataDetails(Intent data) {
        mVoiceCheckData = data;
        // This might end up running before getView above, in which
        // case mSettingsIcon && mRadioButton will be null. In this case
        // getView will set the right values.
        if (mVoiceCheckData != null) {
            mTtsSettingsEnabled = true;
        } else {
            mTtsSettingsEnabled = false;
        }
    }

    /**
     * Ask the current default engine to launch the matching INSTALL_TTS_DATA
     * activity so the required TTS files are properly installed.
     */
    private void installVoiceData() {
        if (TextUtils.isEmpty(mCurrentEngine)) {
            return;
        }
        Intent intent = new Intent(TextToSpeech.Engine.ACTION_INSTALL_TTS_DATA);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setPackage(mCurrentEngine);
        try {
            Log.v(TAG, "Installing voice data: " + intent.toUri(0));
            startActivity(intent);
        } catch (ActivityNotFoundException ex) {
            Log.e(TAG, "Failed to install TTS data, no acitivty found for " + intent + ")");
        }
    }

    @Override
    protected Object getInitialState() {
        return ActionType.ACCESSIBILITY_OVERVIEW;
    }
}
