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

package com.android.omadm.service;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.KeyguardManager;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.PowerManager;
import android.text.InputFilter;
import android.text.InputType;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.EditText;

import java.util.Arrays;

public class DMAlertActivity extends Activity {
    private static final String TAG = "DMAlertActivity";
    private static final boolean DBG = DMClientService.DBG;

    private static final int DISPLAY_ALERT_DIALOG = 1;

    private static final int CONFIRM_ALERT_DIALOG = 2;

    private static final int TEXTINPUT_ALERT_DIALOG = 3;

    private static final int SINGLECHOICE_ALERT_DIALOG = 4;

    private static final int MULTICHOICE_ALERT_DIALOG = 5;

    private static final int SHOW_PROGRESS_ALERT_DIALOG = 6;

    private static final int HIDE_PROGRESS_ALERT_DIALOG = 7;

    private static final int CANCEL_ALERT_DIALOG = 8;

    private static AlertDialog mDialog;

    private Runnable mDialogTimeout;

    private KeyReceiver mKeyReceiver;

    private static PowerManager.WakeLock wl;

    private KeyguardManager.KeyguardLock mKeyguardLock;

    private int mType;

    private String mMessage;

    private String mTitle;

    private int mIcon;

    private int mTimeout;

    private int mMaxLength;

    private EditText mInputBox;

    private int mInputType;

    private String[] mChoices;

    private int mCheckedItem;

    private boolean[] mCheckedItems;

    private int mResultCode; // == DMAlert.DM_SERVICE_ALERT_RESP_SUCCESS (0)

    private String mResultData;

    private Handler mHandler;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (DBG) {
            logd("onCreate");
        }

        final Intent intent = getIntent();
        mMessage = intent.getStringExtra("Message");
        mTitle = intent.getStringExtra("Title");
        mIcon = intent.getIntExtra("Icon", -1);
        mTimeout = intent.getIntExtra("Timeout", 0);
        if (DBG) {
            logd("mMessage: " + mMessage);
        }
        if (DBG) {
            logd("mTitle: " + mTitle);
        }
        if (DBG) {
            logd("mIcon: " + mIcon);
        }
        if (DBG) {
            logd("mTimeout: " + mTimeout);
        }

        String action = intent.getAction();
        if (action.equals(DMIntent.SHOW_DISPLAY_ALERT_DLG)) {
            mType = DISPLAY_ALERT_DIALOG;
        } else if (action.equals(DMIntent.SHOW_CONFIRM_ALERT_DLG)) {
            mType = CONFIRM_ALERT_DIALOG;
        } else if (action.equals(DMIntent.SHOW_TEXTINPUT_ALERT_DLG)) {
            mType = TEXTINPUT_ALERT_DIALOG;
            mMaxLength = intent.getIntExtra("MaxLength", 80);
            if (DBG) {
                logd("mMaxLength: " + mMaxLength);
            }
            mInputType = intent.getIntExtra("InputType", InputType.TYPE_CLASS_TEXT);
            if (DBG) {
                logd("mInputType: " + mInputType);
            }
            mResultData = intent.getStringExtra("DefaultText");
            if (DBG) {
                logd("mResultData: " + mResultData);
            }
        } else if (action.equals(DMIntent.SHOW_SINGLECHOICE_ALERT_DLG)) {
            mType = SINGLECHOICE_ALERT_DIALOG;
            mChoices = intent.getStringArrayExtra("Choices");
            mCheckedItem = intent.getIntExtra("CheckedItem", -1);
            for (int i = 0; i < mChoices.length; i++) {
                if (DMClientService.DBG) {
                    logd("mChoices: " + i + ": " + mChoices[i]);
                }
            }
            if (DMClientService.DBG) {
                logd("mCheckedItem: " + mCheckedItem);
            }
        } else if (action.equals(DMIntent.SHOW_MULTICHOICE_ALERT_DLG)) {
            mType = MULTICHOICE_ALERT_DIALOG;
            mChoices = intent.getStringArrayExtra("Choices");
            for (int i = 0; i < mChoices.length; i++) {
                if (DMClientService.DBG) {
                    logd("mChoices " + i + ": " + mChoices[i]);
                }
            }
            mCheckedItems = intent.getBooleanArrayExtra("CheckedItems");
            for (int i = 0; i < mCheckedItems.length; i++) {
                if (DMClientService.DBG) {
                    logd("CheckedItems " + i + ": " + mCheckedItems[i]);
                }
            }
        } else if (action.equals(DMIntent.SHOW_PROGRESS_ALERT_DLG)) {
            mType = SHOW_PROGRESS_ALERT_DIALOG;
        } else if (action.equals(DMIntent.HIDE_PROGRESS_ALERT_DLG)) {
            mType = HIDE_PROGRESS_ALERT_DIALOG;
            onFinish();
            if (DMClientService.DBG) {
                logd("HIDE_PROGRESS_ALERT_DLG");
            }
            return;
        } else if (action.equals(DMIntent.CANCEL_ALERT_DLG)) {
            mType = CANCEL_ALERT_DIALOG;
            onFinish();
            if (DMClientService.DBG) {
                logd("CANCEL_ALERT_DIALOG");
            }
            return;
        } else {
            if (DMClientService.DBG) {
                logd("TODO: new dialog type support!");
            }
            return;
        }

        if (null != mDialog) {
            if (DMClientService.DBG) {
                logd("Cancel previous dialog!");
            }
            mDialog.dismiss();
            mDialog = null;
        }

        setVisible(false);

        showDialog(mType);

        mKeyReceiver = new KeyReceiver();
        IntentFilter filter = new IntentFilter(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        IntentFilter searchkeyFilter = new IntentFilter(Intent.ACTION_SEARCH);
        IntentFilter searchkeyFilterLongPress = new IntentFilter(Intent.ACTION_SEARCH_LONG_PRESS);
        registerReceiver(mKeyReceiver, searchkeyFilter);
        registerReceiver(mKeyReceiver, searchkeyFilterLongPress);
        registerReceiver(mKeyReceiver, filter);

        if (mTimeout <= 0 || mTimeout > 300) {
            if (DMClientService.DBG) {
                logd("Forcing the default and max Timeout to 5mins");
            }
            mTimeout = 300; //forcing the default and max timeout to 5mins
            //return;
        }

        mHandler = new Handler();
        mDialogTimeout = new Runnable() {
            @Override
            public void run() {
                mResultCode = DMAlert.DM_SERVICE_ALERT_RESP_TIMEOUT;
                onFinish();
                if (DMClientService.DBG) {
                    logd("DMAlert.DM_SERVICE_ALERT_RESP_TIMEOUT");
                }
            }
        };
        mHandler.postDelayed(mDialogTimeout, mTimeout * 1000);
    }

    private final class KeyReceiver extends BroadcastReceiver {

        KeyReceiver() {
        }

        @Override
        public void onReceive(Context arg0, Intent arg1) {
            Log.i(TAG, "receive the broadcast " + arg1.getAction());
            mResultCode = DMAlert.DM_SERVICE_ALERT_RESP_CANCEL;
            onFinish();
        }
    }

    @Override
    protected AlertDialog onCreateDialog(int id) {
        if (DMClientService.DBG) {
            logd("Dialog Type: " + id);
        }
        getKeyGuardAndWakeLock();

        switch (id) {
            case DISPLAY_ALERT_DIALOG:
                mDialog = new AlertDialog.Builder(this)
                        .setTitle(mTitle)
                        .setMessage(mMessage)
                        .setCancelable(true)
                        .setView(null)
                        .create();
                break;
            case CONFIRM_ALERT_DIALOG:
                mDialog = new AlertDialog.Builder(this)
                        .setTitle(mTitle)
                        .setMessage(mMessage)
                        .setPositiveButton(
                                getResources().getString(R.string.DM_SERVICE_ALERT_BUTTON_YES),
                                mKeyYesClickListener)
                        .setNegativeButton(
                                getResources().getString(R.string.DM_SERVICE_ALERT_BUTTON_NO),
                                mKeyNoClickListener)
                        .setCancelable(true)
                        .create();
                break;
            case TEXTINPUT_ALERT_DIALOG:
                View view = LayoutInflater.from(this).inflate(R.layout.textinput_dialog, null);
                mInputBox = (EditText) view.findViewById(R.id.textinput_content);
                mInputBox.requestFocus();
                mInputBox.setText(mResultData);
                mInputBox.setInputType(mInputType);
                InputFilter[] f = new InputFilter[1];
                f[0] = new InputFilter.LengthFilter(mMaxLength);
                mInputBox.setFilters(f);

                mDialog = new AlertDialog.Builder(this)
                        .setTitle(mTitle)
                        .setMessage(mMessage)
                        .setView(view)
                        .setPositiveButton(
                                getResources().getString(R.string.DM_SERVICE_ALERT_BUTTON_YES),
                                mKeyYesClickListener)
                        .setNegativeButton(
                                getResources().getString(R.string.DM_SERVICE_ALERT_BUTTON_NO),
                                mKeyNoClickListener)
                        .create();
                break;
            case SINGLECHOICE_ALERT_DIALOG:
                mDialog = new AlertDialog.Builder(this)
                        .setTitle(mMessage)
                        .setSingleChoiceItems(mChoices, mCheckedItem, mSingleChoiceClickListener)
                        .setPositiveButton(
                                getResources().getString(R.string.DM_SERVICE_ALERT_BUTTON_YES),
                                mKeyYesClickListener)
                        .setNegativeButton(
                                getResources().getString(R.string.DM_SERVICE_ALERT_BUTTON_NO),
                                mKeyNoClickListener)
                        .setCancelable(true)
                        .create();
                break;
            case MULTICHOICE_ALERT_DIALOG:
                mDialog = new AlertDialog.Builder(this)
                        .setTitle(mMessage)
                        .setMultiChoiceItems(mChoices, mCheckedItems, mMultiChoiceClickListener)
                        .setPositiveButton(
                                getResources().getString(R.string.DM_SERVICE_ALERT_BUTTON_YES),
                                mKeyYesClickListener)
                        .setNegativeButton(
                                getResources().getString(R.string.DM_SERVICE_ALERT_BUTTON_NO),
                                mKeyNoClickListener)
                        .setCancelable(true)
                        .create();
                break;

            case SHOW_PROGRESS_ALERT_DIALOG:
                mDialog = new ProgressDialog(this);
                mDialog.setTitle(mTitle);
                mDialog.setMessage(mMessage);
                mDialog.setCancelable(true);
                if (DMClientService.DBG) {
                    logd("SHOW_PROGRESS_ALERT_DIALOG");
                }
                break;

            default:
                Log.i(TAG, "Unknown dialog type: " + id);
                return null;
        }

        if (-1 != mIcon) {
            mDialog.setIcon(mIcon);
        }

        mDialog.setOnCancelListener(mOnCancelListener);
        return mDialog;
    }

    private void getKeyGuardAndWakeLock() {
        logd("Inside getKeyGuardAndWakeLock");
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        wl = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK |
                PowerManager.ACQUIRE_CAUSES_WAKEUP |
                PowerManager.ON_AFTER_RELEASE, TAG);
        logd("Acquiring the wakelock");
        wl.acquire();

        KeyguardManager keyguardManager = (KeyguardManager) getSystemService(
                Context.KEYGUARD_SERVICE);
        mKeyguardLock = keyguardManager.newKeyguardLock(TAG);
        logd("Disabling the KeyGuard");
        mKeyguardLock.disableKeyguard();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (DMClientService.DBG) {
            logd("onKeyDown - keyCode: " + keyCode + " DialogType: " + mType);
        }
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            onFinish();
            return true;
        }
        return false;
    }

    @Override
    public void onDestroy() {
        if (DMClientService.DBG) {
            logd("onDestroy - DialogType: " + mType);
        }
        if (mKeyReceiver != null) {
            unregisterReceiver(mKeyReceiver);
        }
        releaseKeyGuardAndWakeLock();
        super.onDestroy();
    }

    private void onFinish() {
        if (DMClientService.DBG) {
            logd("OnFinish: Dialog - " + mType);
        }

        if (mTimeout > 0) {
            mHandler.removeCallbacks(mDialogTimeout);
        }

        if (null != mDialog) {
            mDialog.dismiss();
        }

        mDialog = null;

        if (mType == DISPLAY_ALERT_DIALOG ||
                mType == SHOW_PROGRESS_ALERT_DIALOG ||
                mType == HIDE_PROGRESS_ALERT_DIALOG ||
                mType == CANCEL_ALERT_DIALOG) {
            finish();
            return;
        }

        Intent i = new Intent(DMIntent.DM_ALERT_DLG_CLOSED);

        i.putExtra("ResultCode", mResultCode);
        if (DMClientService.DBG) {
            logd("onFinish - ResultCode: " + mResultCode);
        }

        if (mType == TEXTINPUT_ALERT_DIALOG) {
            i.putExtra("ResultData", mResultData);
            if (DMClientService.DBG) {
                logd("onFinish - ResultData: " + mResultData);
            }
        } else if (mType == SINGLECHOICE_ALERT_DIALOG) {
            i.putExtra("ResultCheckedItem", mCheckedItem);
            if (DMClientService.DBG) {
                logd("onFinish - ResultCheckedItem: " + mCheckedItem);
            }
        } else if (mType == MULTICHOICE_ALERT_DIALOG) {
            i.putExtra("ResultCheckedItems", mCheckedItems);
            if (DMClientService.DBG) {
                logd("onFinish - ResultCheckedItems: "
                        + Arrays.toString(mCheckedItems));
            }
        }

        sendBroadcast(i);

        finish();
    }

    private void releaseKeyGuardAndWakeLock() {
        logd("Inside releaseKeyGuardAndWakeLock");
        if (wl != null) {
            logd("Releasing the wakelock");
            wl.release();
            wl = null;
        }

        if (mKeyguardLock != null) {
            logd("Reenable the KeyGuard");
            mKeyguardLock.reenableKeyguard();
            mKeyguardLock = null;
        }
    }

    private final DialogInterface.OnClickListener mKeyYesClickListener
            = new DialogInterface.OnClickListener() {
        @Override
        public void onClick(DialogInterface dialog, int which) {
            if (mTimeout > 0) {
                mHandler.removeCallbacks(mDialogTimeout);
            }

            if (mType == TEXTINPUT_ALERT_DIALOG) {
                mResultData = mInputBox.getText().toString();
            }

            mResultCode = DMAlert.DM_SERVICE_ALERT_RESP_YES;
            onFinish();
            if (DMClientService.DBG) {
                logd("DMAlert.DM_SERVICE_ALERT_RESP_YES");
            }
        }
    };

    private final DialogInterface.OnClickListener mKeyNoClickListener
            = new DialogInterface.OnClickListener() {
        @Override
        public void onClick(DialogInterface dialog, int which) {
            if (mTimeout > 0) {
                mHandler.removeCallbacks(mDialogTimeout);
            }

            mResultCode = DMAlert.DM_SERVICE_ALERT_RESP_NO;
            onFinish();
            if (DMClientService.DBG) {
                logd("DMAlert.DM_SERVICE_ALERT_RESP_NO");
            }
        }
    };

    private final DialogInterface.OnCancelListener mOnCancelListener
            = new DialogInterface.OnCancelListener() {
        @Override
        public void onCancel(DialogInterface dialog) {
            if (DMClientService.DBG) {
                logd("OnCancel: Dialog - " + mType);
            }
            mResultCode = DMAlert.DM_SERVICE_ALERT_RESP_CANCEL;
            if (DMClientService.DBG) {
                logd("DMAlert.DM_SERVICE_ALERT_RESP_CANCEL");
            }
            onFinish();
        }
    };

    private final DialogInterface.OnClickListener mSingleChoiceClickListener
            = new DialogInterface.OnClickListener() {
        @Override
        public void onClick(DialogInterface dialog, int which) {
            mCheckedItem = which;
        }
    };

    private final DialogInterface.OnMultiChoiceClickListener mMultiChoiceClickListener
            = new DialogInterface.OnMultiChoiceClickListener() {
        @Override
        public void onClick(DialogInterface dialog, int which, boolean isChecked) {
            mCheckedItems[which] = isChecked;
        }
    };

    private static void logd(String msg) {
        Log.d(TAG, msg);
    }
}

