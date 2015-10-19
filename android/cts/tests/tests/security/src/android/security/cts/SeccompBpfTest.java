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

package android.security.cts;

import android.test.AndroidTestCase;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.ConditionVariable;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;

/**
 * Test for seccomp-bpf sandboxing technology. This makes use of the
 * SeccompDeathTestService to run sandboxing death tests out-of-process.
 */
public class SeccompBpfTest extends AndroidTestCase implements ServiceConnection,
       IBinder.DeathRecipient {
    static final String TAG = "SeccompBpfTest";

    /**
     * Message sent from the SeccompDeathTestService before it runs a test.
     */
    static final int MSG_TEST_STARTED = 1;
    /**
     * Message sent from the SeccompDeathTestService after a test exits cleanly.
     */
    static final int MSG_TEST_ENDED_CLEAN = 2;

    /**
     * Dedicated thread used to receive messages from the SeccompDeathTestService.
     */
    final private HandlerThread mHandlerThread = new HandlerThread("SeccompBpfTest handler");
    /**
     * Messenger that runs on mHandlerThread.
     */
    private Messenger mMessenger;

    /**
     * Condition that blocks the test/instrumentation thread that runs the
     * test cases, while the SeccompDeathTestService runs the test out-of-process.
     */
    final private ConditionVariable mCondition = new ConditionVariable();

    /**
     * The SeccompDeathTestService number to run.
     */
    private int mTestNumber = -1;

    /**
     * If the test has started.
     */
    private boolean mTestStarted = false;
    /**
     * If the test ended (either cleanly or with death).
     */
    private boolean mTestEnded = false;
    /**
     * If the test ended cleanly or died.
     */
    private boolean mTestDied = false;

    public void testDeathTest() {
        runDeathTest(SeccompDeathTestService.TEST_DEATH_TEST);
        assertTrue(mTestDied);
    }

    public void testCleanTest() {
        runDeathTest(SeccompDeathTestService.TEST_CLEAN_TEST);
        assertFalse(mTestDied);
    }

    public void testSigSysSelf() {
        runDeathTest(SeccompDeathTestService.TEST_SIGSYS_SELF);
        assertTrue(mTestDied);
    }

    /**
     * Runs a death test by its test number, which needs to match a value in
     * SeccompDeathTestService.
     *
     * This blocks until the completion of the test, after which the test body
     * can use mTestEnded/mTestDied to see if the test died.
     */
    public void runDeathTest(final int testNumber) {
        mTestStarted = false;
        mTestEnded = false;
        mTestDied = false;

        mTestNumber = testNumber;

        Log.d(TAG, "Starting runDeathTest");
        launchDeathTestService();
        mCondition.block();

        assertTrue(mTestStarted);
        assertTrue(mTestEnded);
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mHandlerThread.start();
        mMessenger = new Messenger(new Handler(mHandlerThread.getLooper()) {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MSG_TEST_STARTED:
                        onTestStarted();
                        break;
                    case MSG_TEST_ENDED_CLEAN:
                        onTestEnded(false);
                        break;
                    default:
                        super.handleMessage(msg);
                }
            }
        });
    }

    @Override
    public void tearDown() throws Exception {
        try {
            mHandlerThread.quitSafely();
        } finally {
            super.tearDown();
        }
    }

    private void launchDeathTestService() {
        Log.d(TAG, "launchDeathTestService");
        mCondition.close();

        Intent intent = new Intent();
        intent.setComponent(new ComponentName("com.android.cts.security", "android.security.cts.SeccompDeathTestService"));

        if (!getContext().bindService(intent, this, Context.BIND_AUTO_CREATE)) {
            mCondition.open();
            fail("Failed to start DeathTestService");
        }
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
        Log.d(TAG, "onServiceConnected");

        Messenger remoteMessenger = new Messenger(service);
        Message msg = Message.obtain(null, SeccompDeathTestService.MSG_RUN_TEST);
        msg.getData().putBinder(SeccompDeathTestService.REPLY_BINDER_NAME, mMessenger.getBinder());
        msg.getData().putInt(SeccompDeathTestService.RUN_TEST_IDENTIFIER, mTestNumber);

        try {
            service.linkToDeath(this, 0);
            remoteMessenger.send(msg);
        } catch (RemoteException e) {
            Log.e(TAG, "Error setting up SeccompDeathTestService: " + e.getMessage());
        }
        Log.d(TAG, "Send MSG_TEST_START");
    }

    private void onTestStarted() {
        Log.d(TAG, "onTestStarted");
        mTestStarted = true;
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
        Log.d(TAG, "onServiceDisconnected");
    }

    @Override
    public void binderDied() {
        Log.d(TAG, "binderDied");
        if (mTestEnded)
            return;
        onTestEnded(true);
    }

    private void onTestEnded(boolean died) {
        Log.d(TAG, "onTestEnded, died=" + died);
        mTestEnded = true;
        mTestDied = died;
        getContext().unbindService(this);
        mCondition.open();
    }
}
