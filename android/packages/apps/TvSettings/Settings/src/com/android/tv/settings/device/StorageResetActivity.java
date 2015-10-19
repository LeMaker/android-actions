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

package com.android.tv.settings.device;

import android.app.ActivityManager;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.text.format.Formatter;

import com.android.tv.settings.device.apps.AppsActivity;
import com.android.tv.settings.device.storage.StorageItem;
import com.android.tv.settings.device.storage.StorageMeasurement;
import com.android.tv.settings.device.storage.StorageMeasurement.MeasurementDetails;
import com.android.tv.settings.device.storage.StorageMeasurement.MeasurementReceiver;
import com.android.tv.settings.device.storage.PercentageBarChart;
import com.android.tv.settings.device.privacy.PrivacyActivity;

import com.android.tv.settings.dialog.SettingsLayoutActivity;
import com.android.tv.settings.dialog.Layout;
import com.android.tv.settings.dialog.Layout.Header;
import com.android.tv.settings.dialog.Layout.Action;
import com.android.tv.settings.dialog.Layout.Status;
import com.android.tv.settings.dialog.Layout.Static;
import com.android.tv.settings.dialog.Layout.StringGetter;
import com.android.tv.settings.dialog.Layout.LayoutGetter;
import com.android.tv.settings.dialog.Layout.DrawableGetter;

import com.android.tv.settings.R;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;

/**
 * Activity to view storage consumption and factory reset device.
 */
public class StorageResetActivity extends SettingsLayoutActivity {

    private static final boolean DEBUG = false;
    private static final String TAG = "StorageResetActivity";
    private static final long INVALID_SIZE = -1;
    private static final int ACTION_RESET_DEVICE = 1;
    private static final int ACTION_CANCEL = 2;
    private static final int ACTION_CLEAR_CACHE = 3;

    /**
     * Support for shutdown-after-reset. If our launch intent has a true value for
     * the boolean extra under the following key, then include it in the intent we
     * use to trigger a factory reset. This will cause us to shut down instead of
     * restart after the reset.
     */
    private static final String SHUTDOWN_INTENT_EXTRA = "shutdown";

    private final MeasurementReceiver mReceiver = new MeasurementReceiver() {

        private MeasurementDetails mLastMeasurementDetails = null;

        @Override
        public void updateApproximate(StorageMeasurement meas, long totalSize, long availSize) {
            if (mLastMeasurementDetails == null) {
                StorageResetActivity.this.updateApproximate(totalSize, availSize);
            }
        }

        @Override
        public void updateDetails(StorageMeasurement meas, MeasurementDetails details) {
            mLastMeasurementDetails = details;
            StorageResetActivity.this.updateDetails(mLastMeasurementDetails);
        }
    };

    private class SizeStringGetter extends StringGetter {
        private long mSize = INVALID_SIZE;

        @Override
        public String get() {
            return String.format(getString(R.string.storage_size), formatSize(mSize));
        }

        public void setSize(long size) {
            mSize = size;
            refreshView();
        }
    };

    private class StorageDrawableGetter extends DrawableGetter {
        Drawable mDrawable = null;

        @Override
        public Drawable get() {
            if (mDrawable == null) {
                return mRes.getDrawable(R.drawable.ic_settings_storage);
            } else {
                return mDrawable;
            }
        }

        void setDrawable(ArrayList<PercentageBarChart.Entry> entries) {
            mDrawable = new PercentageBarChart(entries, mRes.getColor(R.color.storage_avail),
                getPixelSize(R.dimen.storage_bar_min_tick_width),
                getPixelSize(R.dimen.content_fragment_icon_width),
                getPixelSize(R.dimen.content_fragment_icon_width), isLayoutRtl());
            refreshView();
        }
    };

    private Resources mRes;
    private StorageMeasurement mMeasure;
    private boolean mResumed = false;
    private final SizeStringGetter mAppsSize = new SizeStringGetter();
    private final SizeStringGetter mDcimSize = new SizeStringGetter();
    private final SizeStringGetter mMusicSize = new SizeStringGetter();
    private final SizeStringGetter mDownloadsSize = new SizeStringGetter();
    private final SizeStringGetter mCacheSize = new SizeStringGetter();
    private final SizeStringGetter mMiscSize = new SizeStringGetter();
    private final SizeStringGetter mAvailSize = new SizeStringGetter();
    private final SizeStringGetter mStorageDescription = new SizeStringGetter();
    private final StorageDrawableGetter mStorageDrawable = new StorageDrawableGetter();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        mRes = getResources();
        super.onCreate(savedInstanceState);
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

    @Override
    public Layout createLayout() {
        final Intent appsIntent = new Intent(this, AppsActivity.class);
        return
            new Layout().breadcrumb(getString(R.string.header_category_device))
                .add(new Header.Builder(mRes)
                        .icon(mStorageDrawable)
                        .title(R.string.device_storage_reset)
                        .build()
                    .add(new Header.Builder(mRes)
                            .title(R.string.storage_title)
                            .description(mStorageDescription)
                            .build()
                        .add(new Action.Builder(mRes, appsIntent)
                            .title(R.string.storage_apps_usage)
                            .icon(R.drawable.storage_indicator_apps)
                            .description(mAppsSize)
                            .build())
                        .add(new Status.Builder(mRes)
                            .title(R.string.storage_dcim_usage)
                            .icon(R.drawable.storage_indicator_dcim)
                            .description(mDcimSize)
                            .build())
                        .add(new Status.Builder(mRes)
                            .title(R.string.storage_music_usage)
                            .icon(R.drawable.storage_indicator_music)
                            .description(mMusicSize)
                            .build())
                        .add(new Status.Builder(mRes)
                            .title(R.string.storage_downloads_usage)
                            .icon(R.drawable.storage_indicator_downloads)
                            .description(mDownloadsSize)
                            .build())
                        .add(new Action.Builder(mRes, ACTION_CLEAR_CACHE)
                            .title(R.string.storage_media_cache_usage)
                            .icon(R.drawable.storage_indicator_cache)
                            .description(mCacheSize)
                            .build())
                        .add(new Status.Builder(mRes)
                            .title(R.string.storage_media_misc_usage)
                            .icon(R.drawable.storage_indicator_misc)
                            .description(mMiscSize)
                            .build())
                        .add(new Status.Builder(mRes)
                            .title(R.string.storage_available)
                            .icon(R.drawable.storage_indicator_available)
                            .description(mAvailSize)
                            .build())
                    )
                    .add(new Header.Builder(mRes)
                            .title(R.string.device_reset)
                            .build()
                        .add(new Header.Builder(mRes)
                                .title(R.string.device_reset)
                                .build()
                            .add(new Action.Builder(mRes, ACTION_RESET_DEVICE)
                                    .title(R.string.confirm_factory_reset_device)
                                    .build()
                            )
                            .add(new Action.Builder(mRes, Action.ACTION_BACK)
                                .title(R.string.title_cancel)
                                .defaultSelection()
                                .build())
                        )
                        .add(new Action.Builder(mRes, Action.ACTION_BACK)
                            .title(R.string.title_cancel)
                            .defaultSelection()
                            .build())
                    )
                );
    }

    @Override
    public void onActionClicked(Action action) {
        switch (action.getId()) {
            case ACTION_RESET_DEVICE:
                if (!ActivityManager.isUserAMonkey()) {
                    Intent resetIntent = new Intent("android.intent.action.MASTER_CLEAR");
                    if (getIntent().getBooleanExtra(SHUTDOWN_INTENT_EXTRA, false)) {
                        resetIntent.putExtra(SHUTDOWN_INTENT_EXTRA, true);
                    }
                    sendBroadcast(resetIntent);
                }
                break;
            case ACTION_CANCEL:
                goBackToTitle(getString(R.string.device_storage_reset));
                break;
            case ACTION_CLEAR_CACHE:
                final DialogFragment fragment = ConfirmClearCacheFragment.newInstance();
                fragment.show(getFragmentManager(), null);
                break;
            default:
                final Intent intent = action.getIntent();
                if (intent != null) {
                    startActivity(intent);
                }
        }
    }

    private void updateStorageSize(long availSize, long appsSize, long dcimSize, long musicSize,
            long downloadsSize, long cacheSize, long miscSize) {
        mAvailSize.setSize(availSize);
        mAppsSize.setSize(appsSize);
        mDcimSize.setSize(dcimSize);
        mMusicSize.setSize(musicSize);
        mDownloadsSize.setSize(downloadsSize);
        mCacheSize.setSize(cacheSize);
        mMiscSize.setSize(miscSize);
    }

    private int getPixelSize(int resource) {
        return mRes.getDimensionPixelSize(resource);
    }

    void updateStorageDescription(long totalSize, ArrayList<PercentageBarChart.Entry> entries) {
        mStorageDescription.setSize(totalSize);
        mStorageDrawable.setDrawable(entries);
    }

    private void updateApproximate(long totalSize, long availSize) {

        final long usedSize = totalSize - availSize;
        ArrayList<PercentageBarChart.Entry> entries = new ArrayList<PercentageBarChart.Entry>();
        entries.add(new PercentageBarChart.Entry(
                0, usedSize / (float) totalSize, android.graphics.Color.GRAY));

        updateStorageSize(availSize, INVALID_SIZE, INVALID_SIZE, INVALID_SIZE, INVALID_SIZE,
                INVALID_SIZE, INVALID_SIZE);
        updateStorageDescription(totalSize, entries);
    }

    private void addEntry(List<PercentageBarChart.Entry> entries, StorageItem storageItem,
            long size, long totalSize) {
        if (size > 0) {
            entries.add(new PercentageBarChart.Entry(
                    storageItem.ordinal(), size / (float) totalSize,
                    storageItem.getColor(getResources())));
        }
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

        updateStorageSize(details.availSize, details.appsSize, dcimSize, musicSize, downloadsSize,
                details.cacheSize, details.miscSize);
        updateStorageDescription(details.totalSize, entries);
    }

    private static long totalValues(HashMap<String, Long> map, String... keys) {
        long total = 0;
        for (String key : keys) {
            if (map.containsKey(key)) {
                total += map.get(key);
            }
        }
        return total;
    }

    private String formatSize(long size) {
        return (size == INVALID_SIZE) ? getString(R.string.storage_calculating_size)
                : Formatter.formatShortFileSize(this, size);
    }

    /**
     * Dialog to request user confirmation before clearing all cache data.
     */
    public static class ConfirmClearCacheFragment extends DialogFragment {
        public static ConfirmClearCacheFragment newInstance() {
            return new ConfirmClearCacheFragment();
        }

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            final Context context = getActivity();

            final AlertDialog.Builder builder = new AlertDialog.Builder(context);
            builder.setTitle(R.string.device_storage_clear_cache_title);
            builder.setMessage(getString(R.string.device_storage_clear_cache_message));

            builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    final PackageManager pm = context.getPackageManager();
                    final List<PackageInfo> infos = pm.getInstalledPackages(0);
                    for (PackageInfo info : infos) {
                        pm.deleteApplicationCacheFiles(info.packageName, null);
                    }
                }
            });
            builder.setNegativeButton(android.R.string.cancel, null);

            return builder.create();
        }
    }

}
