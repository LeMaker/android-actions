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

package com.android.tv.settings.device.storage;

import com.android.tv.settings.R;
import com.android.tv.settings.device.storage.StorageMeasurement.MeasurementDetails;
import com.android.tv.settings.device.storage.StorageMeasurement.MeasurementReceiver;
import com.android.tv.settings.dialog.old.Action;
import com.android.tv.settings.dialog.old.ActionAdapter;
import com.android.tv.settings.dialog.old.ActionFragment;
import com.android.tv.settings.dialog.old.ContentFragment;
import com.android.tv.settings.dialog.old.DialogActivity;

import android.app.Activity;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.text.format.Formatter;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;

/**
 * Activity that shows storage information.
 */
public class StorageActivity extends DialogActivity {

    private static final long INVALID_SIZE = -1;

    private static final int UPDATE_MSG = 1;

    private ActionFragment mActionFragment;
    private StorageContentFragment mContentFragment;
    private StorageMeasurement mMeasure;
    private boolean mResumed;

    private final MeasurementReceiver mReceiver = new MeasurementReceiver() {

        private long[] mLastApproximateDetails = null;
        private MeasurementDetails mLastMeasurementDetails = null;

        private final Handler mUpdateHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                if (!mResumed) {
                    return;
                }
                switch (msg.what) {
                    case UPDATE_MSG:
                        if (mLastMeasurementDetails != null) {
                            StorageActivity.this.updateDetails(mLastMeasurementDetails);
                        } else if (mLastApproximateDetails != null) {
                            StorageActivity.this.updateApproximate(
                                    mLastApproximateDetails[0], mLastApproximateDetails[1]);
                        }
                        break;
                    default:
                        break;
                }
            }
        };

        private void startUiUpdateTimer() {
            mUpdateHandler.sendEmptyMessageDelayed(UPDATE_MSG, 1000);
        }

        @Override
        public void updateApproximate(StorageMeasurement meas, long totalSize, long availSize) {
            mLastApproximateDetails = new long[] { totalSize, availSize };
            startUiUpdateTimer();
        }

        @Override
        public void updateDetails(StorageMeasurement meas, MeasurementDetails details) {
            mLastMeasurementDetails = details;
            startUiUpdateTimer();
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mActionFragment = ActionFragment.newInstance(getActions());
        mContentFragment = StorageContentFragment.newInstance(getString(R.string.storage_title),
                getString(R.string.header_category_device),
                String.format(getString(R.string.storage_size), formatSize(INVALID_SIZE)),
                R.drawable.ic_settings_storage, getResources().getColor(R.color.icon_background));
        setContentAndActionFragments(mContentFragment, mActionFragment);
        mMeasure = StorageMeasurement.getInstance(this, null);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mResumed = true;
        mMeasure.setReceiver(mReceiver);
        mMeasure.invalidate();
        mMeasure.measure();
    }

    @Override
    protected void onPause() {
        mMeasure.cleanUp();
        mResumed = false;
        super.onPause();
    }

    private ArrayList<Action> getActions() {
        return getActions(INVALID_SIZE);
    }

    private ArrayList<Action> getActions(long availSize) {
        return getActions(availSize, INVALID_SIZE, INVALID_SIZE, INVALID_SIZE, INVALID_SIZE,
                INVALID_SIZE, INVALID_SIZE);
    }

    private ArrayList<Action> getActions(long availSize, long appsSize, long dcimSize,
            long musicSize, long downloadsSize, long cacheSize, long miscSize) {
        ArrayList<Action> actions = new ArrayList<Action>();
        actions.add(StorageItem.APPS.toAction(getResources(), formatSize(appsSize)));
        actions.add(StorageItem.PICTURES_VIDEO.toAction(getResources(), formatSize(dcimSize)));
        actions.add(StorageItem.AUDIO.toAction(getResources(), formatSize(musicSize)));
        actions.add(StorageItem.DOWNLOADS.toAction(getResources(), formatSize(downloadsSize)));
        actions.add(StorageItem.CACHED_DATA.toAction(getResources(), formatSize(cacheSize)));
        actions.add(StorageItem.MISC.toAction(getResources(), formatSize(miscSize)));
        actions.add(StorageItem.AVAILABLE.toAction(getResources(), formatSize(availSize)));
        return actions;
    }

    private void updateActions(ArrayList<Action> actions) {
        ((ActionAdapter) mActionFragment.getAdapter()).setActions(actions);
    }

    private void updateApproximate(long totalSize, long availSize) {

        final long usedSize = totalSize - availSize;

        ArrayList<PercentageBarChart.Entry> entries = new ArrayList<PercentageBarChart.Entry>();
        entries.add(new PercentageBarChart.Entry(
                0, usedSize / (float) totalSize, android.graphics.Color.GRAY));

        updateActions(getActions(availSize));
        updateStorageContentFragment(totalSize, entries);
    }

    private void updateDetails(MeasurementDetails details) {

        final long dcimSize = totalValues(details.mediaSize, Environment.DIRECTORY_DCIM,
                Environment.DIRECTORY_MOVIES, Environment.DIRECTORY_PICTURES);

        final long musicSize = totalValues(details.mediaSize, Environment.DIRECTORY_MUSIC,
                Environment.DIRECTORY_ALARMS, Environment.DIRECTORY_NOTIFICATIONS,
                Environment.DIRECTORY_RINGTONES, Environment.DIRECTORY_PODCASTS);

        final long downloadsSize = totalValues(details.mediaSize, Environment.DIRECTORY_DOWNLOADS);

        ArrayList<PercentageBarChart.Entry> entries = new ArrayList<PercentageBarChart.Entry>();

        addEntry(entries, StorageItem.APPS, details.appsSize, details.totalSize);
        addEntry(entries, StorageItem.PICTURES_VIDEO, dcimSize, details.totalSize);
        addEntry(entries, StorageItem.AUDIO, musicSize, details.totalSize);
        addEntry(entries, StorageItem.DOWNLOADS, downloadsSize, details.totalSize);
        addEntry(entries, StorageItem.CACHED_DATA, details.cacheSize, details.totalSize);
        addEntry(entries, StorageItem.MISC, details.miscSize, details.totalSize);

        Collections.sort(entries);

        updateActions(getActions(
                details.availSize, details.appsSize, dcimSize, musicSize, downloadsSize,
                details.cacheSize, details.miscSize));
        updateStorageContentFragment(details.totalSize, entries);
    }

    private static long totalValues(HashMap<String, Long> map, String... keys) {
        long total = 0;
        for (String key : keys) {
            total += map.get(key);
        }
        return total;
    }

    private String formatSize(long size) {
        return (size == INVALID_SIZE) ? getString(R.string.storage_calculating_size)
                : Formatter.formatShortFileSize(this, size);
    }

    private void addEntry(List<PercentageBarChart.Entry> entries, StorageItem storageItem,
            long size, long totalSize) {
        if (size > 0) {
            entries.add(new PercentageBarChart.Entry(
                    storageItem.ordinal(), size / (float) totalSize,
                    storageItem.getColor(getResources())));
        }
    }

    private void updateStorageContentFragment(long totalSize) {
        updateStorageContentFragment(totalSize, new ArrayList<PercentageBarChart.Entry>());
    }

    private void updateStorageContentFragment(
            long totalSize, ArrayList<PercentageBarChart.Entry> entries) {
        mContentFragment.updateEntries(entries);
        mContentFragment.setDescriptionText(
                String.format(getString(R.string.storage_size), formatSize(totalSize)));
    }
}
