/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */
package com.android.tv.quicksettings;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.support.v17.leanback.widget.VerticalGridView;
import android.text.TextUtils;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.TextView;

import com.android.tv.quicksettings.SettingsDialog;

import java.util.ArrayList;
import java.util.List;

public class QuickSettings extends Activity {

    private static final String TAG = "QuickSettings";
    private static final int REQUEST_CODE_SET_SETTINGS = 1;
    static final int PRESET_SETTING_INDEX = 0;
    static final int INTEGER_SETTING_START_INDEX = 1;

    private int mSlidOutTranslationX;
    private View mRootView;
    private ArrayList<Setting> mSettings;
    private PanelAdapter mAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        WindowManager.LayoutParams lp = getWindow().getAttributes();
        lp.gravity = Gravity.RIGHT;
        getWindow().setAttributes(lp);

        setContentView(R.layout.side_quicksettings);
        getWindow().setLayout(WindowManager.LayoutParams.WRAP_CONTENT,
                WindowManager.LayoutParams.MATCH_PARENT);

        mSlidOutTranslationX = getResources().getDimensionPixelSize(R.dimen.panel_width);

        mRootView = getWindow().getDecorView().findViewById(android.R.id.content);
        mRootView.setTranslationX(mSlidOutTranslationX);

        final VerticalGridView panelList = (VerticalGridView) findViewById(R.id.side_panel_list);

        mSettings = getSettings();
        mAdapter = new PanelAdapter(mSettings, new SettingClickedListener() {
            @Override
            public void onSettingClicked(Setting s) {
                Log.d(TAG, "Clicked Setting " + s.getTitle());
                if (s.getType() != Setting.TYPE_UNKNOWN) {
                    Intent intent = new Intent(QuickSettings.this, SettingsDialog.class);
                    intent.putExtra(SettingsDialog.EXTRA_START_POS,
                            panelList.getSelectedPosition());
                    intent.putExtra(SettingsDialog.EXTRA_SETTINGS, mSettings);
                    startActivityForResult(intent, REQUEST_CODE_SET_SETTINGS);
                } else {
                    new AlertDialog.Builder(QuickSettings.this).setPositiveButton(
                            android.R.string.ok, new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int id) {
                                    // User clicked OK button
                                    String[] presetSettingChoices = getResources().getStringArray(
                                            R.array.setting_preset_choices);
                                    mSettings.get(PRESET_SETTING_INDEX).setValue(
                                            presetSettingChoices[getResources().getInteger(
                                                    R.integer.standard_setting_index)]);
                                    int[] newSettingValues = getResources().getIntArray(
                                            R.array.standard_setting_values);
                                    for (int i = 0; i < newSettingValues.length; i++) {
                                        mSettings.get(i + INTEGER_SETTING_START_INDEX).setValue(
                                                newSettingValues[i]);
                                    }
                                    mAdapter.notifyDataSetChanged();
                                }
                            }).setNegativeButton(android.R.string.cancel,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int id) {
                                    // User cancelled the dialog - do nothing
                                }
                            }).setTitle(R.string.reset_dialog_message).create().show();

                }
            }
        });

        panelList.setAdapter(mAdapter);
        panelList.setSelectedPosition(0);
        panelList.requestFocus();
    }

    @Override
    protected void onResume() {
        super.onResume();
        mRootView.animate().cancel();
        mRootView.animate().translationX(0).start();
    }

    @Override
    protected void onPause() {
        mRootView.animate().cancel();
        mRootView.animate().translationX(mSlidOutTranslationX).start();
        super.onPause();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == REQUEST_CODE_SET_SETTINGS) {
            if (resultCode == RESULT_OK) {
                mSettings = data.getParcelableArrayListExtra(
                        SettingsDialog.RESULT_EXTRA_NEW_SETTINGS_VALUES);
                mAdapter.setSettings(mSettings);
            }
        } else {
            super.onActivityResult(requestCode, resultCode, data);
        }
    }

    private ArrayList<Setting> getSettings() {
        ArrayList<Setting> settings = new ArrayList<Setting>();

        String[] presetSettingChoices = getResources().getStringArray(
                R.array.setting_preset_choices);
        settings.add(new Setting(getString(R.string.setting_preset_name),
                presetSettingChoices[getResources().getInteger(R.integer.standard_setting_index)]));
        String[] settingNames = getResources().getStringArray(R.array.setting_names);
        int[] standardSettingValues = getResources().getIntArray(R.array.standard_setting_values);
        int[] maxSettingValues = getResources().getIntArray(R.array.setting_max_values);
        for (int i = 0; i < settingNames.length; i++) {
            settings.add(
                    new Setting(settingNames[i], standardSettingValues[i], maxSettingValues[i]));
        }
        settings.add(new Setting(getString(R.string.setting_reset_defaults_name)));

        return settings;
    }
}
