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

package com.android.cts.tradefed.testtype;

import com.android.cts.tradefed.build.CtsBuildHelper;
import com.android.cts.tradefed.targetprep.SettingsToggler;
import com.android.cts.util.AbiUtils;
import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner.TestSize;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.StringEscapeUtils;

import java.io.FileNotFoundException;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Running the print tests requires modification of secure settings. Secure
 * settings cannot be changed from device CTS tests since system signature
 * permission is required. Such settings can be modified by the shell user,
 * so a host side test driver is used for enabling these services, running
 * the tests, and disabling the services.
 */
public class PrintTestRunner implements IBuildReceiver, IRemoteTest, IDeviceTest  {

    private static final String PRINT_TEST_AND_SERVICES_APP_NAME =
            "CtsPrintTestCases.apk";

    private static final String PRINT_TESTS_PACKAGE_NAME =
            "com.android.cts.print";

    private static final String FIRST_PRINT_SERVICE_NAME =
            "android.print.cts.services.FirstPrintService";

    private static final String SECOND_PRINT_SERVICE_NAME =
            "android.print.cts.services.SecondPrintService";

    private static final String SHELL_USER_FOLDER = "data/local/tmp";

    private static final String PRINT_INSTRUMENT_JAR = "CtsPrintInstrument.jar";

    private static final String PRINT_INSTRUMENT_SCRIPT = "print-instrument";

    private ITestDevice mDevice;

    private CtsBuildHelper mCtsBuild;

    private IAbi mAbi;
    private String mPackageName;
    private String mRunnerName = "android.test.InstrumentationTestRunner";
    private String mTestClassName;
    private String mTestMethodName;
    private String mTestPackageName;
    private int mTestTimeout = 10 * 60 * 1000;  // 10 minutes
    private String mTestSize;
    private String mRunName = null;
    private Map<String, String> mInstrArgMap = new HashMap<String, String>();

    /**
     * @param abi The ABI to run the test on
     */
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mCtsBuild = CtsBuildHelper.createBuildHelper(buildInfo);
    }

    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    public void setPackageName(String packageName) {
        mPackageName = packageName;
    }

    public void setRunnerName(String runnerName) {
        mRunnerName = runnerName;
    }

    public void setClassName(String testClassName) {
        mTestClassName = testClassName;
    }

    public void setMethodName(String testMethodName) {
        mTestMethodName = StringEscapeUtils.escapeShell(testMethodName);
    }

    public void setTestPackageName(String testPackageName) {
        mTestPackageName = testPackageName;
    }

    public void setTestSize(String size) {
        mTestSize = size;
    }

    public void setRunName(String runName) {
        mRunName = runName;
    }

    @Override
    public void run(final ITestInvocationListener listener) throws DeviceNotAvailableException {
        installShellProgramAndScriptFiles();
        installTestsAndServicesApk();
        enablePrintServices();
        doRunTests(listener);
        disablePrintServices();
        uninstallTestsAndServicesApk();
        uninstallShellProgramAndScriptFiles();
    }

    private void doRunTests(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        if (mPackageName == null) {
            throw new IllegalArgumentException("package name has not been set");
        }
        if (mDevice == null) {
            throw new IllegalArgumentException("Device has not been set");
        }

        IRemoteAndroidTestRunner runner =  new PrintTestRemoteTestRunner(mPackageName,
                mRunnerName, mDevice.getIDevice());

        if (mTestClassName != null) {
            if (mTestMethodName != null) {
                runner.setMethodName(mTestClassName, mTestMethodName);
            } else {
                runner.setClassName(mTestClassName);
            }
        } else if (mTestPackageName != null) {
            runner.setTestPackageName(mTestPackageName);
        }
        if (mTestSize != null) {
            runner.setTestSize(TestSize.getTestSize(mTestSize));
        }
        runner.setMaxTimeToOutputResponse(mTestTimeout, TimeUnit.MILLISECONDS);
        if (mRunName != null) {
            runner.setRunName(mRunName);
        }
        for (Map.Entry<String, String> argEntry : mInstrArgMap.entrySet()) {
            runner.addInstrumentationArg(argEntry.getKey(), argEntry.getValue());
        }

        mDevice.runInstrumentationTests(runner, listener);
    }

    private void installShellProgramAndScriptFiles() throws DeviceNotAvailableException {
        installFile(PRINT_INSTRUMENT_JAR);
        installFile(PRINT_INSTRUMENT_SCRIPT);
    }

    private void installFile(String fileName) throws DeviceNotAvailableException {
        try {
            final boolean success = getDevice().pushFile(mCtsBuild.getTestApp(
                    fileName), SHELL_USER_FOLDER + "/" + fileName);
            if (!success) {
                throw new IllegalArgumentException("Failed to install "
                        + fileName + " on " + getDevice().getSerialNumber());
           }
        } catch (FileNotFoundException fnfe) {
            throw new IllegalArgumentException("Cannot find file: " + fileName);
        }
    }

    private void uninstallShellProgramAndScriptFiles() throws DeviceNotAvailableException {
        getDevice().executeShellCommand("rm " + SHELL_USER_FOLDER + "/"
                + PRINT_INSTRUMENT_JAR);
        getDevice().executeShellCommand("rm " + SHELL_USER_FOLDER + "/"
                + PRINT_INSTRUMENT_SCRIPT);
    }

    private void installTestsAndServicesApk() throws DeviceNotAvailableException {
        try {
            String[] options = {AbiUtils.createAbiFlag(mAbi.getName())};
            String installCode = getDevice().installPackage(mCtsBuild.getTestApp(
                    PRINT_TEST_AND_SERVICES_APP_NAME), true, options);
            if (installCode != null) {
                throw new IllegalArgumentException("Failed to install "
                        + PRINT_TEST_AND_SERVICES_APP_NAME + " on " + getDevice().getSerialNumber()
                        + ". Reason: " + installCode);
           }
        } catch (FileNotFoundException fnfe) {
            throw new IllegalArgumentException("Cannot find file: "
                    + PRINT_TEST_AND_SERVICES_APP_NAME);
        }
    }

    private void uninstallTestsAndServicesApk() throws DeviceNotAvailableException {
        getDevice().uninstallPackage(PRINT_TESTS_PACKAGE_NAME);
    }

    private void enablePrintServices() throws DeviceNotAvailableException {
        String enabledServicesValue = PRINT_TESTS_PACKAGE_NAME + "/" + FIRST_PRINT_SERVICE_NAME
                + ":" + PRINT_TESTS_PACKAGE_NAME + "/" + SECOND_PRINT_SERVICE_NAME;
        SettingsToggler.setSecureString(getDevice(), "enabled_print_services",
                enabledServicesValue);
    }

    private void disablePrintServices() throws DeviceNotAvailableException {
        SettingsToggler.setSecureString(getDevice(), "enabled_print_services", "");
    }
}
