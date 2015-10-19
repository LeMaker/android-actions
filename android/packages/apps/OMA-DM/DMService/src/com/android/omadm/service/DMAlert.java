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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.text.InputType;
import android.util.Log;

import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class DMAlert {

    /* DMServiceAlert Response Code */
    private static final int DM_SERVICE_ALERT_RESP_FAIL = -1;

    public static final int DM_SERVICE_ALERT_RESP_SUCCESS = 0;

    public static final int DM_SERVICE_ALERT_RESP_NO = 1;

    public static final int DM_SERVICE_ALERT_RESP_YES = 2;

    public static final int DM_SERVICE_ALERT_RESP_CANCEL = 3;

    public static final int DM_SERVICE_ALERT_RESP_TIMEOUT = 4;

    // DMServiceAlert Input Type
    private static final int DM_SERVICE_ALERT_INPUT_ALPHA = 0;

    private static final int DM_SERVICE_ALERT_INPUT_NUMERIC = 1;

    private static final int DM_SERVICE_ALERT_INPUT_DATE = 2;

    private static final int DM_SERVICE_ALERT_INPUT_TIME = 3;

    private static final int DM_SERVICE_ALERT_INPUT_PHONE_NUM = 4;

    private static final int DM_SERVICE_ALERT_INPUT_IP_ADDR = 5;

    // DMServiceAlert Echo Type
    public static final int DM_SERVICE_ALERT_ECHO_TEXT = 0;

    private static final int DM_SERVICE_ALERT_ECHO_PASSWD = 1;

    // DMServiceAlert Icon Type
    private static final int DM_SERVICE_ALERT_ICON_GENERIC = 0;

    public static final int DM_SERVICE_ALERT_ICON_PROGRESS = 1;

    public static final int DM_SERVICE_ALERT_ICON_OK = 2;

    public static final int DM_SERVICE_ALERT_ICON_ERROR = 3;

    public static final int DM_SERVICE_ALERT_ICON_CONFIRM = 4;

    public static final int DM_SERVICE_ALERT_ICON_ACTION = 5;

    public static final int DM_SERVICE_ALERT_ICON_INFO = 6;

    // DMServiceAlert Title Type
    private static final int DM_SERVICE_ALERT_TITLE_DEVICE_MGMT = 0;

    public static final int DM_SERVICE_ALERT_TITLE_SYSTEM_UPDATE = 1;

    public static final int DM_SERVICE_ALERT_TITLE_NEED_AUTHENTICATION = 2;

    public static final int DM_SERVICE_ALERT_TITLE_UPDATE_COMPLETE = 3;

    private static final int DM_SERVICE_ALERT_TITLE_SYSTEM_MESSAGE = 4;

    public static final int DM_SERVICE_ALERT_TITLE_AUTHENTICATION_FAILED = 5;

    public static final int DM_SERVICE_ALERT_TITLE_UPDATE_ERROR = 6;

    public static final int DM_SERVICE_ALERT_TITLE_UPDATE_CANCELLED = 7;

    public static final int DM_SERVICE_ALERT_TITLE_PROFILE_FOR_BROWSER = 8;

    public static final int DM_SERVICE_ALERT_TITLE_PROFILE_FOR_MMS = 9;

    public static final int DM_SERVICE_ALERT_TITLE_CONNECTION_FAILED = 10;

    public static final int DM_SERVICE_ALERT_TITLE_CONNECTION_FAILURE = 11;

    public static final int DM_SERVICE_ALERT_TITLE_SW_UPDATE = 12;

    private final Context mCtx;

    private boolean mUIMode;

    private Lock mLock;

    private Condition mCond;

    private int mResultCode;

    private String mResultData;

    private int mResultCheckedItem;

    private boolean[] mResultCheckedItems;

    public DMAlert(Context ctx) {
        mCtx = ctx;
        mUIMode = true;
    }

    public void setUIMode(boolean uiMode) {
        mUIMode = uiMode;
    }

    // used for the test only
    public boolean getUIMode() {
        return mUIMode;
    }

    /**
     * Display a text message. Called from JNI code.
     *
     * @param minDisplayTime minimum display time, in seconds.
     * @param msg            messages to display
     * @param title          resource ID for dialog title
     * @param icon           resource ID for dialog icon
     * @return SYNCML_DM_SUCCESS on successful completion; SYNCML_DM_FAIL or another more specific
     *         error code on failure.
     */
    public int showDisplayAlert(int minDisplayTime, String msg, int title, int icon) {
        //if ( !mUIMode) return DM_SERVICE_ALERT_RESP_FAIL;

        IntentFilter filter = new IntentFilter();
        filter.addAction(DMIntent.DM_ALERT_DLG_CLOSED);

        Intent intent = new Intent(DMIntent.SHOW_DISPLAY_ALERT_DLG);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setClass(mCtx, DMAlertActivity.class);

        intent.putExtra("Message", msg);
        intent.putExtra("Title", getTitle(title));
        intent.putExtra("Icon", getIcon(icon));
        intent.putExtra("Timeout", minDisplayTime);

        try {
            mCtx.startActivity(intent);
        } catch (RuntimeException e) {
            e.printStackTrace();
            return DM_SERVICE_ALERT_RESP_FAIL;
        }

        return DM_SERVICE_ALERT_RESP_SUCCESS;
    }

    /**
     * Display a confirmation alert dialog: user can confirm or cancel the action. Called from JNI
     * code.
     *
     * @param maxDisplayTime maximum display time (for timeout), in seconds.
     * @param msg            messages to display
     * @param title          resource ID for dialog title
     * @param icon           resource ID for dialog icon
     * @return SYNCML_DM_SUCCESS on successful completion; SYNCML_DM_FAIL or another more specific
     *         error code on failure.
     */
    public int showConfirmAlert(int maxDisplayTime, String msg, int title, int icon) {
        //if ( !mUIMode) return DM_SERVICE_ALERT_RESP_FAIL;

        mLock = new ReentrantLock();
        mCond = mLock.newCondition();

        IntentFilter filter = new IntentFilter();
        filter.addAction(DMIntent.DM_ALERT_DLG_CLOSED);
        mCtx.registerReceiver(mAlertCloseListener, filter);

        Intent intent = new Intent(DMIntent.SHOW_CONFIRM_ALERT_DLG);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setClass(mCtx, DMAlertActivity.class);

        intent.putExtra("Message", msg);
        intent.putExtra("Title", getTitle(title));
        intent.putExtra("Icon", getIcon(icon));
        intent.putExtra("Timeout", maxDisplayTime);

        mCtx.startActivity(intent);

        mLock.lock();
        try {
            mCond.await();
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            mLock.unlock();
            mCtx.unregisterReceiver(mAlertCloseListener);
        }

        return mResultCode;
    }

    /**
     * Display a text input message box for user to enter input. Called from JNI code.
     *
     * @param maxDisplayTime maximum display time (for timeout), in seconds.
     * @param msg            messages to display
     * @param defaultText    default user action when timeout
     * @param maxLength      length allowed in user input
     * @param inputType      data format as specified in DM_ALERT_INPUT_T
     * @param echoType       whether to echo user input (hidden for password ) as specified in
     *                       DM_ALERT_ECHO_T
     * @param title          either DM_SERVICE_ALERT_TITLE_DEVICE_MGMT or DM_SERVICE_ALERT_TITLE_SYSTEM_MESSAGE
     * @param icon           is DM_SERVICE_ALERT_ICON_GENERIC
     * @return SYNCML_DM_SUCCESS on successful completion; SYNCML_DM_FAIL or another more specific
     *         error code on failure.
     */
    public String showTextInputAlert(int maxDisplayTime, String msg, String defaultText,
            int maxLength, int inputType, int echoType,
            int title, int icon) {
        //if ( !mUIMode) return String.valueOf(DM_SERVICE_ALERT_RESP_FAIL);

        mLock = new ReentrantLock();
        mCond = mLock.newCondition();

        IntentFilter filter = new IntentFilter();
        filter.addAction(DMIntent.DM_ALERT_DLG_CLOSED);
        mCtx.registerReceiver(mAlertCloseListener, filter);

        Intent intent = new Intent(DMIntent.SHOW_TEXTINPUT_ALERT_DLG);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setClass(mCtx, DMAlertActivity.class);

        intent.putExtra("Message", msg);
        intent.putExtra("Title", getTitle(title));
        intent.putExtra("Icon", getIcon(icon));
        intent.putExtra("Timeout", maxDisplayTime);
        intent.putExtra("DefaultText", defaultText);
        intent.putExtra("MaxLength", maxLength);
        intent.putExtra("InputType", getInputType(inputType, echoType));

        Log.d("showTextInputAlert", "Dialog Intent: " + intent);

        mCtx.startActivity(intent);

        mLock.lock();
        try {
            mCond.await();
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            mLock.unlock();
            mCtx.unregisterReceiver(mAlertCloseListener);
        }

        return String.valueOf(mResultCode) + ':' + mResultData;
    }

    /**
     * Display a single choice message box for user to pick up one entry. Called from JNI code.
     *
     * @param maxDisplayTime maximum display time (for timeout), in seconds.
     * @param msg            messages to display
     * @param choices        an array of strings containing the text for each choice
     * @param checkedItem    the default checked entry
     * @param title          resource ID for dialog title
     * @param icon           resource ID for dialog icon
     * @return SYNCML_DM_SUCCESS on successful completion; SYNCML_DM_FAIL or another more specific
     *         error code on failure.
     */
    public String showSingleChoiceAlert(int maxDisplayTime, String msg, String[] choices,
            int checkedItem, int title, int icon) {
        //if ( !mUIMode) return String.valueOf(DM_SERVICE_ALERT_RESP_FAIL);

        mLock = new ReentrantLock();
        mCond = mLock.newCondition();

        IntentFilter filter = new IntentFilter();
        filter.addAction(DMIntent.DM_ALERT_DLG_CLOSED);
        mCtx.registerReceiver(mAlertCloseListener, filter);

        Intent intent = new Intent(DMIntent.SHOW_SINGLECHOICE_ALERT_DLG);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setClass(mCtx, DMAlertActivity.class);

        intent.putExtra("Message", msg);
        intent.putExtra("Title", getTitle(title));
        intent.putExtra("Icon", getIcon(icon));
        intent.putExtra("Timeout", maxDisplayTime);
        intent.putExtra("Choices", choices);
        intent.putExtra("CheckedItem", checkedItem);

        Log.d("showSingleChoiceAlert", "Dialog Intent: " + intent);

        mCtx.startActivity(intent);

        mLock.lock();
        try {
            mCond.await();
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            mLock.unlock();
            mCtx.unregisterReceiver(mAlertCloseListener);
        }

        return String.valueOf(mResultCode) + ':' + mResultCheckedItem;
    }

    /**
     * Display a multiple choice message box for user to pick up zero to many entry. Called from JNI
     * code.
     *
     * @param maxDisplayTime maximum display time (for timeout), in seconds.
     * @param msg            messages to display
     * @param choices        an array of strings to hold text for each choice
     * @param checkedItems   an array of booleans to indicate the checked items
     * @param title          resource ID for dialog title
     * @param icon           resource ID for dialog icon
     * @return SYNCML_DM_SUCCESS on successful completion; SYNCML_DM_FAIL or another more specific
     *         error code on failure.
     */
    public int showMultipleChoiceAlert(int maxDisplayTime, String msg, String[] choices,
            boolean[] checkedItems, int title, int icon) {
        //if ( !mUIMode) return DM_SERVICE_ALERT_RESP_FAIL;

        mLock = new ReentrantLock();
        mCond = mLock.newCondition();

        IntentFilter filter = new IntentFilter();
        filter.addAction(DMIntent.DM_ALERT_DLG_CLOSED);
        mCtx.registerReceiver(mAlertCloseListener, filter);

        Intent intent = new Intent(DMIntent.SHOW_MULTICHOICE_ALERT_DLG);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setClass(mCtx, DMAlertActivity.class);

        intent.putExtra("Message", msg);
        intent.putExtra("Title", getTitle(title));
        intent.putExtra("Icon", getIcon(icon));
        intent.putExtra("Timeout", maxDisplayTime);
        intent.putExtra("Choices", choices);
        intent.putExtra("CheckedItems", checkedItems);

        Log.d("showMultiChoiceAlert", "Dialog Intent: " + intent);

        mCtx.startActivity(intent);

        mLock.lock();
        try {
            mCond.await();
        } catch (InterruptedException e) {
            Log.e("DMAlert", "lock/await interrupted", e);  // FIXME logging
        } finally {
            mLock.unlock();
            mCtx.unregisterReceiver(mAlertCloseListener);
        }

        int checkedItemsLength = checkedItems.length;
        for (int i = 0; DM_SERVICE_ALERT_RESP_YES == mResultCode && i < checkedItemsLength; i++) {
            checkedItems[i] = mResultCheckedItems[i];
        }

        return mResultCode;
    }

    /**
     * Display a progress bar. FIXME: remove if unused.
     *
     * @param minDisplayTime minimum display time, in seconds.
     * @param msg            messages to display
     * @return Upon successful completion, the SYNCML_DM_SUCCESS is returned, otherwise
     *         SYNCML_DM_FAIL or other more specific error codes.
     */
    public int showProgressAlert(int minDisplayTime, String msg, int title, int icon) {
        IntentFilter filter = new IntentFilter();
        filter.addAction(DMIntent.DM_ALERT_DLG_CLOSED);

        Intent intent = new Intent(DMIntent.SHOW_PROGRESS_ALERT_DLG);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setClass(mCtx, DMAlertActivity.class);

        intent.putExtra("Message", msg);
        intent.putExtra("Title", getTitle(title));
        intent.putExtra("Icon", getIcon(icon));
        intent.putExtra("Timeout", minDisplayTime);

        try {
            mCtx.startActivity(intent);
        } catch (Exception e) {
            e.printStackTrace();
            return DM_SERVICE_ALERT_RESP_FAIL;
        }

        return DM_SERVICE_ALERT_RESP_SUCCESS;
    }

    /**
     * Hide a progress bar. FIXME: remove if unneeded.
     *
     * @return Upon successful completion, the SYNCML_DM_SUCCESS is returned, otherwise
     *         SYNCML_DM_FAIL or other more specific error codes.
     */
    public int hideProgressAlert() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(DMIntent.DM_ALERT_DLG_CLOSED);

        Intent intent = new Intent(DMIntent.HIDE_PROGRESS_ALERT_DLG);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setClass(mCtx, DMAlertActivity.class);

        try {
            mCtx.startActivity(intent);
        } catch (Exception e) {
            e.printStackTrace();
            return DM_SERVICE_ALERT_RESP_FAIL;
        }

        return DM_SERVICE_ALERT_RESP_SUCCESS;
    }

    /**
     * Cancel alert when DM session times out.
     */
    public void cancelSession() {
        if (!mUIMode) {
            return;
        }

        Intent intent = new Intent(DMIntent.CANCEL_ALERT_DLG);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setClass(mCtx, DMAlertActivity.class);

        try {
            mCtx.startActivity(intent);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private String getTitle(int t) {
        switch (t) {
            case DM_SERVICE_ALERT_TITLE_DEVICE_MGMT:
                return mCtx.getResources().getString(R.string.DM_SERVICE_ALERT_TITLE_DEVICE_MGMT);
            case DM_SERVICE_ALERT_TITLE_SYSTEM_MESSAGE:
                return mCtx.getResources()
                        .getString(R.string.DM_SERVICE_ALERT_TITLE_SYSTEM_MESSAGE);
            default:
                break;
        }
        return "Title ID: " + t;
    }

    private static int getIcon(int i) {
        switch (i) {
            case DM_SERVICE_ALERT_ICON_GENERIC:
                return R.drawable.alert_dialog_icon;
            default:
                break;
        }
        return R.drawable.alert_dialog_icon;
    }

    private static int getInputType(int i, int e) {
        int t = InputType.TYPE_CLASS_TEXT;
        switch (i) {
            case DM_SERVICE_ALERT_INPUT_ALPHA:
                t = InputType.TYPE_CLASS_TEXT;
                break;
            case DM_SERVICE_ALERT_INPUT_NUMERIC:
                t = InputType.TYPE_CLASS_NUMBER;
                break;
            case DM_SERVICE_ALERT_INPUT_DATE:
                t = InputType.TYPE_DATETIME_VARIATION_DATE;
                break;
            case DM_SERVICE_ALERT_INPUT_TIME:
                t = InputType.TYPE_DATETIME_VARIATION_TIME;
                break;
            case DM_SERVICE_ALERT_INPUT_PHONE_NUM:
                t = InputType.TYPE_CLASS_PHONE;
                break;
            case DM_SERVICE_ALERT_INPUT_IP_ADDR:
                t = InputType.TYPE_CLASS_TEXT;
                break;
            default:
                break;
        }

        if (e == DM_SERVICE_ALERT_ECHO_PASSWD) {
            t |= InputType.TYPE_TEXT_VARIATION_PASSWORD;
        }
        return t;
    }

    private final BroadcastReceiver mAlertCloseListener = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            mLock.lock();
            try {
                onReceiveLocked(intent);
            } finally {
                mLock.unlock();
            }
        }

        private void onReceiveLocked(Intent intent) {
            mResultCode = intent.getIntExtra("ResultCode", DM_SERVICE_ALERT_RESP_SUCCESS);
            if (DMClientService.DBG) {
                Log.d("mAlertCloseListener", "Dialog ResultCode: " + mResultCode);
            }

            mResultData = intent.getStringExtra("ResultData");
            if (null != mResultData) {
                if (DMClientService.DBG) {
                    Log.d("mAlertCloseListener", "Dialog ResultData: " + mResultData);
                }
            }

            mResultCheckedItem = intent.getIntExtra("ResultCheckedItem", -1);
            if (-1 != mResultCheckedItem) {
                if (DMClientService.DBG) {
                    Log.d("mAlertCloseListener", "Dialog ResultCheckedItem: " + mResultCheckedItem);
                }
            }

            boolean[] resultCheckedItems = intent.getBooleanArrayExtra("ResultCheckedItems");
            mResultCheckedItems = resultCheckedItems;

            if (DMClientService.DBG && resultCheckedItems != null) {
                int resultCheckedItemsLength = resultCheckedItems.length;
                for (int i = 0; i < resultCheckedItemsLength; i++) {
                    Log.d("mAlertCloseListener",
                            "Dialog ResultCheckedItems " + i + ": " + resultCheckedItems[i]);
                }
            }

            mCond.signalAll();
        }
    };
}
