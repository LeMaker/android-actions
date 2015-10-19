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

import android.app.admin.DevicePolicyManager;

/**
 * Tests for {@link DevicePolicyManager#setScreenCaptureDisabled} and
 * {@link DevicePolicyManager#getScreenCaptureDisabled} APIs.
 */
public class ScreenCaptureDisabledTest extends BaseDeviceOwnerTest {

    public void testSetScreenCaptureDisabled_false() throws Exception {
        mDevicePolicyManager.setScreenCaptureDisabled(getWho(), false);
        assertFalse(mDevicePolicyManager.getScreenCaptureDisabled(getWho()));
    }

    public void testSetScreenCaptureDisabled_true() throws Exception {
        mDevicePolicyManager.setScreenCaptureDisabled(getWho(), true);
        assertTrue(mDevicePolicyManager.getScreenCaptureDisabled(getWho()));
    }

    public void testSetScreenCaptureDisabled_anyAdminTrue() {
        mDevicePolicyManager.setScreenCaptureDisabled(getWho(), true);
        assertTrue(mDevicePolicyManager.getScreenCaptureDisabled(null /* any admin */));
    }

    public void testSetScreenCaptureDisabled_anyAdminFalse() {
        mDevicePolicyManager.setScreenCaptureDisabled(getWho(), false);
        assertFalse(mDevicePolicyManager.getScreenCaptureDisabled(null /* any admin */));
    }
}
