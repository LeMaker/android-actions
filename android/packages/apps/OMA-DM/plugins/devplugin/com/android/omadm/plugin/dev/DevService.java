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

package com.android.omadm.plugin.dev;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import com.android.omadm.plugin.IDmtPlugin;

public class DevService extends Service {
    private static final String TAG = "DM_DevService";
    private static final boolean DBG = true;

    private IDmtPlugin.Stub mBinder;

    @Override
    public IBinder onBind(Intent intent) {
        String action = intent.getAction();
        if (DevPlugin.class.getName().equals(action)) {
            String rootPath = intent.getStringExtra("rootPath");
            if (DBG) logd("rootPath: " + rootPath);
            IDmtPlugin.Stub binder = mBinder;
            if (binder == null) {
                if (DBG) logd("mBinder is null, create it!");
                binder = new DevPlugin(this);
                mBinder = binder;
            } else {
                if (DBG) logd("mBinder already exists");
            }
            return binder;
        }
        if (DBG) logd("onBind(): unknown action: " + action + " for intent: " + intent);
        return null;
    }

    @Override
    public boolean onUnbind(Intent intent) {
        super.onUnbind(intent);
        if (DBG) logd("enter onUnbind()");
        mBinder = null;
        return true;
    }

    private static void logd(String msg) {
        Log.d(TAG, msg);
    }
}
