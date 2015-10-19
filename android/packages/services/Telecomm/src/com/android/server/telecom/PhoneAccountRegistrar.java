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
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.content.pm.UserInfo;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Process;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings;
import android.telecom.ConnectionService;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.telephony.PhoneNumberUtils;
import android.telephony.SubscriptionManager;
import android.text.TextUtils;
import android.util.AtomicFile;
import android.util.Base64;
import android.util.Xml;

// TODO: Needed for move to system service: import com.android.internal.R;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.util.FastXmlSerializer;
import com.android.internal.util.IndentingPrintWriter;
import com.android.internal.util.XmlUtils;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.Integer;
import java.lang.SecurityException;
import java.lang.String;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * Handles writing and reading PhoneAccountHandle registration entries. This is a simple verbatim
 * delegate for all the account handling methods on {@link android.telecom.TelecomManager} as implemented in
 * {@link TelecomServiceImpl}, with the notable exception that {@link TelecomServiceImpl} is
 * responsible for security checking to make sure that the caller has proper authority over
 * the {@code ComponentName}s they are declaring in their {@code PhoneAccountHandle}s.
 */
public final class PhoneAccountRegistrar {

    public static final PhoneAccountHandle NO_ACCOUNT_SELECTED =
            new PhoneAccountHandle(new ComponentName("null", "null"), "NO_ACCOUNT_SELECTED");

    public abstract static class Listener {
        public void onAccountsChanged(PhoneAccountRegistrar registrar) {}
        public void onDefaultOutgoingChanged(PhoneAccountRegistrar registrar) {}
        public void onSimCallManagerChanged(PhoneAccountRegistrar registrar) {}
    }

    private static final String FILE_NAME = "phone-account-registrar-state.xml";
    @VisibleForTesting
    public static final int EXPECTED_STATE_VERSION = 5;

    /** Keep in sync with the same in SipSettings.java */
    private static final String SIP_SHARED_PREFERENCES = "SIP_PREFERENCES";

    private final List<Listener> mListeners = new CopyOnWriteArrayList<>();
    private final AtomicFile mAtomicFile;
    private final Context mContext;
    private final UserManager mUserManager;
    private final SubscriptionManager mSubscriptionManager;
    private State mState;
    private UserHandle mCurrentUserHandle;

    @VisibleForTesting
    public PhoneAccountRegistrar(Context context) {
        this(context, FILE_NAME);
    }

    @VisibleForTesting
    public PhoneAccountRegistrar(Context context, String fileName) {
        // TODO: This file path is subject to change -- it is storing the phone account registry
        // state file in the path /data/system/users/0/, which is likely not correct in a
        // multi-user setting.
        /** UNCOMMENT_FOR_MOVE_TO_SYSTEM_SERVICE
        String filePath = Environment.getUserSystemDirectory(UserHandle.myUserId()).
                getAbsolutePath();
        mAtomicFile = new AtomicFile(new File(filePath, fileName));
         UNCOMMENT_FOR_MOVE_TO_SYSTEM_SERVICE */
        mAtomicFile = new AtomicFile(new File(context.getFilesDir(), fileName));

        mState = new State();
        mContext = context;
        mUserManager = UserManager.get(context);
        mSubscriptionManager = SubscriptionManager.from(mContext);
        mCurrentUserHandle = Process.myUserHandle();
        read();
    }

    /**
     * Retrieves the subscription id for a given phone account if it exists. Subscription ids
     * apply only to PSTN/SIM card phone accounts so all other accounts should not have a
     * subscription id.
     * @param accountHandle The handle for the phone account for which to retrieve the
     * subscription id.
     * @return The value of the subscription id or -1 if it does not exist or is not valid.
     */
    public int getSubscriptionIdForPhoneAccount(PhoneAccountHandle accountHandle) {
        PhoneAccount account = getPhoneAccountInternal(accountHandle);
        if (account == null
                || !account.hasCapabilities(PhoneAccount.CAPABILITY_SIM_SUBSCRIPTION)
                || !TextUtils.isDigitsOnly(accountHandle.getId())
                || !isVisibleForUser(accountHandle)) {
            // Since no decimals or negative numbers can be valid subscription ids, only a string of
            // numbers can be subscription id
            return SubscriptionManager.INVALID_SUBSCRIPTION_ID;
        }
        return Integer.parseInt(accountHandle.getId());
    }

    /**
     * Retrieves the default outgoing phone account supporting the specified uriScheme. Note that if
     * {@link #mCurrentUserHandle} does not have visibility into the current default, {@code null}
     * will be returned.
     *
     * @param uriScheme The URI scheme for the outgoing call.
     * @return The {@link PhoneAccountHandle} to use.
     */
    public PhoneAccountHandle getDefaultOutgoingPhoneAccount(String uriScheme) {
        final PhoneAccountHandle userSelected = getUserSelectedOutgoingPhoneAccount();

        if (userSelected != null) {
            // If there is a default PhoneAccount, ensure it supports calls to handles with the
            // specified uriScheme.
            final PhoneAccount userSelectedAccount = getPhoneAccountInternal(userSelected);
            if (userSelectedAccount.supportsUriScheme(uriScheme)
                    && isVisibleForUser(userSelected)) {
                return userSelected;
            }
        }

        List<PhoneAccountHandle> outgoing = getCallCapablePhoneAccounts(uriScheme);
        switch (outgoing.size()) {
            case 0:
                // There are no accounts, so there can be no default
                return null;
            case 1:
                // There is only one account, which is by definition the default.
                PhoneAccountHandle onlyHandle = outgoing.get(0);
                if (isVisibleForUser(onlyHandle)) {
                    return outgoing.get(0);
                }
                return null;
            default:
                // There are multiple accounts with no selected default
                return null;
        }
    }

    /**
     * @return The user-selected outgoing {@link PhoneAccount}, or null if it hasn't been set (or
     *      if it was set by another user).
     */
    PhoneAccountHandle getUserSelectedOutgoingPhoneAccount() {
        if (mState.defaultOutgoing != null) {
            // Return the registered outgoing default iff it still exists (we keep a sticky
            // default to survive account deletion and re-addition)
            for (int i = 0; i < mState.accounts.size(); i++) {
                if (mState.accounts.get(i).getAccountHandle().equals(mState.defaultOutgoing)
                        && isVisibleForUser(mState.defaultOutgoing)) {
                    return mState.defaultOutgoing;
                }
            }
            // At this point, there was a registered default but it has been deleted; proceed
            // as though there were no default
        }
        return null;
    }

    public void setUserSelectedOutgoingPhoneAccount(PhoneAccountHandle accountHandle) {
        if (accountHandle == null) {
            // Asking to clear the default outgoing is a valid request
            mState.defaultOutgoing = null;
        } else {
            boolean found = false;
            for (PhoneAccount m : mState.accounts) {
                if (Objects.equals(accountHandle, m.getAccountHandle())) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                Log.w(this, "Trying to set nonexistent default outgoing %s",
                        accountHandle);
                return;
            }

            if (!getPhoneAccountInternal(accountHandle).hasCapabilities(
                    PhoneAccount.CAPABILITY_CALL_PROVIDER)) {
                Log.w(this, "Trying to set non-call-provider default outgoing %s",
                        accountHandle);
                return;
            }

            if (getPhoneAccountInternal(accountHandle).hasCapabilities(
                    PhoneAccount.CAPABILITY_SIM_SUBSCRIPTION)) {
                // If the account selected is a SIM account, propagate down to the subscription
                // record.
                int subId = getSubscriptionIdForPhoneAccount(accountHandle);
                mSubscriptionManager.setDefaultVoiceSubId(subId);
            }

            mState.defaultOutgoing = accountHandle;
        }

        write();
        fireDefaultOutgoingChanged();
    }

    boolean isUserSelectedSmsPhoneAccount(PhoneAccountHandle accountHandle) {
        return getSubscriptionIdForPhoneAccount(accountHandle) ==
                SubscriptionManager.getDefaultSmsSubId();
    }

    public void setSimCallManager(PhoneAccountHandle callManager) {
        if (callManager != null) {
            PhoneAccount callManagerAccount = getPhoneAccountInternal(callManager);
            if (callManagerAccount == null) {
                Log.d(this, "setSimCallManager: Nonexistent call manager: %s", callManager);
                return;
            } else if (!callManagerAccount.hasCapabilities(
                    PhoneAccount.CAPABILITY_CONNECTION_MANAGER)) {
                Log.d(this, "setSimCallManager: Not a call manager: %s", callManagerAccount);
                return;
            }
        } else {
            callManager = NO_ACCOUNT_SELECTED;
        }
        mState.simCallManager = callManager;

        write();
        fireSimCallManagerChanged();
    }

    /**
     * @return The {@link PhoneAccount}s which are visible to {@link #mCurrentUserHandle}.
     */
    public PhoneAccountHandle getSimCallManager() {
        if (mState.simCallManager != null) {
            if (NO_ACCOUNT_SELECTED.equals(mState.simCallManager)) {
                return null;
            }
            // Return the registered sim call manager iff it still exists (we keep a sticky
            // setting to survive account deletion and re-addition)
            for (int i = 0; i < mState.accounts.size(); i++) {
                if (mState.accounts.get(i).getAccountHandle().equals(mState.simCallManager)
                        && !resolveComponent(mState.simCallManager).isEmpty()
                        && isVisibleForUser(mState.simCallManager)) {
                    return mState.simCallManager;
                }
            }
        }

        // See if the OEM has specified a default one.
        String defaultConnectionMgr =
                mContext.getResources().getString(R.string.default_connection_manager_component);
        if (!TextUtils.isEmpty(defaultConnectionMgr)) {
            ComponentName componentName = ComponentName.unflattenFromString(defaultConnectionMgr);
            // Make sure that the component can be resolved.
            List<ResolveInfo> resolveInfos = resolveComponent(componentName, null);
            if (!resolveInfos.isEmpty()) {
                // See if there is registered PhoneAccount by this component.
                List<PhoneAccountHandle> handles = getAllPhoneAccountHandles();
                for (PhoneAccountHandle handle : handles) {
                    if (componentName.equals(handle.getComponentName())
                            && isVisibleForUser(handle)) {
                        return handle;
                    }
                }
                Log.d(this, "%s does not have a PhoneAccount; not using as default", componentName);
            } else {
                Log.d(this, "%s could not be resolved; not using as default", componentName);
            }
        } else {
            Log.v(this, "No default connection manager specified");
        }

        return null;
    }

    /**
     * A version of {@link #getPhoneAccount} which does not guard for the current user.
     *
     * @param handle
     * @return
     */
    PhoneAccount getPhoneAccountInternal(PhoneAccountHandle handle) {
        for (PhoneAccount m : mState.accounts) {
            if (Objects.equals(handle, m.getAccountHandle())) {
                return m;
            }
        }
        return null;
    }

    /**
     * Update the current UserHandle to track when users are switched. This will allow the
     * PhoneAccountRegistar to self-filter the PhoneAccounts to make sure we don't leak anything
     * across users.
     *
     * @param userHandle The {@link UserHandle}, as delivered by
     *          {@link Intent#ACTION_USER_SWITCHED}.
     */
    public void setCurrentUserHandle(UserHandle userHandle) {
        if (userHandle == null) {
            Log.d(this, "setCurrentUserHandle, userHandle = null");
            userHandle = Process.myUserHandle();
        }
        Log.d(this, "setCurrentUserHandle, %s", userHandle);
        mCurrentUserHandle = userHandle;
    }

    private boolean isVisibleForUser(PhoneAccountHandle accountHandle) {
        if (accountHandle == null) {
            return false;
        }

        return isVisibleForUser(getPhoneAccountInternal(accountHandle));
    }

    private boolean isVisibleForUser(PhoneAccount account) {
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

        if (mCurrentUserHandle == null) {
            Log.d(this, "Current user is null; assuming true");
            return true;
        }

        // Unlike in TelecomServiceImpl, we only care about *profiles* here. We want to make sure
        // that we don't resolve PhoneAccount across *users*, but resolving across *profiles* is
        // fine.
        List<UserInfo> profileUsers = mUserManager.getProfiles(mCurrentUserHandle.getIdentifier());

        for (UserInfo profileInfo : profileUsers) {
            if (profileInfo.getUserHandle().equals(phoneAccountUserHandle)) {
                return true;
            }
        }
        return false;
    }

    private List<ResolveInfo> resolveComponent(PhoneAccountHandle phoneAccountHandle) {
        return resolveComponent(phoneAccountHandle.getComponentName(),
                    phoneAccountHandle.getUserHandle());
    }

    private List<ResolveInfo> resolveComponent(ComponentName componentName,
            UserHandle userHandle) {
        PackageManager pm = mContext.getPackageManager();
        Intent intent = new Intent(ConnectionService.SERVICE_INTERFACE);
        intent.setComponent(componentName);
        if (userHandle != null) {
            return pm.queryIntentServicesAsUser(intent, 0, userHandle.getIdentifier());
        } else {
            return pm.queryIntentServices(intent, 0);
        }
    }

    /**
     * Retrieves a list of all {@link PhoneAccountHandle}s registered.
     *
     * @return The list of {@link PhoneAccountHandle}s.
     */
    public List<PhoneAccountHandle> getAllPhoneAccountHandles() {
        List<PhoneAccountHandle> accountHandles = new ArrayList<>();
        for (PhoneAccount m : mState.accounts) {
            if (isVisibleForUser(m)) {
                accountHandles.add(m.getAccountHandle());
            }
        }
        return accountHandles;
    }

    public List<PhoneAccount> getAllPhoneAccounts() {
        List<PhoneAccount> accounts = new ArrayList<>(mState.accounts.size());
        for (PhoneAccount account : mState.accounts) {
            if (isVisibleForUser(account)) {
                accounts.add(account);
            }
        }
        return accounts;
    }

    /**
     * Retrieves a list of all call provider phone accounts.
     *
     * @return The phone account handles.
     */
    public List<PhoneAccountHandle> getCallCapablePhoneAccounts() {
        return getPhoneAccountHandles(PhoneAccount.CAPABILITY_CALL_PROVIDER);
    }

    /**
     * Retrieves a list of all phone account call provider phone accounts supporting the
     * specified URI scheme.
     *
     * @param uriScheme The URI scheme.
     * @return The phone account handles.
     */
    public List<PhoneAccountHandle> getCallCapablePhoneAccounts(String uriScheme) {
        return getPhoneAccountHandles(PhoneAccount.CAPABILITY_CALL_PROVIDER, uriScheme);
    }

    /**
     * Retrieves a list of all phone accounts registered by a specified package.
     *
     * @param packageName The name of the package that registered the phone accounts.
     * @return The phone account handles.
     */
    public List<PhoneAccountHandle> getPhoneAccountsForPackage(String packageName) {
        List<PhoneAccountHandle> accountHandles = new ArrayList<>();
        for (PhoneAccount m : mState.accounts) {
            if (Objects.equals(
                    packageName,
                    m.getAccountHandle().getComponentName().getPackageName())
                    && isVisibleForUser(m)) {
                accountHandles.add(m.getAccountHandle());
            }
        }
        return accountHandles;
    }

    /**
     * Retrieves a list of all phone account handles with the connection manager capability.
     *
     * @return The phone account handles.
     */
    public List<PhoneAccountHandle> getConnectionManagerPhoneAccounts() {
        return getPhoneAccountHandles(PhoneAccount.CAPABILITY_CONNECTION_MANAGER,
                null /* supportedUriScheme */);
    }

    public PhoneAccount getPhoneAccount(PhoneAccountHandle handle) {
        for (PhoneAccount m : mState.accounts) {
            if (Objects.equals(handle, m.getAccountHandle())
                    && isVisibleForUser(m)) {
                return m;
            }
        }
        return null;
    }

    // TODO: Should we implement an artificial limit for # of accounts associated with a single
    // ComponentName?
    public void registerPhoneAccount(PhoneAccount account) {
        // Enforce the requirement that a connection service for a phone account has the correct
        // permission.
        if (!phoneAccountHasPermission(account.getAccountHandle())) {
            Log.w(this, "Phone account %s does not have BIND_CONNECTION_SERVICE permission.",
                    account.getAccountHandle());
            throw new SecurityException(
                    "PhoneAccount connection service requires BIND_CONNECTION_SERVICE permission.");
        }

        addOrReplacePhoneAccount(account);
    }

    /**
     * Adds a {@code PhoneAccount}, replacing an existing one if found.
     *
     * @param account The {@code PhoneAccount} to add or replace.
     */
    private void addOrReplacePhoneAccount(PhoneAccount account) {
        Log.d(this, "addOrReplacePhoneAccount(%s -> %s)",
                account.getAccountHandle(), account);

        mState.accounts.add(account);
        // Search for duplicates and remove any that are found.
        for (int i = 0; i < mState.accounts.size() - 1; i++) {
            if (Objects.equals(
                    account.getAccountHandle(), mState.accounts.get(i).getAccountHandle())) {
                // replace existing entry.
                mState.accounts.remove(i);
                break;
            }
        }

        write();
        fireAccountsChanged();
    }

    public void unregisterPhoneAccount(PhoneAccountHandle accountHandle) {
        for (int i = 0; i < mState.accounts.size(); i++) {
            PhoneAccountHandle handle = mState.accounts.get(i).getAccountHandle();
            if (Objects.equals(accountHandle, handle)) {
                mState.accounts.remove(i);
                break;
            }
        }

        write();
        fireAccountsChanged();
    }

    /**
     * Un-registers all phone accounts associated with a specified package.
     *
     * @param packageName The package for which phone accounts will be removed.
     * @param userHandle The {@link UserHandle} the package is running under.
     */
    public void clearAccounts(String packageName, UserHandle userHandle) {
        boolean accountsRemoved = false;
        Iterator<PhoneAccount> it = mState.accounts.iterator();
        while (it.hasNext()) {
            PhoneAccount phoneAccount = it.next();
            PhoneAccountHandle handle = phoneAccount.getAccountHandle();
            if (Objects.equals(packageName, handle.getComponentName().getPackageName())
                    && Objects.equals(userHandle, handle.getUserHandle())) {
                Log.i(this, "Removing phone account " + phoneAccount.getLabel());
                it.remove();
                accountsRemoved = true;
            }
        }

        if (accountsRemoved) {
            write();
            fireAccountsChanged();
        }
    }

    public boolean isVoiceMailNumber(PhoneAccountHandle accountHandle, String number) {
        int subId = getSubscriptionIdForPhoneAccount(accountHandle);
        return PhoneNumberUtils.isVoiceMailNumber(subId, number);
    }

    public void addListener(Listener l) {
        mListeners.add(l);
    }

    public void removeListener(Listener l) {
        if (l != null) {
            mListeners.remove(l);
        }
    }

    private void fireAccountsChanged() {
        for (Listener l : mListeners) {
            l.onAccountsChanged(this);
        }
    }

    private void fireDefaultOutgoingChanged() {
        for (Listener l : mListeners) {
            l.onDefaultOutgoingChanged(this);
        }
    }

    private void fireSimCallManagerChanged() {
        for (Listener l : mListeners) {
            l.onSimCallManagerChanged(this);
        }
    }

    /**
     * Determines if the connection service specified by a {@link PhoneAccountHandle} has the
     * {@link Manifest.permission#BIND_CONNECTION_SERVICE} permission.
     *
     * @param phoneAccountHandle The phone account to check.
     * @return {@code True} if the phone account has permission.
     */
    public boolean phoneAccountHasPermission(PhoneAccountHandle phoneAccountHandle) {
        PackageManager packageManager = mContext.getPackageManager();
        try {
            ServiceInfo serviceInfo = packageManager.getServiceInfo(
                    phoneAccountHandle.getComponentName(), 0);

            return serviceInfo.permission != null &&
                    serviceInfo.permission.equals(Manifest.permission.BIND_CONNECTION_SERVICE);
        } catch (PackageManager.NameNotFoundException e) {
            Log.w(this, "Name not found %s", e);
            return false;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * Returns a list of phone account handles with the specified flag.
     *
     * @param flags Flags which the {@code PhoneAccount} must have.
     */
    private List<PhoneAccountHandle> getPhoneAccountHandles(int flags) {
        return getPhoneAccountHandles(flags, null);
    }

    /**
     * Returns a list of phone account handles with the specified flag, supporting the specified
     * URI scheme.
     *
     * @param flags Flags which the {@code PhoneAccount} must have.
     * @param uriScheme URI schemes the PhoneAccount must handle.  {@code Null} bypasses the
     *                  URI scheme check.
     */
    private List<PhoneAccountHandle> getPhoneAccountHandles(int flags, String uriScheme) {
        List<PhoneAccountHandle> accountHandles = new ArrayList<>();
        for (PhoneAccount m : mState.accounts) {
            if (!m.hasCapabilities(flags)) {
                // Account doesn't have the right capabilities; skip this one.
                continue;
            }
            if (uriScheme != null && !m.supportsUriScheme(uriScheme)) {
                // Account doesn't support this URI scheme; skip this one.
                continue;
            }
            if (resolveComponent(m.getAccountHandle()).isEmpty()) {
                // This component cannot be resolved anymore; skip this one.
                continue;
            }
            if (!isVisibleForUser(m)) {
                // Account is not visible for the current user; skip this one.
                continue;
            }
            accountHandles.add(m.getAccountHandle());
        }
        return accountHandles;
    }

    /**
     * The state of this {@code PhoneAccountRegistrar}.
     */
    @VisibleForTesting
    public static class State {
        /**
         * The account selected by the user to be employed by default for making outgoing calls.
         * If the user has not made such a selection, then this is null.
         */
        public PhoneAccountHandle defaultOutgoing = null;

        /**
         * A {@code PhoneAccount} having {@link PhoneAccount#CAPABILITY_CONNECTION_MANAGER} which
         * manages and optimizes a user's PSTN SIM connections.
         */
        public PhoneAccountHandle simCallManager;

        /**
         * The complete list of {@code PhoneAccount}s known to the Telecom subsystem.
         */
        public final List<PhoneAccount> accounts = new ArrayList<>();

        /**
         * The version number of the State data.
         */
        public int versionNumber;
    }

    /**
     * Dumps the state of the {@link CallsManager}.
     *
     * @param pw The {@code IndentingPrintWriter} to write the state to.
     */
    public void dump(IndentingPrintWriter pw) {
        if (mState != null) {
            pw.println("xmlVersion: " + mState.versionNumber);
            pw.println("defaultOutgoing: " + (mState.defaultOutgoing == null ? "none" :
                    mState.defaultOutgoing));
            pw.println("simCallManager: " + (mState.simCallManager == null ? "none" :
                    mState.simCallManager));
            pw.println("phoneAccounts:");
            pw.increaseIndent();
            for (PhoneAccount phoneAccount : mState.accounts) {
                pw.println(phoneAccount);
            }
            pw.decreaseIndent();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // State management
    //

    private void write() {
        final FileOutputStream os;
        try {
            os = mAtomicFile.startWrite();
            boolean success = false;
            try {
                XmlSerializer serializer = new FastXmlSerializer();
                serializer.setOutput(new BufferedOutputStream(os), "utf-8");
                writeToXml(mState, serializer, mContext);
                serializer.flush();
                success = true;
            } finally {
                if (success) {
                    mAtomicFile.finishWrite(os);
                } else {
                    mAtomicFile.failWrite(os);
                }
            }
        } catch (IOException e) {
            Log.e(this, e, "Writing state to XML file");
        }
    }

    private void read() {
        final InputStream is;
        try {
            is = mAtomicFile.openRead();
        } catch (FileNotFoundException ex) {
            return;
        }

        boolean versionChanged = false;

        XmlPullParser parser;
        try {
            parser = Xml.newPullParser();
            parser.setInput(new BufferedInputStream(is), null);
            parser.nextTag();
            mState = readFromXml(parser, mContext);
            versionChanged = mState.versionNumber < EXPECTED_STATE_VERSION;

        } catch (IOException | XmlPullParserException e) {
            Log.e(this, e, "Reading state from XML file");
            mState = new State();
        } finally {
            try {
                is.close();
            } catch (IOException e) {
                Log.e(this, e, "Closing InputStream");
            }
        }

        // Verify all of the UserHandles.
        List<PhoneAccount> badAccounts = new ArrayList<>();
        for (PhoneAccount phoneAccount : mState.accounts) {
            UserHandle userHandle = phoneAccount.getAccountHandle().getUserHandle();
            if (userHandle == null) {
                Log.w(this, "Missing UserHandle for %s", phoneAccount);
                badAccounts.add(phoneAccount);
            } else if (mUserManager.getSerialNumberForUser(userHandle) == -1) {
                Log.w(this, "User does not exist for %s", phoneAccount);
                badAccounts.add(phoneAccount);
            }
        }
        mState.accounts.removeAll(badAccounts);

        // If an upgrade occurred, write out the changed data.
        if (versionChanged || !badAccounts.isEmpty()) {
            write();
        }
    }

    private static void writeToXml(State state, XmlSerializer serializer, Context context)
            throws IOException {
        sStateXml.writeToXml(state, serializer, context);
    }

    private static State readFromXml(XmlPullParser parser, Context context)
            throws IOException, XmlPullParserException {
        State s = sStateXml.readFromXml(parser, 0, context);
        return s != null ? s : new State();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // XML serialization
    //

    @VisibleForTesting
    public abstract static class XmlSerialization<T> {
        private static final String LENGTH_ATTRIBUTE = "length";
        private static final String VALUE_TAG = "value";

        /**
         * Write the supplied object to XML
         */
        public abstract void writeToXml(T o, XmlSerializer serializer, Context context)
                throws IOException;

        /**
         * Read from the supplied XML into a new object, returning null in case of an
         * unrecoverable schema mismatch or other data error. 'parser' must be already
         * positioned at the first tag that is expected to have been emitted by this
         * object's writeToXml(). This object tries to fail early without modifying
         * 'parser' if it does not recognize the data it sees.
         */
        public abstract T readFromXml(XmlPullParser parser, int version, Context context)
                throws IOException, XmlPullParserException;

        protected void writeTextIfNonNull(String tagName, Object value, XmlSerializer serializer)
                throws IOException {
            if (value != null) {
                serializer.startTag(null, tagName);
                serializer.text(Objects.toString(value));
                serializer.endTag(null, tagName);
            }
        }

        /**
         * Serializes a string array.
         *
         * @param tagName The tag name for the string array.
         * @param values The string values to serialize.
         * @param serializer The serializer.
         * @throws IOException
         */
        protected void writeStringList(String tagName, List<String> values,
                XmlSerializer serializer)
                throws IOException {

            serializer.startTag(null, tagName);
            if (values != null) {
                serializer.attribute(null, LENGTH_ATTRIBUTE, Objects.toString(values.size()));
                for (String toSerialize : values) {
                    serializer.startTag(null, VALUE_TAG);
                    if (toSerialize != null ){
                        serializer.text(toSerialize);
                    }
                    serializer.endTag(null, VALUE_TAG);
                }
            } else {
                serializer.attribute(null, LENGTH_ATTRIBUTE, "0");
            }
            serializer.endTag(null, tagName);
        }

        protected void writeBitmapIfNonNull(String tagName, Bitmap value, XmlSerializer serializer)
                throws IOException {
            if (value != null && value.getByteCount() > 0) {
                ByteArrayOutputStream stream = new ByteArrayOutputStream();
                value.compress(Bitmap.CompressFormat.PNG, 100, stream);
                byte[] imageByteArray = stream.toByteArray();
                String text = Base64.encodeToString(imageByteArray, 0, imageByteArray.length, 0);

                serializer.startTag(null, tagName);
                serializer.text(text);
                serializer.endTag(null, tagName);
            }
        }

        protected void writeLong(String tagName, long value, XmlSerializer serializer)
                throws IOException {
            serializer.startTag(null, tagName);
            serializer.text(Long.valueOf(value).toString());
            serializer.endTag(null, tagName);
        }

        /**
         * Reads a string array from the XML parser.
         *
         * @param parser The XML parser.
         * @return String array containing the parsed values.
         * @throws IOException Exception related to IO.
         * @throws XmlPullParserException Exception related to parsing.
         */
        protected List<String> readStringList(XmlPullParser parser)
                throws IOException, XmlPullParserException {

            int length = Integer.parseInt(parser.getAttributeValue(null, LENGTH_ATTRIBUTE));
            List<String> arrayEntries = new ArrayList<String>(length);
            String value = null;

            if (length == 0) {
                return arrayEntries;
            }

            int outerDepth = parser.getDepth();
            while (XmlUtils.nextElementWithin(parser, outerDepth)) {
                if (parser.getName().equals(VALUE_TAG)) {
                    parser.next();
                    value = parser.getText();
                    arrayEntries.add(value);
                }
            }

            return arrayEntries;
        }

        protected Bitmap readBitmap(XmlPullParser parser)
                throws IOException, XmlPullParserException {
            byte[] imageByteArray = Base64.decode(parser.getText(), 0);
            return BitmapFactory.decodeByteArray(imageByteArray, 0, imageByteArray.length);
        }
    }

    @VisibleForTesting
    public static final XmlSerialization<State> sStateXml =
            new XmlSerialization<State>() {
        private static final String CLASS_STATE = "phone_account_registrar_state";
        private static final String DEFAULT_OUTGOING = "default_outgoing";
        private static final String SIM_CALL_MANAGER = "sim_call_manager";
        private static final String ACCOUNTS = "accounts";
        private static final String VERSION = "version";

        @Override
        public void writeToXml(State o, XmlSerializer serializer, Context context)
                throws IOException {
            if (o != null) {
                serializer.startTag(null, CLASS_STATE);
                serializer.attribute(null, VERSION, Objects.toString(EXPECTED_STATE_VERSION));

                if (o.defaultOutgoing != null) {
                    serializer.startTag(null, DEFAULT_OUTGOING);
                    sPhoneAccountHandleXml.writeToXml(o.defaultOutgoing, serializer, context);
                    serializer.endTag(null, DEFAULT_OUTGOING);
                }

                if (o.simCallManager != null) {
                    serializer.startTag(null, SIM_CALL_MANAGER);
                    sPhoneAccountHandleXml.writeToXml(o.simCallManager, serializer, context);
                    serializer.endTag(null, SIM_CALL_MANAGER);
                }

                serializer.startTag(null, ACCOUNTS);
                for (PhoneAccount m : o.accounts) {
                    sPhoneAccountXml.writeToXml(m, serializer, context);
                }
                serializer.endTag(null, ACCOUNTS);

                serializer.endTag(null, CLASS_STATE);
            }
        }

        @Override
        public State readFromXml(XmlPullParser parser, int version, Context context)
                throws IOException, XmlPullParserException {
            if (parser.getName().equals(CLASS_STATE)) {
                State s = new State();

                String rawVersion = parser.getAttributeValue(null, VERSION);
                s.versionNumber = TextUtils.isEmpty(rawVersion) ? 1 :
                        Integer.parseInt(rawVersion);

                int outerDepth = parser.getDepth();
                while (XmlUtils.nextElementWithin(parser, outerDepth)) {
                    if (parser.getName().equals(DEFAULT_OUTGOING)) {
                        parser.nextTag();
                        s.defaultOutgoing = sPhoneAccountHandleXml.readFromXml(parser,
                                s.versionNumber, context);
                    } else if (parser.getName().equals(SIM_CALL_MANAGER)) {
                        parser.nextTag();
                        s.simCallManager = sPhoneAccountHandleXml.readFromXml(parser,
                                s.versionNumber, context);
                        if (s.simCallManager.getUserHandle() == null) {
                            // This should never happen, but handle the upgrade case.
                            s.simCallManager = new PhoneAccountHandle(
                                    s.simCallManager.getComponentName(),
                                    s.simCallManager.getId(),
                                    Process.myUserHandle());
                        }
                    } else if (parser.getName().equals(ACCOUNTS)) {
                        int accountsDepth = parser.getDepth();
                        while (XmlUtils.nextElementWithin(parser, accountsDepth)) {
                            PhoneAccount account = sPhoneAccountXml.readFromXml(parser,
                                    s.versionNumber, context);

                            if (account != null && s.accounts != null) {
                                s.accounts.add(account);
                            }
                        }
                    }
                }
                return s;
            }
            return null;
        }
    };

    @VisibleForTesting
    public static final XmlSerialization<PhoneAccount> sPhoneAccountXml =
            new XmlSerialization<PhoneAccount>() {
        private static final String CLASS_PHONE_ACCOUNT = "phone_account";
        private static final String ACCOUNT_HANDLE = "account_handle";
        private static final String ADDRESS = "handle";
        private static final String SUBSCRIPTION_ADDRESS = "subscription_number";
        private static final String CAPABILITIES = "capabilities";
        private static final String ICON_RES_ID = "icon_res_id";
        private static final String ICON_PACKAGE_NAME = "icon_package_name";
        private static final String ICON_BITMAP = "icon_bitmap";
        private static final String ICON_TINT = "icon_tint";
        private static final String HIGHLIGHT_COLOR = "highlight_color";
        private static final String LABEL = "label";
        private static final String SHORT_DESCRIPTION = "short_description";
        private static final String SUPPORTED_URI_SCHEMES = "supported_uri_schemes";

        @Override
        public void writeToXml(PhoneAccount o, XmlSerializer serializer, Context context)
                throws IOException {
            if (o != null) {
                serializer.startTag(null, CLASS_PHONE_ACCOUNT);

                if (o.getAccountHandle() != null) {
                    serializer.startTag(null, ACCOUNT_HANDLE);
                    sPhoneAccountHandleXml.writeToXml(o.getAccountHandle(), serializer, context);
                    serializer.endTag(null, ACCOUNT_HANDLE);
                }

                writeTextIfNonNull(ADDRESS, o.getAddress(), serializer);
                writeTextIfNonNull(SUBSCRIPTION_ADDRESS, o.getSubscriptionAddress(), serializer);
                writeTextIfNonNull(CAPABILITIES, Integer.toString(o.getCapabilities()), serializer);
                writeTextIfNonNull(ICON_RES_ID, Integer.toString(o.getIconResId()), serializer);
                writeTextIfNonNull(ICON_PACKAGE_NAME, o.getIconPackageName(), serializer);
                writeBitmapIfNonNull(ICON_BITMAP, o.getIconBitmap(), serializer);
                writeTextIfNonNull(ICON_TINT, Integer.toString(o.getIconTint()), serializer);
                writeTextIfNonNull(HIGHLIGHT_COLOR,
                        Integer.toString(o.getHighlightColor()), serializer);
                writeTextIfNonNull(LABEL, o.getLabel(), serializer);
                writeTextIfNonNull(SHORT_DESCRIPTION, o.getShortDescription(), serializer);
                writeStringList(SUPPORTED_URI_SCHEMES, o.getSupportedUriSchemes(), serializer);

                serializer.endTag(null, CLASS_PHONE_ACCOUNT);
            }
        }

        public PhoneAccount readFromXml(XmlPullParser parser, int version, Context context)
                throws IOException, XmlPullParserException {
            if (parser.getName().equals(CLASS_PHONE_ACCOUNT)) {
                int outerDepth = parser.getDepth();
                PhoneAccountHandle accountHandle = null;
                Uri address = null;
                Uri subscriptionAddress = null;
                int capabilities = 0;
                int iconResId = PhoneAccount.NO_RESOURCE_ID;
                String iconPackageName = null;
                Bitmap iconBitmap = null;
                int iconTint = PhoneAccount.NO_ICON_TINT;
                int highlightColor = PhoneAccount.NO_HIGHLIGHT_COLOR;
                String label = null;
                String shortDescription = null;
                List<String> supportedUriSchemes = null;

                while (XmlUtils.nextElementWithin(parser, outerDepth)) {
                    if (parser.getName().equals(ACCOUNT_HANDLE)) {
                        parser.nextTag();
                        accountHandle = sPhoneAccountHandleXml.readFromXml(parser, version,
                                context);
                    } else if (parser.getName().equals(ADDRESS)) {
                        parser.next();
                        address = Uri.parse(parser.getText());
                    } else if (parser.getName().equals(SUBSCRIPTION_ADDRESS)) {
                        parser.next();
                        String nextText = parser.getText();
                        subscriptionAddress = nextText == null ? null : Uri.parse(nextText);
                    } else if (parser.getName().equals(CAPABILITIES)) {
                        parser.next();
                        capabilities = Integer.parseInt(parser.getText());
                    } else if (parser.getName().equals(ICON_RES_ID)) {
                        parser.next();
                        iconResId = Integer.parseInt(parser.getText());
                    } else if (parser.getName().equals(ICON_PACKAGE_NAME)) {
                        parser.next();
                        iconPackageName = parser.getText();
                    } else if (parser.getName().equals(ICON_BITMAP)) {
                        parser.next();
                        iconBitmap = readBitmap(parser);
                    } else if (parser.getName().equals(ICON_TINT)) {
                        parser.next();
                        iconTint = Integer.parseInt(parser.getText());
                    } else if (parser.getName().equals(HIGHLIGHT_COLOR)) {
                        parser.next();
                        highlightColor = Integer.parseInt(parser.getText());
                    } else if (parser.getName().equals(LABEL)) {
                        parser.next();
                        label = parser.getText();
                    } else if (parser.getName().equals(SHORT_DESCRIPTION)) {
                        parser.next();
                        shortDescription = parser.getText();
                    } else if (parser.getName().equals(SUPPORTED_URI_SCHEMES)) {
                        supportedUriSchemes = readStringList(parser);
                    }
                }

                // Upgrade older phone accounts to specify the supported URI schemes.
                if (version < 2) {
                    ComponentName sipComponentName = new ComponentName("com.android.phone",
                            "com.android.services.telephony.sip.SipConnectionService");

                    supportedUriSchemes = new ArrayList<>();

                    // Handle the SIP connection service.
                    // Check the system settings to see if it also should handle "tel" calls.
                    if (accountHandle.getComponentName().equals(sipComponentName)) {
                        boolean useSipForPstn = useSipForPstnCalls(context);
                        supportedUriSchemes.add(PhoneAccount.SCHEME_SIP);
                        if (useSipForPstn) {
                            supportedUriSchemes.add(PhoneAccount.SCHEME_TEL);
                        }
                    } else {
                        supportedUriSchemes.add(PhoneAccount.SCHEME_TEL);
                        supportedUriSchemes.add(PhoneAccount.SCHEME_VOICEMAIL);
                    }
                }

                // Upgrade older phone accounts with explicit package name
                if (version < 5) {
                    if (iconBitmap == null) {
                        iconPackageName = accountHandle.getComponentName().getPackageName();
                    }
                }

                PhoneAccount.Builder builder = PhoneAccount.builder(accountHandle, label)
                        .setAddress(address)
                        .setSubscriptionAddress(subscriptionAddress)
                        .setCapabilities(capabilities)
                        .setShortDescription(shortDescription)
                        .setSupportedUriSchemes(supportedUriSchemes)
                        .setHighlightColor(highlightColor);

                if (iconBitmap == null) {
                    builder.setIcon(iconPackageName, iconResId, iconTint);
                } else {
                    builder.setIcon(iconBitmap);
                }

                return builder.build();
            }
            return null;
        }

        /**
         * Determines if the SIP call settings specify to use SIP for all calls, including PSTN calls.
         *
         * @param context The context.
         * @return {@code True} if SIP should be used for all calls.
         */
        private boolean useSipForPstnCalls(Context context) {
            String option = Settings.System.getString(context.getContentResolver(),
                    Settings.System.SIP_CALL_OPTIONS);
            option = (option != null) ? option : Settings.System.SIP_ADDRESS_ONLY;
            return option.equals(Settings.System.SIP_ALWAYS);
        }
    };

    @VisibleForTesting
    public static final XmlSerialization<PhoneAccountHandle> sPhoneAccountHandleXml =
            new XmlSerialization<PhoneAccountHandle>() {
        private static final String CLASS_PHONE_ACCOUNT_HANDLE = "phone_account_handle";
        private static final String COMPONENT_NAME = "component_name";
        private static final String ID = "id";
        private static final String USER_SERIAL_NUMBER = "user_serial_number";

        @Override
        public void writeToXml(PhoneAccountHandle o, XmlSerializer serializer, Context context)
                throws IOException {
            if (o != null) {
                serializer.startTag(null, CLASS_PHONE_ACCOUNT_HANDLE);

                if (o.getComponentName() != null) {
                    writeTextIfNonNull(
                            COMPONENT_NAME, o.getComponentName().flattenToString(), serializer);
                }

                writeTextIfNonNull(ID, o.getId(), serializer);

                if (o.getUserHandle() != null && context != null) {
                    UserManager userManager = UserManager.get(context);
                    writeLong(USER_SERIAL_NUMBER,
                            userManager.getSerialNumberForUser(o.getUserHandle()), serializer);
                }

                serializer.endTag(null, CLASS_PHONE_ACCOUNT_HANDLE);
            }
        }

        @Override
        public PhoneAccountHandle readFromXml(XmlPullParser parser, int version, Context context)
                throws IOException, XmlPullParserException {
            if (parser.getName().equals(CLASS_PHONE_ACCOUNT_HANDLE)) {
                String componentNameString = null;
                String idString = null;
                String userSerialNumberString = null;
                int outerDepth = parser.getDepth();

                UserManager userManager = UserManager.get(context);

                while (XmlUtils.nextElementWithin(parser, outerDepth)) {
                    if (parser.getName().equals(COMPONENT_NAME)) {
                        parser.next();
                        componentNameString = parser.getText();
                    } else if (parser.getName().equals(ID)) {
                        parser.next();
                        idString = parser.getText();
                    } else if (parser.getName().equals(USER_SERIAL_NUMBER)) {
                        parser.next();
                        userSerialNumberString = parser.getText();
                    }
                }
                if (componentNameString != null) {
                    UserHandle userHandle = null;
                    if (userSerialNumberString != null) {
                        try {
                            long serialNumber = Long.parseLong(userSerialNumberString);
                            userHandle = userManager.getUserForSerialNumber(serialNumber);
                        } catch (NumberFormatException e) {
                            Log.e(this, e, "Could not parse UserHandle " + userSerialNumberString);
                        }
                    }
                    return new PhoneAccountHandle(
                            ComponentName.unflattenFromString(componentNameString),
                            idString,
                            userHandle);
                }
            }
            return null;
        }
    };
}
