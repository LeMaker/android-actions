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

import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothHeadset;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.IBluetoothHeadsetPhone;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.telecom.CallState;
import android.telecom.Connection;
import android.telecom.PhoneAccount;
import android.telephony.PhoneNumberUtils;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

import com.android.server.telecom.CallsManager.CallsManagerListener;

import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Bluetooth headset manager for Telecom. This class shares the call state with the bluetooth device
 * and accepts call-related commands to perform on behalf of the BT device.
 */
public final class BluetoothPhoneService extends Service {
    /**
     * Request object for performing synchronous requests to the main thread.
     */
    private static class MainThreadRequest {
        private static final Object RESULT_NOT_SET = new Object();
        Object result = RESULT_NOT_SET;
        int param;

        MainThreadRequest(int param) {
            this.param = param;
        }

        void setResult(Object value) {
            result = value;
            synchronized (this) {
                notifyAll();
            }
        }
    }

    private static final String TAG = "BluetoothPhoneService";

    private static final int MSG_ANSWER_CALL = 1;
    private static final int MSG_HANGUP_CALL = 2;
    private static final int MSG_SEND_DTMF = 3;
    private static final int MSG_PROCESS_CHLD = 4;
    private static final int MSG_GET_NETWORK_OPERATOR = 5;
    private static final int MSG_LIST_CURRENT_CALLS = 6;
    private static final int MSG_QUERY_PHONE_STATE = 7;
    private static final int MSG_GET_SUBSCRIBER_NUMBER = 8;

    // match up with bthf_call_state_t of bt_hf.h
    private static final int CALL_STATE_ACTIVE = 0;
    private static final int CALL_STATE_HELD = 1;
    private static final int CALL_STATE_DIALING = 2;
    private static final int CALL_STATE_ALERTING = 3;
    private static final int CALL_STATE_INCOMING = 4;
    private static final int CALL_STATE_WAITING = 5;
    private static final int CALL_STATE_IDLE = 6;

    // match up with bthf_call_state_t of bt_hf.h
    // Terminate all held or set UDUB("busy") to a waiting call
    private static final int CHLD_TYPE_RELEASEHELD = 0;
    // Terminate all active calls and accepts a waiting/held call
    private static final int CHLD_TYPE_RELEASEACTIVE_ACCEPTHELD = 1;
    // Hold all active calls and accepts a waiting/held call
    private static final int CHLD_TYPE_HOLDACTIVE_ACCEPTHELD = 2;
    // Add all held calls to a conference
    private static final int CHLD_TYPE_ADDHELDTOCONF = 3;

    private int mNumActiveCalls = 0;
    private int mNumHeldCalls = 0;
    private int mBluetoothCallState = CALL_STATE_IDLE;
    private String mRingingAddress = null;
    private int mRingingAddressType = 0;
    private Call mOldHeldCall = null;

    /**
     * Binder implementation of IBluetoothHeadsetPhone. Implements the command interface that the
     * bluetooth headset code uses to control call.
     */
    private final IBluetoothHeadsetPhone.Stub mBinder = new IBluetoothHeadsetPhone.Stub() {
        @Override
        public boolean answerCall() throws RemoteException {
            enforceModifyPermission();
            Log.i(TAG, "BT - answering call");
            return sendSynchronousRequest(MSG_ANSWER_CALL);
        }

        @Override
        public boolean hangupCall() throws RemoteException {
            enforceModifyPermission();
            Log.i(TAG, "BT - hanging up call");
            return sendSynchronousRequest(MSG_HANGUP_CALL);
        }

        @Override
        public boolean sendDtmf(int dtmf) throws RemoteException {
            enforceModifyPermission();
            Log.i(TAG, "BT - sendDtmf %c", Log.DEBUG ? dtmf : '.');
            return sendSynchronousRequest(MSG_SEND_DTMF, dtmf);
        }

        @Override
        public String getNetworkOperator() throws RemoteException {
            Log.i(TAG, "getNetworkOperator");
            enforceModifyPermission();
            return sendSynchronousRequest(MSG_GET_NETWORK_OPERATOR);
        }

        @Override
        public String getSubscriberNumber() throws RemoteException {
            Log.i(TAG, "getSubscriberNumber");
            enforceModifyPermission();
            return sendSynchronousRequest(MSG_GET_SUBSCRIBER_NUMBER);
        }

        @Override
        public boolean listCurrentCalls() throws RemoteException {
            // only log if it is after we recently updated the headset state or else it can clog
            // the android log since this can be queried every second.
            boolean logQuery = mHeadsetUpdatedRecently;
            mHeadsetUpdatedRecently = false;

            if (logQuery) {
                Log.i(TAG, "listcurrentCalls");
            }
            enforceModifyPermission();
            return sendSynchronousRequest(MSG_LIST_CURRENT_CALLS, logQuery ? 1 : 0);
        }

        @Override
        public boolean queryPhoneState() throws RemoteException {
            Log.i(TAG, "queryPhoneState");
            enforceModifyPermission();
            return sendSynchronousRequest(MSG_QUERY_PHONE_STATE);
        }

        @Override
        public boolean processChld(int chld) throws RemoteException {
            Log.i(TAG, "processChld %d", chld);
            enforceModifyPermission();
            return sendSynchronousRequest(MSG_PROCESS_CHLD, chld);
        }

        @Override
        public void updateBtHandsfreeAfterRadioTechnologyChange() throws RemoteException {
            Log.d(TAG, "RAT change");
            // deprecated
        }

        @Override
        public void cdmaSetSecondCallState(boolean state) throws RemoteException {
            Log.d(TAG, "cdma 1");
            // deprecated
        }

        @Override
        public void cdmaSwapSecondCallState() throws RemoteException {
            Log.d(TAG, "cdma 2");
            // deprecated
        }
    };

    /**
     * Main-thread handler for BT commands.  Since telecom logic runs on a single thread, commands
     * that are sent to it from the headset need to be moved over to the main thread before
     * executing. This handler exists for that reason.
     */
    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            MainThreadRequest request = msg.obj instanceof MainThreadRequest ?
                    (MainThreadRequest) msg.obj : null;
            CallsManager callsManager = getCallsManager();
            Call call = null;

            Log.d(TAG, "handleMessage(%d) w/ param %s",
                    msg.what, request == null ? null : request.param);

            switch (msg.what) {
                case MSG_ANSWER_CALL:
                    try {
                        call = callsManager.getRingingCall();
                        if (call != null) {
                            getCallsManager().answerCall(call, 0);
                        }
                    } finally {
                        request.setResult(call != null);
                    }
                    break;

                case MSG_HANGUP_CALL:
                    try {
                        call = callsManager.getForegroundCall();
                        if (call != null) {
                            callsManager.disconnectCall(call);
                        }
                    } finally {
                        request.setResult(call != null);
                    }
                    break;

                case MSG_SEND_DTMF:
                    try {
                        call = callsManager.getForegroundCall();
                        if (call != null) {
                            // TODO: Consider making this a queue instead of starting/stopping
                            // in quick succession.
                            callsManager.playDtmfTone(call, (char) request.param);
                            callsManager.stopDtmfTone(call);
                        }
                    } finally {
                        request.setResult(call != null);
                    }
                    break;

                case MSG_PROCESS_CHLD:
                    Boolean result = false;
                    try {
                        result = processChld(request.param);
                    } finally {
                        request.setResult(result);
                    }
                    break;

                case MSG_GET_SUBSCRIBER_NUMBER:
                    String address = null;
                    try {
                        PhoneAccount account = getBestPhoneAccount();
                        if (account != null) {
                            Uri addressUri = account.getAddress();
                            if (addressUri != null) {
                                address = addressUri.getSchemeSpecificPart();
                            }
                        }

                        if (TextUtils.isEmpty(address)) {
                            address = TelephonyManager.from(BluetoothPhoneService.this)
                                    .getLine1Number();
                        }
                    } finally {
                        request.setResult(address);
                    }
                    break;

                case MSG_GET_NETWORK_OPERATOR:
                    String label = null;
                    try {
                        PhoneAccount account = getBestPhoneAccount();
                        if (account != null) {
                            label = account.getLabel().toString();
                        } else {
                            // Finally, just get the network name from telephony.
                            label = TelephonyManager.from(BluetoothPhoneService.this)
                                    .getNetworkOperatorName();
                        }
                    } finally {
                        request.setResult(label);
                    }
                    break;

                case MSG_LIST_CURRENT_CALLS:
                    try {
                        sendListOfCalls(request.param == 1);
                    } finally {
                        request.setResult(true);
                    }
                    break;

                case MSG_QUERY_PHONE_STATE:
                    try {
                        updateHeadsetWithCallState(true /* force */);
                    } finally {
                        if (request != null) {
                            request.setResult(true);
                        }
                    }
                    break;
            }
        }
    };

    /**
     * Listens to call changes from the CallsManager and calls into methods to update the bluetooth
     * headset with the new states.
     */
    private CallsManagerListener mCallsManagerListener = new CallsManagerListenerBase() {
        @Override
        public void onCallAdded(Call call) {
            updateHeadsetWithCallState(false /* force */);
        }

        @Override
        public void onCallRemoved(Call call) {
            mClccIndexMap.remove(call);
            updateHeadsetWithCallState(false /* force */);
        }

        @Override
        public void onCallStateChanged(Call call, int oldState, int newState) {
            // If a call is being put on hold because of a new connecting call, ignore the
            // CONNECTING since the BT state update needs to send out the numHeld = 1 + dialing
            // state atomically.
            // When the call later transitions to DIALING/DISCONNECTED we will then send out the
            // aggregated update.
            if (oldState == CallState.ACTIVE && newState == CallState.ON_HOLD) {
                for (Call otherCall : CallsManager.getInstance().getCalls()) {
                    if (otherCall.getState() == CallState.CONNECTING) {
                        return;
                    }
                }
            }

            // To have an active call and another dialing at the same time is an invalid BT
            // state. We can assume that the active call will be automatically held which will
            // send another update at which point we will be in the right state.
            if (CallsManager.getInstance().getActiveCall() != null
                    && oldState == CallState.CONNECTING && newState == CallState.DIALING) {
                return;
            }
            updateHeadsetWithCallState(false /* force */);
        }

        @Override
        public void onForegroundCallChanged(Call oldForegroundCall, Call newForegroundCall) {
            // The BluetoothPhoneService does not need to respond to changes in foreground calls,
            // which are always accompanied by call state changes anyway.
        }

        @Override
        public void onIsConferencedChanged(Call call) {
            /*
             * Filter certain onIsConferencedChanged callbacks. Unfortunately this needs to be done
             * because conference change events are not atomic and multiple callbacks get fired
             * when two calls are conferenced together. This confuses updateHeadsetWithCallState
             * if it runs in the middle of two calls being conferenced and can cause spurious and
             * incorrect headset state updates. One of the scenarios is described below for CDMA
             * conference calls.
             *
             * 1) Call 1 and Call 2 are being merged into conference Call 3.
             * 2) Call 1 has its parent set to Call 3, but Call 2 does not have a parent yet.
             * 3) updateHeadsetWithCallState now thinks that there are two active calls (Call 2 and
             * Call 3) when there is actually only one active call (Call 3).
             */
            if (call.getParentCall() != null) {
                // If this call is newly conferenced, ignore the callback. We only care about the
                // one sent for the parent conference call.
                Log.d(this, "Ignoring onIsConferenceChanged from child call with new parent");
                return;
            }
            if (call.getChildCalls().size() == 1) {
                // If this is a parent call with only one child, ignore the callback as well since
                // the minimum number of child calls to start a conference call is 2. We expect
                // this to be called again when the parent call has another child call added.
                Log.d(this, "Ignoring onIsConferenceChanged from parent with only one child call");
                return;
            }
            updateHeadsetWithCallState(false /* force */);
        }
    };

    /**
     * Listens to connections and disconnections of bluetooth headsets.  We need to save the current
     * bluetooth headset so that we know where to send call updates.
     */
    private BluetoothProfile.ServiceListener mProfileListener =
            new BluetoothProfile.ServiceListener() {
        @Override
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            mBluetoothHeadset = (BluetoothHeadset) proxy;
        }

        @Override
        public void onServiceDisconnected(int profile) {
            mBluetoothHeadset = null;
        }
    };

    /**
     * Receives events for global state changes of the bluetooth adapter.
     */
    private final BroadcastReceiver mBluetoothAdapterReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
            Log.d(TAG, "Bluetooth Adapter state: %d", state);
            if (state == BluetoothAdapter.STATE_ON) {
                mHandler.sendEmptyMessage(MSG_QUERY_PHONE_STATE);
            }
        }
    };

    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothHeadset mBluetoothHeadset;

    // A map from Calls to indexes used to identify calls for CLCC (C* List Current Calls).
    private Map<Call, Integer> mClccIndexMap = new HashMap<>();

    private boolean mHeadsetUpdatedRecently = false;

    public BluetoothPhoneService() {
        Log.v(TAG, "Constructor");
    }

    public static final void start(Context context) {
        if (BluetoothAdapter.getDefaultAdapter() != null) {
            context.startService(new Intent(context, BluetoothPhoneService.class));
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG, "Binding service");
        return mBinder;
    }

    @Override
    public void onCreate() {
        Log.d(TAG, "onCreate");

        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mBluetoothAdapter == null) {
            Log.d(TAG, "BluetoothPhoneService shutting down, no BT Adapter found.");
            return;
        }
        mBluetoothAdapter.getProfileProxy(this, mProfileListener, BluetoothProfile.HEADSET);

        IntentFilter intentFilter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
        registerReceiver(mBluetoothAdapterReceiver, intentFilter);

        CallsManager.getInstance().addListener(mCallsManagerListener);
        updateHeadsetWithCallState(false /* force */);
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy");
        CallsManager.getInstance().removeListener(mCallsManagerListener);
        super.onDestroy();
    }

    private boolean processChld(int chld) {
        CallsManager callsManager = CallsManager.getInstance();
        Call activeCall = callsManager.getActiveCall();
        Call ringingCall = callsManager.getRingingCall();
        Call heldCall = callsManager.getHeldCall();

        // TODO: Keeping as Log.i for now.  Move to Log.d after L release if BT proves stable.
        Log.i(TAG, "Active: %s\nRinging: %s\nHeld: %s", activeCall, ringingCall, heldCall);

        if (chld == CHLD_TYPE_RELEASEHELD) {
            if (ringingCall != null) {
                callsManager.rejectCall(ringingCall, false, null);
                return true;
            } else if (heldCall != null) {
                callsManager.disconnectCall(heldCall);
                return true;
            }
        } else if (chld == CHLD_TYPE_RELEASEACTIVE_ACCEPTHELD) {
            if (activeCall != null) {
                callsManager.disconnectCall(activeCall);
                if (ringingCall != null) {
                    callsManager.answerCall(ringingCall, 0);
                } else if (heldCall != null) {
                    callsManager.unholdCall(heldCall);
                }
                return true;
            }
        } else if (chld == CHLD_TYPE_HOLDACTIVE_ACCEPTHELD) {
            if (activeCall != null && activeCall.can(Connection.CAPABILITY_SWAP_CONFERENCE)) {
                activeCall.swapConference();
                return true;
            } else if (ringingCall != null) {
                callsManager.answerCall(ringingCall, 0);
                return true;
            } else if (heldCall != null) {
                // CallsManager will hold any active calls when unhold() is called on a
                // currently-held call.
                callsManager.unholdCall(heldCall);
                return true;
            } else if (activeCall != null && activeCall.can(Connection.CAPABILITY_HOLD)) {
                callsManager.holdCall(activeCall);
                return true;
            }
        } else if (chld == CHLD_TYPE_ADDHELDTOCONF) {
            if (activeCall != null) {
                if (activeCall.can(Connection.CAPABILITY_MERGE_CONFERENCE)) {
                    activeCall.mergeConference();
                    return true;
                } else {
                    List<Call> conferenceable = activeCall.getConferenceableCalls();
                    if (!conferenceable.isEmpty()) {
                        callsManager.conference(activeCall, conferenceable.get(0));
                        return true;
                   }
                }
            }
        }
        return false;
    }

    private void enforceModifyPermission() {
        enforceCallingOrSelfPermission(android.Manifest.permission.MODIFY_PHONE_STATE, null);
    }

    private <T> T sendSynchronousRequest(int message) {
        return sendSynchronousRequest(message, 0);
    }

    private <T> T sendSynchronousRequest(int message, int param) {
        if (Looper.myLooper() == mHandler.getLooper()) {
            Log.w(TAG, "This method will deadlock if called from the main thread.");
        }

        MainThreadRequest request = new MainThreadRequest(param);
        mHandler.obtainMessage(message, request).sendToTarget();
        synchronized (request) {
            while (request.result == MainThreadRequest.RESULT_NOT_SET) {
                try {
                    request.wait();
                } catch (InterruptedException e) {
                    // Do nothing, go back and wait until the request is complete.
                    Log.e(TAG, e, "InterruptedException");
                }
            }
        }
        if (request.result != null) {
            @SuppressWarnings("unchecked")
            T retval = (T) request.result;
            return retval;
        }
        return null;
    }

    private void sendListOfCalls(boolean shouldLog) {
        Collection<Call> mCalls = getCallsManager().getCalls();
        for (Call call : mCalls) {
            // We don't send the parent conference call to the bluetooth device.
            if (!call.isConference()) {
                sendClccForCall(call, shouldLog);
            }
        }
        sendClccEndMarker();
    }

    /**
     * Sends a single clcc (C* List Current Calls) event for the specified call.
     */
    private void sendClccForCall(Call call, boolean shouldLog) {
        boolean isForeground = getCallsManager().getForegroundCall() == call;
        int state = convertCallState(call.getState(), isForeground);
        boolean isPartOfConference = false;

        if (state == CALL_STATE_IDLE) {
            return;
        }

        Call conferenceCall = call.getParentCall();
        if (conferenceCall != null) {
            isPartOfConference = true;

            // Run some alternative states for Conference-level merge/swap support.
            // Basically, if call supports swapping or merging at the conference-level, then we need
            // to expose the calls as having distinct states (ACTIVE vs CAPABILITY_HOLD) or the
            // functionality won't show up on the bluetooth device.

            // Before doing any special logic, ensure that we are dealing with an ACTIVE call and
            // that the conference itself has a notion of the current "active" child call.
            Call activeChild = conferenceCall.getConferenceLevelActiveCall();
            if (state == CALL_STATE_ACTIVE && activeChild != null) {
                // Reevaluate state if we can MERGE or if we can SWAP without previously having
                // MERGED.
                boolean shouldReevaluateState =
                        conferenceCall.can(Connection.CAPABILITY_MERGE_CONFERENCE) ||
                        (conferenceCall.can(Connection.CAPABILITY_SWAP_CONFERENCE) &&
                         !conferenceCall.wasConferencePreviouslyMerged());

                if (shouldReevaluateState) {
                    isPartOfConference = false;
                    if (call == activeChild) {
                        state = CALL_STATE_ACTIVE;
                    } else {
                        // At this point we know there is an "active" child and we know that it is
                        // not this call, so set it to HELD instead.
                        state = CALL_STATE_HELD;
                    }
                }
            }
        }

        int index = getIndexForCall(call);
        int direction = call.isIncoming() ? 1 : 0;
        final Uri addressUri;
        if (call.getGatewayInfo() != null) {
            addressUri = call.getGatewayInfo().getOriginalAddress();
        } else {
            addressUri = call.getHandle();
        }
        String address = addressUri == null ? null : addressUri.getSchemeSpecificPart();
        int addressType = address == null ? -1 : PhoneNumberUtils.toaFromString(address);

        if (shouldLog) {
            Log.i(this, "sending clcc for call %d, %d, %d, %b, %s, %d",
                    index, direction, state, isPartOfConference, Log.piiHandle(address),
                    addressType);
        }

        if (mBluetoothHeadset != null) {
            mBluetoothHeadset.clccResponse(
                    index, direction, state, 0, isPartOfConference, address, addressType);
        }
    }

    private void sendClccEndMarker() {
        // End marker is recognized with an index value of 0. All other parameters are ignored.
        if (mBluetoothHeadset != null) {
            mBluetoothHeadset.clccResponse(0 /* index */, 0, 0, 0, false, null, 0);
        }
    }

    /**
     * Returns the caches index for the specified call.  If no such index exists, then an index is
     * given (smallest number starting from 1 that isn't already taken).
     */
    private int getIndexForCall(Call call) {
        if (mClccIndexMap.containsKey(call)) {
            return mClccIndexMap.get(call);
        }

        int i = 1;  // Indexes for bluetooth clcc are 1-based.
        while (mClccIndexMap.containsValue(i)) {
            i++;
        }

        // NOTE: Indexes are removed in {@link #onCallRemoved}.
        mClccIndexMap.put(call, i);
        return i;
    }

    /**
     * Sends an update of the current call state to the current Headset.
     *
     * @param force {@code true} if the headset state should be sent regardless if no changes to the
     *      state have occurred, {@code false} if the state should only be sent if the state has
     *      changed.
     */
    private void updateHeadsetWithCallState(boolean force) {
        CallsManager callsManager = getCallsManager();
        Call activeCall = callsManager.getActiveCall();
        Call ringingCall = callsManager.getRingingCall();
        Call heldCall = callsManager.getHeldCall();

        int bluetoothCallState = getBluetoothCallStateForUpdate();

        String ringingAddress = null;
        int ringingAddressType = 128;
        if (ringingCall != null && ringingCall.getHandle() != null) {
            ringingAddress = ringingCall.getHandle().getSchemeSpecificPart();
            if (ringingAddress != null) {
                ringingAddressType = PhoneNumberUtils.toaFromString(ringingAddress);
            }
        }
        if (ringingAddress == null) {
            ringingAddress = "";
        }

        int numActiveCalls = activeCall == null ? 0 : 1;
        int numHeldCalls = callsManager.getNumHeldCalls();

        // For conference calls which support swapping the active call within the conference
        // (namely CDMA calls) we need to expose that as a held call in order for the BT device
        // to show "swap" and "merge" functionality.
        boolean ignoreHeldCallChange = false;
        if (activeCall != null && activeCall.isConference()) {
            if (activeCall.can(Connection.CAPABILITY_SWAP_CONFERENCE)) {
                // Indicate that BT device should show SWAP command by indicating that there is a
                // call on hold, but only if the conference wasn't previously merged.
                numHeldCalls = activeCall.wasConferencePreviouslyMerged() ? 0 : 1;
            } else if (activeCall.can(Connection.CAPABILITY_MERGE_CONFERENCE)) {
                numHeldCalls = 1;  // Merge is available, so expose via numHeldCalls.
            }

            for (Call childCall : activeCall.getChildCalls()) {
                // Held call has changed due to it being combined into a CDMA conference. Keep
                // track of this and ignore any future update since it doesn't really count as
                // a call change.
                if (mOldHeldCall == childCall) {
                    ignoreHeldCallChange = true;
                    break;
                }
            }
        }

        if (mBluetoothHeadset != null &&
                (numActiveCalls != mNumActiveCalls ||
                 numHeldCalls != mNumHeldCalls ||
                 bluetoothCallState != mBluetoothCallState ||
                 !TextUtils.equals(ringingAddress, mRingingAddress) ||
                 ringingAddressType != mRingingAddressType ||
                 (heldCall != mOldHeldCall && !ignoreHeldCallChange) ||
                 force)) {

            // If the call is transitioning into the alerting state, send DIALING first.
            // Some devices expect to see a DIALING state prior to seeing an ALERTING state
            // so we need to send it first.
            boolean sendDialingFirst = mBluetoothCallState != bluetoothCallState &&
                    bluetoothCallState == CALL_STATE_ALERTING;

            mOldHeldCall = heldCall;
            mNumActiveCalls = numActiveCalls;
            mNumHeldCalls = numHeldCalls;
            mBluetoothCallState = bluetoothCallState;
            mRingingAddress = ringingAddress;
            mRingingAddressType = ringingAddressType;

            if (sendDialingFirst) {
                // Log in full to make logs easier to debug.
                Log.i(TAG, "updateHeadsetWithCallState " +
                        "numActive %s, " +
                        "numHeld %s, " +
                        "callState %s, " +
                        "ringing number %s, " +
                        "ringing type %s",
                        mNumActiveCalls,
                        mNumHeldCalls,
                        CALL_STATE_DIALING,
                        Log.pii(mRingingAddress),
                        mRingingAddressType);
                mBluetoothHeadset.phoneStateChanged(
                        mNumActiveCalls,
                        mNumHeldCalls,
                        CALL_STATE_DIALING,
                        mRingingAddress,
                        mRingingAddressType);
            }

            Log.i(TAG, "updateHeadsetWithCallState " +
                    "numActive %s, " +
                    "numHeld %s, " +
                    "callState %s, " +
                    "ringing number %s, " +
                    "ringing type %s",
                    mNumActiveCalls,
                    mNumHeldCalls,
                    mBluetoothCallState,
                    Log.pii(mRingingAddress),
                    mRingingAddressType);

            mBluetoothHeadset.phoneStateChanged(
                    mNumActiveCalls,
                    mNumHeldCalls,
                    mBluetoothCallState,
                    mRingingAddress,
                    mRingingAddressType);

            mHeadsetUpdatedRecently = true;
        }
    }

    private int getBluetoothCallStateForUpdate() {
        CallsManager callsManager = getCallsManager();
        Call ringingCall = callsManager.getRingingCall();
        Call dialingCall = callsManager.getDialingCall();

        //
        // !! WARNING !!
        // You will note that CALL_STATE_WAITING, CALL_STATE_HELD, and CALL_STATE_ACTIVE are not
        // used in this version of the call state mappings.  This is on purpose.
        // phone_state_change() in btif_hf.c is not written to handle these states. Only with the
        // listCalls*() method are WAITING and ACTIVE used.
        // Using the unsupported states here caused problems with inconsistent state in some
        // bluetooth devices (like not getting out of ringing state after answering a call).
        //
        int bluetoothCallState = CALL_STATE_IDLE;
        if (ringingCall != null) {
            bluetoothCallState = CALL_STATE_INCOMING;
        } else if (dialingCall != null) {
            bluetoothCallState = CALL_STATE_ALERTING;
        }
        return bluetoothCallState;
    }

    private int convertCallState(int callState, boolean isForegroundCall) {
        switch (callState) {
            case CallState.NEW:
            case CallState.ABORTED:
            case CallState.DISCONNECTED:
            case CallState.CONNECTING:
            case CallState.PRE_DIAL_WAIT:
                return CALL_STATE_IDLE;

            case CallState.ACTIVE:
                return CALL_STATE_ACTIVE;

            case CallState.DIALING:
                // Yes, this is correctly returning ALERTING.
                // "Dialing" for BT means that we have sent information to the service provider
                // to place the call but there is no confirmation that the call is going through.
                // When there finally is confirmation, the ringback is played which is referred to
                // as an "alert" tone, thus, ALERTING.
                // TODO: We should consider using the ALERTING terms in Telecom because that
                // seems to be more industry-standard.
                return CALL_STATE_ALERTING;

            case CallState.ON_HOLD:
                return CALL_STATE_HELD;

            case CallState.RINGING:
                if (isForegroundCall) {
                    return CALL_STATE_INCOMING;
                } else {
                    return CALL_STATE_WAITING;
                }
        }
        return CALL_STATE_IDLE;
    }

    private CallsManager getCallsManager() {
        return CallsManager.getInstance();
    }

    /**
     * Returns the best phone account to use for the given state of all calls.
     * First, tries to return the phone account for the foreground call, second the default
     * phone account for PhoneAccount.SCHEME_TEL.
     */
    private PhoneAccount getBestPhoneAccount() {
        PhoneAccountRegistrar registry = TelecomGlobals.getInstance().getPhoneAccountRegistrar();
        if (registry == null) {
            return null;
        }

        Call call = getCallsManager().getForegroundCall();

        PhoneAccount account = null;
        if (call != null) {
            // First try to get the network name of the foreground call.
            account = registry.getPhoneAccount(call.getTargetPhoneAccount());
        }

        if (account == null) {
            // Second, Try to get the label for the default Phone Account.
            account = registry.getPhoneAccount(
                    registry.getDefaultOutgoingPhoneAccount(PhoneAccount.SCHEME_TEL));
        }
        return account;
    }
}
