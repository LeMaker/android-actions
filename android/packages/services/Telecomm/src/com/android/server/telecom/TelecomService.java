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

package com.android.server.telecom;

import android.Manifest;
import android.annotation.SdkConstant;
import android.app.AppOpsManager;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.net.Uri;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.UserHandle;
import android.os.UserManager;
import android.telecom.CallState;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

// TODO: Needed for move to system service: import com.android.internal.R;
import com.android.internal.telecom.ITelecomService;
import com.android.internal.util.IndentingPrintWriter;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

/**
 * Implementation of the ITelecom interface.
 */
public class TelecomService extends Service {
    /**
     * The {@link Intent} that must be declared as handled by the service.
     */
    @SdkConstant(SdkConstant.SdkConstantType.SERVICE_ACTION)
    public static final String SERVICE_INTERFACE = "android.telecom.ITelecomService";

    /** The context. */
    private Context mContext;

    /**
     * A request object for use with {@link MainThreadHandler}. Requesters should wait() on the
     * request after sending. The main thread will notify the request when it is complete.
     */
    private static final class MainThreadRequest {
        /** The result of the request that is run on the main thread */
        public Object result;
        /** Object that can be used to store non-integer arguments */
        public Object arg;
    }

    /**
     * A handler that processes messages on the main thread. Since many of the method calls are not
     * thread safe this is needed to shuttle the requests from the inbound binder threads to the
     * main thread.
     */
    private final class MainThreadHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            if (msg.obj instanceof MainThreadRequest) {
                MainThreadRequest request = (MainThreadRequest) msg.obj;
                Object result = null;
                switch (msg.what) {
                    case MSG_SILENCE_RINGER:
                        mCallsManager.getRinger().silence();
                        break;
                    case MSG_SHOW_CALL_SCREEN:
                        mCallsManager.getInCallController().bringToForeground(msg.arg1 == 1);
                        break;
                    case MSG_END_CALL:
                        result = endCallInternal();
                        break;
                    case MSG_ACCEPT_RINGING_CALL:
                        acceptRingingCallInternal();
                        break;
                    case MSG_CANCEL_MISSED_CALLS_NOTIFICATION:
                        mMissedCallNotifier.clearMissedCalls();
                        break;
                    case MSG_IS_TTY_SUPPORTED:
                        result = mCallsManager.isTtySupported();
                        break;
                    case MSG_GET_CURRENT_TTY_MODE:
                        result = mCallsManager.getCurrentTtyMode();
                        break;
                    case MSG_NEW_INCOMING_CALL:
                        if (request.arg == null || !(request.arg instanceof Intent)) {
                            Log.w(this, "Invalid new incoming call request");
                            break;
                        }
                        CallReceiver.processIncomingCallIntent((Intent) request.arg);
                        break;
                }

                if (result != null) {
                    request.result = result;
                    synchronized(request) {
                        request.notifyAll();
                    }
                }
            }
        }
    }

    private static final String TAG = TelecomService.class.getSimpleName();

    private static final String SERVICE_NAME = "telecom";

    private static final int MSG_SILENCE_RINGER = 1;
    private static final int MSG_SHOW_CALL_SCREEN = 2;
    private static final int MSG_END_CALL = 3;
    private static final int MSG_ACCEPT_RINGING_CALL = 4;
    private static final int MSG_CANCEL_MISSED_CALLS_NOTIFICATION = 5;
    private static final int MSG_IS_TTY_SUPPORTED = 6;
    private static final int MSG_GET_CURRENT_TTY_MODE = 7;
    private static final int MSG_NEW_INCOMING_CALL = 8;

    private final MainThreadHandler mMainThreadHandler = new MainThreadHandler();

    private CallsManager mCallsManager;
    private MissedCallNotifier mMissedCallNotifier;
    private PhoneAccountRegistrar mPhoneAccountRegistrar;
    private AppOpsManager mAppOpsManager;
    private UserManager mUserManager;
    private PackageManager mPackageManager;
    private TelecomServiceImpl mServiceImpl;

    @Override
    public void onCreate() {
        super.onCreate();

        Log.d(this, "onCreate");
        mContext = this;
        mAppOpsManager = (AppOpsManager) mContext.getSystemService(Context.APP_OPS_SERVICE);
        mServiceImpl = new TelecomServiceImpl();

        TelecomGlobals globals = TelecomGlobals.getInstance();
        globals.initialize(this);

        mMissedCallNotifier = globals.getMissedCallNotifier();
        mPhoneAccountRegistrar = globals.getPhoneAccountRegistrar();
        mCallsManager = globals.getCallsManager();
        mUserManager = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
        mPackageManager = mContext.getPackageManager();
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(this, "onBind");
        return mServiceImpl;
    }

    /**
     * Implementation of the ITelecomService interface.
     * TODO: Reorganize this inner class to top of file.
     */
    class TelecomServiceImpl extends ITelecomService.Stub {
        @Override
        public PhoneAccountHandle getDefaultOutgoingPhoneAccount(String uriScheme) {
            enforceReadPermission();
            long token = Binder.clearCallingIdentity();
            try {
                PhoneAccountHandle defaultOutgoingPhoneAccount =
                        mPhoneAccountRegistrar.getDefaultOutgoingPhoneAccount(uriScheme);
                // Make sure that the calling user can see this phone account.
                if (defaultOutgoingPhoneAccount != null
                        && !isVisibleToCaller(defaultOutgoingPhoneAccount)) {
                    Log.w(this, "No account found for the calling user");
                    return null;
                }
                return defaultOutgoingPhoneAccount;
            } catch (Exception e) {
                Log.e(this, e, "getDefaultOutgoingPhoneAccount");
                throw e;
            } finally {
                Binder.restoreCallingIdentity(token);
            }
        }

        @Override
        public PhoneAccountHandle getUserSelectedOutgoingPhoneAccount() {
            try {
                PhoneAccountHandle userSelectedOutgoingPhoneAccount =
                        mPhoneAccountRegistrar.getUserSelectedOutgoingPhoneAccount();
                // Make sure that the calling user can see this phone account.
                if (!isVisibleToCaller(userSelectedOutgoingPhoneAccount)) {
                    Log.w(this, "No account found for the calling user");
                    return null;
                }
                return userSelectedOutgoingPhoneAccount;
            } catch (Exception e) {
                Log.e(this, e, "getUserSelectedOutgoingPhoneAccount");
                throw e;
            }
        }

        @Override
        public void setUserSelectedOutgoingPhoneAccount(PhoneAccountHandle accountHandle) {
            enforceModifyPermission();

            try {
                mPhoneAccountRegistrar.setUserSelectedOutgoingPhoneAccount(accountHandle);
            } catch (Exception e) {
                Log.e(this, e, "setUserSelectedOutgoingPhoneAccount");
                throw e;
            }
        }

        @Override
        public List<PhoneAccountHandle> getCallCapablePhoneAccounts() {
            enforceReadPermission();
            long token = Binder.clearCallingIdentity();
            try {
                return filterForAccountsVisibleToCaller(
                        mPhoneAccountRegistrar.getCallCapablePhoneAccounts());
            } catch (Exception e) {
                Log.e(this, e, "getCallCapablePhoneAccounts");
                throw e;
            } finally {
                Binder.restoreCallingIdentity(token);
            }
        }

        @Override
        public List<PhoneAccountHandle> getPhoneAccountsSupportingScheme(String uriScheme) {
            enforceReadPermission();
            long token = Binder.clearCallingIdentity();
            try {
                return filterForAccountsVisibleToCaller(
                        mPhoneAccountRegistrar.getCallCapablePhoneAccounts(uriScheme));
            } catch (Exception e) {
                Log.e(this, e, "getPhoneAccountsSupportingScheme %s", uriScheme);
                throw e;
            } finally {
                Binder.restoreCallingIdentity(token);
            }
        }

        @Override
        public List<PhoneAccountHandle> getPhoneAccountsForPackage(String packageName) {
            try {
                return filterForAccountsVisibleToCaller(
                        mPhoneAccountRegistrar.getPhoneAccountsForPackage(packageName));
            } catch (Exception e) {
                Log.e(this, e, "getPhoneAccountsForPackage %s", packageName);
                throw e;
            }
        }

        @Override
        public PhoneAccount getPhoneAccount(PhoneAccountHandle accountHandle) {
            try {
                if (!isVisibleToCaller(accountHandle)) {
                    Log.w(this, "%s is not visible for the calling user", accountHandle);
                    return null;
                }
                return mPhoneAccountRegistrar.getPhoneAccountInternal(accountHandle);
            } catch (Exception e) {
                Log.e(this, e, "getPhoneAccount %s", accountHandle);
                throw e;
            }
        }

        @Override
        public int getAllPhoneAccountsCount() {
            try {
                // This list is pre-filtered for the calling user.
                return getAllPhoneAccounts().size();
            } catch (Exception e) {
                Log.e(this, e, "getAllPhoneAccountsCount");
                throw e;
            }
        }

        @Override
        public List<PhoneAccount> getAllPhoneAccounts() {
            try {
                List<PhoneAccount> allPhoneAccounts = mPhoneAccountRegistrar.getAllPhoneAccounts();
                List<PhoneAccount> profilePhoneAccounts = new ArrayList<>(allPhoneAccounts.size());
                for (PhoneAccount phoneAccount : profilePhoneAccounts) {
                    if (isVisibleToCaller(phoneAccount)) {
                        profilePhoneAccounts.add(phoneAccount);
                    }
                }
                return profilePhoneAccounts;
            } catch (Exception e) {
                Log.e(this, e, "getAllPhoneAccounts");
                throw e;
            }
        }

        @Override
        public List<PhoneAccountHandle> getAllPhoneAccountHandles() {
            try {
                return filterForAccountsVisibleToCaller(
                        mPhoneAccountRegistrar.getAllPhoneAccountHandles());
            } catch (Exception e) {
                Log.e(this, e, "getAllPhoneAccounts");
                throw e;
            }
        }

        @Override
        public PhoneAccountHandle getSimCallManager() {
            try {
                PhoneAccountHandle accountHandle = mPhoneAccountRegistrar.getSimCallManager();
                if (!isVisibleToCaller(accountHandle)) {
                    Log.w(this, "%s is not visible for the calling user", accountHandle);
                    return null;
                }
                return accountHandle;
            } catch (Exception e) {
                Log.e(this, e, "getSimCallManager");
                throw e;
            }
        }

        @Override
        public void setSimCallManager(PhoneAccountHandle accountHandle) {
            enforceModifyPermission();

            try {
                mPhoneAccountRegistrar.setSimCallManager(accountHandle);
            } catch (Exception e) {
                Log.e(this, e, "setSimCallManager");
                throw e;
            }
        }

        @Override
        public List<PhoneAccountHandle> getSimCallManagers() {
            enforceReadPermission();
            long token = Binder.clearCallingIdentity();
            try {
                return filterForAccountsVisibleToCaller(
                        mPhoneAccountRegistrar.getConnectionManagerPhoneAccounts());
            } catch (Exception e) {
                Log.e(this, e, "getSimCallManagers");
                throw e;
            } finally {
                Binder.restoreCallingIdentity(token);
            }
        }

        @Override
        public void registerPhoneAccount(PhoneAccount account) {
            try {
                enforcePhoneAccountModificationForPackage(
                        account.getAccountHandle().getComponentName().getPackageName());
                if (account.hasCapabilities(PhoneAccount.CAPABILITY_CALL_PROVIDER)) {
                    enforceRegisterCallProviderPermission();
                }
                if (account.hasCapabilities(PhoneAccount.CAPABILITY_SIM_SUBSCRIPTION)) {
                    enforceRegisterSimSubscriptionPermission();
                }
                if (account.hasCapabilities(PhoneAccount.CAPABILITY_CONNECTION_MANAGER)) {
                    enforceRegisterConnectionManagerPermission();
                }
                if (account.hasCapabilities(PhoneAccount.CAPABILITY_MULTI_USER)) {
                    enforceRegisterMultiUser();
                }
                enforceUserHandleMatchesCaller(account.getAccountHandle());

                mPhoneAccountRegistrar.registerPhoneAccount(account);
            } catch (Exception e) {
                Log.e(this, e, "registerPhoneAccount %s", account);
                throw e;
            }
        }

        @Override
        public void unregisterPhoneAccount(PhoneAccountHandle accountHandle) {
            try {
                enforcePhoneAccountModificationForPackage(
                        accountHandle.getComponentName().getPackageName());
                enforceUserHandleMatchesCaller(accountHandle);
                mPhoneAccountRegistrar.unregisterPhoneAccount(accountHandle);
            } catch (Exception e) {
                Log.e(this, e, "unregisterPhoneAccount %s", accountHandle);
                throw e;
            }
        }

        @Override
        public void clearAccounts(String packageName) {
            try {
                enforcePhoneAccountModificationForPackage(packageName);
                mPhoneAccountRegistrar.clearAccounts(packageName, Binder.getCallingUserHandle());
            } catch (Exception e) {
                Log.e(this, e, "clearAccounts %s", packageName);
                throw e;
            }
        }

        /**
         * @see android.telecom.TelecomManager#isVoiceMailNumber
         */
        @Override
        public boolean isVoiceMailNumber(PhoneAccountHandle accountHandle, String number) {
            enforceReadPermissionOrDefaultDialer();
            try {
                if (!isVisibleToCaller(accountHandle)) {
                    Log.w(this, "%s is not visible for the calling user", accountHandle);
                    return false;
                }
                return mPhoneAccountRegistrar.isVoiceMailNumber(accountHandle, number);
            } catch (Exception e) {
                Log.e(this, e, "getSubscriptionIdForPhoneAccount");
                throw e;
            }
        }

        /**
         * @see android.telecom.TelecomManager#hasVoiceMailNumber
         */
        @Override
        public boolean hasVoiceMailNumber(PhoneAccountHandle accountHandle) {
            enforceReadPermissionOrDefaultDialer();
            try {
                if (!isVisibleToCaller(accountHandle)) {
                    Log.w(this, "%s is not visible for the calling user", accountHandle);
                    return false;
                }

                int subId = mPhoneAccountRegistrar.getSubscriptionIdForPhoneAccount(accountHandle);
                return !TextUtils.isEmpty(getTelephonyManager().getVoiceMailNumber(subId));
            } catch (Exception e) {
                Log.e(this, e, "getSubscriptionIdForPhoneAccount");
                throw e;
            }
        }

        /**
         * @see android.telecom.TelecomManager#getLine1Number
         */
        @Override
        public String getLine1Number(PhoneAccountHandle accountHandle) {
            enforceReadPermissionOrDefaultDialer();
            try {
                if (!isVisibleToCaller(accountHandle)) {
                    Log.w(this, "%s is not visible for the calling user", accountHandle);
                    return null;
                }
                int subId = mPhoneAccountRegistrar.getSubscriptionIdForPhoneAccount(accountHandle);
                return getTelephonyManager().getLine1NumberForSubscriber(subId);
            } catch (Exception e) {
                Log.e(this, e, "getSubscriptionIdForPhoneAccount");
                throw e;
            }
        }

        /**
         * @see android.telecom.TelecomManager#silenceRinger
         */
        @Override
        public void silenceRinger() {
            Log.d(this, "silenceRinger");
            enforceModifyPermission();
            sendRequestAsync(MSG_SILENCE_RINGER, 0);
        }

        /**
         * @see android.telecom.TelecomManager#getDefaultPhoneApp
         */
        @Override
        public ComponentName getDefaultPhoneApp() {
            Resources resources = mContext.getResources();
            return new ComponentName(
                    resources.getString(R.string.ui_default_package),
                    resources.getString(R.string.dialer_default_class));
        }

        /**
         * @see android.telecom.TelecomManager#isInCall
         */
        @Override
        public boolean isInCall() {
            enforceReadPermission();
            // Do not use sendRequest() with this method since it could cause a deadlock with
            // audio service, which we call into from the main thread: AudioManager.setMode().
            final int callState = mCallsManager.getCallState();
            return callState == TelephonyManager.CALL_STATE_OFFHOOK
                    || callState == TelephonyManager.CALL_STATE_RINGING;
        }

        /**
         * @see android.telecom.TelecomManager#isRinging
         */
        @Override
        public boolean isRinging() {
            enforceReadPermission();
            return mCallsManager.getCallState() == TelephonyManager.CALL_STATE_RINGING;
        }

        /**
         * @see TelecomManager#getCallState
         */
        @Override
        public int getCallState() {
            return mCallsManager.getCallState();
        }

        /**
         * @see android.telecom.TelecomManager#endCall
         */
        @Override
        public boolean endCall() {
            enforceModifyPermission();
            return (boolean) sendRequest(MSG_END_CALL);
        }

        /**
         * @see android.telecom.TelecomManager#acceptRingingCall
         */
        @Override
        public void acceptRingingCall() {
            enforceModifyPermission();
            sendRequestAsync(MSG_ACCEPT_RINGING_CALL, 0);
        }

        /**
         * @see android.telecom.TelecomManager#showInCallScreen
         */
        @Override
        public void showInCallScreen(boolean showDialpad) {
            enforceReadPermissionOrDefaultDialer();
            sendRequestAsync(MSG_SHOW_CALL_SCREEN, showDialpad ? 1 : 0);
        }

        /**
         * @see android.telecom.TelecomManager#cancelMissedCallsNotification
         */
        @Override
        public void cancelMissedCallsNotification() {
            enforceModifyPermissionOrDefaultDialer();
            sendRequestAsync(MSG_CANCEL_MISSED_CALLS_NOTIFICATION, 0);
        }

        /**
         * @see android.telecom.TelecomManager#handleMmi
         */
        @Override
        public boolean handlePinMmi(String dialString) {
            enforceModifyPermissionOrDefaultDialer();

            // Switch identity so that TelephonyManager checks Telecom's permissions instead.
            long token = Binder.clearCallingIdentity();
            boolean retval = false;
            try {
                retval = getTelephonyManager().handlePinMmi(dialString);
            } finally {
                Binder.restoreCallingIdentity(token);
            }

            return retval;
        }

        /**
         * @see android.telecom.TelecomManager#handleMmi
         */
        @Override
        public boolean handlePinMmiForPhoneAccount(PhoneAccountHandle accountHandle,
                String dialString) {
            enforceModifyPermissionOrDefaultDialer();

            if (!isVisibleToCaller(accountHandle)) {
                Log.w(this, "%s is not visible for the calling user", accountHandle);
                return false;
            }

            // Switch identity so that TelephonyManager checks Telecom's permissions instead.
            long token = Binder.clearCallingIdentity();
            boolean retval = false;
            try {
                int subId = mPhoneAccountRegistrar.getSubscriptionIdForPhoneAccount(accountHandle);
                retval = getTelephonyManager().handlePinMmiForSubscriber(subId, dialString);
            } finally {
                Binder.restoreCallingIdentity(token);
            }

            return retval;
        }

        /**
         * @see android.telecom.TelecomManager#getAdnUriForPhoneAccount
         */
        @Override
        public Uri getAdnUriForPhoneAccount(PhoneAccountHandle accountHandle) {
            enforceModifyPermissionOrDefaultDialer();

            if (!isVisibleToCaller(accountHandle)) {
                Log.w(this, "%s is not visible for the calling user", accountHandle);
                return null;
            }

            // Switch identity so that TelephonyManager checks Telecom's permissions instead.
            long token = Binder.clearCallingIdentity();
            String retval = "content://icc/adn/";
            try {
                long subId = mPhoneAccountRegistrar.getSubscriptionIdForPhoneAccount(accountHandle);
                retval = retval + "subId/" + subId;
            } finally {
                Binder.restoreCallingIdentity(token);
            }

            return Uri.parse(retval);
        }

        /**
         * @see android.telecom.TelecomManager#isTtySupported
         */
        @Override
        public boolean isTtySupported() {
            enforceReadPermission();
            return (boolean) sendRequest(MSG_IS_TTY_SUPPORTED);
        }

        /**
         * @see android.telecom.TelecomManager#getCurrentTtyMode
         */
        @Override
        public int getCurrentTtyMode() {
            enforceReadPermission();
            return (int) sendRequest(MSG_GET_CURRENT_TTY_MODE);
        }

        /**
         * @see android.telecom.TelecomManager#addNewIncomingCall
         */
        @Override
        public void addNewIncomingCall(PhoneAccountHandle phoneAccountHandle, Bundle extras) {
            Log.i(this, "Adding new incoming call with phoneAccountHandle %s", phoneAccountHandle);
            if (phoneAccountHandle != null && phoneAccountHandle.getComponentName() != null) {
                mAppOpsManager.checkPackage(
                        Binder.getCallingUid(), phoneAccountHandle.getComponentName().getPackageName());

                // Make sure it doesn't cross the UserHandle boundary
                enforceUserHandleMatchesCaller(phoneAccountHandle);

                Intent intent = new Intent(TelecomManager.ACTION_INCOMING_CALL);
                intent.putExtra(TelecomManager.EXTRA_PHONE_ACCOUNT_HANDLE, phoneAccountHandle);
                intent.putExtra(CallReceiver.KEY_IS_INCOMING_CALL, true);
                if (extras != null) {
                    intent.putExtra(TelecomManager.EXTRA_INCOMING_CALL_EXTRAS, extras);
                }
                sendRequestAsync(MSG_NEW_INCOMING_CALL, 0, intent);
            } else {
                Log.w(this, "Null phoneAccountHandle. Ignoring request to add new incoming call");
            }
        }

        /**
         * @see android.telecom.TelecomManager#addNewUnknownCall
         */
        @Override
        public void addNewUnknownCall(PhoneAccountHandle phoneAccountHandle, Bundle extras) {
            if (phoneAccountHandle != null && phoneAccountHandle.getComponentName() != null &&
                    TelephonyUtil.isPstnComponentName(phoneAccountHandle.getComponentName())) {
                mAppOpsManager.checkPackage(
                        Binder.getCallingUid(), phoneAccountHandle.getComponentName().getPackageName());

                // Make sure it doesn't cross the UserHandle boundary
                enforceUserHandleMatchesCaller(phoneAccountHandle);

                Intent intent = new Intent(TelecomManager.ACTION_NEW_UNKNOWN_CALL);
                intent.setClass(mContext, CallReceiver.class);
                intent.setFlags(Intent.FLAG_RECEIVER_FOREGROUND);
                intent.putExtras(extras);
                intent.putExtra(CallReceiver.KEY_IS_UNKNOWN_CALL, true);
                intent.putExtra(TelecomManager.EXTRA_PHONE_ACCOUNT_HANDLE, phoneAccountHandle);
                mContext.sendBroadcastAsUser(intent, phoneAccountHandle.getUserHandle());
            } else {
                Log.i(this, "Null phoneAccountHandle or not initiated by Telephony. Ignoring request"
                        + " to add new unknown call.");
            }
        }

        /**
         * Dumps the current state of the TelecomService.  Used when generating problem reports.
         *
         * @param fd The file descriptor.
         * @param writer The print writer to dump the state to.
         * @param args Optional dump arguments.
         */
        @Override
        protected void dump(FileDescriptor fd, final PrintWriter writer, String[] args) {
            if (mContext.checkCallingOrSelfPermission(
                    android.Manifest.permission.DUMP)
                    != PackageManager.PERMISSION_GRANTED) {
                writer.println("Permission Denial: can't dump TelecomService " +
                        "from from pid=" + Binder.getCallingPid() + ", uid=" +
                        Binder.getCallingUid());
                return;
            }

            final IndentingPrintWriter pw = new IndentingPrintWriter(writer, "  ");
            if (mCallsManager != null) {
                pw.println("mCallsManager: ");
                pw.increaseIndent();
                mCallsManager.dump(pw);
                pw.decreaseIndent();

                pw.println("mPhoneAccountRegistrar: ");
                pw.increaseIndent();
                mPhoneAccountRegistrar.dump(pw);
                pw.decreaseIndent();
            }
        }
    }

    //
    // Supporting methods for the ITelecomService interface implementation.
    //

    private boolean isVisibleToCaller(PhoneAccountHandle accountHandle) {
        if (accountHandle == null) {
            return false;
        }

        return isVisibleToCaller(mPhoneAccountRegistrar.getPhoneAccountInternal(accountHandle));
    }

    private boolean isVisibleToCaller(PhoneAccount account) {
        if (account == null) {
            return false;
        }

        // If this PhoneAccount has CAPABILITY_MULTI_USER, it should be visible to all users and
        // all profiles. Only Telephony and SIP accounts should have this capability.
        if (account.hasCapabilities(PhoneAccount.CAPABILITY_MULTI_USER)) {
            return true;
        }

        UserHandle phoneAccountUserHandle = account.getAccountHandle().getUserHandle();
        if (phoneAccountUserHandle == null) {
            return false;
        }

        List<UserHandle> profileUserHandles;
        if (isCallerSystemApp()) {
            // If the caller lives in /system/priv-app, it can see PhoneAccounts for all of the
            // *profiles* that the calling user owns, but not for any other *users*.
            profileUserHandles = mUserManager.getUserProfiles();
        } else {
            // Otherwise, it has to be owned by the current caller's profile.
            profileUserHandles = new ArrayList<>(1);
            profileUserHandles.add(Binder.getCallingUserHandle());
        }

        return profileUserHandles.contains(phoneAccountUserHandle);
    }

    /**
     * Given a list of {@link PhoneAccountHandle}s, filter them to the ones that the calling
     * user can see.
     *
     * @param phoneAccountHandles Unfiltered list of account handles.
     *
     * @return {@link PhoneAccountHandle}s visible to the calling user and its profiles.
     */
    private List<PhoneAccountHandle> filterForAccountsVisibleToCaller(
            List<PhoneAccountHandle> phoneAccountHandles) {
        List<PhoneAccountHandle> profilePhoneAccountHandles =
                new ArrayList<>(phoneAccountHandles.size());
        for (PhoneAccountHandle phoneAccountHandle : phoneAccountHandles) {
            if (isVisibleToCaller(phoneAccountHandle)) {
                profilePhoneAccountHandles.add(phoneAccountHandle);
            }
        }
        return profilePhoneAccountHandles;
    }

    private boolean isCallerSystemApp() {
        int uid = Binder.getCallingUid();
        String[] packages = mPackageManager.getPackagesForUid(uid);
        for (String packageName : packages) {
            if (isPackageSystemApp(packageName)) {
                return true;
            }
        }
        return false;
    }

    private boolean isPackageSystemApp(String packageName) {
        try {
            ApplicationInfo applicationInfo = mPackageManager.getApplicationInfo(packageName,
                    PackageManager.GET_META_DATA);
            if ((applicationInfo.flags & ApplicationInfo.FLAG_SYSTEM) != 0) {
                return true;
            }
        } catch (PackageManager.NameNotFoundException e) {
        }
        return false;
    }

    private void acceptRingingCallInternal() {
        Call call = mCallsManager.getFirstCallWithState(CallState.RINGING);
        if (call != null) {
            call.answer(call.getVideoState());
        }
    }

    private boolean endCallInternal() {
        // Always operate on the foreground call if one exists, otherwise get the first call in
        // priority order by call-state.
        Call call = mCallsManager.getForegroundCall();
        if (call == null) {
            call = mCallsManager.getFirstCallWithState(
                    CallState.ACTIVE,
                    CallState.DIALING,
                    CallState.RINGING,
                    CallState.ON_HOLD);
        }

        if (call != null) {
            if (call.getState() == CallState.RINGING) {
                call.reject(false /* rejectWithMessage */, null);
            } else {
                call.disconnect();
            }
            return true;
        }

        return false;
    }

    private void enforcePhoneAccountModificationForPackage(String packageName) {
        // TODO: Use a new telecomm permission for this instead of reusing modify.

        int result = mContext.checkCallingOrSelfPermission(Manifest.permission.MODIFY_PHONE_STATE);

        // Callers with MODIFY_PHONE_STATE can use the PhoneAccount mechanism to implement
        // built-in behavior even when PhoneAccounts are not exposed as a third-part API. They
        // may also modify PhoneAccounts on behalf of any 'packageName'.

        if (result != PackageManager.PERMISSION_GRANTED) {
            // Other callers are only allowed to modify PhoneAccounts if the relevant system
            // feature is enabled ...
            enforceConnectionServiceFeature();
            // ... and the PhoneAccounts they refer to are for their own package.
            enforceCallingPackage(packageName);
        }
    }

    private void enforceReadPermissionOrDefaultDialer() {
        if (!isDefaultDialerCalling()) {
            enforceReadPermission();
        }
    }

    private void enforceModifyPermissionOrDefaultDialer() {
        if (!isDefaultDialerCalling()) {
            enforceModifyPermission();
        }
    }

    private void enforceCallingPackage(String packageName) {
        mAppOpsManager.checkPackage(Binder.getCallingUid(), packageName);
    }

    private void enforceConnectionServiceFeature() {
        enforceFeature(PackageManager.FEATURE_CONNECTION_SERVICE);
    }

    private void enforceRegisterCallProviderPermission() {
        enforcePermission(android.Manifest.permission.REGISTER_CALL_PROVIDER);
    }

    private void enforceRegisterSimSubscriptionPermission() {
        enforcePermission(android.Manifest.permission.REGISTER_SIM_SUBSCRIPTION);
    }

    private void enforceRegisterConnectionManagerPermission() {
        enforcePermission(android.Manifest.permission.REGISTER_CONNECTION_MANAGER);
    }

    private void enforceReadPermission() {
        enforcePermission(Manifest.permission.READ_PHONE_STATE);
    }

    private void enforceModifyPermission() {
        enforcePermission(Manifest.permission.MODIFY_PHONE_STATE);
    }

    private void enforcePermission(String permission) {
        mContext.enforceCallingOrSelfPermission(permission, null);
    }

    private void enforceRegisterMultiUser() {
        if (!isCallerSystemApp()) {
            throw new SecurityException("CAPABILITY_MULTI_USER is only available to system apps.");
        }
    }

    private void enforceUserHandleMatchesCaller(PhoneAccountHandle accountHandle) {
        if (!Binder.getCallingUserHandle().equals(accountHandle.getUserHandle())) {
            throw new SecurityException("Calling UserHandle does not match PhoneAccountHandle's");
        }
    }

    private void enforceFeature(String feature) {
        PackageManager pm = mContext.getPackageManager();
        if (!pm.hasSystemFeature(feature)) {
            throw new UnsupportedOperationException(
                    "System does not support feature " + feature);
        }
    }

    private boolean isDefaultDialerCalling() {
        ComponentName defaultDialerComponent = getDefaultPhoneAppInternal();
        if (defaultDialerComponent != null) {
            try {
                mAppOpsManager.checkPackage(
                        Binder.getCallingUid(), defaultDialerComponent.getPackageName());
                return true;
            } catch (SecurityException e) {
                Log.e(TAG, e, "Could not get default dialer.");
            }
        }
        return false;
    }

    private ComponentName getDefaultPhoneAppInternal() {
        Resources resources = mContext.getResources();
        return new ComponentName(
                resources.getString(R.string.ui_default_package),
                resources.getString(R.string.dialer_default_class));
    }

    private TelephonyManager getTelephonyManager() {
        return (TelephonyManager)mContext.getSystemService(Context.TELEPHONY_SERVICE);
    }

    private MainThreadRequest sendRequestAsync(int command, int arg1) {
        return sendRequestAsync(command, arg1, null);
    }

    private MainThreadRequest sendRequestAsync(int command, int arg1, Object arg) {
        MainThreadRequest request = new MainThreadRequest();
        request.arg = arg;
        mMainThreadHandler.obtainMessage(command, arg1, 0, request).sendToTarget();
        return request;
    }

    /**
     * Posts the specified command to be executed on the main thread, waits for the request to
     * complete, and returns the result.
     */
    private Object sendRequest(int command) {
        if (Looper.myLooper() == mMainThreadHandler.getLooper()) {
            MainThreadRequest request = new MainThreadRequest();
            mMainThreadHandler.handleMessage(mMainThreadHandler.obtainMessage(command, request));
            return request.result;
        } else {
            MainThreadRequest request = sendRequestAsync(command, 0);

            // Wait for the request to complete
            synchronized (request) {
                while (request.result == null) {
                    try {
                        request.wait();
                    } catch (InterruptedException e) {
                        // Do nothing, go back and wait until the request is complete
                    }
                }
            }
            return request.result;
        }
    }
}
