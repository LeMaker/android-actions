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

package android.print.cts;

import android.app.ActivityManagerNative;
import android.app.IActivityManager;
import android.app.IInstrumentationWatcher;
import android.app.Instrumentation;
import android.app.UiAutomationConnection;
import android.content.ComponentName;
import android.content.pm.IPackageDataObserver;
import android.os.Binder;
import android.os.Bundle;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.util.AndroidException;
import android.view.IWindowManager;

import com.android.internal.os.BaseCommand;

import java.io.PrintStream;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.atomic.AtomicBoolean;

public final class PrintInstrument extends BaseCommand {

    private static final String ARG_PRIVILEGED_OPS = "ARG_PRIVILEGED_OPS";

    private IActivityManager mAm;

    public static void main(String[] args) {
        PrintInstrument instrumenter = new PrintInstrument();
        instrumenter.run(args);
    }

    @Override
    public void onRun() throws Exception {
        mAm = ActivityManagerNative.getDefault();
        if (mAm == null) {
            System.err.println(NO_SYSTEM_ERROR_CODE);
            throw new AndroidException("Can't connect to activity manager;"
                    + " is the system running?");
        }

        String op = nextArgRequired();

        if (op.equals("instrument")) {
            runInstrument();
        } else {
            showError("Error: unknown command '" + op + "'");
        }
    }

    @Override
    public void onShowUsage(PrintStream out) {
        /* do nothing */
    }

    @SuppressWarnings("deprecation")
    private void runInstrument() throws Exception {
        String profileFile = null;
        boolean wait = false;
        boolean rawMode = false;
        boolean no_window_animation = false;
        int userId = UserHandle.USER_CURRENT;
        Bundle args = new Bundle();
        String argKey = null, argValue = null;
        IWindowManager wm = IWindowManager.Stub.asInterface(ServiceManager.getService("window"));

        String opt;
        while ((opt=nextOption()) != null) {
            if (opt.equals("-p")) {
                profileFile = nextArgRequired();
            } else if (opt.equals("-w")) {
                wait = true;
            } else if (opt.equals("-r")) {
                rawMode = true;
            } else if (opt.equals("-e")) {
                argKey = nextArgRequired();
                argValue = nextArgRequired();
                args.putString(argKey, argValue);
            } else if (opt.equals("--no_window_animation")
                    || opt.equals("--no-window-animation")) {
                no_window_animation = true;
            } else if (opt.equals("--user")) {
                userId = parseUserArg(nextArgRequired());
            } else {
                System.err.println("Error: Unknown option: " + opt);
                return;
            }
        }

        if (userId == UserHandle.USER_ALL) {
            System.err.println("Error: Can't start instrumentation with user 'all'");
            return;
        }

        String cnArg = nextArgRequired();
        ComponentName cn = ComponentName.unflattenFromString(cnArg);
        if (cn == null) throw new IllegalArgumentException("Bad component name: " + cnArg);

        InstrumentationWatcher watcher = null;
        UiAutomationConnection connection = null;
        if (wait) {
            watcher = new InstrumentationWatcher();
            watcher.setRawOutput(rawMode);
            connection = new UiAutomationConnection();
        }

        float[] oldAnims = null;
        if (no_window_animation) {
            oldAnims = wm.getAnimationScales();
            wm.setAnimationScale(0, 0.0f);
            wm.setAnimationScale(1, 0.0f);
        }

        args.putIBinder(ARG_PRIVILEGED_OPS, new PrivilegedOperations(mAm));

        if (!mAm.startInstrumentation(cn, profileFile, 0, args, watcher, connection, userId, null)) {
            throw new AndroidException("INSTRUMENTATION_FAILED: " + cn.flattenToString());
        }

        if (watcher != null) {
            if (!watcher.waitForFinish()) {
                System.out.println("INSTRUMENTATION_ABORTED: System has crashed.");
            }
        }

        if (oldAnims != null) {
            wm.setAnimationScales(oldAnims);
        }
    }

    private int parseUserArg(String arg) {
        int userId;
        if ("all".equals(arg)) {
            userId = UserHandle.USER_ALL;
        } else if ("current".equals(arg) || "cur".equals(arg)) {
            userId = UserHandle.USER_CURRENT;
        } else {
            userId = Integer.parseInt(arg);
        }
        return userId;
    }

    private class InstrumentationWatcher extends IInstrumentationWatcher.Stub {
        private boolean mFinished = false;
        private boolean mRawMode = false;

        /**
         * Set or reset "raw mode".  In "raw mode", all bundles are dumped.  In "pretty mode",
         * if a bundle includes Instrumentation.REPORT_KEY_STREAMRESULT, just print that.
         * @param rawMode true for raw mode, false for pretty mode.
         */
        public void setRawOutput(boolean rawMode) {
            mRawMode = rawMode;
        }

        @Override
        public void instrumentationStatus(ComponentName name, int resultCode, Bundle results) {
            synchronized (this) {
                // pretty printer mode?
                String pretty = null;
                if (!mRawMode && results != null) {
                    pretty = results.getString(Instrumentation.REPORT_KEY_STREAMRESULT);
                }
                if (pretty != null) {
                    System.out.print(pretty);
                } else {
                    if (results != null) {
                        for (String key : results.keySet()) {
                            System.out.println(
                                    "INSTRUMENTATION_STATUS: " + key + "=" + results.get(key));
                        }
                    }
                    System.out.println("INSTRUMENTATION_STATUS_CODE: " + resultCode);
                }
                notifyAll();
            }
        }

        @Override
        public void instrumentationFinished(ComponentName name, int resultCode,
                Bundle results) {
            synchronized (this) {
                // pretty printer mode?
                String pretty = null;
                if (!mRawMode && results != null) {
                    pretty = results.getString(Instrumentation.REPORT_KEY_STREAMRESULT);
                }
                if (pretty != null) {
                    System.out.println(pretty);
                } else {
                    if (results != null) {
                        for (String key : results.keySet()) {
                            System.out.println(
                                    "INSTRUMENTATION_RESULT: " + key + "=" + results.get(key));
                        }
                    }
                    System.out.println("INSTRUMENTATION_CODE: " + resultCode);
                }
                mFinished = true;
                notifyAll();
            }
        }

        public boolean waitForFinish() {
            synchronized (this) {
                while (!mFinished) {
                    try {
                        if (!mAm.asBinder().pingBinder()) {
                            return false;
                        }
                        wait(1000);
                    } catch (InterruptedException e) {
                        throw new IllegalStateException(e);
                    }
                }
            }
            return true;
        }
    }

    private static final class PrivilegedOperations extends IPrivilegedOperations.Stub {
        private final IActivityManager mAm;

        public PrivilegedOperations(IActivityManager am) {
            mAm = am;
        }

        @Override
        public boolean clearApplicationUserData(final String clearedPackageName)
                throws RemoteException {
            final long identity = Binder.clearCallingIdentity();
            try {
                final AtomicBoolean success = new AtomicBoolean();
                final CountDownLatch completionLatch = new CountDownLatch(1);

                mAm.clearApplicationUserData(clearedPackageName,
                        new IPackageDataObserver.Stub() {
                            @Override
                            public void onRemoveCompleted(String packageName, boolean succeeded) {
                                if (clearedPackageName.equals(packageName) && succeeded) {
                                    success.set(true);
                                } else {
                                    success.set(false);
                                }
                                completionLatch.countDown();
                            }
                }, UserHandle.USER_CURRENT);

                try {
                    completionLatch.await();
                } catch (InterruptedException ie) {
                    /* ignore */
                }

                return success.get();
            } finally {
                Binder.restoreCallingIdentity(identity);
            }
        }
    }
}
