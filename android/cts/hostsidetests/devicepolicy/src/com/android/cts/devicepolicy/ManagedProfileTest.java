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

package com.android.cts.devicepolicy;

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;

/**
 * Set of tests for Managed Profile use cases.
 */
public class ManagedProfileTest extends BaseDevicePolicyTest {

    private static final String MANAGED_PROFILE_PKG = "com.android.cts.managedprofile";
    private static final String MANAGED_PROFILE_APK = "CtsManagedProfileApp.apk";

    private static final String INTENT_SENDER_PKG = "com.android.cts.intent.sender";
    private static final String INTENT_SENDER_APK = "CtsIntentSenderApp.apk";

    private static final String INTENT_RECEIVER_PKG = "com.android.cts.intent.receiver";
    private static final String INTENT_RECEIVER_APK = "CtsIntentReceiverApp.apk";

    private static final String ADMIN_RECEIVER_TEST_CLASS =
            MANAGED_PROFILE_PKG + ".BaseManagedProfileTest$BasicAdminReceiver";

    private static final String FEATURE_BLUETOOTH = "android.hardware.bluetooth";
    private int mUserId;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        // We need multi user to be supported in order to create a profile of the user owner.
        mHasFeature = mHasFeature && (getMaxNumberOfUsersSupported() > 1) && hasDeviceFeature(
                "android.software.managed_users");

        if (mHasFeature) {
            mUserId = createManagedProfile();
            installApp(MANAGED_PROFILE_APK);
            installApp(INTENT_RECEIVER_APK);
            installApp(INTENT_SENDER_APK);
            setProfileOwner(MANAGED_PROFILE_PKG + "/" + ADMIN_RECEIVER_TEST_CLASS, mUserId);
            startUser(mUserId);
        }
    }

    @Override
    protected void tearDown() throws Exception {
        if (mHasFeature) {
            removeUser(mUserId);
            getDevice().uninstallPackage(MANAGED_PROFILE_PKG);
            getDevice().uninstallPackage(INTENT_SENDER_PKG);
            getDevice().uninstallPackage(INTENT_RECEIVER_PKG);
        }
        super.tearDown();
    }

    public void testManagedProfileSetup() throws Exception {
        if (!mHasFeature) {
            return;
        }
        assertTrue(runDeviceTestsAsUser(
                MANAGED_PROFILE_PKG, MANAGED_PROFILE_PKG + ".ManagedProfileSetupTest", mUserId));
    }

    /**
     *  wipeData() test removes the managed profile, so it needs to separated from other tests.
     */
    public void testWipeData() throws Exception {
        if (!mHasFeature) {
            return;
        }
        assertTrue(listUsers().contains(mUserId));
        assertTrue(runDeviceTestsAsUser(
                MANAGED_PROFILE_PKG, MANAGED_PROFILE_PKG + ".WipeDataTest", mUserId));
        // Note: the managed profile is removed by this test, which will make removeUserCommand in
        // tearDown() to complain, but that should be OK since its result is not asserted.
        assertFalse(listUsers().contains(mUserId));
    }

    public void testMaxUsersStrictlyMoreThanOne() throws Exception {
        if (hasDeviceFeature("android.software.managed_users")) {
            assertTrue("Device must support more than 1 user "
                    + "if android.software.managed_users feature is available",
            getMaxNumberOfUsersSupported() > 1);
        }
    }

    public void testCrossProfileIntentFilters() throws Exception {
        if (!mHasFeature) {
            return;
        }
        // Set up activities: ManagedProfileActivity will only be enabled in the managed profile and
        // PrimaryUserActivity only in the primary one
        disableActivityForUser("ManagedProfileActivity", 0);
        disableActivityForUser("PrimaryUserActivity", mUserId);

        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG,
                MANAGED_PROFILE_PKG + ".ManagedProfileTest", mUserId));

        // Set up filters from primary to managed profile
        String command = "am start -W --user " + mUserId  + " " + MANAGED_PROFILE_PKG
                + "/.PrimaryUserFilterSetterActivity";
        CLog.logAndDisplay(LogLevel.INFO, "Output for command " + command + ": "
              + getDevice().executeShellCommand(command));
        assertTrue(runDeviceTests(MANAGED_PROFILE_PKG, MANAGED_PROFILE_PKG + ".PrimaryUserTest"));
        // TODO: Test with startActivity
    }

    public void testSettingsIntents() throws Exception {
        if (!mHasFeature) {
            return;
        }

        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".SettingsIntentsTest", mUserId));
    }

    public void testCrossProfileContent() throws Exception {
        if (!mHasFeature) {
            return;
        }

        // Test from parent to managed
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                "removeAllFilters", mUserId));
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                "addManagedCanAccessParentFilters", mUserId));
        assertTrue(runDeviceTestsAsUser(INTENT_SENDER_PKG, ".ContentTest", 0));

        // Test from managed to parent
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                "removeAllFilters", mUserId));
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                "addParentCanAccessManagedFilters", mUserId));
        assertTrue(runDeviceTestsAsUser(INTENT_SENDER_PKG, ".ContentTest", mUserId));

    }

    public void testCrossProfileCopyPaste() throws Exception {
        if (!mHasFeature) {
            return;
        }

        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                "allowCrossProfileCopyPaste", mUserId));
        // Test that managed can see what is copied in the parent.
        testCrossProfileCopyPasteInternal(mUserId, true);
        // Test that the parent can see what is copied in managed.
        testCrossProfileCopyPasteInternal(0, true);

        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                "disallowCrossProfileCopyPaste", mUserId));
        // Test that managed can still see what is copied in the parent.
        testCrossProfileCopyPasteInternal(mUserId, true);
        // Test that the parent cannot see what is copied in managed.
        testCrossProfileCopyPasteInternal(0, false);
    }

    private void testCrossProfileCopyPasteInternal(int userId, boolean shouldSucceed)
            throws DeviceNotAvailableException {
        final String direction = (userId == 0) ? "addManagedCanAccessParentFilters"
                : "addParentCanAccessManagedFilters";
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                "removeAllFilters", mUserId));
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".CrossProfileUtils",
                direction, mUserId));
        if (shouldSucceed) {
            assertTrue(runDeviceTestsAsUser(INTENT_SENDER_PKG, ".CopyPasteTest",
                    "testCanReadAcrossProfiles", userId));
            assertTrue(runDeviceTestsAsUser(INTENT_SENDER_PKG, ".CopyPasteTest",
                    "testIsNotified", userId));
        } else {
            assertTrue(runDeviceTestsAsUser(INTENT_SENDER_PKG, ".CopyPasteTest",
                    "testCannotReadAcrossProfiles", userId));
        }
    }

    // TODO: This test is not specific to managed profiles, but applies to multi-user in general.
    // Move it to a MultiUserTest class when there is one. Should probably move
    // UserRestrictionActivity to a more generic apk too as it might be useful for different kinds
    // of tests (same applies to ComponentDisablingActivity).
    public void testNoDebuggingFeaturesRestriction() throws Exception {
        if (!mHasFeature) {
            return;
        }
        String restriction = "no_debugging_features";  // UserManager.DISALLOW_DEBUGGING_FEATURES
        String command = "add-restriction";

        String addRestrictionCommandOutput =
                changeUserRestrictionForUser(restriction, command, mUserId);
        assertTrue("Command was expected to succeed " + addRestrictionCommandOutput,
                addRestrictionCommandOutput.contains("Status: ok"));

        // This should now fail, as the shell is not available to start activities under a different
        // user once the restriction is in place.
        addRestrictionCommandOutput =
                changeUserRestrictionForUser(restriction, command, mUserId);
        assertTrue(
                "Expected SecurityException when starting the activity "
                        + addRestrictionCommandOutput,
                addRestrictionCommandOutput.contains("SecurityException"));
    }

    // Test the bluetooth API from a managed profile.
    public void testBluetooth() throws Exception {
        boolean mHasBluetooth = hasDeviceFeature(FEATURE_BLUETOOTH);
        if (!mHasFeature || !mHasBluetooth) {
            return ;
        }

        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".BluetoothTest",
                "testEnableDisable", mUserId));
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".BluetoothTest",
                "testGetAddress", mUserId));
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".BluetoothTest",
                "testListenUsingRfcommWithServiceRecord", mUserId));
        assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".BluetoothTest",
                "testGetRemoteDevice", mUserId));
    }

    public void testManagedContacts() throws Exception {
        if (!mHasFeature) {
            return;
        }

        try {
            // Insert Primary profile Contacts
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testPrimaryProfilePhoneLookup_insertedAndfound", 0));
            // Insert Managed profile Contacts
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testManagedProfilePhoneLookup_insertedAndfound", mUserId));

            // Set cross profile caller id to enabled
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testSetCrossProfileCallerIdDisabled_false", mUserId));

            // Managed user can use ENTERPRISE_CONTENT_FILTER_URI
            // To access managed contacts but not primary contacts
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testManagedProfilePhoneLookup_canAccessEnterpriseContact", mUserId));
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testManagedProfilePhoneLookup_canNotAccessPrimaryContact", mUserId));

            // Primary user can use ENTERPRISE_CONTENT_FILTER_URI
            // To access both primary and managed contacts
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testPrimaryProfileEnterprisePhoneLookup_canAccessEnterpriseContact", 0));
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testPrimaryProfilePhoneLookup_canAccessPrimaryContact", 0));

            // Set cross profile caller id to disabled
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testSetCrossProfileCallerIdDisabled_true", mUserId));

            // Primary user cannot use ENTERPRISE_CONTENT_FILTER_URI to access managed contacts
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testPrimaryProfilePhoneLookup_canNotAccessEnterpriseContact", 0));
            // Managed user cannot use ENTERPRISE_CONTENT_FILTER_URI to access primary contacts
            assertTrue(runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testManagedProfilePhoneLookup_canNotAccessPrimaryContact", mUserId));
        } finally {
            // Clean up in managed profile and primary profile
            runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testCurrentProfileContacts_removeContacts", mUserId);
            runDeviceTestsAsUser(MANAGED_PROFILE_PKG, ".ContactsTest",
                    "testCurrentProfileContacts_removeContacts", 0);
        }
    }

    private void disableActivityForUser(String activityName, int userId)
            throws DeviceNotAvailableException {
        String command = "am start -W --user " + userId
                + " --es extra-package " + MANAGED_PROFILE_PKG
                + " --es extra-class-name " + MANAGED_PROFILE_PKG + "." + activityName
                + " " + MANAGED_PROFILE_PKG + "/.ComponentDisablingActivity ";
        CLog.logAndDisplay(LogLevel.INFO, "Output for command " + command + ": "
                + getDevice().executeShellCommand(command));
    }

    private String changeUserRestrictionForUser(String key, String command, int userId)
            throws DeviceNotAvailableException {
        String adbCommand = "am start -W --user " + userId
                + " -c android.intent.category.DEFAULT "
                + " --es extra-command " + command
                + " --es extra-restriction-key " + key
                + " " + MANAGED_PROFILE_PKG + "/.UserRestrictionActivity";
        String commandOutput = getDevice().executeShellCommand(adbCommand);
        CLog.logAndDisplay(LogLevel.INFO,
                "Output for command " + adbCommand + ": " + commandOutput);
        return commandOutput;
    }
}
