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

import android.view.WindowManager;
import com.android.tv.settings.R;
import com.android.tv.settings.dialog.old.Action;
import com.android.tv.settings.dialog.old.ActionAdapter;
import com.android.tv.settings.dialog.old.ActionFragment;
import com.android.tv.settings.dialog.old.DialogActivity;

import android.app.Fragment;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.Html;
import android.text.InputFilter;
import android.text.InputType;
import android.text.InputFilter.LengthFilter;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.android.tv.settings.util.AccessibilityHelper;

import java.util.ArrayList;
import java.util.Locale;

/**
 * BluetoothPairingDialog asks the user to enter a PIN / Passkey / simple
 * confirmation for pairing with a remote Bluetooth device.
 */
public class BluetoothPairingDialog extends DialogActivity {

    private static final String KEY_PAIR = "action_pair";
    private static final String KEY_CANCEL = "action_cancel";

    private static final String TAG = "aah.BluetoothPairingDialog";
    private static final boolean DEBUG = false;

    private static final int BLUETOOTH_PIN_MAX_LENGTH = 16;
    private static final int BLUETOOTH_PASSKEY_MAX_LENGTH = 6;

    private BluetoothDevice mDevice;
    private int mType;
    private String mPairingKey;

    private ActionFragment mActionFragment;
    private Fragment mContentFragment;
    private ArrayList<Action> mActions;

    private RelativeLayout mTopLayout;
    protected ColorDrawable mBgDrawable = new ColorDrawable();
    private TextView mTitleText;
    private TextView mInstructionText;
    private EditText mTextInput;


    /**
     * Dismiss the dialog if the bond state changes to bonded or none, or if
     * pairing was canceled for {@link #mDevice}.
     */
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (DEBUG) {
                Log.d(TAG, "onReceive. Broadcast Intent = " + intent.toString());
            }
            if (BluetoothDevice.ACTION_BOND_STATE_CHANGED.equals(action)) {
                int bondState = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE,
                        BluetoothDevice.ERROR);
                if (bondState == BluetoothDevice.BOND_BONDED ||
                        bondState == BluetoothDevice.BOND_NONE) {
                    dismiss();
                }
            } else if (BluetoothDevice.ACTION_PAIRING_CANCEL.equals(action)) {
                BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                if (device == null || device.equals(mDevice)) {
                    dismiss();
                }
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent intent = getIntent();
        if (!BluetoothDevice.ACTION_PAIRING_REQUEST.equals(intent.getAction())) {
            Log.e(TAG, "Error: this activity may be started only with intent " +
                    BluetoothDevice.ACTION_PAIRING_REQUEST);
            finish();
            return;
        }

        mDevice = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
        mType = intent.getIntExtra(BluetoothDevice.EXTRA_PAIRING_VARIANT, BluetoothDevice.ERROR);

        if (DEBUG) {
            Log.d(TAG, "Requested pairing Type = " + mType + " , Device = " + mDevice);
        }

        mActions = new ArrayList<Action>();

        switch (mType) {
            case BluetoothDevice.PAIRING_VARIANT_PIN:
            case BluetoothDevice.PAIRING_VARIANT_PASSKEY:
                createUserEntryDialog();
                break;

            case BluetoothDevice.PAIRING_VARIANT_PASSKEY_CONFIRMATION:
                int passkey =
                    intent.getIntExtra(BluetoothDevice.EXTRA_PAIRING_KEY, BluetoothDevice.ERROR);
                if (passkey == BluetoothDevice.ERROR) {
                    Log.e(TAG, "Invalid Confirmation Passkey received, not showing any dialog");
                    finish();
                    return;
                }
                mPairingKey = String.format(Locale.US, "%06d", passkey);
                createConfirmationDialog();
                break;

            case BluetoothDevice.PAIRING_VARIANT_CONSENT:
            case BluetoothDevice.PAIRING_VARIANT_OOB_CONSENT:
                createConfirmationDialog();
                break;

            case BluetoothDevice.PAIRING_VARIANT_DISPLAY_PASSKEY:
            case BluetoothDevice.PAIRING_VARIANT_DISPLAY_PIN:
                int pairingKey =
                    intent.getIntExtra(BluetoothDevice.EXTRA_PAIRING_KEY, BluetoothDevice.ERROR);
                if (pairingKey == BluetoothDevice.ERROR) {
                    Log.e(TAG,
                            "Invalid Confirmation Passkey or PIN received, not showing any dialog");
                    finish();
                    return;
                }
                if (mType == BluetoothDevice.PAIRING_VARIANT_DISPLAY_PASSKEY) {
                    mPairingKey = String.format("%06d", pairingKey);
                } else {
                    mPairingKey = String.format("%04d", pairingKey);
                }
                createConfirmationDialog();
                break;

            default:
                Log.e(TAG, "Incorrect pairing type received, not showing any dialog");
                finish();
                return;
        }

        ViewGroup contentView = (ViewGroup) findViewById(android.R.id.content);
        mTopLayout = (RelativeLayout) contentView.getChildAt(0);

        // Fade out the old activity, and fade in the new activity.
        overridePendingTransition(R.anim.fade_in, R.anim.fade_out);

        // Set the activity background
        int bgColor = getResources().getColor(R.color.dialog_activity_background);
        mBgDrawable.setColor(bgColor);
        mBgDrawable.setAlpha(255);
        mTopLayout.setBackground(mBgDrawable);

        // Make sure pairing wakes up day dream
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD |
                WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED |
                WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON |
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    @Override
    protected void onResume() {
        super.onResume();

        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_PAIRING_CANCEL);
        filter.addAction(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        registerReceiver(mReceiver, filter);
    }

    @Override
    protected void onPause() {
        unregisterReceiver(mReceiver);

        // Finish the activity if we get placed in the background and cancel pairing
        cancelPairing();
        dismiss();

        super.onPause();
    }

    @Override
    public void onActionClicked(Action action) {
        String key = action.getKey();
        if (KEY_PAIR.equals(key)) {
            onPair(null);
            dismiss();
        } else if (KEY_CANCEL.equals(key)) {
            cancelPairing();
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            cancelPairing();
        }
        return super.onKeyDown(keyCode, event);
    }

    private ArrayList<Action> getActions() {
        ArrayList<Action> actions = new ArrayList<Action>();

        switch (mType) {
            case BluetoothDevice.PAIRING_VARIANT_PASSKEY_CONFIRMATION:
            case BluetoothDevice.PAIRING_VARIANT_CONSENT:
            case BluetoothDevice.PAIRING_VARIANT_OOB_CONSENT:
                actions.add(new Action.Builder()
                        .key(KEY_PAIR)
                        .title(getString(R.string.bluetooth_pair))
                        .build());

                actions.add(new Action.Builder()
                        .key(KEY_CANCEL)
                        .title(getString(R.string.bluetooth_cancel))
                        .build());
                break;
            case BluetoothDevice.PAIRING_VARIANT_DISPLAY_PIN:
            case BluetoothDevice.PAIRING_VARIANT_DISPLAY_PASSKEY:
                actions.add(new Action.Builder()
                        .key(KEY_CANCEL)
                        .title(getString(R.string.bluetooth_cancel))
                        .build());
                break;
        }

        return actions;
    }

    private void dismiss() {
        finish();
    }

    private void cancelPairing() {
        if (DEBUG) {
            Log.d(TAG, "cancelPairing");
        }
        mDevice.cancelPairingUserInput();
    }

    private void createUserEntryDialog() {
        setContentView(R.layout.bt_pairing_passkey_entry);

        mTitleText = (TextView) findViewById(R.id.title_text);
        mTextInput = (EditText) findViewById(R.id.text_input);

        String instructions = getString(R.string.bluetooth_confirm_passkey_msg,
                mDevice.getName(), mPairingKey);
        int maxLength;
        switch (mType) {
            case BluetoothDevice.PAIRING_VARIANT_PIN:
                instructions = getString(R.string.bluetooth_enter_pin_msg, mDevice.getName());
                mInstructionText = (TextView) findViewById(R.id.hint_text);
                mInstructionText.setText(getString(R.string.bluetooth_pin_values_hint));
                // Maximum of 16 characters in a PIN
                maxLength = BLUETOOTH_PIN_MAX_LENGTH;
                mTextInput.setInputType(InputType.TYPE_CLASS_NUMBER);
                break;

            case BluetoothDevice.PAIRING_VARIANT_PASSKEY:
                instructions = getString(R.string.bluetooth_enter_passkey_msg, mDevice.getName());
                // Maximum of 6 digits for passkey
                maxLength = BLUETOOTH_PASSKEY_MAX_LENGTH;
                mTextInput.setInputType(InputType.TYPE_CLASS_TEXT);
                break;

            default:
                Log.e(TAG, "Incorrect pairing type for createPinEntryView: " + mType);
                dismiss();
                return;
        }

        mTitleText.setText(Html.fromHtml(instructions));

        mTextInput.setFilters(new InputFilter[] { new LengthFilter(maxLength) });
    }

    private void createConfirmationDialog() {
        // Build a Dialog activity view, with Action Fragment

        mActions = getActions();

        mActionFragment = ActionFragment.newInstance(mActions);
        mContentFragment = new Fragment() {
            @Override
            public View onCreateView(LayoutInflater inflater, ViewGroup container,
                    Bundle savedInstanceState) {
                View v = inflater.inflate(R.layout.bt_pairing_passkey_display, container, false);

                mTitleText = (TextView) v.findViewById(R.id.title);
                mInstructionText = (TextView) v.findViewById(R.id.pairing_instructions);

                mTitleText.setText(getString(R.string.bluetooth_pairing_request));

                if (AccessibilityHelper.forceFocusableViews(getActivity())) {
                    mTitleText.setFocusable(true);
                    mTitleText.setFocusableInTouchMode(true);
                    mInstructionText.setFocusable(true);
                    mInstructionText.setFocusableInTouchMode(true);
                }

                String instructions;

                switch (mType) {
                    case BluetoothDevice.PAIRING_VARIANT_DISPLAY_PASSKEY:
                    case BluetoothDevice.PAIRING_VARIANT_DISPLAY_PIN:
                        instructions = getString(R.string.bluetooth_display_passkey_pin_msg,
                                mDevice.getName(), mPairingKey);

                        // Since its only a notification, send an OK to the framework,
                        // indicating that the dialog has been displayed.
                        if (mType == BluetoothDevice.PAIRING_VARIANT_DISPLAY_PASSKEY) {
                            mDevice.setPairingConfirmation(true);
                        } else if (mType == BluetoothDevice.PAIRING_VARIANT_DISPLAY_PIN) {
                            byte[] pinBytes = BluetoothDevice.convertPinToBytes(mPairingKey);
                            mDevice.setPin(pinBytes);
                        }
                        break;

                    case BluetoothDevice.PAIRING_VARIANT_PASSKEY_CONFIRMATION:
                        instructions = getString(R.string.bluetooth_confirm_passkey_msg,
                                mDevice.getName(), mPairingKey);
                        break;

                    case BluetoothDevice.PAIRING_VARIANT_CONSENT:
                    case BluetoothDevice.PAIRING_VARIANT_OOB_CONSENT:
                        instructions = getString(R.string.bluetooth_incoming_pairing_msg,
                                mDevice.getName());

                        break;
                    default:
                        instructions = new String();
                }

                mInstructionText.setText(Html.fromHtml(instructions));

                return v;
            }
        };

        setContentAndActionFragments(mContentFragment, mActionFragment);
    }

    private void onPair(String value) {
        if (DEBUG) {
            Log.d(TAG, "onPair: " + value);
        }
        switch (mType) {
            case BluetoothDevice.PAIRING_VARIANT_PIN:
                byte[] pinBytes = BluetoothDevice.convertPinToBytes(value);
                if (pinBytes == null) {
                    return;
                }
                mDevice.setPin(pinBytes);
                break;

            case BluetoothDevice.PAIRING_VARIANT_PASSKEY:
                int passkey = Integer.parseInt(value);
                mDevice.setPasskey(passkey);
                break;

            case BluetoothDevice.PAIRING_VARIANT_PASSKEY_CONFIRMATION:
            case BluetoothDevice.PAIRING_VARIANT_CONSENT:
                mDevice.setPairingConfirmation(true);
                break;

            case BluetoothDevice.PAIRING_VARIANT_DISPLAY_PASSKEY:
            case BluetoothDevice.PAIRING_VARIANT_DISPLAY_PIN:
                // Do nothing.
                break;

            case BluetoothDevice.PAIRING_VARIANT_OOB_CONSENT:
                mDevice.setRemoteOutOfBandData();
                break;

            default:
                Log.e(TAG, "Incorrect pairing type received");
        }
    }

}
