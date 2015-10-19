/*
 * Copyright 2014, The Android Open Source Project
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

package com.android.managedprovisioning;

import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_ACCOUNT_TO_MIGRATE;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AccountManagerFuture;
import android.accounts.AuthenticatorException;
import android.accounts.OperationCanceledException;
import android.app.Activity;
import android.app.ActivityManagerNative;
import android.app.AlertDialog;
import android.app.IActivityManager;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings;
import android.support.v4.content.LocalBroadcastManager;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.IOException;

/**
 * Profile owner provisioning sets up a separate profile on a device whose primary user is already
 * set up.
 *
 * <p>
 * The typical example is setting up a corporate profile that is controlled by their employer on a
 * users personal device to keep personal and work data separate.
 *
 * <p>
 * The activity handles the UI for managed profile provisioning and starts the
 * {@link ProfileOwnerProvisioningService}, which runs through the setup steps in an
 * async task.
 */
public class ProfileOwnerProvisioningActivity extends Activity {
    protected static final String ACTION_CANCEL_PROVISIONING =
            "com.android.managedprovisioning.CANCEL_PROVISIONING";

    private BroadcastReceiver mServiceMessageReceiver;

    // Provisioning service started
    private static final int CANCELSTATUS_PROVISIONING = 1;
    // Back button pressed during provisioning, confirm dialog showing.
    private static final int CANCELSTATUS_CONFIRMING = 2;
    // Cancel confirmed, waiting for the provisioning service to complete.
    private static final int CANCELSTATUS_CANCELLING = 3;
    // Cancelling not possible anymore, provisioning already finished succesfully.
    private static final int CANCELSTATUS_FINALIZING = 4;

    private static final String KEY_CANCELSTATUS= "cancelstatus";
    private static final String KEY_PENDING_INTENT = "pending_intent";

    private int mCancelStatus = CANCELSTATUS_PROVISIONING;
    private Intent mPendingProvisioningResult = null;
    private ProgressDialog mCancelProgressDialog = null;
    private AccountManager mAccountManager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ProvisionLogger.logd("Profile owner provisioning activity ONCREATE");
        mAccountManager = (AccountManager) getSystemService(Context.ACCOUNT_SERVICE);

        if (savedInstanceState != null) {
            mCancelStatus = savedInstanceState.getInt(KEY_CANCELSTATUS, CANCELSTATUS_PROVISIONING);
            mPendingProvisioningResult = savedInstanceState.getParcelable(KEY_PENDING_INTENT);
        }

        final LayoutInflater inflater = getLayoutInflater();
        View contentView = inflater.inflate(R.layout.progress, null);
        setContentView(contentView);
        TextView textView = (TextView) findViewById(R.id.prog_text);
        if (textView != null) textView.setText(getString(R.string.setting_up_workspace));


        if (mCancelStatus == CANCELSTATUS_CONFIRMING) {
            showCancelProvisioningDialog();
        } else if (mCancelStatus == CANCELSTATUS_CANCELLING) {
            showCancelProgressDialog();
        }
    }


    @Override
    protected void onResume() {
        super.onResume();

        // Setup broadcast receiver for feedback from service.
        mServiceMessageReceiver = new ServiceMessageReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(ProfileOwnerProvisioningService.ACTION_PROVISIONING_SUCCESS);
        filter.addAction(ProfileOwnerProvisioningService.ACTION_PROVISIONING_ERROR);
        filter.addAction(ProfileOwnerProvisioningService.ACTION_PROVISIONING_CANCELLED);
        LocalBroadcastManager.getInstance(this).registerReceiver(mServiceMessageReceiver, filter);

        // Start service async to make sure the UI is loaded first.
        final Handler handler = new Handler(getMainLooper());
        handler.post(new Runnable() {
                @Override
                public void run() {
                    Intent intent = new Intent(ProfileOwnerProvisioningActivity.this,
                            ProfileOwnerProvisioningService.class);
                    intent.putExtras(getIntent());
                    startService(intent);
                }
            });
    }

    class ServiceMessageReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (mCancelStatus == CANCELSTATUS_CONFIRMING) {
                // Store the incoming intent and only process it after the user has responded to
                // the cancel dialog
                mPendingProvisioningResult = intent;
                return;
            }
            handleProvisioningResult(intent);
        }
    }

    private void handleProvisioningResult(Intent intent) {
        String action = intent.getAction();
        if (ProfileOwnerProvisioningService.ACTION_PROVISIONING_SUCCESS.equals(action)) {
            if (mCancelStatus == CANCELSTATUS_CANCELLING) {
                return;
            }

            ProvisionLogger.logd("Successfully provisioned."
                    + "Finishing ProfileOwnerProvisioningActivity");

            Intent pendingIntent = (Intent) intent.getParcelableExtra(
                    ProfileOwnerProvisioningService.EXTRA_PENDING_SUCCESS_INTENT);
            int serialNumber = intent.getIntExtra(
                    ProfileOwnerProvisioningService.EXTRA_PROFILE_USER_SERIAL_NUMBER, -1);

            int userId = intent.getIntExtra(ProfileOwnerProvisioningService.EXTRA_PROFILE_USER_ID,
                    -1);
            onProvisioningSuccess(pendingIntent, userId, serialNumber);
        } else if (ProfileOwnerProvisioningService.ACTION_PROVISIONING_ERROR.equals(action)) {
            if (mCancelStatus == CANCELSTATUS_CANCELLING){
                return;
            }
            String errorLogMessage = intent.getStringExtra(
                    ProfileOwnerProvisioningService.EXTRA_LOG_MESSAGE_KEY);
            ProvisionLogger.logd("Error reported: " + errorLogMessage);
            error(R.string.managed_provisioning_error_text, errorLogMessage);
        } if (ProfileOwnerProvisioningService.ACTION_PROVISIONING_CANCELLED.equals(action)) {
            if (mCancelStatus != CANCELSTATUS_CANCELLING) {
                return;
            }
            mCancelProgressDialog.dismiss();
            ProfileOwnerProvisioningActivity.this.setResult(Activity.RESULT_CANCELED);
            stopService(new Intent(ProfileOwnerProvisioningActivity.this,
                            ProfileOwnerProvisioningService.class));
            ProfileOwnerProvisioningActivity.this.finish();
        }
    }

    @Override
    public void onBackPressed() {
        if (mCancelStatus == CANCELSTATUS_PROVISIONING) {
            showCancelProvisioningDialog();
        }
    }

    private void showCancelProvisioningDialog() {
        mCancelStatus = CANCELSTATUS_CONFIRMING;
        AlertDialog alertDialog = new AlertDialog.Builder(this)
                        .setCancelable(false)
                        .setTitle(R.string.profile_owner_cancel_title)
                        .setMessage(R.string.profile_owner_cancel_message)
                        .setNegativeButton(R.string.profile_owner_cancel_cancel,
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog,int id) {
                                        mCancelStatus = CANCELSTATUS_PROVISIONING;
                                        if (mPendingProvisioningResult != null) {
                                            handleProvisioningResult(mPendingProvisioningResult);
                                        }
                                    }
                        })
                        .setPositiveButton(R.string.profile_owner_cancel_ok,
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog,int id) {
                                        confirmCancel();
                                    }
                        })
                        .create();
        alertDialog.show();
    }

    protected void showCancelProgressDialog() {
        mCancelProgressDialog = new ProgressDialog(this);
        mCancelProgressDialog.setMessage(getText(R.string.profile_owner_cancelling));
        mCancelProgressDialog.setCancelable(false);
        mCancelProgressDialog.setCanceledOnTouchOutside(false);
        mCancelProgressDialog.show();
    }

    public void error(int resourceId, String logText) {
        ProvisionLogger.loge(logText);
        new AlertDialog.Builder(this)
                .setTitle(R.string.provisioning_error_title)
                .setMessage(getString(resourceId))
                .setCancelable(false)
                .setPositiveButton(R.string.device_owner_error_ok, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog,int id) {
                            confirmCancel();
                        }
                    }).show();
    }

    private void confirmCancel() {
        mCancelStatus = CANCELSTATUS_CANCELLING;
        Intent intent = new Intent(ProfileOwnerProvisioningActivity.this,
                ProfileOwnerProvisioningService.class);
        intent.setAction(ACTION_CANCEL_PROVISIONING);
        startService(intent);
        showCancelProgressDialog();
    }

    /**
     * Notify the mdm that provisioning has completed. When the mdm has received the intent, stop
     * the service and notify the {@link ProfileOwnerProvisioningActivity} so that it can finish itself.
     */
    private void onProvisioningSuccess(Intent pendingSuccessIntent, int userId, int serialNumber) {
        mCancelStatus = CANCELSTATUS_FINALIZING;
        Settings.Secure.putIntForUser(getContentResolver(), Settings.Secure.USER_SETUP_COMPLETE,
                1 /* true- > setup complete */, userId);

        UserManager userManager = (UserManager) getSystemService(Context.USER_SERVICE);
        UserHandle userHandle = userManager.getUserForSerialNumber(serialNumber);

        // Use an ordered broadcast, so that we only finish when the mdm has received it.
        // Avoids a lag in the transition between provisioning and the mdm.
        BroadcastReceiver mdmReceivedSuccessReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                ProvisionLogger.logd("ACTION_PROFILE_PROVISIONING_COMPLETE broadcast received by"
                        + " mdm");
                ProfileOwnerProvisioningActivity.this.setResult(Activity.RESULT_OK);

                // Now cleanup the primary profile if necessary
                if (getIntent().hasExtra(EXTRA_PROVISIONING_ACCOUNT_TO_MIGRATE)) {
                    ProvisionLogger.logd("Cleaning up account from the primary user.");
                    final Account account = (Account) getIntent().getParcelableExtra(
                            EXTRA_PROVISIONING_ACCOUNT_TO_MIGRATE);
                    new AsyncTask<Void, Void, Void>() {
                        @Override
                        protected Void doInBackground(Void... params) {
                            removeAccount(account);
                            return null;
                        }
                    }.execute();
                }

                ProfileOwnerProvisioningActivity.this.finish();
                stopService(new Intent(ProfileOwnerProvisioningActivity.this,
                                ProfileOwnerProvisioningService.class));
            }
        };

        sendOrderedBroadcastAsUser(pendingSuccessIntent, userHandle, null,
                mdmReceivedSuccessReceiver, null, Activity.RESULT_OK, null, null);
        ProvisionLogger.logd("Provisioning complete broadcast has been sent to user "
            + userHandle.getIdentifier());
    }

    private void removeAccount(Account account) {
        try {
            AccountManagerFuture<Bundle> bundle = mAccountManager.removeAccount(account,
                    this, null /* callback */, null /* handler */);
            if (bundle.getResult().getBoolean(AccountManager.KEY_BOOLEAN_RESULT, false)) {
                ProvisionLogger.logw("Account removed from the primary user.");
            } else {
                ProvisionLogger.logw("Could not remove account from the primary user.");
            }
        } catch (OperationCanceledException | AuthenticatorException | IOException e) {
            ProvisionLogger.logw("Exception removing account from the primary user.", e);
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        outState.putInt(KEY_CANCELSTATUS, mCancelStatus);
        outState.putParcelable(KEY_PENDING_INTENT, mPendingProvisioningResult);
    }

    @Override
    public void onPause() {
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mServiceMessageReceiver);
        super.onPause();
    }
}

