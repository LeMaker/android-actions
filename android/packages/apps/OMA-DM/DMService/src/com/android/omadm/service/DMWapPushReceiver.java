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

package com.android.omadm.service;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.provider.Telephony;
import android.util.Log;

public class DMWapPushReceiver extends BroadcastReceiver {

    private static final String TAG = "DMWapPushReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (DMHelper.disableIfSecondaryUser(context)) {
            return;
        }
        String action = intent.getAction();
        if (Telephony.Sms.Intents.WAP_PUSH_RECEIVED_ACTION.equals(action)) {
            Log.d(TAG, action);
            Intent iIntent = new Intent(DMIntent.ACTION_WAP_PUSH_RECEIVED_INTERNAL);
            iIntent.replaceExtras(intent);
            context.sendBroadcast(iIntent);
        }
    }
}
