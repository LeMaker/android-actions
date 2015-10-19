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
package com.android.cts.deviceowner;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

/**
 * Functionality tests for setApplicationRestrictions and getApplicationRestrictions
 * in DevicePolicyManager.
 *
 * First of all, these two APIs are executed locally to assert that what you set
 * can later be retrieved via the getter. It also fires up an external activity
 * (which runs in com.google.android.xts.gmscore, unlike the test code itself
 * which runs in the test target package com.google.android.gms due to
 * instrumentation) to observe an application's view of its restrictions.
 * The activity listens to ACTION_APPLICATION_RESTRICTIONS_CHANGED broadcast
 * which is fired by the system whenever its restriction is modified,
 * and relays the value back to this test for verification.
 */
public class ApplicationRestrictionsTest extends BaseDeviceOwnerTest {

    private static final String[] testStrings = new String[] {
            "<bad/>",
            ">worse!\"£$%^&*()'<",
            "<JSON>\"{ \\\"One\\\": { \\\"OneOne\\\": \\\"11\\\", \\\""
                    + "OneTwo\\\": \\\"12\\\" }, \\\"Two\\\": \\\"2\\\" } <JSON/>\""
    };

    private final Semaphore mOnRegisteredSemaphore = new Semaphore(0);
    private final Semaphore mOnRestrictionSemaphore = new Semaphore(0);
    private Bundle mReceivedRestrictions;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        IntentFilter filter = new IntentFilter();
        filter.addAction(ApplicationRestrictionActivity.REGISTERED_ACTION);
        filter.addAction(ApplicationRestrictionActivity.RESTRICTION_ACTION);
        mContext.registerReceiver(mReceiver, filter);
    }

    @Override
    protected void tearDown() throws Exception {
        mContext.unregisterReceiver(mReceiver);
        super.tearDown();
    }

    public void testSetApplicationRestrictions() {
        final String CTS_PACKAGE = PACKAGE_NAME;
        final String OTHER_PACKAGE = CTS_PACKAGE + "dummy";

        startAndWait();

        Bundle bundle0 = createBundle0();
        Bundle bundle1 = createBundle1();

        // Test setting restrictions
        mDevicePolicyManager.setApplicationRestrictions(getWho(), CTS_PACKAGE, bundle0);
        mDevicePolicyManager.setApplicationRestrictions(getWho(), OTHER_PACKAGE, bundle1);

        // Retrieve restrictions locally and make sure they are what we put in.
        assertBundle0(mDevicePolicyManager.getApplicationRestrictions(getWho(), CTS_PACKAGE));
        assertBundle1(mDevicePolicyManager.getApplicationRestrictions(getWho(), OTHER_PACKAGE));

        // The test activity should have received a change_restriction broadcast
        // and relay the value back to us.
        assertBundle0(waitForChangedRestriction());

        // Test overwriting
        mDevicePolicyManager.setApplicationRestrictions(getWho(), CTS_PACKAGE, bundle1);
        assertBundle1(mDevicePolicyManager.getApplicationRestrictions(getWho(), CTS_PACKAGE));
        assertBundle1(waitForChangedRestriction());

        // Cleanup
        mDevicePolicyManager.setApplicationRestrictions(getWho(), CTS_PACKAGE, new Bundle());
        assertTrue(
                mDevicePolicyManager.getApplicationRestrictions(getWho(), CTS_PACKAGE).isEmpty());
        assertTrue(waitForChangedRestriction().isEmpty());
        mDevicePolicyManager.setApplicationRestrictions(getWho(), OTHER_PACKAGE, new Bundle());
        assertTrue(
                mDevicePolicyManager.getApplicationRestrictions(getWho(), OTHER_PACKAGE).isEmpty());

        finish();
    }

    // Should be consistent with assertBundle0
    private Bundle createBundle0() {
        Bundle result = new Bundle();
        // Tests for four allowed types: Integer, Boolean, String and String[]
        // Also test for string escaping handling
        result.putBoolean("boolean_0", false);
        result.putBoolean("boolean_1", true);
        result.putInt("integer", 0x7fffffff);
        // If a null is stored, "" will be read back
        result.putString("empty", "");
        result.putString("string", "text");
        result.putStringArray("string[]", testStrings);
        return result;
    }

    // Should be consistent with createBundle0
    private void assertBundle0(Bundle bundle) {
        assertEquals(6, bundle.size());
        assertEquals(false, bundle.getBoolean("boolean_0"));
        assertEquals(true, bundle.getBoolean("boolean_1"));
        assertEquals(0x7fffffff, bundle.getInt("integer"));
        assertEquals("", bundle.getString("empty"));
        assertEquals("text", bundle.getString("string"));

        String[] strings = bundle.getStringArray("string[]");
        assertTrue(strings != null && strings.length == testStrings.length);
        for (int i = 0; i < strings.length; i++) {
            assertEquals(strings[i], testStrings[i]);
        }
    }

    // Should be consistent with assertBundle1
    private Bundle createBundle1() {
        Bundle result = new Bundle();
        result.putInt("dummy", 1);
        return result;
    }

    // Should be consistent with createBundle1
    private void assertBundle1(Bundle bundle) {
        assertEquals(1, bundle.size());
        assertEquals(1, bundle.getInt("dummy"));
    }

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (ApplicationRestrictionActivity.REGISTERED_ACTION.equals(action)) {
                mOnRegisteredSemaphore.release();
            } else if (ApplicationRestrictionActivity.RESTRICTION_ACTION.equals(action)) {
                mReceivedRestrictions = intent.getBundleExtra("value");
                mOnRestrictionSemaphore.release();
            }
        }
    };

    private void startTestActivity(String command) {
        Intent intent = new Intent();
        intent.setClassName(PACKAGE_NAME, ApplicationRestrictionActivity.class.getName());
        intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);

        if (command != null) {
            intent.putExtra(command, true);
        }
        mContext.startActivity(intent);
    }

    private void startAndWait() {
        startTestActivity(null);
        // Wait until the activity has registered its broadcast receiver and ready for incoming
        // restriction changes.
        try {
            assertTrue(mOnRegisteredSemaphore.tryAcquire(5, TimeUnit.SECONDS));
        } catch (InterruptedException e) {
            fail("Start ApplicationRestrictionActivity interrupted");
        }
    }

    private Bundle waitForChangedRestriction() {
        try {
            assertTrue(mOnRestrictionSemaphore.tryAcquire(5, TimeUnit.SECONDS));
        } catch (InterruptedException e) {
            fail("getRestrictionsAndWait() interrupted");
        }

        return mReceivedRestrictions;
    }

    private void finish() {
        startTestActivity(ApplicationRestrictionActivity.FINISH);
    }

}
