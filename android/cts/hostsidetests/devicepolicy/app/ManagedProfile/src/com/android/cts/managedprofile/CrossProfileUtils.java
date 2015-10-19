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

package com.android.cts.managedprofile;

import static com.android.cts.managedprofile.BaseManagedProfileTest.ADMIN_RECEIVER_COMPONENT;

import android.app.admin.DevicePolicyManager;
import android.content.Context;
import android.content.IntentFilter;
import android.os.UserManager;
import android.test.AndroidTestCase;

/**
 * The methods in this class are not really tests.
 * They are just performing an action that is needed for a test.
 * But we're still using an AndroidTestCase because it's an easy way to call
 * device-side methods from the host.
 */
public class CrossProfileUtils extends AndroidTestCase {
    private static final String ACTION_READ_FROM_URI = "com.android.cts.action.READ_FROM_URI";

    private static final String ACTION_WRITE_TO_URI = "com.android.cts.action.WRITE_TO_URI";

    private static final String ACTION_TAKE_PERSISTABLE_URI_PERMISSION =
            "com.android.cts.action.TAKE_PERSISTABLE_URI_PERMISSION";

    private static String ACTION_COPY_TO_CLIPBOARD = "com.android.cts.action.COPY_TO_CLIPBOARD";

    public void addParentCanAccessManagedFilters() {
        removeAllFilters();

        final DevicePolicyManager dpm = (DevicePolicyManager) getContext().getSystemService(
                Context.DEVICE_POLICY_SERVICE);
        dpm.addCrossProfileIntentFilter(ADMIN_RECEIVER_COMPONENT, getIntentFilter(),
                DevicePolicyManager.FLAG_PARENT_CAN_ACCESS_MANAGED);
    }

    public void addManagedCanAccessParentFilters() {
        removeAllFilters();

        final DevicePolicyManager dpm = (DevicePolicyManager) getContext().getSystemService(
                Context.DEVICE_POLICY_SERVICE);
        dpm.addCrossProfileIntentFilter(ADMIN_RECEIVER_COMPONENT, getIntentFilter(),
                DevicePolicyManager.FLAG_MANAGED_CAN_ACCESS_PARENT);
    }

    public IntentFilter getIntentFilter() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(ACTION_READ_FROM_URI);
        intentFilter.addAction(ACTION_WRITE_TO_URI);
        intentFilter.addAction(ACTION_TAKE_PERSISTABLE_URI_PERMISSION);
        intentFilter.addAction(ACTION_COPY_TO_CLIPBOARD);
        return intentFilter;
    }

    public void removeAllFilters() {
        final DevicePolicyManager dpm = (DevicePolicyManager) getContext().getSystemService(
                Context.DEVICE_POLICY_SERVICE);
        dpm.clearCrossProfileIntentFilters(ADMIN_RECEIVER_COMPONENT);
    }

    public void disallowCrossProfileCopyPaste() {
        DevicePolicyManager dpm = (DevicePolicyManager)
               getContext().getSystemService(Context.DEVICE_POLICY_SERVICE);
        dpm.addUserRestriction(ADMIN_RECEIVER_COMPONENT,
                UserManager.DISALLOW_CROSS_PROFILE_COPY_PASTE);
    }

    public void allowCrossProfileCopyPaste() {
        DevicePolicyManager dpm = (DevicePolicyManager)
               getContext().getSystemService(Context.DEVICE_POLICY_SERVICE);
        dpm.clearUserRestriction(ADMIN_RECEIVER_COMPONENT,
                UserManager.DISALLOW_CROSS_PROFILE_COPY_PASTE);
    }
}
