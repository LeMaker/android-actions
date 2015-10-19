/*
 * Copyright 2014, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.managedprovisioning;

import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_ACCOUNT_TO_MIGRATE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_NAME;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

/**
 * Class that handles the resuming process that takes place after a reboot for encryption
 * during the provisioning process.
 */
public class BootReminder extends BroadcastReceiver {
    private static final int NOTIFY_ID = 1;

    /*
     * Profile owner parameters that are stored in the IntentStore for resuming provisioning.
     */
    private static final String PROFILE_OWNER_PREFERENCES_NAME =
            "profile-owner-provisioning-resume";

    private static final String[] PROFILE_OWNER_STRING_EXTRAS = {
        // Key for the device admin package name
        EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_NAME
    };

    private static final String[] PROFILE_OWNER_PERSISTABLE_BUNDLE_EXTRAS = {
        // Key for the admin extras bundle
        EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE
    };

    private static final String[] PROFILE_OWNER_ACCOUNT_EXTRAS = {
        // Key for the account extras
        EXTRA_PROVISIONING_ACCOUNT_TO_MIGRATE
    };

    private static final ComponentName PROFILE_OWNER_INTENT_TARGET =
            ProfileOwnerPreProvisioningActivity.ALIAS_NO_CHECK_CALLER;

    /*
     * Device owner parameters that are stored in the IntentStore for resuming provisioning.
     */
    private static final String DEVICE_OWNER_PREFERENCES_NAME =
            "device-owner-provisioning-resume";

    private static final ComponentName DEVICE_OWNER_INTENT_TARGET =
            new ComponentName("com.android.managedprovisioning",
                    "com.android.managedprovisioning.DeviceOwnerProvisioningActivity");

    @Override
    public void onReceive(Context context, Intent intent) {
        if (android.content.Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())) {

            // Resume profile owner provisioning if applicable.
            IntentStore profileOwnerIntentStore = getProfileOwnerIntentStore(context);
            final Intent resumeProfileOwnerPrvIntent = profileOwnerIntentStore.load();
            if (resumeProfileOwnerPrvIntent != null) {
                if (EncryptDeviceActivity.isDeviceEncrypted()) {
                    // Show reminder notification and then forget about it for next boot
                    profileOwnerIntentStore.clear();
                    setNotification(context, resumeProfileOwnerPrvIntent);
                }
            }

            // Resume device owner provisioning if applicable.
            IntentStore deviceOwnerIntentStore = getDeviceOwnerIntentStore(context);
            Intent resumeDeviceOwnerPrvIntent = deviceOwnerIntentStore.load();
            if (resumeDeviceOwnerPrvIntent != null) {
                deviceOwnerIntentStore.clear();
                resumeDeviceOwnerPrvIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(resumeDeviceOwnerPrvIntent);
            }
        }
    }

    /**
     * Schedule a provisioning reminder notification for the next reboot.
     *
     * {@code extras} should be a Bundle containing the
     * {@link EncryptDeviceActivity.EXTRA_RESUME_TARGET}.
     * This field has only two supported values {@link EncryptDeviceActivity.TARGET_PROFILE_OWNER}
     * and {@link EncryptDeviceActivity.TARGET_DEVICE_OWNER}
     *
     * <p> In case of TARGET_PROFILE_OWNER {@code extras} should further contain a value for at
     * least the key: {@link EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_NAME}, a {@link String} which
     * specifies the package to set as profile owner.
     *
     * <p>
     * See {@link MessageParser} for the TARGET_DEVICE_OWNER case.
     * </ul>
     *
     * <p> These fields will be persisted and restored to the provisioner after rebooting. Any other
     * key/value pairs will be ignored.
     */
    public static void setProvisioningReminder(Context context, Bundle extras) {
        IntentStore intentStore;
        String resumeTarget = extras.getString(EncryptDeviceActivity.EXTRA_RESUME_TARGET, null);
        if (resumeTarget == null) {
            return;
        }
        if (resumeTarget.equals(EncryptDeviceActivity.TARGET_PROFILE_OWNER)) {
            intentStore = getProfileOwnerIntentStore(context);
        } else if (resumeTarget.equals(EncryptDeviceActivity.TARGET_DEVICE_OWNER)) {
            intentStore = getDeviceOwnerIntentStore(context);
        } else {
            ProvisionLogger.loge("Unknown resume target for bootreminder.");
            return;
        }
        intentStore.save(extras);
    }

    /**
     * Cancel all active provisioning reminders.
     */
    public static void cancelProvisioningReminder(Context context) {
        getProfileOwnerIntentStore(context).clear();
        getDeviceOwnerIntentStore(context).clear();
        setNotification(context, null);
    }

    private static IntentStore getProfileOwnerIntentStore(Context context) {
        return new IntentStore(context,PROFILE_OWNER_INTENT_TARGET, PROFILE_OWNER_PREFERENCES_NAME)
                .setStringKeys(PROFILE_OWNER_STRING_EXTRAS)
                .setPersistableBundleKeys(PROFILE_OWNER_PERSISTABLE_BUNDLE_EXTRAS)
                .setAccountKeys(PROFILE_OWNER_ACCOUNT_EXTRAS);
    }

    private static IntentStore getDeviceOwnerIntentStore(Context context) {
        return new IntentStore(context, DEVICE_OWNER_INTENT_TARGET, DEVICE_OWNER_PREFERENCES_NAME)
                .setStringKeys(MessageParser.DEVICE_OWNER_STRING_EXTRAS)
                .setLongKeys(MessageParser.DEVICE_OWNER_LONG_EXTRAS)
                .setIntKeys(MessageParser.DEVICE_OWNER_INT_EXTRAS)
                .setBooleanKeys(MessageParser.DEVICE_OWNER_BOOLEAN_EXTRAS)
                .setPersistableBundleKeys(MessageParser.DEVICE_OWNER_PERSISTABLE_BUNDLE_EXTRAS);
    }

    /** Create and show the provisioning reminder notification. */
    private static void setNotification(Context context, Intent intent) {
        final NotificationManager notificationManager = (NotificationManager)
                context.getSystemService(Context.NOTIFICATION_SERVICE);
        if (intent == null) {
            notificationManager.cancel(NOTIFY_ID);
            return;
        }
        final PendingIntent resumePendingIntent = PendingIntent.getActivity(
                context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
        final Notification.Builder notify = new Notification.Builder(context)
                .setContentIntent(resumePendingIntent)
                .setContentTitle(context.getString(R.string.continue_provisioning_notify_title))
                .setContentText(context.getString(R.string.continue_provisioning_notify_text))
                .setSmallIcon(com.android.internal.R.drawable.ic_corp_statusbar_icon)
                .setVisibility(Notification.VISIBILITY_PUBLIC)
                .setColor(context.getResources().getColor(
                        com.android.internal.R.color.system_notification_accent_color))
                .setAutoCancel(true);
        notificationManager.notify(NOTIFY_ID, notify.build());
    }
}
