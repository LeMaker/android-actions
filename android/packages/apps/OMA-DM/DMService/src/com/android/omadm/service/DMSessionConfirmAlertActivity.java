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
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

public class DMSessionConfirmAlertActivity extends Activity {
    private static final String TAG = "DMSessionConfirmAlertActivity";
    private static final boolean DBG = DMClientService.DBG;

    private AlertDialog mDialog;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        DMHelper.cancelNotification(this, DMHelper.NOTIFICATION_CONFIRMATION_ID);
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onResume() {
        logd("Activity DM session confirmation has been started.");
        super.onResume();
        showDialog();
    }

    public static class MyAlertDialogFragment extends DialogFragment {

        public static MyAlertDialogFragment newInstance() {
            MyAlertDialogFragment frag = new MyAlertDialogFragment();
            Bundle args = new Bundle();
            frag.setArguments(args);
            return frag;
        }

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {

            return new AlertDialog.Builder(getActivity())
                    .setIcon(R.drawable.alert_dialog_icon)
                    .setTitle(R.string.dm_session_confirmation_title)
                    .setMessage(R.string.dm_session_confirmation_message)
                    .setCancelable(false)
                    .setPositiveButton(getResources().getString(R.string.dm_session_button_confirm),
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int whichButton) {
                                    ((DMSessionConfirmAlertActivity) getActivity())
                                            .doPositiveClick();
                                }
                            }
                    )
                    .setNegativeButton(getResources().getString(R.string.dm_session_button_reject),
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int whichButton) {
                                    ((DMSessionConfirmAlertActivity) getActivity())
                                            .doNegativeClick();
                                }
                            }
                    )
                    .create();
        }
    }

    void doNegativeClick() {
        userDeniedSession();
    }

    void doPositiveClick() {
        userConfirmedSession();
    }

    private void userConfirmedSession() {
        logd("userConfirmedSession");
        Intent intent = new Intent(DMIntent.ACTION_USER_CONFIRMED_DM_SESSION);
        sendBroadcast(intent);

        finishMe();

    }

    private void userDeniedSession() {
        logd("userDeniedSession");
        DMHelper.cleanAllResources(this);

        finishMe();

    }

    private void showDialog() {
        DialogFragment newFragment = MyAlertDialogFragment.newInstance();
        newFragment.show(getFragmentManager(), "dialog");
    }

    private void finishMe() {

        if (null != mDialog) {
            mDialog.dismiss();
        }

        mDialog = null;
        finish();
    }

    private static void logd(String msg) {
        Log.d(TAG, msg);
    }
}
