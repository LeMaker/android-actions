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

package com.android.server.telecom;

import android.app.Application;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.UserHandle;

/**
 * Top-level Application class for Telecom.
 */
public final class TelecomGlobals {
    private static final String TAG = TelecomGlobals.class.getSimpleName();

    private static final IntentFilter USER_SWITCHED_FILTER =
            new IntentFilter(Intent.ACTION_USER_SWITCHED);

    private static final TelecomGlobals INSTANCE = new TelecomGlobals();

    /**
     * The Telecom service implementation.
     */
    private TelecomService mTelecomService;

    /**
     * Missed call notifier. Exists here so that the instance can be shared with
     * {@link TelecomBroadcastReceiver}.
     */
    private MissedCallNotifier mMissedCallNotifier;

    /**
     * Maintains the list of registered {@link android.telecom.PhoneAccountHandle}s.
     */
    private PhoneAccountRegistrar mPhoneAccountRegistrar;

    /**
     * The calls manager for the Telecom service.
     */
    private CallsManager mCallsManager;

    /**
     * The application context.
     */
    private Context mContext;

    private final BroadcastReceiver mUserSwitchedReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            int userHandleId = intent.getIntExtra(Intent.EXTRA_USER_HANDLE, 0);
            UserHandle currentUserHandle = new UserHandle(userHandleId);
            mPhoneAccountRegistrar.setCurrentUserHandle(currentUserHandle);
        }
    };

    static TelecomGlobals getInstance() {
        return INSTANCE;
    }

    void initialize(Context context) {
        if (mContext != null) {
            Log.e(TAG, new Exception(), "Attempting to intialize TelecomGlobals a second time.");
            return;
        } else {
            Log.i(TAG, "TelecomGlobals initializing");
        }
        mContext = context.getApplicationContext();

        mMissedCallNotifier = new MissedCallNotifier(mContext);
        mPhoneAccountRegistrar = new PhoneAccountRegistrar(mContext);

        mCallsManager = new CallsManager(mContext, mMissedCallNotifier, mPhoneAccountRegistrar);
        CallsManager.initialize(mCallsManager);
        Log.i(this, "CallsManager initialized");

        // Start the BluetoothPhoneService
        BluetoothPhoneService.start(mContext);

        mContext.registerReceiver(mUserSwitchedReceiver, USER_SWITCHED_FILTER);
    }

    MissedCallNotifier getMissedCallNotifier() {
        return mMissedCallNotifier;
    }

    PhoneAccountRegistrar getPhoneAccountRegistrar() {
        return mPhoneAccountRegistrar;
    }

    CallsManager getCallsManager() {
        return mCallsManager;
    }
}
