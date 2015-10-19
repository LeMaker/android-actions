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

package com.android.cts.verifier.tv;

import android.app.Activity;
import android.content.ContentUris;
import android.content.ContentValues;
import android.database.Cursor;
import android.media.tv.TvContract;
import android.media.tv.TvInputInfo;
import android.net.Uri;
import android.os.Bundle;
import android.util.Pair;
import android.view.View;

public class MockTvInputSetupActivity extends Activity {
    private static final String TAG = "MockTvInputSetupActivity";

    private static final String CHANNEL_NUMBER = "999-0";
    private static final String CHANNEL_NAME = "Dummy";

    private static Object sLock = new Object();
    private static Pair<View, Runnable> sLaunchCallback = null;

    static void expectLaunch(View postTarget, Runnable successCallback) {
        synchronized (sLock) {
            sLaunchCallback = Pair.create(postTarget, successCallback);
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        try {
            super.onCreate(savedInstanceState);
            final String inputId = getIntent().getStringExtra(TvInputInfo.EXTRA_INPUT_ID);
            final Uri uri = TvContract.buildChannelsUriForInput(inputId);
            final String[] projection = { TvContract.Channels._ID };
            try (Cursor cursor = getContentResolver().query(uri, projection, null, null, null)) {
                // If we already have channels, just finish without doing anything.
                if (cursor != null && cursor.getCount() > 0) {
                    return;
                }
            }
            ContentValues values = new ContentValues();
            values.put(TvContract.Channels.COLUMN_INPUT_ID, inputId);
            values.put(TvContract.Channels.COLUMN_DISPLAY_NUMBER, CHANNEL_NUMBER);
            values.put(TvContract.Channels.COLUMN_DISPLAY_NAME, CHANNEL_NAME);
            Uri channelUri = getContentResolver().insert(uri, values);
            // If the channel's ID happens to be zero, we add another and delete the one.
            if (ContentUris.parseId(channelUri) == 0) {
                getContentResolver().insert(uri, values);
                getContentResolver().delete(channelUri, null, null);
            }
        } finally {
            Pair<View, Runnable> launchCallback = null;
            synchronized (sLock) {
                launchCallback = sLaunchCallback;
                sLaunchCallback = null;
            }
            if (launchCallback != null) {
                launchCallback.first.post(launchCallback.second);
            }

            setResult(Activity.RESULT_OK);
            finish();
        }
    }
}
