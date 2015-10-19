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
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;

public class DMDialog extends Activity {

    private static final String TAG = "DMDialog";

    private static final int PKG0_ALERT_DLG = 1;

    private static final int PKG0_INFO_DLG = 2;

    private static final int UPDATE_CANCEL_DLG = 3;

    private static final int USER_PRESS_OK = 1;

    private static final int USER_PRESS_CANCEL = 2;

    private String mTitle;

    private String mMessage;

    private int mResult;

    private String mDlgType;

    private AlertDialog mDialog;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (DMClientService.DBG) {
            logd("onCreate");
        }
        final Intent intent = getIntent();
        mDlgType = intent.getAction();

        if (mDlgType.equals(DMIntent.SHOW_PKG0_ALERT_DLG)) {
            mTitle = intent.getStringExtra("Title");
            mMessage = intent.getStringExtra("Message");

            showDialog(PKG0_ALERT_DLG);
        }

        if (mDlgType.equals(DMIntent.SHOW_PKG0_INFO_DLG)) {
            mTitle = intent.getStringExtra("Title");
            mMessage = intent.getStringExtra("Message");
            showDialog(PKG0_INFO_DLG);

            Handler handler = new Handler();
            Runnable closeDialogTask = new Runnable() {
                @Override
                public void run() {
                    mDialog.dismiss();
                    finish();
                }
            };

            handler.postDelayed(closeDialogTask, 40 * 1000);
        }

        if (mDlgType.equals(DMIntent.SHOW_UPDATE_CANCEL_DLG)) {

            mTitle = intent.getStringExtra("Title");
            mMessage = intent.getStringExtra("Message");
            showDialog(UPDATE_CANCEL_DLG);
        }
    }

    @Override
    protected Dialog onCreateDialog(int id) {
        switch (id) {
            case PKG0_ALERT_DLG:
                mDialog = new AlertDialog.Builder(this)
                        .setTitle(mTitle)
                        .setIcon(R.drawable.alert_dialog_icon)
                        .setMessage(mMessage)
                        .setNegativeButton(android.R.string.cancel, mCancelClickListener)
                        .setPositiveButton(android.R.string.ok, mOKClickListener)
                        .setCancelable(false)
                        .create();
                return mDialog;
            case PKG0_INFO_DLG:
                mDialog = new AlertDialog.Builder(this)
                        .setTitle(mTitle)
                        .setIcon(R.drawable.alert_dialog_icon)
                        .setMessage(mMessage)
                        .setCancelable(false)
                        .create();
                return mDialog;
            case UPDATE_CANCEL_DLG:
                mDialog = new AlertDialog.Builder(this)
                        .setTitle(mTitle)
                        .setIcon(R.drawable.alert_dialog_icon)
                        .setMessage(mMessage)
                        .setPositiveButton(android.R.string.ok, mOKClickListener)
                        .setCancelable(false)
                        .create();
                return mDialog;
        }
        return null;
    }

    private final DialogInterface.OnClickListener mOKClickListener = new DialogInterface.OnClickListener() {
        @Override
        public void onClick(DialogInterface dialog, int which) {
            mResult = USER_PRESS_OK;
            if (DMClientService.DBG) {
                logd("Press:OK");
            }
            mDialog.dismiss();
            finish();
        }
    };

    private final DialogInterface.OnClickListener mCancelClickListener = new DialogInterface.OnClickListener() {
        @Override
        public void onClick(DialogInterface dialog, int which) {
            mResult = USER_PRESS_CANCEL;
            if (DMClientService.DBG) {
                logd("Press:Cancel");
            }
            mDialog.dismiss();
            finish();
        }
    };

    @Override
    public void onDestroy() {
        Log.i(TAG, "onDestroy");
        if (mDlgType.equals(DMIntent.SHOW_PKG0_ALERT_DLG)) {
            Intent i = new Intent(DMIntent.SHOW_PKG0_ALERT_DLG_CLOSE);
            i.putExtra("Result", mResult);
            sendBroadcast(i);
        }
        super.onDestroy();
    }

    private static void logd(String msg) {
        Log.d(TAG, msg);
    }
}
