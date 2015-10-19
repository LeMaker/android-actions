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

package com.android.example.testplugin;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import com.android.omadm.plugin.IDMClientService;

public class StartClientSessionActivity extends Activity {
    static final String TAG = "StartClientSessionActivity";

    // Binder interface to client service
    IDMClientService mDMClientService;

    // Service connector object to attach/detach binder interface
    private final DMClientServiceConnector mDMServiceConnector = new DMClientServiceConnector();

    // test alert 1226 string for Sprint HFA FUMO update (no-op for Android)
    static final String ALERT_TYPE_HFA_CIFUMO_UPDATE
            = "org.openmobilealliance.dm.firmwareupdate.devicerequest";


    private class DMClientServiceConnector implements ServiceConnection {
        // package local constructor (called by outer class)
        DMClientServiceConnector() {}

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            Log.d(TAG, "onServiceConnected for " + name + " service " + service);
            mDMClientService = IDMClientService.Stub.asInterface(service);

            try {
                Log.d(TAG, "starting client session with hardcoded values");
                mDMClientService.startClientSession("sprint", null, null,
                        ALERT_TYPE_HFA_CIFUMO_UPDATE, null, null, null);
            } catch (RemoteException e) {
                Log.e(TAG, "remote exception thrown", e);
            }

            Log.d(TAG, "waiting for callback");
            // TODO
            Log.d(TAG, "dumping OMA DM tree");
            // TODO
            Log.d(TAG, "finishing activity");
            finish();
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            Log.d(TAG, "onServiceDisconnected for " + name);
            mDMClientService = null;
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.start_client_session_activity);
    }

    @Override
    protected void onResume() {
        super.onResume();
        // bind to the OMA DM client service
        Intent intent = new Intent();
        intent.setClassName("com.android.omadm.service", "com.android.omadm.service.DMClientService");
        bindService(intent, mDMServiceConnector,
                Context.BIND_AUTO_CREATE | Context.BIND_IMPORTANT);
    }

    @Override
    protected void onPause() {
        // unbind the OMA DM client service
        unbindService(mDMServiceConnector);
        super.onPause();
    }
}
