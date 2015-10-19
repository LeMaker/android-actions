package com.android.cts.tradefed.testtype;

import com.android.cts.tradefed.build.CtsBuildHelper;
import com.android.cts.util.AbiUtils;
import com.android.ddmlib.MultiLineReceiver;
import com.android.ddmlib.testrunner.TestIdentifier;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

/**
 * Test runner for dEQP tests
 *
 * Supports running drawElements Quality Program tests found under external/deqp.
 */
public class DeqpTestRunner implements IBuildReceiver, IDeviceTest, IRemoteTest {

    private static final String DEQP_ONDEVICE_APK = "com.drawelements.deqp.apk";
    private static final String DEQP_ONDEVICE_PKG = "com.drawelements.deqp";
    private static final String INCOMPLETE_LOG_MESSAGE = "Crash: Incomplete test log";

    private final int TESTCASE_BATCH_LIMIT = 1000;

    private boolean mLogData;

    private ITestDevice mDevice;

    private final String mPackageName;
    private final String mName;
    private Collection<TestIdentifier> mTests;
    private IAbi mAbi;
    private CtsBuildHelper mCtsBuild;

    private TestIdentifier mCurrentTestId;
    private boolean mGotTestResult;
    private String mCurrentTestLog;

    private ITestInvocationListener mListener;

    public DeqpTestRunner(String packageName, String name, Collection<TestIdentifier> tests) {
        mPackageName = packageName;
        mName = name;
        mTests = tests;
        mLogData = false;
    }

    /**
     * @param abi the ABI to run the test on
     */
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mCtsBuild = CtsBuildHelper.createBuildHelper(buildInfo);
    }

    /**
     * Set the CTS build container.
     * <p/>
     * Exposed so unit tests can mock the provided build.
     *
     * @param buildHelper
     */
    public void setBuildHelper(CtsBuildHelper buildHelper) {
        mCtsBuild = buildHelper;
    }

    /**
     * Enable or disable raw dEQP test log collection.
     */
    public void setCollectLogs(boolean logData) {
        mLogData = logData;
    }

    /**
     * dEQP instrumentation parser
     */
    class InstrumentationParser extends MultiLineReceiver {
        private DeqpTestRunner mDeqpTests;

        private Map<String, String> mValues;
        private String mCurrentName;
        private String mCurrentValue;


        public InstrumentationParser(DeqpTestRunner tests) {
            mDeqpTests = tests;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void processNewLines(String[] lines) {
            for (String line : lines) {
                if (mValues == null) mValues = new HashMap<String, String>();

                if (line.startsWith("INSTRUMENTATION_STATUS_CODE: ")) {
                    if (mCurrentName != null) {
                        mValues.put(mCurrentName, mCurrentValue);

                        mCurrentName = null;
                        mCurrentValue = null;
                    }

                    mDeqpTests.handleStatus(mValues);
                    mValues = null;
                } else if (line.startsWith("INSTRUMENTATION_STATUS: dEQP-")) {
                    if (mCurrentName != null) {
                        mValues.put(mCurrentName, mCurrentValue);

                        mCurrentValue = null;
                        mCurrentName = null;
                    }

                    String prefix = "INSTRUMENTATION_STATUS: ";
                    int nameBegin = prefix.length();
                    int nameEnd = line.indexOf('=');
                    int valueBegin = nameEnd + 1;

                    mCurrentName = line.substring(nameBegin, nameEnd);
                    mCurrentValue = line.substring(valueBegin);
                } else if (mCurrentValue != null) {
                    mCurrentValue = mCurrentValue + line;
                }
            }
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void done() {
            if (mCurrentName != null) {
                mValues.put(mCurrentName, mCurrentValue);

                mCurrentName = null;
                mCurrentValue = null;
            }

            if (mValues != null) {
                mDeqpTests.handleStatus(mValues);
                mValues = null;
            }
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean isCancelled() {
            return false;
        }
    }

    /**
     * Converts dEQP testcase path to TestIdentifier.
     */
    private TestIdentifier pathToIdentifier(String testPath) {
        String[] components = testPath.split("\\.");
        String name = components[components.length - 1];
        String className = null;

        for (int i = 0; i < components.length - 1; i++) {
            if (className == null) {
                className = components[i];
            } else {
                className = className + "." + components[i];
            }
        }

        return new TestIdentifier(className, name);
    }

    /**
     * Handles beginning of dEQP session.
     */
    private void handleBeginSession(Map<String, String> values) {
        String id = AbiUtils.createId(mAbi.getName(), mPackageName);
        mListener.testRunStarted(id, mTests.size());
    }

    /**
     * Handles end of dEQP session.
     */
    private void handleEndSession(Map<String, String> values) {
        Map <String, String> emptyMap = Collections.emptyMap();
        mListener.testRunEnded(0, emptyMap);
    }

    /**
     * Handles beginning of dEQP testcase.
     */
    private void handleBeginTestCase(Map<String, String> values) {
        mCurrentTestId = pathToIdentifier(values.get("dEQP-BeginTestCase-TestCasePath"));
        mCurrentTestLog = "";
        mGotTestResult = false;

        mListener.testStarted(mCurrentTestId);
        mTests.remove(mCurrentTestId);
    }

    /**
     * Handles end of dEQP testcase.
     */
    private void handleEndTestCase(Map<String, String> values) {
        Map <String, String> emptyMap = Collections.emptyMap();

        if (!mGotTestResult) {
            mListener.testFailed(mCurrentTestId,
                    INCOMPLETE_LOG_MESSAGE);
        }

        if (mLogData && mCurrentTestLog != null && mCurrentTestLog.length() > 0) {
            ByteArrayInputStreamSource source
                    = new ByteArrayInputStreamSource(mCurrentTestLog.getBytes());

            mListener.testLog(mCurrentTestId.getClassName() + "."
                    + mCurrentTestId.getTestName(), LogDataType.XML, source);

            source.cancel();
        }

        mListener.testEnded(mCurrentTestId, emptyMap);
        mCurrentTestId = null;
    }

    /**
     * Handles dEQP testcase result.
     */
    private void handleTestCaseResult(Map<String, String> values) {
        String code = values.get("dEQP-TestCaseResult-Code");
        String details = values.get("dEQP-TestCaseResult-Details");

        if (code.compareTo("Pass") == 0) {
            mGotTestResult = true;
        } else if (code.compareTo("NotSupported") == 0) {
            mGotTestResult = true;
        } else if (code.compareTo("QualityWarning") == 0) {
            mGotTestResult = true;
        } else if (code.compareTo("CompatibilityWarning") == 0) {
            mGotTestResult = true;
        } else if (code.compareTo("Fail") == 0 || code.compareTo("ResourceError") == 0
                || code.compareTo("InternalError") == 0 || code.compareTo("Crash") == 0
                || code.compareTo("Timeout") == 0) {
            mListener.testFailed(mCurrentTestId,
                    code + ": " + details);
            mGotTestResult = true;
        } else {
            mListener.testFailed(mCurrentTestId,
                    "Unknown result code: " + code + ": " + details);
            mGotTestResult = true;
        }
    }

    /**
     * Handles terminated dEQP testcase.
     */
    private void handleTestCaseTerminate(Map<String, String> values) {
        Map <String, String> emptyMap = Collections.emptyMap();

        String reason = values.get("dEQP-TerminateTestCase-Reason");
        mListener.testFailed(mCurrentTestId,
                "Terminated: " + reason);
        mListener.testEnded(mCurrentTestId, emptyMap);

        if (mLogData && mCurrentTestLog != null && mCurrentTestLog.length() > 0) {
            ByteArrayInputStreamSource source
                    = new ByteArrayInputStreamSource(mCurrentTestLog.getBytes());

            mListener.testLog(mCurrentTestId.getClassName() + "."
                    + mCurrentTestId.getTestName(), LogDataType.XML, source);

            source.cancel();
        }

        mCurrentTestId = null;
        mGotTestResult = true;
    }

    /**
     * Handles dEQP testlog data.
     */
    private void handleTestLogData(Map<String, String> values) {
        mCurrentTestLog = mCurrentTestLog + values.get("dEQP-TestLogData-Log");
    }

    /**
     * Handles new instrumentation status message.
     */
    public void handleStatus(Map<String, String> values) {
        String eventType = values.get("dEQP-EventType");

        if (eventType == null) {
            return;
        }

        if (eventType.compareTo("BeginSession") == 0) {
            handleBeginSession(values);
        } else if (eventType.compareTo("EndSession") == 0) {
            handleEndSession(values);
        } else if (eventType.compareTo("BeginTestCase") == 0) {
            handleBeginTestCase(values);
        } else if (eventType.compareTo("EndTestCase") == 0) {
            handleEndTestCase(values);
        } else if (eventType.compareTo("TestCaseResult") == 0) {
            handleTestCaseResult(values);
        } else if (eventType.compareTo("TerminateTestCase") == 0) {
            handleTestCaseTerminate(values);
        } else if (eventType.compareTo("TestLogData") == 0) {
            handleTestLogData(values);
        }
    }

    /**
     * Generates tescase trie from dEQP testcase paths. Used to define which testcases to execute.
     */
    private String generateTestCaseTrieFromPaths(Collection<String> tests) {
        String result = "{";
        boolean first = true;

        // Add testcases to results
        for (Iterator<String> iter = tests.iterator(); iter.hasNext();) {
            String test = iter.next();
            String[] components = test.split("\\.");

            if (components.length == 1) {
                if (!first) {
                    result = result + ",";
                }
                first = false;

                result += components[0];
                iter.remove();
            }
        }

        if (!tests.isEmpty()) {
            HashMap<String, ArrayList<String> > testGroups = new HashMap<>();

            // Collect all sub testgroups
            for (String test : tests) {
                String[] components = test.split("\\.");
                ArrayList<String> testGroup = testGroups.get(components[0]);

                if (testGroup == null) {
                    testGroup = new ArrayList<String>();
                    testGroups.put(components[0], testGroup);
                }

                testGroup.add(test.substring(components[0].length()+1));
            }

            for (String testGroup : testGroups.keySet()) {
                if (!first) {
                    result = result + ",";
                }

                first = false;
                result = result + testGroup
                        + generateTestCaseTrieFromPaths(testGroups.get(testGroup));
            }
        }

        return result + "}";
    }

    /**
     * Generates testcase trie from TestIdentifiers.
     */
    private String generateTestCaseTrie(Collection<TestIdentifier> tests) {
        ArrayList<String> testPaths = new ArrayList<String>();

        for (TestIdentifier test : tests) {
            testPaths.add(test.getClassName() + "." + test.getTestName());

            // Limit number of testcases for each run
            if (testPaths.size() > TESTCASE_BATCH_LIMIT)
                break;
        }

        return generateTestCaseTrieFromPaths(testPaths);
    }

    /**
     * Executes tests on the device.
     */
    private void executeTests(ITestInvocationListener listener) throws DeviceNotAvailableException {
        InstrumentationParser parser = new InstrumentationParser(this);
        String caseListFileName = "/sdcard/dEQP-TestCaseList.txt";
        String logFileName = "/sdcard/TestLog.qpa";
        String testCases = generateTestCaseTrie(mTests);

        mDevice.executeShellCommand("rm " + caseListFileName);
        mDevice.executeShellCommand("rm " + logFileName);
        mDevice.pushString(testCases + "\n", caseListFileName);

        String instrumentationName =
                "com.drawelements.deqp/com.drawelements.deqp.testercore.DeqpInstrumentation";

        String command = String.format(
                "am instrument %s -w -e deqpLogFileName \"%s\" -e deqpCmdLine \""
                    + "--deqp-caselist-file=%s --deqp-gl-config-name=rgba8888d24s8\""
                    + " -e deqpLogData \"%s\" %s",
                AbiUtils.createAbiFlag(mAbi.getName()), logFileName, caseListFileName, mLogData,
                instrumentationName);

        mDevice.executeShellCommand(command, parser);
        parser.flush();
    }

    /**
     * Check if device supports OpenGL ES version.
     */
    static boolean isSupportedGles(ITestDevice device, int requiredMajorVersion, int requiredMinorVersion) throws DeviceNotAvailableException {
        String roOpenglesVersion = device.getProperty("ro.opengles.version");

        if (roOpenglesVersion == null)
            return false;

        int intValue = Integer.parseInt(roOpenglesVersion);

        int majorVersion = ((intValue & 0xffff0000) >> 16);
        int minorVersion = (intValue & 0xffff);

        return (majorVersion > requiredMajorVersion)
                || (majorVersion == requiredMajorVersion && minorVersion >= requiredMinorVersion);
    }

    /**
     * Install dEQP OnDevice Package
     */
    private void installTestApk() throws DeviceNotAvailableException {
        try {
            File apkFile = mCtsBuild.getTestApp(DEQP_ONDEVICE_APK);
            String[] options = {AbiUtils.createAbiFlag(mAbi.getName())};
            String errorCode = getDevice().installPackage(apkFile, true, options);
            if (errorCode != null) {
                CLog.e("Failed to install %s. Reason: %s", DEQP_ONDEVICE_APK, errorCode);
            }
        } catch (FileNotFoundException e) {
            CLog.e("Could not find test apk %s", DEQP_ONDEVICE_APK);
        }
    }

    /**
     * Uninstall dEQP OnDevice Package
     */
    private void uninstallTestApk() throws DeviceNotAvailableException {
        getDevice().uninstallPackage(DEQP_ONDEVICE_PKG);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        mListener = listener;

        if ((mName.equals( "dEQP-GLES3") && isSupportedGles(mDevice, 3, 0))
            || (mName.equals("dEQP-GLES31") && isSupportedGles(mDevice, 3, 1))) {

            // Make sure there is no pre-existing package form earlier interrupted test run.
            uninstallTestApk();
            installTestApk();

            while (!mTests.isEmpty()) {
                executeTests(listener);

                // Set test to failed if it didn't receive test result
                if (mCurrentTestId != null) {
                    Map <String, String> emptyMap = Collections.emptyMap();

                    if (mLogData && mCurrentTestLog != null && mCurrentTestLog.length() > 0) {
                        ByteArrayInputStreamSource source
                                = new ByteArrayInputStreamSource(mCurrentTestLog.getBytes());

                        mListener.testLog(mCurrentTestId.getClassName() + "."
                                + mCurrentTestId.getTestName(), LogDataType.XML, source);

                        source.cancel();
                    }
                    if (!mGotTestResult) {
                        mListener.testFailed(mCurrentTestId,
                            INCOMPLETE_LOG_MESSAGE);
                    }

                    mListener.testEnded(mCurrentTestId, emptyMap);
                    mCurrentTestId = null;
                    mListener.testRunEnded(0, emptyMap);
                }
            }

            uninstallTestApk();
        } else {
            /* Pass all tests if OpenGL ES version is not supported */
            Map <String, String> emptyMap = Collections.emptyMap();
            String id = AbiUtils.createId(mAbi.getName(), mPackageName);
            mListener.testRunStarted(id, mTests.size());

            for (TestIdentifier test : mTests) {
                CLog.d("Skipping test '%s', Opengl ES version not supported", test.toString());
                mListener.testStarted(test);
                mListener.testEnded(test, emptyMap);
            }

            mListener.testRunEnded(0, emptyMap);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }
}
