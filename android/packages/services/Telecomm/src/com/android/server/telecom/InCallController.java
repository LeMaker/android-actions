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
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.content.res.Resources;
import android.net.Uri;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.Trace;
import android.os.UserHandle;
import android.telecom.AudioState;
import android.telecom.CallProperties;
import android.telecom.CallState;
import android.telecom.Connection;
import android.telecom.InCallService;
import android.telecom.ParcelableCall;
import android.telecom.TelecomManager;
import android.util.ArrayMap;

// TODO: Needed for move to system service: import com.android.internal.R;
import com.android.internal.telecom.IInCallService;
import com.android.internal.util.IndentingPrintWriter;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Binds to {@link IInCallService} and provides the service to {@link CallsManager} through which it
 * can send updates to the in-call app. This class is created and owned by CallsManager and retains
 * a binding to the {@link IInCallService} (implemented by the in-call app).
 */
public final class InCallController extends CallsManagerListenerBase {
    /**
     * Used to bind to the in-call app and triggers the start of communication between
     * this class and in-call app.
     */
    private class InCallServiceConnection implements ServiceConnection {
        /** {@inheritDoc} */
        @Override public void onServiceConnected(ComponentName name, IBinder service) {
            Log.d(this, "onServiceConnected: %s", name);
            onConnected(name, service);
        }

        /** {@inheritDoc} */
        @Override public void onServiceDisconnected(ComponentName name) {
            Log.d(this, "onDisconnected: %s", name);
            onDisconnected(name);
        }
    }

    private final Call.Listener mCallListener = new Call.ListenerBase() {
        @Override
        public void onConnectionCapabilitiesChanged(Call call) {
            updateCall(call);
        }

        @Override
        public void onCannedSmsResponsesLoaded(Call call) {
            updateCall(call);
        }

        @Override
        public void onVideoCallProviderChanged(Call call) {
            updateCall(call);
        }

        @Override
        public void onStatusHintsChanged(Call call) {
            updateCall(call);
        }

        @Override
        public void onHandleChanged(Call call) {
            updateCall(call);
        }

        @Override
        public void onCallerDisplayNameChanged(Call call) {
            updateCall(call);
        }

        @Override
        public void onVideoStateChanged(Call call) {
            updateCall(call);
        }

        @Override
        public void onTargetPhoneAccountChanged(Call call) {
            updateCall(call);
        }

        @Override
        public void onConferenceableCallsChanged(Call call) {
            updateCall(call);
        }
    };

    /**
     * Maintains a binding connection to the in-call app(s).
     * ConcurrentHashMap constructor params: 8 is initial table size, 0.9f is
     * load factor before resizing, 1 means we only expect a single thread to
     * access the map so make only a single shard
     */
    private final Map<ComponentName, InCallServiceConnection> mServiceConnections =
            new ConcurrentHashMap<ComponentName, InCallServiceConnection>(8, 0.9f, 1);

    /** The in-call app implementations, see {@link IInCallService}. */
    private final Map<ComponentName, IInCallService> mInCallServices = new ArrayMap<>();

    private final CallIdMapper mCallIdMapper = new CallIdMapper("InCall");

    /** The {@link ComponentName} of the default InCall UI. */
    private final ComponentName mInCallComponentName;

    private final Context mContext;

    public InCallController(Context context) {
        mContext = context;
        Resources resources = mContext.getResources();

        mInCallComponentName = new ComponentName(
                resources.getString(R.string.ui_default_package),
                resources.getString(R.string.incall_default_class));
    }

    @Override
    public void onCallAdded(Call call) {
        if (mInCallServices.isEmpty()) {
            bind(call);
        } else {
            Log.i(this, "onCallAdded: %s", call);
            // Track the call if we don't already know about it.
            addCall(call);

            for (Map.Entry<ComponentName, IInCallService> entry : mInCallServices.entrySet()) {
                ComponentName componentName = entry.getKey();
                IInCallService inCallService = entry.getValue();

                ParcelableCall parcelableCall = toParcelableCall(call,
                        componentName.equals(mInCallComponentName) /* includeVideoProvider */);
                try {
                    inCallService.addCall(parcelableCall);
                } catch (RemoteException ignored) {
                }
            }
        }
    }

    @Override
    public void onCallRemoved(Call call) {
        Log.i(this, "onCallRemoved: %s", call);
        if (CallsManager.getInstance().getCalls().isEmpty()) {
            // TODO: Wait for all messages to be delivered to the service before unbinding.
            unbind();
        }
        call.removeListener(mCallListener);
        mCallIdMapper.removeCall(call);
    }

    @Override
    public void onCallStateChanged(Call call, int oldState, int newState) {
        updateCall(call);
    }

    @Override
    public void onConnectionServiceChanged(
            Call call,
            ConnectionServiceWrapper oldService,
            ConnectionServiceWrapper newService) {
        updateCall(call);
    }

    @Override
    public void onAudioStateChanged(AudioState oldAudioState, AudioState newAudioState) {
        if (!mInCallServices.isEmpty()) {
            Log.i(this, "Calling onAudioStateChanged, audioState: %s -> %s", oldAudioState,
                    newAudioState);
            for (IInCallService inCallService : mInCallServices.values()) {
                try {
                    inCallService.onAudioStateChanged(newAudioState);
                } catch (RemoteException ignored) {
                }
            }
        }
    }

    @Override
    public void onCanAddCallChanged(boolean canAddCall) {
        if (!mInCallServices.isEmpty()) {
            Log.i(this, "onCanAddCallChanged : %b", canAddCall);
            for (IInCallService inCallService : mInCallServices.values()) {
                try {
                    inCallService.onCanAddCallChanged(canAddCall);
                } catch (RemoteException ignored) {
                }
            }
        }
    }

    void onPostDialWait(Call call, String remaining) {
        if (!mInCallServices.isEmpty()) {
            Log.i(this, "Calling onPostDialWait, remaining = %s", remaining);
            for (IInCallService inCallService : mInCallServices.values()) {
                try {
                    inCallService.setPostDialWait(mCallIdMapper.getCallId(call), remaining);
                } catch (RemoteException ignored) {
                }
            }
        }
    }

    @Override
    public void onIsConferencedChanged(Call call) {
        Log.d(this, "onIsConferencedChanged %s", call);
        updateCall(call);
    }

    void bringToForeground(boolean showDialpad) {
        if (!mInCallServices.isEmpty()) {
            for (IInCallService inCallService : mInCallServices.values()) {
                try {
                    inCallService.bringToForeground(showDialpad);
                } catch (RemoteException ignored) {
                }
            }
        } else {
            Log.w(this, "Asking to bring unbound in-call UI to foreground.");
        }
    }

    /**
     * Unbinds an existing bound connection to the in-call app.
     */
    private void unbind() {
        ThreadUtil.checkOnMainThread();
        Iterator<Map.Entry<ComponentName, InCallServiceConnection>> iterator =
            mServiceConnections.entrySet().iterator();
        while (iterator.hasNext()) {
            Log.i(this, "Unbinding from InCallService %s");
            mContext.unbindService(iterator.next().getValue());
            iterator.remove();
        }
        mInCallServices.clear();
    }

    /**
     * Binds to the in-call app if not already connected by binding directly to the saved
     * component name of the {@link IInCallService} implementation.
     *
     * @param call The newly added call that triggered the binding to the in-call services.
     */
    private void bind(Call call) {
        ThreadUtil.checkOnMainThread();
        if (mInCallServices.isEmpty()) {
            PackageManager packageManager = mContext.getPackageManager();
            Intent serviceIntent = new Intent(InCallService.SERVICE_INTERFACE);

            for (ResolveInfo entry : packageManager.queryIntentServices(serviceIntent, 0)) {
                ServiceInfo serviceInfo = entry.serviceInfo;
                if (serviceInfo != null) {
                    boolean hasServiceBindPermission = serviceInfo.permission != null &&
                            serviceInfo.permission.equals(
                                    Manifest.permission.BIND_INCALL_SERVICE);
                    boolean hasControlInCallPermission = packageManager.checkPermission(
                            Manifest.permission.CONTROL_INCALL_EXPERIENCE,
                            serviceInfo.packageName) == PackageManager.PERMISSION_GRANTED;

                    if (!hasServiceBindPermission) {
                        Log.w(this, "InCallService does not have BIND_INCALL_SERVICE permission: " +
                                serviceInfo.packageName);
                        continue;
                    }

                    if (!hasControlInCallPermission) {
                        Log.w(this,
                                "InCall UI does not have CONTROL_INCALL_EXPERIENCE permission: " +
                                        serviceInfo.packageName);
                        continue;
                    }

                    InCallServiceConnection inCallServiceConnection = new InCallServiceConnection();
                    ComponentName componentName = new ComponentName(serviceInfo.packageName,
                            serviceInfo.name);

                    Log.i(this, "Attempting to bind to InCall %s, is dupe? %b ",
                            serviceInfo.packageName,
                            mServiceConnections.containsKey(componentName));

                    if (!mServiceConnections.containsKey(componentName)) {
                        Intent intent = new Intent(InCallService.SERVICE_INTERFACE);
                        intent.setComponent(componentName);

                        final int bindFlags;
                        if (mInCallComponentName.equals(componentName)) {
                            bindFlags = Context.BIND_AUTO_CREATE | Context.BIND_IMPORTANT;
                            if (!call.isIncoming()) {
                                intent.putExtra(TelecomManager.EXTRA_OUTGOING_CALL_EXTRAS,
                                        call.getExtras());
                                intent.putExtra(TelecomManager.EXTRA_PHONE_ACCOUNT_HANDLE,
                                        call.getTargetPhoneAccount());
                            }
                        } else {
                            bindFlags = Context.BIND_AUTO_CREATE;
                        }

                        if (mContext.bindServiceAsUser(intent, inCallServiceConnection, bindFlags,
                                UserHandle.CURRENT)) {
                            mServiceConnections.put(componentName, inCallServiceConnection);
                        }
                    }
                }
            }
        }
    }

    /**
     * Persists the {@link IInCallService} instance and starts the communication between
     * this class and in-call app by sending the first update to in-call app. This method is
     * called after a successful binding connection is established.
     *
     * @param componentName The service {@link ComponentName}.
     * @param service The {@link IInCallService} implementation.
     */
    private void onConnected(ComponentName componentName, IBinder service) {
        ThreadUtil.checkOnMainThread();
        Trace.beginSection("onConnected: " + componentName);
        Log.i(this, "onConnected to %s", componentName);

        IInCallService inCallService = IInCallService.Stub.asInterface(service);

        try {
            inCallService.setInCallAdapter(new InCallAdapter(CallsManager.getInstance(),
                    mCallIdMapper));
            mInCallServices.put(componentName, inCallService);
        } catch (RemoteException e) {
            Log.e(this, e, "Failed to set the in-call adapter.");
            Trace.endSection();
            return;
        }

        // Upon successful connection, send the state of the world to the service.
        Collection<Call> calls = CallsManager.getInstance().getCalls();
        if (!calls.isEmpty()) {
            Log.i(this, "Adding %s calls to InCallService after onConnected: %s", calls.size(),
                    componentName);
            for (Call call : calls) {
                try {
                    // Track the call if we don't already know about it.
                    Log.i(this, "addCall after binding: %s", call);
                    addCall(call);

                    inCallService.addCall(toParcelableCall(call,
                            componentName.equals(mInCallComponentName) /* includeVideoProvider */));
                } catch (RemoteException ignored) {
                }
            }
            onAudioStateChanged(null, CallsManager.getInstance().getAudioState());
            onCanAddCallChanged(CallsManager.getInstance().canAddCall());
        } else {
            unbind();
        }
        Trace.endSection();
    }

    /**
     * Cleans up an instance of in-call app after the service has been unbound.
     *
     * @param disconnectedComponent The {@link ComponentName} of the service which disconnected.
     */
    private void onDisconnected(ComponentName disconnectedComponent) {
        Log.i(this, "onDisconnected from %s", disconnectedComponent);
        ThreadUtil.checkOnMainThread();

        if (mInCallServices.containsKey(disconnectedComponent)) {
            mInCallServices.remove(disconnectedComponent);
        }

        if (mServiceConnections.containsKey(disconnectedComponent)) {
            // One of the services that we were bound to has disconnected. If the default in-call UI
            // has disconnected, disconnect all calls and un-bind all other InCallService
            // implementations.
            if (disconnectedComponent.equals(mInCallComponentName)) {
                Log.i(this, "In-call UI %s disconnected.", disconnectedComponent);
                CallsManager.getInstance().disconnectAllCalls();
                unbind();
            } else {
                Log.i(this, "In-Call Service %s suddenly disconnected", disconnectedComponent);
                // Else, if it wasn't the default in-call UI, then one of the other in-call services
                // disconnected and, well, that's probably their fault.  Clear their state and
                // ignore.
                InCallServiceConnection serviceConnection =
                        mServiceConnections.get(disconnectedComponent);

                // We still need to call unbind even though it disconnected.
                mContext.unbindService(serviceConnection);

                mServiceConnections.remove(disconnectedComponent);
                mInCallServices.remove(disconnectedComponent);
            }
        }
    }

    /**
     * Informs all {@link InCallService} instances of the updated call information.  Changes to the
     * video provider are only communicated to the default in-call UI.
     *
     * @param call The {@link Call}.
     */
    private void updateCall(Call call) {
        if (!mInCallServices.isEmpty()) {
            for (Map.Entry<ComponentName, IInCallService> entry : mInCallServices.entrySet()) {
                ComponentName componentName = entry.getKey();
                IInCallService inCallService = entry.getValue();
                ParcelableCall parcelableCall = toParcelableCall(call,
                        componentName.equals(mInCallComponentName) /* includeVideoProvider */);
                Log.v(this, "updateCall %s ==> %s", call, parcelableCall);
                try {
                    inCallService.updateCall(parcelableCall);
                } catch (RemoteException ignored) {
                }
            }
        }
    }

    /**
     * Parcels all information for a {@link Call} into a new {@link ParcelableCall} instance.
     *
     * @param call The {@link Call} to parcel.
     * @param includeVideoProvider When {@code true}, the {@link IVideoProvider} is included in the
     *      parceled call.  When {@code false}, the {@link IVideoProvider} is not included.
     * @return The {@link ParcelableCall} containing all call information from the {@link Call}.
     */
    private ParcelableCall toParcelableCall(Call call, boolean includeVideoProvider) {
        String callId = mCallIdMapper.getCallId(call);

        int state = call.getState();
        int capabilities = convertConnectionToCallCapabilities(call.getConnectionCapabilities());

        // If this is a single-SIM device, the "default SIM" will always be the only SIM.
        boolean isDefaultSmsAccount =
                CallsManager.getInstance().getPhoneAccountRegistrar().isUserSelectedSmsPhoneAccount(
                        call.getTargetPhoneAccount());
        if (call.isRespondViaSmsCapable() && isDefaultSmsAccount) {
            capabilities |= android.telecom.Call.Details.CAPABILITY_RESPOND_VIA_TEXT;
        }

        if (call.isEmergencyCall()) {
            capabilities = removeCapability(
                    capabilities, android.telecom.Call.Details.CAPABILITY_MUTE);
        }

        if (state == CallState.DIALING) {
            capabilities = removeCapability(
                    capabilities, android.telecom.Call.Details.CAPABILITY_SUPPORTS_VT_LOCAL);
            capabilities = removeCapability(
                    capabilities, android.telecom.Call.Details.CAPABILITY_SUPPORTS_VT_REMOTE);
        }

        if (state == CallState.ABORTED) {
            state = CallState.DISCONNECTED;
        }

        if (call.isLocallyDisconnecting() && state != CallState.DISCONNECTED) {
            state = CallState.DISCONNECTING;
        }

        String parentCallId = null;
        Call parentCall = call.getParentCall();
        if (parentCall != null) {
            parentCallId = mCallIdMapper.getCallId(parentCall);
        }

        long connectTimeMillis = call.getConnectTimeMillis();
        List<Call> childCalls = call.getChildCalls();
        List<String> childCallIds = new ArrayList<>();
        if (!childCalls.isEmpty()) {
            long childConnectTimeMillis = Long.MAX_VALUE;
            for (Call child : childCalls) {
                if (child.getConnectTimeMillis() > 0) {
                    childConnectTimeMillis = Math.min(child.getConnectTimeMillis(),
                            childConnectTimeMillis);
                }
                childCallIds.add(mCallIdMapper.getCallId(child));
            }

            if (childConnectTimeMillis != Long.MAX_VALUE) {
                connectTimeMillis = childConnectTimeMillis;
            }
        }

        Uri handle = call.getHandlePresentation() == TelecomManager.PRESENTATION_ALLOWED ?
                call.getHandle() : null;
        String callerDisplayName = call.getCallerDisplayNamePresentation() ==
                TelecomManager.PRESENTATION_ALLOWED ?  call.getCallerDisplayName() : null;

        List<Call> conferenceableCalls = call.getConferenceableCalls();
        List<String> conferenceableCallIds = new ArrayList<String>(conferenceableCalls.size());
        for (Call otherCall : conferenceableCalls) {
            String otherId = mCallIdMapper.getCallId(otherCall);
            if (otherId != null) {
                conferenceableCallIds.add(otherId);
            }
        }

        int properties = call.isConference() ? CallProperties.CONFERENCE : 0;
        return new ParcelableCall(
                callId,
                state,
                call.getDisconnectCause(),
                call.getCannedSmsResponses(),
                capabilities,
                properties,
                connectTimeMillis,
                handle,
                call.getHandlePresentation(),
                callerDisplayName,
                call.getCallerDisplayNamePresentation(),
                call.getGatewayInfo(),
                call.getTargetPhoneAccount(),
                includeVideoProvider ? call.getVideoProvider() : null,
                parentCallId,
                childCallIds,
                call.getStatusHints(),
                call.getVideoState(),
                conferenceableCallIds,
                call.getExtras());
    }

    private static final int[] CONNECTION_TO_CALL_CAPABILITY = new int[] {
        Connection.CAPABILITY_HOLD,
        android.telecom.Call.Details.CAPABILITY_HOLD,

        Connection.CAPABILITY_SUPPORT_HOLD,
        android.telecom.Call.Details.CAPABILITY_SUPPORT_HOLD,

        Connection.CAPABILITY_MERGE_CONFERENCE,
        android.telecom.Call.Details.CAPABILITY_MERGE_CONFERENCE,

        Connection.CAPABILITY_SWAP_CONFERENCE,
        android.telecom.Call.Details.CAPABILITY_SWAP_CONFERENCE,

        Connection.CAPABILITY_UNUSED,
        android.telecom.Call.Details.CAPABILITY_UNUSED,

        Connection.CAPABILITY_RESPOND_VIA_TEXT,
        android.telecom.Call.Details.CAPABILITY_RESPOND_VIA_TEXT,

        Connection.CAPABILITY_MUTE,
        android.telecom.Call.Details.CAPABILITY_MUTE,

        Connection.CAPABILITY_MANAGE_CONFERENCE,
        android.telecom.Call.Details.CAPABILITY_MANAGE_CONFERENCE,

        Connection.CAPABILITY_SUPPORTS_VT_LOCAL,
        android.telecom.Call.Details.CAPABILITY_SUPPORTS_VT_LOCAL,

        Connection.CAPABILITY_SUPPORTS_VT_REMOTE,
        android.telecom.Call.Details.CAPABILITY_SUPPORTS_VT_REMOTE,

        Connection.CAPABILITY_HIGH_DEF_AUDIO,
        android.telecom.Call.Details.CAPABILITY_HIGH_DEF_AUDIO,

        Connection.CAPABILITY_VoWIFI,
        android.telecom.Call.Details.CAPABILITY_VoWIFI,

        Connection.CAPABILITY_SEPARATE_FROM_CONFERENCE,
        android.telecom.Call.Details.CAPABILITY_SEPARATE_FROM_CONFERENCE,

        Connection.CAPABILITY_DISCONNECT_FROM_CONFERENCE,
        android.telecom.Call.Details.CAPABILITY_DISCONNECT_FROM_CONFERENCE,

        Connection.CAPABILITY_GENERIC_CONFERENCE,
        android.telecom.Call.Details.CAPABILITY_GENERIC_CONFERENCE
    };

    private static int convertConnectionToCallCapabilities(int connectionCapabilities) {
        int callCapabilities = 0;
        for (int i = 0; i < CONNECTION_TO_CALL_CAPABILITY.length; i += 2) {
            if ((CONNECTION_TO_CALL_CAPABILITY[i] & connectionCapabilities) != 0) {
                callCapabilities |= CONNECTION_TO_CALL_CAPABILITY[i + 1];
            }
        }
        return callCapabilities;
    }

    /**
     * Adds the call to the list of calls tracked by the {@link InCallController}.
     * @param call The call to add.
     */
    private void addCall(Call call) {
        if (mCallIdMapper.getCallId(call) == null) {
            mCallIdMapper.addCall(call);
            call.addListener(mCallListener);
        }
    }

    /**
     * Removes the specified capability from the set of capabilities bits and returns the new set.
     */
    private static int removeCapability(int capabilities, int capability) {
        return capabilities & ~capability;
    }

    /**
     * Dumps the state of the {@link InCallController}.
     *
     * @param pw The {@code IndentingPrintWriter} to write the state to.
     */
    public void dump(IndentingPrintWriter pw) {
        pw.println("mInCallServices (InCalls registered):");
        pw.increaseIndent();
        for (ComponentName componentName : mInCallServices.keySet()) {
            pw.println(componentName);
        }
        pw.decreaseIndent();

        pw.println("mServiceConnections (InCalls bound):");
        pw.increaseIndent();
        for (ComponentName componentName : mServiceConnections.keySet()) {
            pw.println(componentName);
        }
        pw.decreaseIndent();
    }
}
