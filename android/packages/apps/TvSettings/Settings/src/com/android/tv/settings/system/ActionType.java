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
import com.android.tv.settings.R;
import com.android.tv.settings.dialog.old.Action;

import android.content.res.Resources;

enum ActionType {
    /*
     * General
     */
    AGREE(R.string.agree),
    DISAGREE(R.string.disagree),
    EMAIL_ADDRESS(R.string.system_email_address),
    OK(R.string.title_ok),
    CANCEL(R.string.title_cancel),
    ON(R.string.on),
    OFF(R.string.off),
    /*
     * Date & Time
     */
    DATE_TIME_OVERVIEW(R.string.system_date_time),
    DATE(R.string.system_date),
    TIME(R.string.system_time),
    DATE_SET_DATE(R.string.system_set_date),
    TIME_SET_TIME(R.string.system_set_time),
    TIME_SET_TIME_ZONE(R.string.system_set_time_zone),
    TIME_CHOOSE_FORMAT(R.string.system_set_time_format),
    AUTO_DATE_TIME(R.string.system_auto_date_time, R.string.desc_auto_date_time),
    /*
     * Location
     */
    LOCATION_OVERVIEW(R.string.system_location),
    NETWORK_LOCATION_CONFIRM(R.string.system_network_location_confirm),
    LOCATION_STATUS(R.string.location_status),
    LOCATION_MODE(R.string.location_mode_title),
    LOCATION_MODE_WIFI(R.string.location_mode_wifi_description),
    LOCATION_RECENT_REQUESTS(R.string.location_category_recent_location_requests),
    LOCATION_NO_RECENT_REQUESTS(R.string.location_no_recent_apps),
    LOCATION_SERVICES(R.string.location_services),
    LOCATION_SERVICES_GOOGLE(R.string.google_location_services_title),
    LOCATION_SERVICES_GOOGLE_SETTINGS(R.string.google_location_services_title),
    LOCATION_SERVICES_GOOGLE_REPORTING(R.string.location_reporting,
            R.string.location_reporting_desc),
    LOCATION_SERVICES_GOOGLE_HISTORY(R.string.location_history, R.string.location_history_desc),
    LOCATION_SERVICES_THIRD_PARTY(R.string.third_party_location_services_title),
    LOCATION_HISTORY_DELETE(R.string.delete_location_history_title,
            R.string.delete_location_history_desc),
    /*
     * Developer Options
     */
    DEVELOPER_OVERVIEW(R.string.system_developer_options),
    DEVELOPER_GENERAL(R.string.system_general),
    DEVELOPER_DEBUGGING(R.string.system_debugging),
    DEVELOPER_INPUT(R.string.system_input),
    DEVELOPER_DRAWING(R.string.system_drawing),
    DEVELOPER_MONITORING(R.string.system_monitoring),
    DEVELOPER_APPS(R.string.system_apps),
    DEVELOPER_GENERAL_STAY_AWAKE(R.string.system_stay_awake,
            R.string.system_desc_stay_awake),
    DEVELOPER_GENERAL_HDCP_CHECKING(R.string.system_hdcp_checking,
            R.string.system_desc_hdcp_checking),
    DEVELOPER_GENERAL_HDMI_OPTIMIZATION(R.string.system_hdmi_optimization,
            R.string.system_desc_hdmi_optimization),
    DEVELOPER_GENERAL_BT_HCI_LOG(R.string.system_bt_hci_log,
            R.string.system_desc_bt_hci_log),
    DEVELOPER_GENERAL_REBOOT(R.string.system_reboot_confirm,
            R.string.system_desc_reboot_confirm),
    DEVELOPER_HDCP_NEVER_CHECK(R.string.system_never_check),
    DEVELOPER_HDCP_CHECK_FOR_DRM_CONTENT_ONLY(R.string.system_check_for_drm_content_only),
    DEVELOPER_HDCP_ALWAYS_CHECK(R.string.system_always_check),
    DEVELOPER_DEBUGGING_USB_DEBUGGING(R.string.system_usb_debugging,
            R.string.system_desc_usb_debugging),
    DEVELOPER_DEBUGGING_ALLOW_MOCK_LOCATIONS(R.string.system_allow_mock_locations),
    DEVELOPER_DEBUGGING_SELECT_DEBUG_APP(R.string.system_select_debug_app),
    DEVELOPER_DEBUGGING_WAIT_FOR_DEBUGGER(R.string.system_wait_for_debugger,
            R.string.system_desc_wait_for_debugger),
    DEVELOPER_DEBUGGING_VERIFY_APPS_OVER_USB(R.string.system_verify_apps_over_usb,
            R.string.system_desc_verify_apps_over_usb),
    DEVELOPER_DEBUGGING_WIFI_VERBOSE_LOGGING(R.string.system_wifi_verbose_logging,
            R.string.system_desc_wifi_verbose_logging),
    DEVELOPER_INPUT_SHOW_TOUCHES(R.string.system_show_touches),
    DEVELOPER_INPUT_POINTER_LOCATION(R.string.system_pointer_location),
    DEVELOPER_DRAWING_SHOW_LAYOUT_BOUNDS(R.string.system_show_layout_bounds,
            R.string.system_desc_show_layout_bounds),
    DEVELOPER_DRAWING_SHOW_GPU_VIEW_UPDATES(R.string.system_show_gpu_view_updates,
            R.string.system_desc_show_gpu_view_updates),
    DEVELOPER_DRAWING_SHOW_GPU_OVERDRAW(R.string.system_show_gpu_overdraw,
                    R.string.system_desc_show_gpu_overdraw),
    DEVELOPER_DRAWING_SHOW_HARDWARE_LAYER(R.string.system_show_hardware_layer,
            R.string.system_desc_show_hardware_layer),
    DEVELOPER_DRAWING_SHOW_SURFACE_UPDATES(R.string.system_show_surface_updates,
            R.string.system_desc_show_surface_updates),
    DEVELOPER_DRAWING_WINDOW_ANIMATION_SCALE(R.string.system_window_animation_scale),
    DEVELOPER_DRAWING_TRANSITION_ANIMATION_SCALE(R.string.system_transition_animation_scale),
    DEVELOPER_DRAWING_ANIMATOR_DURATION_SCALE(R.string.system_animator_duration_scale),
    DEVELOPER_MONITORING_STRICT_MODE_ENABLED(R.string.system_strict_mode_enabled,
            R.string.system_desc_strict_mode_enabled),
    DEVELOPER_MONITORING_SHOW_CPU_USAGE(R.string.system_show_cpu_usage,
            R.string.system_desc_show_cpu_usage),
    DEVELOPER_MONITORING_PROFILE_GPU_RENDERING(R.string.system_profile_gpu_rendering,
            R.string.system_desc_profile_gpu_rendering),
    DEVELOPER_MONITORING_ENABLE_TRACES(R.string.system_enable_traces),
    DEVELOPER_APPS_DONT_KEEP_ACTIVITIES(R.string.system_dont_keep_activities),
    DEVELOPER_APPS_BACKGROUND_PROCESS_LIMIT(R.string.system_background_process_limit),
    DEVELOPER_APPS_SHOW_ALL_ANRS(R.string.system_show_all_anrs),
    /*
     * Keyboard
     */
    KEYBOARD_OVERVIEW(R.string.system_keyboard),
    KEYBOARD_OVERVIEW_CURRENT_KEYBOARD(R.string.title_current_keyboard),
    KEYBOARD_OVERVIEW_CONFIGURE(R.string.title_configure, R.string.desc_configure_keyboard),

    /*
     * Security
     */
    SECURITY_OVERVIEW(R.string.system_security),
    SECURITY_UNKNOWN_SOURCES(R.string.security_unknown_sources_title,
            R.string.security_unknown_sources_desc),
    SECURITY_UNKNOWN_SOURCES_CONFIRM(R.string.security_unknown_sources_title,
                    R.string.security_unknown_sources_confirm_desc),
    SECURITY_VERIFY_APPS(R.string.security_verify_apps_title,
             R.string.security_verify_apps_desc),

    /*
     * StorageReset
     */
    STORAGERESET_OVERVIEW(R.string.device_storage_reset),
    STORAGERESET_FACTORY_RESET(R.string.device_reset),
    STORAGERESET_STORAGE(R.string.device_storage),

    /*
     * Accessibility
     */
    ACCESSIBILITY_OVERVIEW(R.string.system_accessibility),
    ACCESSIBILITY_SERVICES(R.string.system_services),
    ACCESSIBILITY_SERVICES_SETTINGS(R.string.accessibility_service_settings),
    ACCESSIBILITY_SERVICES_STATUS(R.string.system_accessibility_status),
    ACCESSIBILITY_SERVICES_CONFIRM_ON(R.string.system_accessibility_status),
    ACCESSIBILITY_SERVICES_CONFIRM_OFF(R.string.system_accessibility_status),
    ACCESSIBILITY_SERVICE_CONFIG(R.string.system_accessibility_config),
    ACCESSIBILITY_CAPTIONS(R.string.accessibility_captions),
    ACCESSIBILITY_SPEAK_PASSWORDS(R.string.system_speak_passwords),
    ACCESSIBILITY_TTS_OUTPUT(R.string.system_accessibility_tts_output),
    ACCESSIBILITY_PREFERRED_ENGINE(R.string.system_preferred_engine),
    ACCESSIBILITY_LANGUAGE(R.string.system_language),
    ACCESSIBILITY_SPEECH_RATE(R.string.system_speech_rate),
    ACCESSIBILITY_PLAY_SAMPLE(R.string.system_play_sample),
    ACCESSIBILITY_INSTALL_VOICE_DATA(R.string.system_install_voice_data),

    CAPTIONS_OVERVIEW(R.string.accessibility_captions,
                      R.string.accessibility_captions_description),
    CAPTIONS_DISPLAY(R.string.captions_display),
    CAPTIONS_CONFIGURE(R.string.captions_configure),
    CAPTIONS_LANGUAGE(R.string.captions_lanaguage),
    CAPTIONS_TEXTSIZE(R.string.captions_textsize),
    CAPTIONS_CAPTIONSTYLE(R.string.captions_captionstyle),
    CAPTIONS_CUSTOMOPTIONS(R.string.captions_customoptions),
    CAPTIONS_FONTFAMILY(R.string.captions_fontfamily),
    CAPTIONS_TEXTCOLOR(R.string.captions_textcolor),
    CAPTIONS_TEXTOPACITY(R.string.captions_textopacity),
    CAPTIONS_EDGETYPE(R.string.captions_edgetype),
    CAPTIONS_EDGECOLOR(R.string.captions_edgecolor),
    CAPTIONS_BACKGROUNDCOLOR(R.string.captions_backgroundcolor),
    CAPTIONS_BACKGROUNDOPACITY(R.string.captions_backgroundopacity),
    CAPTIONS_WINDOWCOLOR(R.string.captions_windowcolor),
    CAPTIONS_WINDOWOPACITY(R.string.captions_windowopacity);

    private final int mTitleResource;
    private final int mDescResource;

    private ActionType(int titleResource) {
        mTitleResource = titleResource;
        mDescResource = 0;
    }

    private ActionType(int titleResource, int descResource) {
        mTitleResource = titleResource;
        mDescResource = descResource;
    }
    String getTitle(Resources resources) {
        return resources.getString(mTitleResource);
    }

    String getDescription(Resources resources) {
        if (mDescResource != 0) {
            return resources.getString(mDescResource);
        }
        return null;
    }

    Action toAction(Resources resources) {
        return toAction(resources, true/*enabled*/);
    }

    Action toAction(Resources resources, boolean enabled) {
        return toAction(resources, getDescription(resources), enabled, false/* not checked */);
    }

    Action toAction(Resources resources, String description) {
        return toAction(resources, description, true/*enabled*/, false /* not checked */);
    }

    Action toAction(Resources resources, String description, boolean enabled) {
        return toAction(resources, description, enabled, false /* not checked */);
    }

    Action toAction(Resources resources, String description, boolean enabled, boolean checked) {
        return new Action.Builder()
                .key(getKey(this, ActionBehavior.INIT))
                .title(getTitle(resources))
                .description(description)
                .enabled(enabled)
                .checked(checked)
                .build();
    }

    private String getKey(ActionType t, ActionBehavior b) {
        return new ActionKey<ActionType, ActionBehavior>(t, b).getKey();
    }
}
