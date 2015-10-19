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

import static android.provider.Settings.Secure.TTS_DEFAULT_RATE;

import com.android.tv.settings.ActionBehavior;
import com.android.tv.settings.ActionKey;
import com.android.tv.settings.BaseSettingsActivity;
import com.android.tv.settings.R;
import com.android.tv.settings.dialog.old.Action;
import com.android.tv.settings.dialog.old.ActionAdapter;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import java.util.Set;

public class BluetoothAccessoryActivity extends BaseSettingsActivity
        implements ActionAdapter.Listener {

    public static final String EXTRA_ACCESSORY_ADDRESS = "accessory_address";
    public static final String EXTRA_ACCESSORY_NAME = "accessory_name";
    public static final String EXTRA_ACCESSORY_ICON_ID = "accessory_icon_res";

    private static final int MSG_UNPAIR_TIMEOUT = 1;

    private static final int UNPAIR_TIMEOUT = 5000;

    private static final String TAG = "aah.BluetoothAccessoryActivity";
    private static final boolean DEBUG = false;

    private BluetoothDevice mDevice;
    protected String mDeviceAddress;
    protected String mDeviceName;
    protected int mDeviceImgId;
    protected boolean mDone;

    public static Intent getIntent(Context context, String deviceAddress,
            String deviceName, int iconId) {
        Intent i = new Intent(context, BluetoothAccessoryActivity.class);
        i.putExtra(EXTRA_ACCESSORY_ADDRESS, deviceAddress);
        i.putExtra(EXTRA_ACCESSORY_NAME, deviceName);
        i.putExtra(EXTRA_ACCESSORY_ICON_ID, iconId);
        return i;
    }

    // Broadcast Receiver for Bluetooth related events
    private final BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
            @Override
        public void onReceive(Context context, Intent intent) {
            BluetoothDevice device = intent
                    .getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
            if (mDone) {
                if (mDevice.equals(device)) {
                    // Done removing device, finish the activity
                    mMsgHandler.removeMessages(MSG_UNPAIR_TIMEOUT);
                    finish();
                }
            }
        }
    };

    // Internal message handler
    private Handler mMsgHandler = new Handler() {
            @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_UNPAIR_TIMEOUT:
                    finish();
                    break;
                default:
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Bundle bundle = getIntent().getExtras();
        if (bundle != null) {
            mDeviceAddress = bundle.getString(EXTRA_ACCESSORY_ADDRESS);
            mDeviceName = bundle.getString(EXTRA_ACCESSORY_NAME);
            mDeviceImgId = bundle.getInt(EXTRA_ACCESSORY_ICON_ID);
        } else {
            mDeviceName = getString(R.string.accessory_options);
            mDeviceImgId = R.drawable.ic_qs_bluetooth_not_connected;
        }

        super.onCreate(savedInstanceState);

        mDone = false;

        BluetoothAdapter btAdapter = BluetoothAdapter.getDefaultAdapter();
        if (btAdapter != null) {
            Set<BluetoothDevice> bondedDevices = btAdapter.getBondedDevices();
            for (BluetoothDevice device : bondedDevices) {
                if (mDeviceAddress.equals(device.getAddress())) {
                    mDevice = device;
                    break;
                }
            }
        }
    }

    @Override
    public void onResume() {
        // Set a broadcast receiver to let us know when the device has been removed
        IntentFilter adapterIntentFilter = new IntentFilter();
        adapterIntentFilter.addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        registerReceiver(mBroadcastReceiver, adapterIntentFilter);
        super.onResume();
    }

    @Override
    public void onPause() {
        unregisterReceiver(mBroadcastReceiver);
        super.onPause();
    }

    @Override
    public void onActionClicked(Action action) {
        if (mDone) {
            return;
        }
        /*
         * For regular states
         */
        ActionKey<ActionType, ActionBehavior> actionKey = new ActionKey<ActionType, ActionBehavior>(
                ActionType.class, ActionBehavior.class, action.getKey());
        final ActionType type = actionKey.getType();
        if (type != null) {
            switch (type) {
                case BLUETOOTH_DEVICE_RENAME:
                    // TODO: Will be implemented in a separate CL
                    return;
                case OK:
                    unpairDevice();
                    return;
                case CANCEL:
                    goBack(); // Cancelled request to STOP service
                    return;
                default:
            }
        }
        setState(type, true);
    }

    @Override
    protected Object getInitialState() {
        return ActionType.BLUETOOTH_DEVICE_OVERVIEW;
    }

    @Override
    protected void goBack() {
        if (!mDone) {
            super.goBack();
        }
    }

    @Override
    protected void refreshActionList() {
        mActions.clear();
        switch ((ActionType) mState) {
            case BLUETOOTH_DEVICE_OVERVIEW:
                // Disabled for now, until the name input screen is implemented
                // mActions.add(ActionType.BLUETOOTH_DEVICE_RENAME.toAction(mResources));
                mActions.add(ActionType.BLUETOOTH_DEVICE_UNPAIR.toAction(mResources));
                break;
            case BLUETOOTH_DEVICE_UNPAIR:
                mActions.add(ActionType.OK.toAction(mResources));
                mActions.add(ActionType.CANCEL.toAction(mResources));
                break;
            default:
                break;
        }
    }

    @Override
    protected void updateView() {
        refreshActionList();
        switch ((ActionType) mState) {
            case BLUETOOTH_DEVICE_OVERVIEW:
                setView(mDeviceName, getString(R.string.header_category_accessories), null,
                        mDeviceImgId);
                break;
            case BLUETOOTH_DEVICE_UNPAIR:
                setView(getString(R.string.accessory_unpair), mDeviceName, null, mDeviceImgId);
                break;
            default:
                break;
        }
    }

    void unpairDevice() {
        if (mDevice != null) {
            int state = mDevice.getBondState();

            if (state == BluetoothDevice.BOND_BONDING) {
                mDevice.cancelBondProcess();
            }

            if (state != BluetoothDevice.BOND_NONE) {
                mDone = true;
                // Set a timeout, just in case we don't receive the unpair notification we
                // use to finish the activity
                mMsgHandler.sendEmptyMessageDelayed(MSG_UNPAIR_TIMEOUT, UNPAIR_TIMEOUT);
                final boolean successful = mDevice.removeBond();
                if (successful) {
                    if (DEBUG) {
                        Log.d(TAG, "Bluetooth device successfully unpaired.");
                    }
                    // set the dialog to a waiting state
                    mActions.clear();
                    setView(getString(R.string.accessory_unpair), mDeviceName,
                            getString(R.string.accessory_unpairing), mDeviceImgId);
                } else {
                    Log.e(TAG, "Failed to unpair Bluetooth Device: " + mDevice.getName());
                }
            }
        } else {
            Log.e(TAG, "Bluetooth device not found. Address = " + mDeviceAddress);
        }
    }

    @Override
    protected void setProperty(boolean enable) {
    }
}
