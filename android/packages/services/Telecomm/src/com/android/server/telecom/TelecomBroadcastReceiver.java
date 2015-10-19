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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.UserHandle;

/**
 * Handles miscellaneous Telecom broadcast intents. This should be visible from outside, but
 * should not be in the "exported" state.
 */
public final class TelecomBroadcastReceiver extends BroadcastReceiver {
    /** The action used to send SMS response for the missed call notification. */
    static final String ACTION_SEND_SMS_FROM_NOTIFICATION =
            "com.android.server.telecom.ACTION_SEND_SMS_FROM_NOTIFICATION";

    /** The action used to call a handle back for the missed call notification. */
    static final String ACTION_CALL_BACK_FROM_NOTIFICATION =
            "com.android.server.telecom.ACTION_CALL_BACK_FROM_NOTIFICATION";

    /** The action used to clear missed calls. */
    static final String ACTION_CLEAR_MISSED_CALLS =
            "com.android.server.telecom.ACTION_CLEAR_MISSED_CALLS";

    /** {@inheritDoc} */
    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();

        Log.v(this, "Action received: %s.", action);

        MissedCallNotifier missedCallNotifier = CallsManager.getInstance().getMissedCallNotifier();

        // Send an SMS from the missed call notification.
        if (ACTION_SEND_SMS_FROM_NOTIFICATION.equals(action)) {
            // Close the notification shade and the notification itself.
            closeSystemDialogs(context);
            missedCallNotifier.clearMissedCalls();

            Intent callIntent = new Intent(Intent.ACTION_SENDTO, intent.getData());
            callIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(callIntent);

        // Call back recent caller from the missed call notification.
        } else if (ACTION_CALL_BACK_FROM_NOTIFICATION.equals(action)) {
            // Close the notification shade and the notification itself.
            closeSystemDialogs(context);
            missedCallNotifier.clearMissedCalls();

            Intent callIntent = new Intent(Intent.ACTION_CALL_PRIVILEGED, intent.getData());
            callIntent.setFlags(
                    Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
            context.startActivity(callIntent);

        // Clear the missed call notification and call log entries.
        } else if (ACTION_CLEAR_MISSED_CALLS.equals(action)) {
            missedCallNotifier.clearMissedCalls();
        }
    }

    /**
     * Closes open system dialogs and the notification shade.
     */
    private void closeSystemDialogs(Context context) {
        Intent intent = new Intent(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        context.sendBroadcastAsUser(intent, UserHandle.ALL);
    }
}
