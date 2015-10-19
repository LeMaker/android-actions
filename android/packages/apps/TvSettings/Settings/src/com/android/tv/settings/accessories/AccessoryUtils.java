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

package com.android.tv.settings.accessories;

import com.android.tv.settings.R;

import android.bluetooth.BluetoothDevice;
import com.android.tv.settings.accessories.InputDeviceCriteria;

/*
 * Provide utiltilties for Remote & Accessories.
 */
public class AccessoryUtils {
    public static int getImageIdForDevice(BluetoothDevice dev) {
        int devClass = dev.getBluetoothClass().getDeviceClass();

        if ((devClass & InputDeviceCriteria.MINOR_DEVICE_CLASS_POINTING) != 0) {
            return R.drawable.ic_settings_mouse;
        } else if ((devClass & InputDeviceCriteria.MINOR_DEVICE_CLASS_JOYSTICK) != 0) {
            return R.drawable.ic_settings_gamepad;
        } else if ((devClass & InputDeviceCriteria.MINOR_DEVICE_CLASS_GAMEPAD) != 0) {
            return R.drawable.ic_settings_gamepad;
        } else if ((devClass & InputDeviceCriteria.MINOR_DEVICE_CLASS_KEYBOARD) != 0) {
            return R.drawable.ic_settings_keyboard;
        }

        // Default for now
        return R.drawable.ic_settings_bt_misc_hid;
    }

    private AccessoryUtils() {
        // do not allow instantiation
    }
}
