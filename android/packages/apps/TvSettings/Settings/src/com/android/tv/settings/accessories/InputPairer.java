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

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothInputDevice;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.input.InputManager;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.util.Log;
import android.view.InputDevice;

import com.android.tv.settings.util.bluetooth.BluetoothScanner;
import com.android.tv.settings.R;

import java.util.ArrayList;
import java.util.List;

/**
 * Monitors available Bluetooth input devices and manages process of pairing
 * and connecting to the device.
 */
public class InputPairer {

    /**
     * This class operates in two modes, automatic and manual.
     *
     * AUTO MODE
     * In auto mode we listen for an input device that looks like it can
     * generate DPAD events. When one is found we wait
     * {@link #DELAY_AUTO_PAIRING} milliseconds before starting the process of
     * connecting to the device. The idea is that a UI making use of this class
     * would give the user a chance to cancel pairing during this window. Once
     * the connection process starts, it is considered uninterruptible.
     *
     * Connection is accomplished in two phases, bonding and socket connection.
     * First we try to create a bond to the device and listen for bond status
     * change broadcasts. Once the bond is made, we connect to the device.
     * Connecting to the device actually opens a socket and hooks the device up
     * to the input system.
     *
     * In auto mode if we see more than one compatible input device before
     * bonding with a candidate device, we stop the process. We don't want to
     * connect to the wrong device and it is up to the user of this class to
     * tell us what to connect to.
     *
     * MANUAL MODE
     * Manual mode is where a user of this class explicitly tells us which
     * device to connect to. To switch to manual mode you can call
     * {@link #cancelPairing()}. It is safe to call this method even if no
     * device connection process is underway. You would then call
     * {@link #start()} to resume scanning for devices. Once one is found
     * that you want to connect to, call {@link #startPairing(BluetoothDevice)}
     * to start the connection process. At this point the same process is
     * followed as when we start connection in auto mode.
     *
     * Even in manual mode there is a timeout before we actually start
     * connecting, but it is {@link #DELAY_MANUAL_PAIRING}.
     */

    public static final String TAG = "aah.InputPairer";
    public static final int STATUS_ERROR = -1;
    public static final int STATUS_NONE = 0;
    public static final int STATUS_SCANNING = 1;
    /**
     * A device to pair with has been identified, we're currently in the
     * timeout period where the process can be cancelled.
     */
    public static final int STATUS_WAITING_TO_PAIR = 2;
    /**
     * Pairing is in progress.
     */
    public static final int STATUS_PAIRING = 3;
    /**
     * Device has been paired with, we are opening a connection to the device.
     */
    public static final int STATUS_CONNECTING = 4;


    public interface EventListener {
        /**
         * The status of the {@link InputPairer} changed.
         */
        public void statusChanged();
    }

    /**
     * Time between when a single input device is found and pairing begins. If
     * one or more other input devices are found before this timeout or
     * {@link #cancelPairing()} is called then pairing will not proceed.
     */
    public static final int DELAY_AUTO_PAIRING = 15 * 1000;
    /**
     * Time between when the call to {@link #startPairing(BluetoothDevice)} is
     * called and when we actually start pairing. This gives the caller a
     * chance to change their mind.
     */
    public static final int DELAY_MANUAL_PAIRING = 5 * 1000;
    /**
     * If there was an error in pairing, we will wait this long before trying
     * again.
     */
    public static final int DELAY_RETRY = 5 * 1000;

    private static final int MSG_PAIR = 1;
    private static final int MSG_START = 2;

    private static final boolean DEBUG = true;

    private static final String[] INVALID_INPUT_KEYBOARD_DEVICE_NAMES = {
        "gpio-keypad", "cec_keyboard", "Virtual", "athome_remote"
    };

    private BluetoothScanner.Listener mBtListener = new BluetoothScanner.Listener() {
        @Override
        public void onDeviceAdded(BluetoothScanner.Device device) {
            if (DEBUG) {
                Log.d(TAG, "Adding device: " + device.btDevice.getAddress());
            }
            onDeviceFound(device.btDevice);
        }

        @Override
        public void onDeviceRemoved(BluetoothScanner.Device device) {
            if (DEBUG) {
                Log.d(TAG, "Device lost: " + device.btDevice.getAddress());
            }
            onDeviceLost(device.btDevice);
        }
    };

    public static boolean hasValidInputDevice(Context context, int[] deviceIds) {
        InputManager inMan = (InputManager) context.getSystemService(Context.INPUT_SERVICE);

        for (int ptr = deviceIds.length - 1; ptr > -1; ptr--) {
            InputDevice device = inMan.getInputDevice(deviceIds[ptr]);
            int sources = device.getSources();

            boolean isCompatible = false;

            if ((sources & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD) {
                isCompatible = true;
            }

            if ((sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) {
                isCompatible = true;
            }

            if ((sources & InputDevice.SOURCE_KEYBOARD) == InputDevice.SOURCE_KEYBOARD) {
                boolean isValidKeyboard = true;
                String keyboardName = device.getName();
                for (int index = 0; index < INVALID_INPUT_KEYBOARD_DEVICE_NAMES.length; ++index) {
                    if (keyboardName.equals(INVALID_INPUT_KEYBOARD_DEVICE_NAMES[index])) {
                        isValidKeyboard = false;
                        break;
                    }
                }

                if (isValidKeyboard) {
                    isCompatible = true;
                }
            }

            if (!device.isVirtual() && isCompatible) {
                return true;
            }
        }
        return false;
    }

    public static boolean hasValidInputDevice(Context context) {
        InputManager inMan = (InputManager) context.getSystemService(Context.INPUT_SERVICE);
        int[] inputDevices = inMan.getInputDeviceIds();

        return hasValidInputDevice(context, inputDevices);
    }

    private BroadcastReceiver mLinkStatusReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            BluetoothDevice device = (BluetoothDevice) intent.getParcelableExtra(
                    BluetoothDevice.EXTRA_DEVICE);
            if (DEBUG) {
                Log.d(TAG, "There was a link status change for: " + device.getAddress());
            }

            if (device.equals(mTarget)) {
                int bondState = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE,
                        BluetoothDevice.BOND_NONE);
                int previousBondState = intent.getIntExtra(
                        BluetoothDevice.EXTRA_PREVIOUS_BOND_STATE, BluetoothDevice.BOND_NONE);

                if (DEBUG) {
                    Log.d(TAG, "Bond states: old = " + previousBondState + ", new = " +
                        bondState);
                }

                if (bondState == BluetoothDevice.BOND_NONE &&
                        previousBondState == BluetoothDevice.BOND_BONDING) {
                    // we seem to have reverted, this is an error
                    // TODO inform user, start scanning again
                    unregisterLinkStatusReceiver();
                    onBondFailed();
                } else if (bondState == BluetoothDevice.BOND_BONDED) {
                    unregisterLinkStatusReceiver();
                    onBonded();
                }
            }
        }
    };

    private BluetoothProfile.ServiceListener mServiceConnection =
            new BluetoothProfile.ServiceListener() {

        @Override
        public void onServiceDisconnected(int profile) {
            // TODO handle unexpected disconnection
            Log.w(TAG, "Service disconected, perhaps unexpectedly");
        }

        @Override
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            if (DEBUG) {
                Log.d(TAG, "Connection made to bluetooth proxy.");
            }
            mInputProxy = (BluetoothInputDevice) proxy;
            if (mTarget != null) {
                registerInputMethodMonitor();
                if (DEBUG) {
                    Log.d(TAG, "Connecting to target: " + mTarget.getAddress());
                }
                // TODO need to start a timer, otherwise if the connection fails we might be
                // stuck here forever
                mInputProxy.connect(mTarget);

                // must set PRIORITY_AUTO_CONNECT or auto-connection will not
                // occur, however this setting does not appear to be sticky
                // across a reboot
                mInputProxy.setPriority(mTarget, BluetoothProfile.PRIORITY_AUTO_CONNECT);
            }
        }
    };

    private InputManager.InputDeviceListener mInputListener =
            new InputManager.InputDeviceListener() {
        @Override
        public void onInputDeviceRemoved(int deviceId) {
            // ignored
        }

        @Override
        public void onInputDeviceChanged(int deviceId) {
            // ignored
        }

        @Override
        public void onInputDeviceAdded(int deviceId) {
           if (hasValidInputDevice(mContext, new int[] {deviceId})) {
               onInputAdded();
           }
        }
    };

    private Runnable mStartRunnable = new Runnable() {
        @Override
        public void run() {
            start();
        }
    };

    private Context mContext;
    private EventListener mListener;
    private int mStatus = STATUS_NONE;
    /**
     * Set to {@code false} when {@link #cancelPairing()} or
     * {@link #startPairing(BluetoothDevice)} or
     * {@link #startPairing(BluetoothDevice, int)} is called. This instance
     * will now no longer automatically start pairing.
     */
    private boolean mAutoMode = true;
    private ArrayList<BluetoothDevice> mVisibleDevices = new ArrayList<BluetoothDevice>();
    private BluetoothDevice mTarget;
    private Handler mHandler;
    private BluetoothInputDevice mInputProxy;
    private long mNextStageTimestamp = -1;
    private boolean mLinkReceiverRegistered = false;

    /**
     * Should be instantiated on a thread with a Looper, perhaps the main thread!
     */
    public InputPairer(Context context, EventListener listener) {
        mContext = context.getApplicationContext();
        mListener = listener;
        mHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MSG_PAIR:
                        startBonding();
                        break;
                    case MSG_START:
                        start();
                        break;
                    default:
                        Log.d(TAG, "No handler case available for message: " + msg.what);
                }
            }
        };
    }

    /**
     * Start listening for devices and begin the pairing process when
     * criteria is met.
     */
    public void start() {
        // TODO instead of this, register a broadcast receiver to listen to
        // Bluetooth state
        if (!BluetoothAdapter.getDefaultAdapter().isEnabled()) {
            Log.d(TAG, "Bluetooth not enabled, delaying startup.");
            mHandler.removeCallbacks(mStartRunnable);
            mHandler.postDelayed(mStartRunnable, 1000);
            return;
        }

        // set status to scanning before we start listening since
        // startListening may result in a transition to STATUS_WAITING_TO_PAIR
        // which might seem odd from a client perspective
        setStatus(STATUS_SCANNING);

        BluetoothScanner.stopListening(mBtListener);
        BluetoothScanner.startListening(mContext, mBtListener, new InputDeviceCriteria());
    }

    public void clearDeviceList() {
        doCancel();
        mVisibleDevices.clear();
    }

    /**
     * Stop any pairing request that is in progress.
     */
    public void cancelPairing() {
        mAutoMode = false;
        doCancel();
    }

    /**
     * Stop doing anything we're doing, release any resources.
     */
    public void dispose() {
        mHandler.removeCallbacksAndMessages(null);
        if (mLinkReceiverRegistered) {
            unregisterLinkStatusReceiver();
        }
        stopScanning();
    }

    /**
     * Start pairing and connection to the specified device.
     * @param device
     */
    public void startPairing(BluetoothDevice device) {
        startPairing(device, DELAY_MANUAL_PAIRING);
    }

    /**
     * See {@link #startPairing(BluetoothDevice)}.
     * @param delay The delay before pairing starts. In this window, cancel may
     * be called.
     */
    public void startPairing(BluetoothDevice device, int delay) {
        startPairing(device, delay, true);
    }

    /**
     * Return our state
     * @return One of the STATE_ constants.
     */
    public int getStatus() {
        return mStatus;
    }

    /**
     * Get the device that we're currently targeting. This will be null if
     * there is no device that is in the process of being connected to.
     */
    public BluetoothDevice getTargetDevice() {
        return mTarget;
    }

    /**
     * When the timer to start the next stage will expire, in {@link SystemClock#elapsedRealtime()}.
     * Will only be valid while waiting to pair and after an error from which we are restarting.
     */
    public long getNextStageTime() {
        return mNextStageTimestamp;
    }

    public List<BluetoothDevice> getAvailableDevices() {
        ArrayList<BluetoothDevice> copy = new ArrayList<BluetoothDevice>(mVisibleDevices.size());
        copy.addAll(mVisibleDevices);
        return copy;
    }

    public void setListener(EventListener listener) {
        mListener = listener;
    }

    public void invalidateDevice(BluetoothDevice device) {
        onDeviceLost(device);
    }

    private void startPairing(BluetoothDevice device, int delay, boolean isManual) {
        // TODO check if we're already paired/bonded to this device

        // cancel auto-mode if applicable
        mAutoMode = !isManual;

        mTarget = device;

        if (isInProgress()) {
            throw new RuntimeException("Pairing already in progress, you must cancel the " +
                    "previous request first");
        }

        mHandler.removeMessages(MSG_PAIR);
        mHandler.removeMessages(MSG_START);

        mNextStageTimestamp = SystemClock.elapsedRealtime() +
                (mAutoMode ? DELAY_AUTO_PAIRING : DELAY_MANUAL_PAIRING);
        mHandler.sendEmptyMessageDelayed(MSG_PAIR,
                mAutoMode ? DELAY_AUTO_PAIRING : DELAY_MANUAL_PAIRING);

        setStatus(STATUS_WAITING_TO_PAIR);
    }

    /**
     * Pairing is in progress and is no longer cancelable.
     */
    public boolean isInProgress() {
        return mStatus != STATUS_NONE && mStatus != STATUS_ERROR && mStatus != STATUS_SCANNING &&
                mStatus != STATUS_WAITING_TO_PAIR;
    }

    private void updateListener() {
        if (mListener != null) {
            mListener.statusChanged();
        }
    }

    private void onDeviceFound(BluetoothDevice device) {
        if (!mVisibleDevices.contains(device)) {
            mVisibleDevices.add(device);
            Log.d(TAG, "Added device to visible list. Name = " + device.getName() + " , class = " +
                    device.getBluetoothClass().getDeviceClass());
        } else {
            return;
        }

        updatePairingState();
        // update the listener because a new device is visible
        updateListener();
    }

    private void onDeviceLost(BluetoothDevice device) {
        // TODO validate removal works as expected
        if (mVisibleDevices.remove(device)) {
            updatePairingState();
            // update the listener because a device disappeared
            updateListener();
        }
    }

    private void updatePairingState() {
        if (mAutoMode) {
            if (isReadyToAutoPair()) {
                mTarget = mVisibleDevices.get(0);
                startPairing(mTarget, DELAY_AUTO_PAIRING, false);
            } else {
                doCancel();
            }
        }
    }

    /**
     * @return {@code true} If there is only one visible input device.
     */
    private boolean isReadyToAutoPair() {
        // we imagine that the conditions under which we decide to pair or
        // not may one day become more complicated, which is why this length
        // check is wrapped in a method call.
        return mVisibleDevices.size() == 1;
    }

    private void doCancel() {
        // TODO allow cancel to be called from any state
        if (isInProgress()) {
            Log.d(TAG, "Pairing process has already begun, it can not be canceled.");
            return;
        }

        // stop scanning, just in case we are
        BluetoothScanner.stopListening(mBtListener);
        BluetoothScanner.stopNow();

        // remove any callbacks
        mHandler.removeMessages(MSG_PAIR);

        // remove bond, if existing
        unpairDevice(mTarget);

        mTarget = null;

        setStatus(STATUS_NONE);

        // resume scanning
        start();
    }

    /**
     * Set the status and update any listener.
     */
    private void setStatus(int status) {
        mStatus = status;
        updateListener();
    }

    private void startBonding() {
        stopScanning();
        setStatus(STATUS_PAIRING);
        if (mTarget.getBondState() != BluetoothDevice.BOND_BONDED) {
            registerLinkStatusReceiver();

            // create bond (pair) to the device
            mTarget.createBond();
        } else {
            onBonded();
        }
    }

    private void onBonded() {
        openConnection();
    }

    private void openConnection() {
        setStatus(STATUS_CONNECTING);

        BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();

        // connect to the Bluetooth service, then registerInputListener
        adapter.getProfileProxy(mContext, mServiceConnection, BluetoothProfile.INPUT_DEVICE);
    }

    private void onInputAdded() {
        unregisterInputMethodMonitor();
        BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
        adapter.closeProfileProxy(BluetoothProfile.INPUT_DEVICE, mInputProxy);
        setStatus(STATUS_NONE);
    }

    private void onBondFailed() {
        Log.w(TAG, "There was an error bonding with the device.");
        setStatus(STATUS_ERROR);

        // remove bond, if existing
        unpairDevice(mTarget);

        // TODO do we need to check Bluetooth for the device and possible delete it?
        mNextStageTimestamp = SystemClock.elapsedRealtime() + DELAY_RETRY;
        mHandler.sendEmptyMessageDelayed(MSG_START, DELAY_RETRY);
    }

    private void registerInputMethodMonitor() {
        InputManager inputManager = (InputManager) mContext.getSystemService(Context.INPUT_SERVICE);
        inputManager.registerInputDeviceListener(mInputListener, mHandler);

        // TO DO: The line below is a workaround for an issue in InputManager.
        // The manager doesn't actually registers itself with the InputService
        // unless we query it for input devices. We should remove this once
        // the problem is fixed in InputManager.
        // Reference bug in Frameworks: b/10415556
        int[] inputDevices = inputManager.getInputDeviceIds();
    }

    private void unregisterInputMethodMonitor() {
        InputManager inputManager = (InputManager) mContext.getSystemService(Context.INPUT_SERVICE);
        inputManager.unregisterInputDeviceListener(mInputListener);
    }

    private void registerLinkStatusReceiver() {
        mLinkReceiverRegistered = true;
        IntentFilter filter = new IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        mContext.registerReceiver(mLinkStatusReceiver, filter);
    }

    private void unregisterLinkStatusReceiver() {
        mLinkReceiverRegistered = false;
        mContext.unregisterReceiver(mLinkStatusReceiver);
    }

    private void stopScanning() {
        BluetoothScanner.stopListening(mBtListener);
        BluetoothScanner.stopNow();
    }

    public boolean unpairDevice(BluetoothDevice device) {
        if (device != null) {
            int state = device.getBondState();

            if (state == BluetoothDevice.BOND_BONDING) {
                device.cancelBondProcess();
            }

            if (state != BluetoothDevice.BOND_NONE) {
                final boolean successful = device.removeBond();
                if (successful) {
                    if (DEBUG) {
                        Log.d(TAG, "Bluetooth device successfully unpaired: " + device.getName());
                    }
                    return true;
                } else {
                    Log.e(TAG, "Failed to unpair Bluetooth Device: " + device.getName());
                }
            }
        }
        return false;
    }
}
