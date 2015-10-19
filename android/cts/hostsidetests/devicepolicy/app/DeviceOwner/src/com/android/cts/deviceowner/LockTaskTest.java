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

import android.app.Activity;
import android.app.ActivityManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.provider.Settings;
import android.os.Bundle;

// This is not a standard test of an android activity (such as
// ActivityInstrumentationTestCase2) as it is attempting to test the actual
// life cycle and how it is affected by lock task, rather than mock intents
// and setup.
public class LockTaskTest extends BaseDeviceOwnerTest {

    private static final String TEST_PACKAGE = "com.google.android.example.somepackage";

    private static final int ACTIVITY_RESUMED_TIMEOUT_MILLIS = 20000;  // 20 seconds
    private static final int ACTIVITY_RUNNING_TIMEOUT_MILLIS = 10000;  // 10 seconds
    private static final int ACTIVITY_DESTROYED_TIMEOUT_MILLIS = 60000;  // 60 seconds

    public static final String RECEIVING_ACTIVITY_CREATED_ACTION
            = "com.android.cts.deviceowner.RECEIVER_ACTIVITY_STARTED_ACTION";
    /**
     * The tests below need to keep detailed track of the state of the activity
     * that is started and stopped frequently.  To do this it sends a number of
     * broadcasts that are caught here and translated into booleans (as well as
     * notify some locks in case we are waiting).  There is also an action used
     * to specify that the activity has finished handling the current command
     * (INTENT_ACTION).
     */
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (LockTaskUtilityActivity.CREATE_ACTION.equals(action)) {
                synchronized (mActivityRunningLock) {
                    mIsActivityRunning = true;
                    mActivityRunningLock.notify();
                }
            } else if (LockTaskUtilityActivity.DESTROY_ACTION.equals(action)) {
                synchronized (mActivityRunningLock) {
                    mIsActivityRunning = false;
                    mActivityRunningLock.notify();
                }
            } else if (LockTaskUtilityActivity.RESUME_ACTION.equals(action)) {
                synchronized (mActivityResumedLock) {
                    mIsActivityResumed = true;
                    mActivityResumedLock.notify();
                }
            } else if (LockTaskUtilityActivity.PAUSE_ACTION.equals(action)) {
                synchronized (mActivityResumedLock) {
                    mIsActivityResumed = false;
                    mActivityResumedLock.notify();
                }
            } else if (LockTaskUtilityActivity.INTENT_ACTION.equals(action)) {
                // Notify that intent has been handled.
                synchronized (LockTaskTest.this) {
                    mIntentHandled = true;
                    LockTaskTest.this.notify();
                }
            } else if (RECEIVING_ACTIVITY_CREATED_ACTION.equals(action)) {
                synchronized(mReceivingActivityCreatedLock) {
                    mReceivingActivityWasCreated = true;
                    mReceivingActivityCreatedLock.notify();
                }
            }
        }
    };

    public static class IntentReceivingActivity extends Activity {
        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            sendBroadcast(new Intent(RECEIVING_ACTIVITY_CREATED_ACTION));
            finish();
        }
    }

    private boolean mIsActivityRunning;
    private boolean mIsActivityResumed;
    private boolean mReceivingActivityWasCreated;
    private final Object mActivityRunningLock = new Object();
    private final Object mActivityResumedLock = new Object();
    private final Object mReceivingActivityCreatedLock = new Object();
    private Boolean mIntentHandled;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        IntentFilter filter = new IntentFilter();
        filter.addAction(LockTaskUtilityActivity.CREATE_ACTION);
        filter.addAction(LockTaskUtilityActivity.DESTROY_ACTION);
        filter.addAction(LockTaskUtilityActivity.INTENT_ACTION);
        filter.addAction(LockTaskUtilityActivity.RESUME_ACTION);
        filter.addAction(LockTaskUtilityActivity.PAUSE_ACTION);
        filter.addAction(RECEIVING_ACTIVITY_CREATED_ACTION);
        mContext.registerReceiver(mReceiver, filter);
    }

    @Override
    protected void tearDown() throws Exception {
        mContext.unregisterReceiver(mReceiver);
        super.tearDown();
    }

    public void testSetLockTaskPackages() {
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[] { TEST_PACKAGE });
        assertTrue(mDevicePolicyManager.isLockTaskPermitted(TEST_PACKAGE));

        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[0]);
        assertFalse(mDevicePolicyManager.isLockTaskPermitted(TEST_PACKAGE));
    }

    // Start lock task, verify that ActivityManager knows thats what is going on.
    public void testStartLockTask() {
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[] { PACKAGE_NAME });
        startLockTask();
        waitForResume();

        // Verify that activity open and activity manager is in lock task.
        ActivityManager activityManager = (ActivityManager)
                mContext.getSystemService(Context.ACTIVITY_SERVICE);
        assertTrue(activityManager.isInLockTaskMode());
        assertTrue(mIsActivityRunning);
        assertTrue(mIsActivityResumed);

        stopAndFinish(activityManager);
    }

    // Verifies that the act of finishing is blocked by ActivityManager in lock task.
    // This results in onDestroy not being called until stopLockTask is called before finish.
    public void testCannotFinish() {
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[] { PACKAGE_NAME });
        startLockTask();

        // If lock task has not exited then the activity shouldn't actually receive onDestroy.
        finishAndWait();
        ActivityManager activityManager = (ActivityManager)
                mContext.getSystemService(Context.ACTIVITY_SERVICE);
        assertTrue(activityManager.isInLockTaskMode());
        assertTrue(mIsActivityRunning);

        stopAndFinish(activityManager);
    }

    // This launches an activity that is in the current task.
    // this should be permitted as a part of lock task (since it isn't a new task).
    public void testStartActivityWithinTask() {
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[] { PACKAGE_NAME });
        startLockTask();
        waitForResume();

        mReceivingActivityWasCreated = false;
        Intent launchIntent = new Intent(mContext, IntentReceivingActivity.class);
        Intent lockTaskUtility = getLockTaskUtility();
        lockTaskUtility.putExtra(LockTaskUtilityActivity.START_ACTIVITY, launchIntent);
        mContext.startActivity(lockTaskUtility);

        synchronized (mReceivingActivityCreatedLock) {
            try {
                mReceivingActivityCreatedLock.wait(ACTIVITY_RESUMED_TIMEOUT_MILLIS);
            } catch (InterruptedException e) {
            }
            assertTrue(mReceivingActivityWasCreated);
        }
        stopAndFinish(null);
    }

    // This launches an activity that is not part of the current task and therefore
    // should be blocked.
    public void testCannotStartActivityOutsideTask() {
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[] { PACKAGE_NAME });
        startLockTask();
        waitForResume();

        mReceivingActivityWasCreated = false;
        Intent launchIntent = new Intent(mContext, IntentReceivingActivity.class);
        launchIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.startActivity(launchIntent);
        synchronized (mReceivingActivityCreatedLock) {
            try {
                mReceivingActivityCreatedLock.wait(ACTIVITY_RESUMED_TIMEOUT_MILLIS);
            } catch (InterruptedException e) {
            }
            assertFalse(mReceivingActivityWasCreated);
        }
        stopAndFinish(null);
    }

    /**
     * Call stopLockTask and finish on the LockTaskUtilityActivity.
     *
     * Verify that the activity is no longer running.
     *
     * If activityManager is not null then verify that the ActivityManager
     * is no longer in lock task mode.
     */
    private void stopAndFinish(ActivityManager activityManager) {
        stopLockTask();
        finishAndWait();
        if (activityManager != null) {
            assertFalse(activityManager.isInLockTaskMode());
        }
        assertFalse(mIsActivityRunning);
    }

    /**
     * Call finish on the LockTaskUtilityActivity and wait for
     * onDestroy to be called.
     */
    private void finishAndWait() {
        synchronized (mActivityRunningLock) {
            finish();
            if (mIsActivityRunning) {
                try {
                    mActivityRunningLock.wait(ACTIVITY_DESTROYED_TIMEOUT_MILLIS);
                } catch (InterruptedException e) {
                }
            }
        }
    }

    /**
     * Wait for onResume to be called on the LockTaskUtilityActivity.
     */
    private void waitForResume() {
        // It may take a moment for the resume to come in.
        synchronized (mActivityResumedLock) {
            if (!mIsActivityResumed) {
                try {
                    mActivityResumedLock.wait(ACTIVITY_RESUMED_TIMEOUT_MILLIS);
                } catch (InterruptedException e) {
                }
            }
        }
    }

    /**
     * Calls startLockTask on the LockTaskUtilityActivity
     */
    private void startLockTask() {
        Intent intent = getLockTaskUtility();
        intent.putExtra(LockTaskUtilityActivity.START_LOCK_TASK, true);
        startAndWait(intent);
    }

    /**
     * Calls stopLockTask on the LockTaskUtilityActivity
     */
    private void stopLockTask() {
        Intent intent = getLockTaskUtility();
        intent.putExtra(LockTaskUtilityActivity.STOP_LOCK_TASK, true);
        startAndWait(intent);
    }

    /**
     * Calls finish on the LockTaskUtilityActivity
     */
    private void finish() {
        Intent intent = getLockTaskUtility();
        intent.putExtra(LockTaskUtilityActivity.FINISH, true);
        startAndWait(intent);
    }

    /**
     * Sends a command intent to the LockTaskUtilityActivity and waits
     * to receive the broadcast back confirming it has finished processing
     * the command.
     */
    private void startAndWait(Intent intent) {
        mIntentHandled = false;
        synchronized (this) {
            mContext.startActivity(intent);
            // Give 20 secs to finish.
            try {
                wait(ACTIVITY_RUNNING_TIMEOUT_MILLIS);
            } catch (InterruptedException e) {
            }
            assertTrue(mIntentHandled);
        }
    }

    /**
     * Get basic intent that points at the LockTaskUtilityActivity.
     *
     * This intent includes the flags to make it act as single top.
     */
    private Intent getLockTaskUtility() {
        Intent intent = new Intent();
        intent.setClassName(PACKAGE_NAME, LockTaskUtilityActivity.class.getName());
        intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }
}
