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
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Parcelable;
import android.support.v17.leanback.widget.OnChildSelectedListener;
import android.support.v17.leanback.widget.VerticalGridView;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.AnimationSet;
import android.view.animation.ScaleAnimation;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.SeekBar;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class SettingsDialog extends Activity {

    private static final String TAG = "SettingsDialog";
    private static final boolean DEBUG = true;

    static final String EXTRA_START_POS = "com.android.tv.quicksettings.START_POS";
    static final String EXTRA_SETTINGS = "com.android.tv.quicksettings.SETTINGS";
    static final String
            RESULT_EXTRA_NEW_SETTINGS_VALUES = "com.android.tv.quicksettings.NEW_SETTINGS_VALUES";
    private static final int SETTING_INT_VALUE_MIN = 0;
    private static final int SETTING_INT_VALUE_STEP = 10;

    private VerticalGridView mPanelList;
    private SeekBar mSeekBar;
    private TextView mSettingValue;
    private DialogAdapter mAdapter;
    private SettingSelectedListener mSettingSelectedListener = new SettingSelectedListener();
    private Setting mFocusedSetting;
    private ArrayList<Setting> mSettings;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        WindowManager.LayoutParams lp = getWindow().getAttributes();
        lp.height = WindowManager.LayoutParams.MATCH_PARENT;
        lp.gravity = Gravity.BOTTOM | Gravity.CENTER;
        lp.y = getResources().getDimensionPixelSize(R.dimen.panel_y_offset);
        getWindow().setAttributes(lp);

        setContentView(R.layout.main_quicksettings);

        Intent intent = getIntent();
        int startPos = intent.getIntExtra(EXTRA_START_POS, -1);
        if (DEBUG)
            Log.d(TAG, "startPos=" + startPos);

        mPanelList = (VerticalGridView) findViewById(R.id.main_panel_list);
        mPanelList.setWindowAlignment(VerticalGridView.WINDOW_ALIGN_NO_EDGE);
        mPanelList.setOnChildSelectedListener(mSettingSelectedListener);

        mSettings = getIntent().getParcelableArrayListExtra(EXTRA_SETTINGS);
        int pivotX = getResources().getDimensionPixelSize(
                R.dimen.main_panel_text_width_minus_padding);
        int pivotY = getResources().getDimensionPixelSize(R.dimen.main_panel_text_height_half);

        mAdapter = new DialogAdapter(mSettings, pivotX, pivotY, new SettingClickedListener() {
            @Override
            public void onSettingClicked(Setting s) {
                if (s.getType() != Setting.TYPE_UNKNOWN) {
                    setResult(RESULT_OK, new Intent().putExtra(RESULT_EXTRA_NEW_SETTINGS_VALUES,
                            mSettings));
                    finish();
                } else {
                    new AlertDialog.Builder(SettingsDialog.this).setPositiveButton(
                            android.R.string.ok, new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int id) {
                                    // User clicked OK button
                                    String[] presetSettingChoices = getResources().getStringArray(
                                            R.array.setting_preset_choices);
                                    mSettings.get(QuickSettings.PRESET_SETTING_INDEX).setValue(
                                            presetSettingChoices[getResources().getInteger(
                                                    R.integer.standard_setting_index)]);
                                    int[] newSettingValues = getResources().getIntArray(
                                            R.array.standard_setting_values);
                                    for (int i = 0; i < newSettingValues.length; i++) {
                                        mSettings.get(i + QuickSettings.INTEGER_SETTING_START_INDEX)
                                                .setValue(
                                                        newSettingValues[i]);
                                    }
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

        mPanelList.setAdapter(mAdapter);
        mPanelList.setSelectedPosition(startPos);
        mPanelList.requestFocus();

        mSeekBar = (SeekBar) findViewById(R.id.main_slider);

        mSettingValue = (TextView) findViewById(R.id.setting_value);
    }

    private class SettingSelectedListener implements OnChildSelectedListener {
        private static final float ALPHA_UNSELECTED = 0.3f;
        private static final float ALPHA_SELECTED = 1.0f;
        private static final float SCALE_UNSELECTED = 1.0f;
        private static final float SCALE_SELECTED = 1.3f;

        @Override
        public void onChildSelected(ViewGroup parent, View view, int position, long id) {
            mFocusedSetting = mSettings.get(position);
            switch (mFocusedSetting.getType()) {
                case Setting.TYPE_STRING:
                    mSettingValue.setVisibility(View.VISIBLE);
                    mSettingValue.setText(mFocusedSetting.getStringValue());
                    mSeekBar.setVisibility(View.GONE);
                    break;
                case Setting.TYPE_INT:
                    mSettingValue.setVisibility(View.VISIBLE);
                    mSettingValue.setText(Integer.toString(mFocusedSetting.getIntValue()));
                    mSeekBar.setMax(mFocusedSetting.getMaxValue());
                    mSeekBar.setProgress(mFocusedSetting.getIntValue());
                    mSeekBar.setVisibility(View.VISIBLE);
                    break;
                default:
                    mSettingValue.setVisibility(View.GONE);
                    mSeekBar.setVisibility(View.GONE);
                    break;
            }
        }
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (mFocusedSetting == null) {
            return super.onKeyUp(keyCode, event);
        }
        switch (mFocusedSetting.getType()) {
            case Setting.TYPE_INT:
                return integerSettingHandleKeyCode(keyCode, event);
            case Setting.TYPE_STRING:
                return stringSettingHandleKeyCode(keyCode, event);
            default:
                return super.onKeyUp(keyCode, event);
        }
    }

    private boolean integerSettingHandleKeyCode(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                setFocusedSettingToValue(Math.min(
                        mFocusedSetting.getIntValue() + SETTING_INT_VALUE_STEP,
                        mFocusedSetting.getMaxValue()));
                return true;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                setFocusedSettingToValue(Math.max(
                        mFocusedSetting.getIntValue() - SETTING_INT_VALUE_STEP,
                        SETTING_INT_VALUE_MIN));
                return true;
            default:
                return super.onKeyUp(keyCode, event);
        }
    }

    private void setFocusedSettingToValue(int value) {
        mFocusedSetting.setValue(value);
        mSeekBar.setProgress(mFocusedSetting.getIntValue());
        mSettingValue.setText(Integer.toString(mFocusedSetting.getIntValue()));
        String[] presetSettingChoices = getResources().getStringArray(
                R.array.setting_preset_choices);
        mSettings.get(QuickSettings.PRESET_SETTING_INDEX).setValue(
                presetSettingChoices[getResources().getInteger(R.integer.custom_setting_index)]);
    }

    private boolean stringSettingHandleKeyCode(int keyCode, KeyEvent event) {
        if (!mFocusedSetting.getTitle().equals(getString(R.string.setting_preset_name))) {
            return super.onKeyUp(keyCode, event);
        }

        String[] presetSettingChoices = getResources().getStringArray(
                R.array.setting_preset_choices);

        int currentIndex = Arrays.asList(presetSettingChoices).indexOf(
                mFocusedSetting.getStringValue());
        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                currentIndex++;
                break;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                currentIndex--;
                break;
            default:
                return super.onKeyUp(keyCode, event);
        }
        int newIndex = (currentIndex + presetSettingChoices.length) % presetSettingChoices.length;
        mFocusedSetting.setValue(presetSettingChoices[newIndex]);
        mSettingValue.setText(mFocusedSetting.getStringValue());
        int[] newSettingValues = null;
        if (newIndex == getResources().getInteger(R.integer.standard_setting_index)) {
            newSettingValues = getResources().getIntArray(R.array.standard_setting_values);
        } else if (newIndex == getResources().getInteger(R.integer.cinema_setting_index)) {
            newSettingValues = getResources().getIntArray(R.array.cinema_setting_values);
        } else if (newIndex == getResources().getInteger(R.integer.vivid_setting_index)) {
            newSettingValues = getResources().getIntArray(R.array.vivid_setting_values);
        } else if (newIndex == getResources().getInteger(R.integer.game_setting_index)) {
            newSettingValues = getResources().getIntArray(R.array.game_setting_values);
        }
        if (newSettingValues != null) {
            for (int i = 0; i < newSettingValues.length; i++) {
                mSettings.get(i + QuickSettings.INTEGER_SETTING_START_INDEX).setValue(
                        newSettingValues[i]);
            }
        }
        return true;
    }
}
