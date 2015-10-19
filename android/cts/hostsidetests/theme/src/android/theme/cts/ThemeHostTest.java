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

package android.theme.cts;

import com.android.cts.tradefed.build.CtsBuildHelper;
import com.android.cts.util.AbiUtils;
import com.android.cts.util.TimeoutReq;
import com.android.ddmlib.Log;
import com.android.ddmlib.Log.LogLevel;
import com.android.ddmlib.IShellOutputReceiver;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IBuildReceiver;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.lang.String;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Scanner;
import java.util.concurrent.Callable;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorCompletionService;
import java.util.concurrent.ExecutorService;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * Test to check the Holo theme has not been changed.
 */
public class ThemeHostTest extends DeviceTestCase implements IAbiReceiver, IBuildReceiver {

    private static final String TAG = ThemeHostTest.class.getSimpleName();

    private static final int CAPTURE_TIMEOUT = 500;//0.5sec in ms

    private static final int ADB_TIMEOUT = 60 * 60 * 1000;//60mins in ms

    /** The package name of the APK. */
    private static final String PACKAGE = "android.theme.app";

    /** The file name of the APK. */
    private static final String APK = "CtsThemeDeviceApp.apk";

    /** The class name of the main activity in the APK. */
    private static final String CLASS = "HoloDeviceActivity";

    /** The command to launch the main activity. */
    private static final String START_CMD = String.format(
            "am start -W -a android.intent.action.MAIN -n %s/%s.%s", PACKAGE, PACKAGE, CLASS);

    private static final String STOP_CMD = String.format("am force-stop %s", PACKAGE);

    private static final String HARDWARE_TYPE_CMD = "dumpsys | grep android.hardware.type";

    private static final String DENSITY_PROP_DEVICE = "ro.sf.lcd_density";

    private static final String DENSITY_PROP_EMULATOR = "qemu.sf.lcd_density";

    // Intent extras
    protected final static String INTENT_STRING_EXTRA = " --es %s %s";

    protected final static String INTENT_BOOLEAN_EXTRA = " --ez %s %b";

    protected final static String INTENT_INTEGER_EXTRA = " --ei %s %d";

    // Intent extra keys
    private static final String EXTRA_THEME = "holo_theme_extra";

    private static final String EXTRA_LAYOUT = "holo_layout_extra";

    private static final String EXTRA_TIMEOUT = "holo_timeout_extra";

    private static final String[] THEMES = {
            "holo",
            "holo_dialog",
            "holo_dialog_minwidth",
            "holo_dialog_noactionbar",
            "holo_dialog_noactionbar_minwidth",
            "holo_dialogwhenlarge",
            "holo_dialogwhenlarge_noactionbar",
            "holo_inputmethod",
            "holo_light",
            "holo_light_darkactionbar",
            "holo_light_dialog",
            "holo_light_dialog_minwidth",
            "holo_light_dialog_noactionbar",
            "holo_light_dialog_noactionbar_minwidth",
            "holo_light_dialogwhenlarge",
            "holo_light_dialogwhenlarge_noactionbar",
            "holo_light_noactionbar",
            "holo_light_noactionbar_fullscreen",
            "holo_light_panel",
            "holo_noactionbar",
            "holo_noactionbar_fullscreen",
            "holo_panel",
            "holo_wallpaper",
            "holo_wallpaper_notitlebar",
    };

    private final int NUM_THEMES = THEMES.length;

    private static final String[] LAYOUTS = {
            "button",
            "button_pressed",
            "checkbox",
            "checkbox_checked",
            "chronometer",
            "color_blue_bright",
            "color_blue_dark",
            "color_blue_light",
            "color_green_dark",
            "color_green_light",
            "color_orange_dark",
            "color_orange_light",
            "color_purple",
            "color_red_dark",
            "color_red_light",
            "datepicker",
            "display_info",
            "edittext",
            "progressbar_horizontal_0",
            "progressbar_horizontal_100",
            "progressbar_horizontal_50",
            "progressbar_large",
            "progressbar_small",
            "progressbar",
            "radiobutton_checked",
            "radiobutton",
            "radiogroup_horizontal",
            "radiogroup_vertical",
            "ratingbar_0",
            "ratingbar_2point5",
            "ratingbar_5",
            "ratingbar_0_pressed",
            "ratingbar_2point5_pressed",
            "ratingbar_5_pressed",
            "searchview_query",
            "searchview_query_hint",
            "seekbar_0",
            "seekbar_100",
            "seekbar_50",
            "spinner",
            "switch_button_checked",
            "switch_button",
            "textview",
            "timepicker",
            "togglebutton_checked",
            "togglebutton",
            "zoomcontrols",
    };

    private final int NUM_LAYOUTS = LAYOUTS.length;

    private final HashMap<String, File> mReferences = new HashMap<String, File>();

    /** The ABI to use. */
    private IAbi mAbi;

    /** A reference to the build. */
    private CtsBuildHelper mBuild;

    /** A reference to the device under test. */
    private ITestDevice mDevice;

    private ExecutorService mExecutionService;

    private ExecutorCompletionService<Boolean> mCompletionService;

    @Override
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        // Get the build, this is used to access the APK.
        mBuild = CtsBuildHelper.createBuildHelper(buildInfo);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        // Get the device, this gives a handle to run commands and install APKs.
        mDevice = getDevice();
        // Remove any previously installed versions of this APK.
        mDevice.uninstallPackage(PACKAGE);
        // Get the APK from the build.
        File app = mBuild.getTestApp(APK);
        // Get the ABI flag.
        String[] options = {AbiUtils.createAbiFlag(mAbi.getName())};
        // Install the APK on the device.
        mDevice.installPackage(app, false, options);

        final String densityProp;

        if (mDevice.getSerialNumber().startsWith("emulator-")) {
            densityProp = DENSITY_PROP_EMULATOR;
        } else {
            densityProp = DENSITY_PROP_DEVICE;
        }

        final String zip = String.format("/%s.zip",
                getDensityBucket(Integer.parseInt(mDevice.getProperty(densityProp))));
        Log.logAndDisplay(LogLevel.INFO, TAG, "Loading resources from " + zip);


        final InputStream zipStream = this.getClass().getResourceAsStream(zip);
        if (zipStream != null) {
            final ZipInputStream in = new ZipInputStream(zipStream);
            try {
                ZipEntry ze;
                final byte[] buffer = new byte[1024];
                while ((ze = in.getNextEntry()) != null) {
                    final String name = ze.getName();
                    final File tmp = File.createTempFile("ref_" + name, ".png");
                    final FileOutputStream out = new FileOutputStream(tmp);
                    int count;
                    while ((count = in.read(buffer)) != -1) {
                        out.write(buffer, 0, count);
                    }
                    out.flush();
                    out.close();
                    mReferences.put(name, tmp);
                }
            } finally {
                in.close();
            }
        }

        mExecutionService = Executors.newFixedThreadPool(2);// 2 worker threads
        mCompletionService = new ExecutorCompletionService<Boolean>(mExecutionService);
    }

    @Override
    protected void tearDown() throws Exception {
        // Delete the temp files
        for (File ref : mReferences.values()) {
            ref.delete();
        }
        mExecutionService.shutdown();
        // Remove the APK.
        mDevice.uninstallPackage(PACKAGE);
        super.tearDown();
    }

    @TimeoutReq(minutes = 60)
    public void testHoloThemes() throws Exception {
        if (checkHardwareTypeSkipTest(
                mDevice.executeShellCommand(HARDWARE_TYPE_CMD).trim())) {
            Log.logAndDisplay(LogLevel.INFO, TAG, "Skipped HoloThemes test for watch and TV");
            return;
        }


        if (mReferences.isEmpty()) {
            Log.logAndDisplay(LogLevel.INFO, TAG,
                    "Skipped HoloThemes test due to no reference images");
            return;
        }

        int numTasks = 0;
        for (int i = 0; i < NUM_THEMES; i++) {
            final String themeName = THEMES[i];
            for (int j = 0; j < NUM_LAYOUTS; j++) {
                final String name = String.format("%s_%s", themeName, LAYOUTS[j]);
                if (runCapture(i, j, name)) {
                    final File ref = mReferences.get(name + ".png");
                    if (!ref.exists()) {
                        Log.logAndDisplay(LogLevel.INFO, TAG,
                                "Skipping theme test due to missing reference for reference image " + name);
                        continue;
                    }
                    mCompletionService.submit(new ComparisonTask(mDevice, ref, name));
                    numTasks++;
                } else {
                    Log.logAndDisplay(LogLevel.ERROR, TAG, "Capture failed: " + name);
                }
            }
        }
        int failures = 0;
        for (int i = 0; i < numTasks; i++) {
            failures += mCompletionService.take().get() ? 0 : 1;
        }
        assertTrue(failures + " failures in theme test", failures == 0);
    }

    private boolean runCapture(int themeId, int layoutId, String imageName) throws Exception {
        final StringBuilder sb = new StringBuilder(START_CMD);
        sb.append(String.format(INTENT_INTEGER_EXTRA, EXTRA_THEME, themeId));
        sb.append(String.format(INTENT_INTEGER_EXTRA, EXTRA_LAYOUT, layoutId));
        sb.append(String.format(INTENT_INTEGER_EXTRA, EXTRA_TIMEOUT, CAPTURE_TIMEOUT));
        final String startCommand = sb.toString();
        // Clear logcat
        mDevice.executeAdbCommand("logcat", "-c");
        // Stop any existing instances
        mDevice.executeShellCommand(STOP_CMD);
        // Start activity
        mDevice.executeShellCommand(startCommand);

        boolean success = false;
        boolean waiting = true;
        while (waiting) {
            // Dump logcat.
            final String logs = mDevice.executeAdbCommand(
                    "logcat", "-v", "brief", "-d", CLASS + ":I", "*:S");
            // Search for string.
            final Scanner in = new Scanner(logs);
            while (in.hasNextLine()) {
                final String line = in.nextLine();
                if (line.startsWith("I/" + CLASS)) {
                    final String[] lineSplit = line.split(":");
                    final String s = lineSplit[1].trim();
                    final String imageNameGenerated = lineSplit[2].trim();
                    if (s.equals("OKAY") && imageNameGenerated.equals(imageName)) {
                        success = true;
                        waiting = false;
                    } else if (s.equals("ERROR") && imageNameGenerated.equals(imageName)) {
                        success = false;
                        waiting = false;
                    }
                }
            }
            in.close();
        }

        return success;
    }

    private static String getDensityBucket(int density) {
        switch (density) {
            case 120:
                return "ldpi";
            case 160:
                return "mdpi";
            case 213:
                return "tvdpi";
            case 240:
                return "hdpi";
            case 320:
                return "xhdpi";
            case 400:
                return "400dpi";
            case 480:
                return "xxhdpi";
            case 560:
                return "560dpi";
            case 640:
                return "xxxhdpi";
            default:
                return "" + density;
        }
    }

    private static boolean checkHardwareTypeSkipTest(String hardwareTypeString) {
        if (hardwareTypeString.contains("android.hardware.type.watch")) {
            return true;
        }
        if (hardwareTypeString.contains("android.hardware.type.television")) {
            return true;
        }
        return false;
    }
}
