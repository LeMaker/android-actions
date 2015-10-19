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
package com.actions.testtvoutsettings;

import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Iterator;
import java.util.Collection;
import java.util.LinkedHashMap;
import com.actions.testtvoutsettings.hdmiBroadCastReceiver;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;
import com.actions.hardware.DisplayManager;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.content.DialogInterface;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.LinearLayout;
import android.widget.Button;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;

public class TestTvoutSettings extends PreferenceActivity implements Preference.OnPreferenceChangeListener {
    private final String TAG = "testTvoutSettins";

    private static final String cvbs = "cvbs";
    private static final String hdmi = "hdmi";
    private static final String display_mode = "mode";
    private static final String displayer = "displayer";
    static final String TVOUT_DISCONNECT = "Disconnect";

    private ListPreference mCvbsList;
    private ListPreference mHdmiList;
    private ListPreference mModeList;
    private ListPreference mDisplayerList;

    Context mContext;
    private Map<String, String> mHdmiCapMap = new LinkedHashMap<String, String>();

    private DisplayManager mDisplayManager;
    private hdmiBroadCastReceiver hdmiReceiver;
    private String str;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.layout.test_tvout_settings);

        Log.e(TAG, "enter TestTvoutSettings's onCreate\n");

        mDisplayManager = new DisplayManager();
        if (mDisplayManager == null) {
            Log.e(TAG, "mDisplayManager == null");
        }

        String[] hdmiEntries;
        String[] hdmiValues;
        if (mDisplayManager.getCableState() != 0) {
            hdmiEntries = getSupportedModesList();
            hdmiValues = getSupportedVidList();
        } else {
            hdmiEntries = getResources().getStringArray(R.array.hdmi_entries);
            hdmiValues = getResources().getStringArray(R.array.hdmi_values);
        }

        mCvbsList = (ListPreference) findPreference(cvbs);
        mCvbsList.setOnPreferenceChangeListener(this);
        mCvbsList.setKey(cvbs);
        mHdmiList = (ListPreference) findPreference(hdmi);
        mHdmiList.setOnPreferenceChangeListener(this);
        mHdmiList.setKey(hdmi);
        mModeList = (ListPreference) findPreference(display_mode);
        mModeList.setOnPreferenceChangeListener(this);
        mModeList.setKey(display_mode);
        mDisplayerList = (ListPreference) findPreference(displayer);
        mDisplayerList.setOnPreferenceChangeListener(this);
        mDisplayerList.setKey(displayer);

        mCvbsList.setEntries(getResources().getStringArray(R.array.cvbs_entries));
        mCvbsList.setEntryValues(getResources().getStringArray(R.array.cvbs_values));
        // mHdmiList.setEntries(getResources().getStringArray(R.array.hdmi_entries));
        // mHdmiList.setEntryValues(getResources().getStringArray(R.array.hdmi_values));
        mHdmiList.setEntries(hdmiEntries);
        mHdmiList.setEntryValues(hdmiValues);
        mModeList.setEntries(getResources().getStringArray(R.array.mode_entries));
        mModeList.setEntryValues(getResources().getStringArray(R.array.mode_values));
        mDisplayerList.setEntries(getResources().getStringArray(R.array.displayer_entries));
        mDisplayerList.setEntryValues(getResources().getStringArray(R.array.displayer_values));

        hdmiReceiver = new hdmiBroadCastReceiver();
        IntentFilter filter = new IntentFilter("android.intent.action.HDMI_PLUGGED");
        TestTvoutSettings.this.registerReceiver(hdmiReceiver, filter);
    }

    private void switchToTv(String devName) {
        try {
            mDisplayManager.setOutputDisplayer(devName);
            // mDisplayManager.enableOutput(true);
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    public String[] getSupportedModesList() {
        int ret = getHdmiCapMap();
        if (ret >= 0) {
            return getHdmiSupportedDisplayNameList();

        } else {
            String[] entries = mContext.getResources().getStringArray(R.array.hdmi_entries);
            return entries;

        }
    }

    private int getHdmiCapMap() {
        if (mHdmiCapMap.size() > 1) {
            return 0;
        }
        int i, j;
        String hdmiCap = mDisplayManager.getHdmiCap();
        if (hdmiCap == null || hdmiCap.isEmpty()) {
            return -1;
        }
        Log.d(TAG, "hdmiCap=" + hdmiCap);
        mHdmiCapMap.clear();
        mHdmiCapMap.put(TVOUT_DISCONNECT, "-1");
        String[] hdmiCapArray = hdmiCap.split(";");
        for (i = 0; i < hdmiCapArray.length; i++) {
            String oneLine[] = hdmiCapArray[i].split(",");
            mHdmiCapMap.put(oneLine[0], oneLine[1]);
        }
        return 0;
    }

    private String[] getHdmiSupportedDisplayNameList() {
        if (mHdmiCapMap.size() <= 1) {
            if (getHdmiCapMap() < 0) {
                return null;
            }
        }

        int i = 0, len = mHdmiCapMap.size();
        String[] displayNames = new String[len];

        Set<String> lname = mHdmiCapMap.keySet();
        Iterator iter = lname.iterator();
        while (iter.hasNext()) {
            displayNames[i++] = (String) iter.next();
        }

        return displayNames;
    }

    public String[] getSupportedVidList() {
        return getHdmiSupportedVidList();

    }

    private String[] getHdmiSupportedVidList() {
        if (mHdmiCapMap.size() <= 1) {
            if (getHdmiCapMap() < 0) {
                return null;
            }
        }

        int i = 0, len = mHdmiCapMap.size();
        String[] vids = new String[len];

        Collection<String> lVid = mHdmiCapMap.values();
        Iterator iter = lVid.iterator();
        while (iter.hasNext()) {
            vids[i++] = (String) iter.next();
        }

        return vids;
    }

    public boolean onPreferenceChange(Preference preference, Object objValue) {
        final String mode = preference.getKey();
        String settings = (String) objValue;
        int index1 = settings.indexOf("_");
        str = settings.substring(index1 + 1);

        if (display_mode.equals(mode)) {// display mode
            if (str.equals("MODE_DISP_SYNC_DEFAULT_TV_GV_LCD_GV"))
                mDisplayManager.setDisplaySingleMode(0);
            else if (str.equals("MODE_DISP_DOUBLE_DEFAULT_NO_SYNC_TV_GV_LCD_GV"))
                mDisplayManager.setDisplaySingleMode(1);
            else if (str.equals("MODE_DISP_DOUBLE_DEFAULT_NO_SYNC_TV_GV_LCD_G"))
                mDisplayManager.setDisplaySingleMode(17);
            else if (str.equals("MODE_DISP_DOUBLE_DEFAULT_NO_SYNC_TV_V_LCD_G"))
                mDisplayManager.setDisplaySingleMode(33);
            else if (str.equals("MODE_DISP_DOUBLE_DEFAULT_SYNC_EXT_TV_GV_LCD_GV"))
                mDisplayManager.setDisplaySingleMode(3);
            else if (str.equals("MODE_DISP_DOUBLE_DEFAULT_SYNC_EXT_TV_GV_LCD_G"))
                mDisplayManager.setDisplaySingleMode(19);
            else if (str.equals("MODE_DISP_DOUBLE_DEFAULT_SYNC_EXT_TV_V_LCD_G"))
                mDisplayManager.setDisplaySingleMode(35);
            else
                Log.e(TAG, "don't support this display mode\n");

        } else if (cvbs.equals(mode)) {// cvbs
            if (str.equals("pal"))
                mDisplayManager.setFormat("pal");
            else if (str.equals("ntsc"))
                mDisplayManager.setFormat("ntsc");
            else
                Log.e(TAG, "don't support this cvbs mode\n");

        } else if (hdmi.equals(mode)) {// hdmi mode
            int value = Integer.parseInt((String) objValue);
            try {
                mDisplayManager.setHdmiVid(value);
            } catch (NumberFormatException e) {
                Log.e(TAG, "could not persist screen timeout setting", e);
            }

        } else if (displayer.equals(mode)) {// displayer
            if (str.equals("LCD0"))
                switchToTv("lcd0");
            else if (str.equals("CVBS_LCD0"))
                switchToTv("cvbs&&lcd0");
            else if (str.equals("HDMI_LCD0"))
                switchToTv("hdmi&&lcd0");
            else if (str.equals("CVBS_HDMI_LCD0"))
                switchToTv("cvbs&&hdmi&&lcd0");
            else
                Log.e(TAG, "don't support this displayer mode\n");
        }

        return true;
    }
}
