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

import android.app.Service;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;

/**
 * Service used to run tests for seccomp-bpf sandboxing. Since sandbox violations
 * result in process termination, they cannot be run from within the test case
 * itself. The SeccompBpfTest starts this service to run code out-of-process and
 * then observes when the Binder channel dies. If the test does not die, the
 * service reports back to the test that it exited cleanly.
 */
public class SeccompDeathTestService extends Service {
    static final String TAG = SeccompBpfTest.TAG;

    static {
        System.loadLibrary("ctssecurity_jni");
    }

    /**
     * Message sent from SeccompBpfTest to run a test.
     */
    final static int MSG_RUN_TEST = 100;
    /**
     * In MSG_RUN_TEST, the test number to run.
     */
    final static String RUN_TEST_IDENTIFIER = "android.security.cts.SeccompDeathTestService.testID";
    /**
     * In MSG_RUN_TEST, the Binder on which to report clean death.
     */
    static final String REPLY_BINDER_NAME = "android.security.cts.SeccompBpfTest";

    // Test numbers that map to test methods in this service.
    final static int TEST_DEATH_TEST = 1;
    final static int TEST_CLEAN_TEST = 2;
    final static int TEST_SIGSYS_SELF = 3;

    final private Messenger mMessenger = new Messenger(new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_RUN_TEST:
                    runTest(msg);
                    break;
                default:
                    super.handleMessage(msg);
            }
        }
    });

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG, "onBind");
        return mMessenger.getBinder();
    }

    private void runTest(Message msg) {
        Log.d(TAG, "runTest");
        IBinder harnessBinder = msg.getData().getBinder(REPLY_BINDER_NAME);
        Messenger harness = new Messenger(harnessBinder);

        try {
            Log.d(TAG, "Send MSG_TEST_STARTED");
            harness.send(Message.obtain(null, SeccompBpfTest.MSG_TEST_STARTED));
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to MSG_TEST_STARTED: " + e.getMessage());
        }

        demuxTest(msg.getData().getInt(RUN_TEST_IDENTIFIER));

        try {
            Log.d(TAG, "Send MSG_TEST_ENDED_CLEAN");
            harness.send(Message.obtain(null, SeccompBpfTest.MSG_TEST_ENDED_CLEAN));
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to MSG_TEST_ENDED_CLEAN: " + e.getMessage());
        }
    }

    private void demuxTest(int testNumber) {
        switch (testNumber) {
            case TEST_DEATH_TEST:
                testDeath();
                break;
            case TEST_CLEAN_TEST:
                break;
            case TEST_SIGSYS_SELF:
                testSigSysSelf();
                break;
            default:
                throw new RuntimeException("Unknown test number " + testNumber);
        }
    }

    public void testDeath() {
        String s = null;
        s.hashCode();
    }

    public native void testSigSysSelf();
}
