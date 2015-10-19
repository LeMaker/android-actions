/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.cts.verifier.managedprovisioning;

import android.app.AlertDialog;
import android.app.admin.DevicePolicyManager;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;
import com.android.cts.verifier.TestListActivity;

import java.util.ArrayList;
import java.util.List;

/**
 * CTS verifier test for BYOD managed provisioning flow.
 * This activity is responsible for starting the managed provisioning flow and verify the outcome of provisioning.
 * It performs the following verifications:
 *   Full disk encryption is enabled.
 *   Profile owner is correctly installed.
 *   Profile owner shows up in the Settings app.
 *   Badged work apps show up in launcher.
 * The first two verifications are performed automatically, by interacting with profile owner using
 * cross-profile intents, while the last two are carried out manually by the user.
 */
public class ByodFlowTestActivity extends PassFailButtons.ListActivity {

    private final String TAG = "ByodFlowTestActivity";
    private static final int REQUEST_STATUS = 1;

    private ComponentName mAdminReceiverComponent;

    private TestAdapter mTestListAdapter;
    private View mStartProvisioningButton;
    private List<TestItem> mTests = new ArrayList<TestItem>();

    protected DevicePolicyManager mDevicePolicyManager;

    private TestItem mProfileOwnerInstalled;
    private TestItem mProfileAccountVisibleTest;
    private TestItem mDeviceAdminVisibleTest;
    private TestItem mWorkAppVisibleTest;
    private TestItem mCrossProfileIntentFiltersTest;
    private TestItem mDisableNonMarketTest;
    private TestItem mEnableNonMarketTest;
    private TestItem mWorkNotificationBadgedTest;
    private TestItem mAppSettingsVisibleTest;
    private TestItem mLocationSettingsVisibleTest;
    private TestItem mCredSettingsVisibleTest;
    private TestItem mPrintSettingsVisibleTest;

    private int mCurrentTestPosition;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mAdminReceiverComponent = new ComponentName(this, DeviceAdminTestReceiver.class.getName());
        mDevicePolicyManager = (DevicePolicyManager) getSystemService(Context.DEVICE_POLICY_SERVICE);
        disableComponent();

        setContentView(R.layout.provisioning_byod);
        setInfoResources(R.string.provisioning_byod, R.string.provisioning_byod_info, -1);
        setPassFailButtonClickListeners();
        getPassButton().setEnabled(false);
        setResult(RESULT_CANCELED);

        setupTests();

        mTestListAdapter = new TestAdapter(this);
        setListAdapter(mTestListAdapter);
        mTestListAdapter.addAll(mTests);

        mCurrentTestPosition = 0;

        mStartProvisioningButton = findViewById(R.id.byod_start);
        mStartProvisioningButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                startByodProvisioning();
            }
        });

        // If we are started by managed provisioning (fresh managed provisioning after encryption
        // reboot), redirect the user back to the main test list. This is because the test result
        // is only saved by the parent TestListActivity, and if we did allow the user to proceed
        // here, the test result would be lost when this activity finishes.
        if (ByodHelperActivity.ACTION_PROFILE_OWNER_STATUS.equals(getIntent().getAction())) {
            startActivity(new Intent(this, TestListActivity.class));
            // Calling super.finish() because we delete managed profile in our overridden of finish(),
            // which is not what we want to do here.
            super.finish();
        } else {
            queryProfileOwner(false);
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        // This is called when managed provisioning completes successfully without reboot.
        super.onNewIntent(intent);
        if (ByodHelperActivity.ACTION_PROFILE_OWNER_STATUS.equals(intent.getAction())) {
            handleStatusUpdate(intent);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        // Called after queryProfileOwner()
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_STATUS && resultCode == RESULT_OK) {
            handleStatusUpdate(data);
        }
    }

    private void handleStatusUpdate(Intent data) {
        boolean provisioned = data.getBooleanExtra(ByodHelperActivity.EXTRA_PROVISIONED, false);
        setTestResult(mProfileOwnerInstalled, provisioned ? TestResult.Passed : TestResult.Failed);
    }

    @Override
    public void finish() {
        // Pass and fail buttons are known to call finish() when clicked, and this is when we want to
        // clean up the provisioned profile.
        requestDeleteProfileOwner();
        super.finish();
    }

    private void setupTests() {
        mProfileOwnerInstalled = new TestItem(this, R.string.provisioning_byod_profileowner) {
            @Override
            public void performTest(ByodFlowTestActivity activity) {
                queryProfileOwner(true);
            }
        };

        /*
         * To keep the image in this test up to date, use the instructions in
         * {@link ByodIconSamplerActivity}.
         */
        mWorkAppVisibleTest = new TestItemWithIcon(this,
                R.string.provisioning_byod_workapps_visible,
                R.string.provisioning_byod_workapps_visible_instruction,
                new Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_HOME),
                R.drawable.badged_icon);

        mWorkNotificationBadgedTest = new TestItemWithIcon(this,
                R.string.provisioning_byod_work_notification,
                R.string.provisioning_byod_work_notification_instruction,
                new Intent(WorkNotificationTestActivity.ACTION_WORK_NOTIFICATION),
                R.drawable.ic_corp_icon);

        mDisableNonMarketTest = new TestItem(this, R.string.provisioning_byod_nonmarket_deny,
                R.string.provisioning_byod_nonmarket_deny_info,
                new Intent(ByodHelperActivity.ACTION_INSTALL_APK)
                        .putExtra(ByodHelperActivity.EXTRA_ALLOW_NON_MARKET_APPS, false));

        mEnableNonMarketTest = new TestItem(this, R.string.provisioning_byod_nonmarket_allow,
                R.string.provisioning_byod_nonmarket_allow_info,
                new Intent(ByodHelperActivity.ACTION_INSTALL_APK)
                        .putExtra(ByodHelperActivity.EXTRA_ALLOW_NON_MARKET_APPS, true));

        mProfileAccountVisibleTest = new TestItem(this, R.string.provisioning_byod_profile_visible,
                R.string.provisioning_byod_profile_visible_instruction,
                new Intent(Settings.ACTION_SETTINGS));

        mAppSettingsVisibleTest = new TestItem(this, R.string.provisioning_byod_app_settings,
                R.string.provisioning_byod_app_settings_instruction,
                new Intent(Settings.ACTION_APPLICATION_SETTINGS));

        mDeviceAdminVisibleTest = new TestItem(this, R.string.provisioning_byod_admin_visible,
                R.string.provisioning_byod_admin_visible_instruction,
                new Intent(Settings.ACTION_SECURITY_SETTINGS));

        mCredSettingsVisibleTest = new TestItem(this, R.string.provisioning_byod_cred_settings,
                R.string.provisioning_byod_cred_settings_instruction,
                new Intent(Settings.ACTION_SECURITY_SETTINGS));

        mLocationSettingsVisibleTest = new TestItem(this,
                R.string.provisioning_byod_location_settings,
                R.string.provisioning_byod_location_settings_instruction,
                new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS));

        mPrintSettingsVisibleTest = new TestItem(this, R.string.provisioning_byod_print_settings,
                R.string.provisioning_byod_print_settings_instruction,
                new Intent(Settings.ACTION_PRINT_SETTINGS));

        Intent intent = new Intent(CrossProfileTestActivity.ACTION_CROSS_PROFILE);
        Intent chooser = Intent.createChooser(intent,
                getResources().getString(R.string.provisioning_cross_profile_chooser));
        mCrossProfileIntentFiltersTest = new TestItem(this,
                R.string.provisioning_byod_cross_profile,
                R.string.provisioning_byod_cross_profile_instruction,
                chooser);

        mTests.add(mProfileOwnerInstalled);

        // Badge related tests
        mTests.add(mWorkAppVisibleTest);
        mTests.add(mWorkNotificationBadgedTest);

        // Settings related tests.
        mTests.add(mProfileAccountVisibleTest);
        mTests.add(mDeviceAdminVisibleTest);
        mTests.add(mCredSettingsVisibleTest);
        mTests.add(mAppSettingsVisibleTest);
        mTests.add(mLocationSettingsVisibleTest);
        mTests.add(mPrintSettingsVisibleTest);

        mTests.add(mCrossProfileIntentFiltersTest);
        mTests.add(mDisableNonMarketTest);
        mTests.add(mEnableNonMarketTest);
    }

    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        super.onListItemClick(l, v, position, id);
        mCurrentTestPosition = position;
        TestItem test = (TestItem) getListAdapter().getItem(position);
        test.performTest(this);
    }

    private void showManualTestDialog(final TestItem test) {
        AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(this)
                .setIcon(android.R.drawable.ic_dialog_info)
                .setTitle(R.string.provisioning_byod)
                .setNeutralButton(R.string.provisioning_byod_go, null)
                .setPositiveButton(R.string.pass_button_text, new AlertDialog.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        clearRemainingState(test);
                        setTestResult(test, TestResult.Passed);
                    }
                })
                .setNegativeButton(R.string.fail_button_text, new AlertDialog.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        clearRemainingState(test);
                        setTestResult(test, TestResult.Failed);
                    }
                });
        View customView = test.getCustomView();
        if (customView != null) {
            dialogBuilder.setView(customView);
        } else {
            dialogBuilder.setMessage(test.getManualTestInstruction());
        }
        final AlertDialog dialog = dialogBuilder.show();
        // Note: Setting the OnClickListener on the Dialog rather than the Builder, prevents the
        // dialog being dismissed on onClick.
        dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                try {
                    ByodFlowTestActivity.this.startActivity(test.getManualTestIntent());
                } catch (ActivityNotFoundException e) {
                    Toast.makeText(ByodFlowTestActivity.this,
                            "Cannot start " + test.getManualTestIntent(), Toast.LENGTH_LONG).show();
                    setTestResult(test, TestResult.Failed);
                    dialog.dismiss();
                }
            }
        });
    }

    private void clearRemainingState(final TestItem test) {
        if (WorkNotificationTestActivity.ACTION_WORK_NOTIFICATION.equals(
                test.getManualTestIntent().getAction())) {
            try {
                ByodFlowTestActivity.this.startActivity(new Intent(
                        WorkNotificationTestActivity.ACTION_CLEAR_WORK_NOTIFICATION));
            } catch (ActivityNotFoundException e) {
                // User shouldn't run this test before work profile is set up.
            }
        }
    }

    private void setTestResult(TestItem test, TestResult result) {
        test.setPassFailState(result);

        boolean testSucceeds = true;
        for(TestItem aTest : mTests) {
            testSucceeds &= (aTest.getPassFailState() == TestResult.Passed);
        }
        getPassButton().setEnabled(testSucceeds);
        mTestListAdapter.notifyDataSetChanged();

        this.getListView().smoothScrollToPosition(mCurrentTestPosition + 1);
    }

    private void startByodProvisioning() {
        Intent sending = new Intent(DevicePolicyManager.ACTION_PROVISION_MANAGED_PROFILE);
        sending.putExtra(DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_NAME,
                mAdminReceiverComponent.getPackageName());
        sending.putExtra(DevicePolicyManager.EXTRA_DEVICE_ADMIN, mAdminReceiverComponent);

        if (sending.resolveActivity(getPackageManager()) != null) {
            // ManagedProvisioning must be started with startActivityForResult, but we don't
            // care about the result, so passing 0 as a requestCode
            startActivityForResult(sending, 0);
        } else {
            showToast(R.string.provisioning_byod_disabled);
        }
    }

    private void queryProfileOwner(boolean showToast) {
        try {
            Intent intent = new Intent(ByodHelperActivity.ACTION_QUERY_PROFILE_OWNER);
            startActivityForResult(intent, REQUEST_STATUS);
        }
        catch (ActivityNotFoundException e) {
            Log.d(TAG, "queryProfileOwner: ActivityNotFoundException", e);
            setTestResult(mProfileOwnerInstalled, TestResult.Failed);
            if (showToast) {
                showToast(R.string.provisioning_byod_no_activity);
            }
        }
    }

    private void requestDeleteProfileOwner() {
        try {
            Intent intent = new Intent(ByodHelperActivity.ACTION_REMOVE_PROFILE_OWNER);
            startActivity(intent);
            showToast(R.string.provisioning_byod_delete_profile);
        }
        catch (ActivityNotFoundException e) {
            Log.d(TAG, "requestDeleteProfileOwner: ActivityNotFoundException", e);
        }
    }

    private void disableComponent() {
        // Disable app components in the current profile, so only the counterpart in the other profile
        // can respond (via cross-profile intent filter)
        getPackageManager().setComponentEnabledSetting(new ComponentName(
                this, ByodHelperActivity.class),
                PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                PackageManager.DONT_KILL_APP);
        getPackageManager().setComponentEnabledSetting(new ComponentName(
                this, WorkNotificationTestActivity.class),
                PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                PackageManager.DONT_KILL_APP);
    }

    private void showToast(int messageId) {
        String message = getString(messageId);
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
    }

    enum TestResult {
        Unknown, Failed, Passed
    }

    static class TestItem {

        private String mDisplayName;
        private TestResult mPassed;
        private boolean mManualTest;
        private String mManualInstruction;
        private Intent mManualIntent;

        public TestItem(Context context, int nameResId) {
            mDisplayName = context.getString(nameResId);
            mPassed = TestResult.Unknown;
            mManualTest = false;
        }

        public void performTest(ByodFlowTestActivity activity) {
            if (isManualTest()) {
                activity.showManualTestDialog(this);
            }
        }

        public TestItem(Context context, int nameResId, int testInstructionResId, Intent testIntent) {
            mDisplayName = context.getString(nameResId);
            mPassed = TestResult.Unknown;
            mManualTest = true;
            mManualInstruction = context.getString(testInstructionResId);
            mManualIntent = testIntent;
        }

        @Override
        public String toString() {
            return mDisplayName;
        }

        TestResult getPassFailState() {
            return mPassed;
        }

        void setPassFailState(TestResult state) {
            mPassed = state;
        }

        public boolean isManualTest() {
            return mManualTest;
        }

        public String getManualTestInstruction() {
            return mManualInstruction;
        }

        public Intent getManualTestIntent() {
            return mManualIntent;
        }

        public View getCustomView() {
            return null;
        }
    }

    static class TestItemWithIcon extends TestItem {

        private int mImageResId;
        private Context mContext;

        public TestItemWithIcon(Context context, int nameResId, int testInstructionResId,
                Intent testIntent, int imageResId) {
            super(context, nameResId, testInstructionResId, testIntent);
            mContext = context;
            mImageResId = imageResId;
        }

        @Override
        public View getCustomView() {
            LayoutInflater layoutInflater = LayoutInflater.from(mContext);
            View view = layoutInflater.inflate(R.layout.byod_custom_view,
                    null /* root */);
            ((ImageView) view.findViewById(R.id.sample_icon)).setImageResource(mImageResId);
            ((TextView) view.findViewById(R.id.message)).setText(getManualTestInstruction());
            return view;
        }
    }

    static class TestAdapter extends ArrayAdapter<TestItem> {

        public TestAdapter(Context context) {
            super(context, android.R.layout.simple_list_item_1);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            TextView view = (TextView) super.getView(position, convertView, parent);

            TestItem item = getItem(position);
            int backgroundResource = 0;
            int iconResource = 0;
            if (item.getPassFailState() == TestResult.Passed) {
                backgroundResource = R.drawable.test_pass_gradient;
                iconResource = R.drawable.fs_good;
            } else if (item.getPassFailState() == TestResult.Failed){
                backgroundResource = R.drawable.test_fail_gradient;
                iconResource = R.drawable.fs_error;
            }
            view.setBackgroundResource(backgroundResource);
            view.setPadding(10, 0, 10, 0);
            view.setCompoundDrawablePadding(10);
            view.setCompoundDrawablesWithIntrinsicBounds(0, 0, iconResource, 0);

            return view;
        }
    }
}
