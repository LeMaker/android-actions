/*
 * Copyright (c) 2013 The Android Open Source Project
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

package com.android.ims;

import com.android.internal.R;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map.Entry;
import java.util.Set;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.os.Message;
import android.telecom.ConferenceParticipant;
import android.telephony.Rlog;
import android.util.Log;

import com.android.ims.internal.ICall;
import com.android.ims.internal.ImsCallSession;
import com.android.ims.internal.ImsStreamMediaSession;
import com.android.internal.annotations.VisibleForTesting;

/**
 * Handles an IMS voice / video call over LTE. You can instantiate this class with
 * {@link ImsManager}.
 *
 * @hide
 */
public class ImsCall implements ICall {
    // Mode of USSD message
    public static final int USSD_MODE_NOTIFY = 0;
    public static final int USSD_MODE_REQUEST = 1;

    private static final String TAG = "ImsCall";
    private static final boolean FORCE_DEBUG = false; /* STOPSHIP if true */
    private static final boolean DBG = FORCE_DEBUG || Rlog.isLoggable(TAG, Log.DEBUG);
    private static final boolean VDBG = FORCE_DEBUG || Rlog.isLoggable(TAG, Log.VERBOSE);

    /**
     * Listener for events relating to an IMS call, such as when a call is being
     * received ("on ringing") or a call is outgoing ("on calling").
     * <p>Many of these events are also received by {@link ImsCallSession.Listener}.</p>
     */
    public static class Listener {
        /**
         * Called when a request is sent out to initiate a new call
         * and 1xx response is received from the network.
         * The default implementation calls {@link #onCallStateChanged}.
         *
         * @param call the call object that carries out the IMS call
         */
        public void onCallProgressing(ImsCall call) {
            onCallStateChanged(call);
        }

        /**
         * Called when the call is established.
         * The default implementation calls {@link #onCallStateChanged}.
         *
         * @param call the call object that carries out the IMS call
         */
        public void onCallStarted(ImsCall call) {
            onCallStateChanged(call);
        }

        /**
         * Called when the call setup is failed.
         * The default implementation calls {@link #onCallError}.
         *
         * @param call the call object that carries out the IMS call
         * @param reasonInfo detailed reason of the call setup failure
         */
        public void onCallStartFailed(ImsCall call, ImsReasonInfo reasonInfo) {
            onCallError(call, reasonInfo);
        }

        /**
         * Called when the call is terminated.
         * The default implementation calls {@link #onCallStateChanged}.
         *
         * @param call the call object that carries out the IMS call
         * @param reasonInfo detailed reason of the call termination
         */
        public void onCallTerminated(ImsCall call, ImsReasonInfo reasonInfo) {
            // Store the call termination reason

            onCallStateChanged(call);
        }

        /**
         * Called when the call is in hold.
         * The default implementation calls {@link #onCallStateChanged}.
         *
         * @param call the call object that carries out the IMS call
         */
        public void onCallHeld(ImsCall call) {
            onCallStateChanged(call);
        }

        /**
         * Called when the call hold is failed.
         * The default implementation calls {@link #onCallError}.
         *
         * @param call the call object that carries out the IMS call
         * @param reasonInfo detailed reason of the call hold failure
         */
        public void onCallHoldFailed(ImsCall call, ImsReasonInfo reasonInfo) {
            onCallError(call, reasonInfo);
        }

        /**
         * Called when the call hold is received from the remote user.
         * The default implementation calls {@link #onCallStateChanged}.
         *
         * @param call the call object that carries out the IMS call
         */
        public void onCallHoldReceived(ImsCall call) {
            onCallStateChanged(call);
        }

        /**
         * Called when the call is in call.
         * The default implementation calls {@link #onCallStateChanged}.
         *
         * @param call the call object that carries out the IMS call
         */
        public void onCallResumed(ImsCall call) {
            onCallStateChanged(call);
        }

        /**
         * Called when the call resume is failed.
         * The default implementation calls {@link #onCallError}.
         *
         * @param call the call object that carries out the IMS call
         * @param reasonInfo detailed reason of the call resume failure
         */
        public void onCallResumeFailed(ImsCall call, ImsReasonInfo reasonInfo) {
            onCallError(call, reasonInfo);
        }

        /**
         * Called when the call resume is received from the remote user.
         * The default implementation calls {@link #onCallStateChanged}.
         *
         * @param call the call object that carries out the IMS call
         */
        public void onCallResumeReceived(ImsCall call) {
            onCallStateChanged(call);
        }

        /**
         * Called when the call is in call.
         * The default implementation calls {@link #onCallStateChanged}.
         *
         * @param call the call object that carries out the IMS call
         * @param swapCalls {@code true} if the foreground and background calls should be swapped
         *                              now that the merge has completed.
         */
        public void onCallMerged(ImsCall call, boolean swapCalls) {
            onCallStateChanged(call);
        }

        /**
         * Called when the call merge is failed.
         * The default implementation calls {@link #onCallError}.
         *
         * @param call the call object that carries out the IMS call
         * @param reasonInfo detailed reason of the call merge failure
         */
        public void onCallMergeFailed(ImsCall call, ImsReasonInfo reasonInfo) {
            onCallError(call, reasonInfo);
        }

        /**
         * Called when the call is updated (except for hold/unhold).
         * The default implementation calls {@link #onCallStateChanged}.
         *
         * @param call the call object that carries out the IMS call
         */
        public void onCallUpdated(ImsCall call) {
            onCallStateChanged(call);
        }

        /**
         * Called when the call update is failed.
         * The default implementation calls {@link #onCallError}.
         *
         * @param call the call object that carries out the IMS call
         * @param reasonInfo detailed reason of the call update failure
         */
        public void onCallUpdateFailed(ImsCall call, ImsReasonInfo reasonInfo) {
            onCallError(call, reasonInfo);
        }

        /**
         * Called when the call update is received from the remote user.
         *
         * @param call the call object that carries out the IMS call
         */
        public void onCallUpdateReceived(ImsCall call) {
            // no-op
        }

        /**
         * Called when the call is extended to the conference call.
         * The default implementation calls {@link #onCallStateChanged}.
         *
         * @param call the call object that carries out the IMS call
         * @param newCall the call object that is extended to the conference from the active call
         */
        public void onCallConferenceExtended(ImsCall call, ImsCall newCall) {
            onCallStateChanged(call);
        }

        /**
         * Called when the conference extension is failed.
         * The default implementation calls {@link #onCallError}.
         *
         * @param call the call object that carries out the IMS call
         * @param reasonInfo detailed reason of the conference extension failure
         */
        public void onCallConferenceExtendFailed(ImsCall call,
                ImsReasonInfo reasonInfo) {
            onCallError(call, reasonInfo);
        }

        /**
         * Called when the conference extension is received from the remote user.
         *
         * @param call the call object that carries out the IMS call
         * @param newCall the call object that is extended to the conference from the active call
         */
        public void onCallConferenceExtendReceived(ImsCall call, ImsCall newCall) {
            onCallStateChanged(call);
        }

        /**
         * Called when the invitation request of the participants is delivered to
         * the conference server.
         *
         * @param call the call object that carries out the IMS call
         */
        public void onCallInviteParticipantsRequestDelivered(ImsCall call) {
            // no-op
        }

        /**
         * Called when the invitation request of the participants is failed.
         *
         * @param call the call object that carries out the IMS call
         * @param reasonInfo detailed reason of the conference invitation failure
         */
        public void onCallInviteParticipantsRequestFailed(ImsCall call,
                ImsReasonInfo reasonInfo) {
            // no-op
        }

        /**
         * Called when the removal request of the participants is delivered to
         * the conference server.
         *
         * @param call the call object that carries out the IMS call
         */
        public void onCallRemoveParticipantsRequestDelivered(ImsCall call) {
            // no-op
        }

        /**
         * Called when the removal request of the participants is failed.
         *
         * @param call the call object that carries out the IMS call
         * @param reasonInfo detailed reason of the conference removal failure
         */
        public void onCallRemoveParticipantsRequestFailed(ImsCall call,
                ImsReasonInfo reasonInfo) {
            // no-op
        }

        /**
         * Called when the conference state is updated.
         *
         * @param call the call object that carries out the IMS call
         * @param state state of the participant who is participated in the conference call
         */
        public void onCallConferenceStateUpdated(ImsCall call, ImsConferenceState state) {
            // no-op
        }

        /**
         * Called when the state of IMS conference participant(s) has changed.
         *
         * @param call the call object that carries out the IMS call.
         * @param participants the participant(s) and their new state information.
         */
        public void onConferenceParticipantsStateChanged(ImsCall call,
                List<ConferenceParticipant> participants) {
            // no-op
        }

        /**
         * Called when the USSD message is received from the network.
         *
         * @param mode mode of the USSD message (REQUEST / NOTIFY)
         * @param ussdMessage USSD message
         */
        public void onCallUssdMessageReceived(ImsCall call,
                int mode, String ussdMessage) {
            // no-op
        }

        /**
         * Called when an error occurs. The default implementation is no op.
         * overridden. The default implementation is no op. Error events are
         * not re-directed to this callback and are handled in {@link #onCallError}.
         *
         * @param call the call object that carries out the IMS call
         * @param reasonInfo detailed reason of this error
         * @see ImsReasonInfo
         */
        public void onCallError(ImsCall call, ImsReasonInfo reasonInfo) {
            // no-op
        }

        /**
         * Called when an event occurs and the corresponding callback is not
         * overridden. The default implementation is no op. Error events are
         * not re-directed to this callback and are handled in {@link #onCallError}.
         *
         * @param call the call object that carries out the IMS call
         */
        public void onCallStateChanged(ImsCall call) {
            // no-op
        }

        /**
         * Called when the call moves the hold state to the conversation state.
         * For example, when merging the active & hold call, the state of all the hold call
         * will be changed from hold state to conversation state.
         * This callback method can be invoked even though the application does not trigger
         * any operations.
         *
         * @param call the call object that carries out the IMS call
         * @param state the detailed state of call state changes;
         *      Refer to CALL_STATE_* in {@link ImsCall}
         */
        public void onCallStateChanged(ImsCall call, int state) {
            // no-op
        }

        /**
         * Called when TTY mode of remote party changed
         *
         * @param call the call object that carries out the IMS call
         * @param mode TTY mode of remote party
         */
        public void onCallSessionTtyModeReceived(ImsCall call, int mode) {
            // no-op
        }
    }



    // List of update operation for IMS call control
    private static final int UPDATE_NONE = 0;
    private static final int UPDATE_HOLD = 1;
    private static final int UPDATE_HOLD_MERGE = 2;
    private static final int UPDATE_RESUME = 3;
    private static final int UPDATE_MERGE = 4;
    private static final int UPDATE_EXTEND_TO_CONFERENCE = 5;
    private static final int UPDATE_UNSPECIFIED = 6;

    // For synchronization of private variables
    private Object mLockObj = new Object();
    private Context mContext;

    // true if the call is established & in the conversation state
    private boolean mInCall = false;
    // true if the call is on hold
    // If it is triggered by the local, mute the call. Otherwise, play local hold tone
    // or network generated media.
    private boolean mHold = false;
    // true if the call is on mute
    private boolean mMute = false;
    // It contains the exclusive call update request. Refer to UPDATE_*.
    private int mUpdateRequest = UPDATE_NONE;

    private ImsCall.Listener mListener = null;

    // When merging two calls together, the "peer" call that will merge into this call.
    private ImsCall mMergePeer = null;
    // When merging two calls together, the "host" call we are merging into.
    private ImsCall mMergeHost = null;

    // Wrapper call session to interworking the IMS service (server).
    private ImsCallSession mSession = null;
    // Call profile of the current session.
    // It can be changed at anytime when the call is updated.
    private ImsCallProfile mCallProfile = null;
    // Call profile to be updated after the application's action (accept/reject)
    // to the call update. After the application's action (accept/reject) is done,
    // it will be set to null.
    private ImsCallProfile mProposedCallProfile = null;
    private ImsReasonInfo mLastReasonInfo = null;

    // Media session to control media (audio/video) operations for an IMS call
    private ImsStreamMediaSession mMediaSession = null;

    // The temporary ImsCallSession that could represent the merged call once
    // we receive notification that the merge was successful.
    private ImsCallSession mTransientConferenceSession = null;
    // While a merge is progressing, we bury any session termination requests
    // made on the original ImsCallSession until we have closure on the merge request
    // If the request ultimately fails, we need to act on the termination request
    // that we buried temporarily. We do this because we feel that timing issues could
    // cause the termination request to occur just because the merge is succeeding.
    private boolean mSessionEndDuringMerge = false;
    // Just like mSessionEndDuringMerge, we need to keep track of the reason why the
    // termination request was made on the original session in case we need to act
    // on it in the case of a merge failure.
    private ImsReasonInfo mSessionEndDuringMergeReasonInfo = null;
    // This flag is used to indicate if this ImsCall was merged into a conference
    // or not.  It is used primarily to determine if a disconnect sound should
    // be heard when the call is terminated.
    private boolean mIsMerged = false;
    // If true, this flag means that this ImsCall is in the process of merging
    // into a conference but it does not yet have closure on if it was
    // actually added to the conference or not. false implies that it either
    // is not part of a merging conference or already knows if it was
    // successfully added.
    private boolean mCallSessionMergePending = false;

    /**
     * Create an IMS call object.
     *
     * @param context the context for accessing system services
     * @param profile the call profile to make/take a call
     */
    public ImsCall(Context context, ImsCallProfile profile) {
        mContext = context;
        mCallProfile = profile;
    }

    /**
     * Closes this object. This object is not usable after being closed.
     */
    @Override
    public void close() {
        synchronized(mLockObj) {
            if (mSession != null) {
                mSession.close();
                mSession = null;
            }

            mCallProfile = null;
            mProposedCallProfile = null;
            mLastReasonInfo = null;
            mMediaSession = null;
        }
    }

    /**
     * Checks if the call has a same remote user identity or not.
     *
     * @param userId the remote user identity
     * @return true if the remote user identity is equal; otherwise, false
     */
    @Override
    public boolean checkIfRemoteUserIsSame(String userId) {
        if (userId == null) {
            return false;
        }

        return userId.equals(mCallProfile.getCallExtra(ImsCallProfile.EXTRA_REMOTE_URI, ""));
    }

    /**
     * Checks if the call is equal or not.
     *
     * @param call the call to be compared
     * @return true if the call is equal; otherwise, false
     */
    @Override
    public boolean equalsTo(ICall call) {
        if (call == null) {
            return false;
        }

        if (call instanceof ImsCall) {
            return this.equals(call);
        }

        return false;
    }

    public static boolean isSessionAlive(ImsCallSession session) {
        return session != null && session.isAlive();
    }

    /**
     * Gets the negotiated (local & remote) call profile.
     *
     * @return a {@link ImsCallProfile} object that has the negotiated call profile
     */
    public ImsCallProfile getCallProfile() {
        synchronized(mLockObj) {
            return mCallProfile;
        }
    }

    /**
     * Gets the local call profile (local capabilities).
     *
     * @return a {@link ImsCallProfile} object that has the local call profile
     */
    public ImsCallProfile getLocalCallProfile() throws ImsException {
        synchronized(mLockObj) {
            if (mSession == null) {
                throw new ImsException("No call session",
                        ImsReasonInfo.CODE_LOCAL_CALL_TERMINATED);
            }

            try {
                return mSession.getLocalCallProfile();
            } catch (Throwable t) {
                loge("getLocalCallProfile :: ", t);
                throw new ImsException("getLocalCallProfile()", t, 0);
            }
        }
    }

    /**
     * Gets the remote call profile (remote capabilities).
     *
     * @return a {@link ImsCallProfile} object that has the remote call profile
     */
    public ImsCallProfile getRemoteCallProfile() throws ImsException {
        synchronized(mLockObj) {
            if (mSession == null) {
                throw new ImsException("No call session",
                        ImsReasonInfo.CODE_LOCAL_CALL_TERMINATED);
            }

            try {
                return mSession.getRemoteCallProfile();
            } catch (Throwable t) {
                loge("getRemoteCallProfile :: ", t);
                throw new ImsException("getRemoteCallProfile()", t, 0);
            }
        }
    }

    /**
     * Gets the call profile proposed by the local/remote user.
     *
     * @return a {@link ImsCallProfile} object that has the proposed call profile
     */
    public ImsCallProfile getProposedCallProfile() {
        synchronized(mLockObj) {
            if (!isInCall()) {
                return null;
            }

            return mProposedCallProfile;
        }
    }

    /**
     * Gets the state of the {@link ImsCallSession} that carries this call.
     * The value returned must be one of the states in {@link ImsCallSession#State}.
     *
     * @return the session state
     */
    public int getState() {
        synchronized(mLockObj) {
            if (mSession == null) {
                return ImsCallSession.State.IDLE;
            }

            return mSession.getState();
        }
    }

    /**
     * Gets the {@link ImsCallSession} that carries this call.
     *
     * @return the session object that carries this call
     * @hide
     */
    public ImsCallSession getCallSession() {
        synchronized(mLockObj) {
            return mSession;
        }
    }

    /**
     * Gets the {@link ImsStreamMediaSession} that handles the media operation of this call.
     * Almost interface APIs are for the VT (Video Telephony).
     *
     * @return the media session object that handles the media operation of this call
     * @hide
     */
    public ImsStreamMediaSession getMediaSession() {
        synchronized(mLockObj) {
            return mMediaSession;
        }
    }

    /**
     * Gets the specified property of this call.
     *
     * @param name key to get the extra call information defined in {@link ImsCallProfile}
     * @return the extra call information as string
     */
    public String getCallExtra(String name) throws ImsException {
        // Lookup the cache

        synchronized(mLockObj) {
            // If not found, try to get the property from the remote
            if (mSession == null) {
                throw new ImsException("No call session",
                        ImsReasonInfo.CODE_LOCAL_CALL_TERMINATED);
            }

            try {
                return mSession.getProperty(name);
            } catch (Throwable t) {
                loge("getCallExtra :: ", t);
                throw new ImsException("getCallExtra()", t, 0);
            }
        }
    }

    /**
     * Gets the last reason information when the call is not established, cancelled or terminated.
     *
     * @return the last reason information
     */
    public ImsReasonInfo getLastReasonInfo() {
        synchronized(mLockObj) {
            return mLastReasonInfo;
        }
    }

    /**
     * Checks if the call has a pending update operation.
     *
     * @return true if the call has a pending update operation
     */
    public boolean hasPendingUpdate() {
        synchronized(mLockObj) {
            return (mUpdateRequest != UPDATE_NONE);
        }
    }

    /**
     * Checks if the call is established.
     *
     * @return true if the call is established
     */
    public boolean isInCall() {
        synchronized(mLockObj) {
            return mInCall;
        }
    }

    /**
     * Checks if the call is muted.
     *
     * @return true if the call is muted
     */
    public boolean isMuted() {
        synchronized(mLockObj) {
            return mMute;
        }
    }

    /**
     * Checks if the call is on hold.
     *
     * @return true if the call is on hold
     */
    public boolean isOnHold() {
        synchronized(mLockObj) {
            return mHold;
        }
    }

    /**
     * Determines if the call is a multiparty call.
     *
     * @return {@code True} if the call is a multiparty call.
     */
    public boolean isMultiparty() {
        synchronized(mLockObj) {
            if (mSession == null) {
                return false;
            }

            return mSession.isMultiparty();
        }
    }

    /**
     * Marks whether an IMS call is merged. This should be set {@code true} when the call merges
     * into a conference.
     *
     * @param isMerged Whether the call is merged.
     */
    public void setIsMerged(boolean isMerged) {
        mIsMerged = isMerged;
    }

    /**
     * @return {@code true} if the call recently merged into a conference call.
     */
    public boolean isMerged() {
        return mIsMerged;
    }

    /**
     * Sets the listener to listen to the IMS call events.
     * The method calls {@link #setListener setListener(listener, false)}.
     *
     * @param listener to listen to the IMS call events of this object; null to remove listener
     * @see #setListener(Listener, boolean)
     */
    public void setListener(ImsCall.Listener listener) {
        setListener(listener, false);
    }

    /**
     * Sets the listener to listen to the IMS call events.
     * A {@link ImsCall} can only hold one listener at a time. Subsequent calls
     * to this method override the previous listener.
     *
     * @param listener to listen to the IMS call events of this object; null to remove listener
     * @param callbackImmediately set to true if the caller wants to be called
     *        back immediately on the current state
     */
    public void setListener(ImsCall.Listener listener, boolean callbackImmediately) {
        boolean inCall;
        boolean onHold;
        int state;
        ImsReasonInfo lastReasonInfo;

        synchronized(mLockObj) {
            mListener = listener;

            if ((listener == null) || !callbackImmediately) {
                return;
            }

            inCall = mInCall;
            onHold = mHold;
            state = getState();
            lastReasonInfo = mLastReasonInfo;
        }

        try {
            if (lastReasonInfo != null) {
                listener.onCallError(this, lastReasonInfo);
            } else if (inCall) {
                if (onHold) {
                    listener.onCallHeld(this);
                } else {
                    listener.onCallStarted(this);
                }
            } else {
                switch (state) {
                    case ImsCallSession.State.ESTABLISHING:
                        listener.onCallProgressing(this);
                        break;
                    case ImsCallSession.State.TERMINATED:
                        listener.onCallTerminated(this, lastReasonInfo);
                        break;
                    default:
                        // Ignore it. There is no action in the other state.
                        break;
                }
            }
        } catch (Throwable t) {
            loge("setListener()", t);
        }
    }

    /**
     * Mutes or unmutes the mic for the active call.
     *
     * @param muted true if the call is muted, false otherwise
     */
    public void setMute(boolean muted) throws ImsException {
        synchronized(mLockObj) {
            if (mMute != muted) {
                mMute = muted;

                try {
                    mSession.setMute(muted);
                } catch (Throwable t) {
                    loge("setMute :: ", t);
                    throwImsException(t, 0);
                }
            }
        }
    }

     /**
      * Attaches an incoming call to this call object.
      *
      * @param session the session that receives the incoming call
      * @throws ImsException if the IMS service fails to attach this object to the session
      */
     public void attachSession(ImsCallSession session) throws ImsException {
         if (DBG) {
             log("attachSession :: session=" + session);
         }

         synchronized(mLockObj) {
             mSession = session;

             try {
                 mSession.setListener(createCallSessionListener());
             } catch (Throwable t) {
                 loge("attachSession :: ", t);
                 throwImsException(t, 0);
             }
         }
     }

    /**
     * Initiates an IMS call with the call profile which is provided
     * when creating a {@link ImsCall}.
     *
     * @param session the {@link ImsCallSession} for carrying out the call
     * @param callee callee information to initiate an IMS call
     * @throws ImsException if the IMS service fails to initiate the call
     */
    public void start(ImsCallSession session, String callee)
            throws ImsException {
        if (DBG) {
            log("start(1) :: session=" + session + ", callee=" + callee);
        }

        synchronized(mLockObj) {
            mSession = session;

            try {
                session.setListener(createCallSessionListener());
                session.start(callee, mCallProfile);
            } catch (Throwable t) {
                loge("start(1) :: ", t);
                throw new ImsException("start(1)", t, 0);
            }
        }
    }

    /**
     * Initiates an IMS conferenca call with the call profile which is provided
     * when creating a {@link ImsCall}.
     *
     * @param session the {@link ImsCallSession} for carrying out the call
     * @param participants participant list to initiate an IMS conference call
     * @throws ImsException if the IMS service fails to initiate the call
     */
    public void start(ImsCallSession session, String[] participants)
            throws ImsException {
        if (DBG) {
            log("start(n) :: session=" + session + ", callee=" + participants);
        }

        synchronized(mLockObj) {
            mSession = session;

            try {
                session.setListener(createCallSessionListener());
                session.start(participants, mCallProfile);
            } catch (Throwable t) {
                loge("start(n) :: ", t);
                throw new ImsException("start(n)", t, 0);
            }
        }
    }

    /**
     * Accepts a call.
     *
     * @see Listener#onCallStarted
     *
     * @param callType The call type the user agreed to for accepting the call.
     * @throws ImsException if the IMS service fails to accept the call
     */
    public void accept(int callType) throws ImsException {
        if (VDBG) {
            log("accept ::");
        }

        accept(callType, new ImsStreamMediaProfile());
    }

    /**
     * Accepts a call.
     *
     * @param callType call type to be answered in {@link ImsCallProfile}
     * @param profile a media profile to be answered (audio/audio & video, direction, ...)
     * @see Listener#onCallStarted
     * @throws ImsException if the IMS service fails to accept the call
     */
    public void accept(int callType, ImsStreamMediaProfile profile) throws ImsException {
        if (VDBG) {
            log("accept :: callType=" + callType + ", profile=" + profile);
        }

        synchronized(mLockObj) {
            if (mSession == null) {
                throw new ImsException("No call to answer",
                        ImsReasonInfo.CODE_LOCAL_CALL_TERMINATED);
            }

            try {
                mSession.accept(callType, profile);
            } catch (Throwable t) {
                loge("accept :: ", t);
                throw new ImsException("accept()", t, 0);
            }

            if (mInCall && (mProposedCallProfile != null)) {
                if (DBG) {
                    log("accept :: call profile will be updated");
                }

                mCallProfile = mProposedCallProfile;
                mProposedCallProfile = null;
            }

            // Other call update received
            if (mInCall && (mUpdateRequest == UPDATE_UNSPECIFIED)) {
                mUpdateRequest = UPDATE_NONE;
            }
        }
    }

    /**
     * Rejects a call.
     *
     * @param reason reason code to reject an incoming call
     * @see Listener#onCallStartFailed
     * @throws ImsException if the IMS service fails to accept the call
     */
    public void reject(int reason) throws ImsException {
        if (VDBG) {
            log("reject :: reason=" + reason);
        }

        synchronized(mLockObj) {
            if (mSession != null) {
                mSession.reject(reason);
            }

            if (mInCall && (mProposedCallProfile != null)) {
                if (DBG) {
                    log("reject :: call profile is not updated; destroy it...");
                }

                mProposedCallProfile = null;
            }

            // Other call update received
            if (mInCall && (mUpdateRequest == UPDATE_UNSPECIFIED)) {
                mUpdateRequest = UPDATE_NONE;
            }
        }
    }

    /**
     * Terminates an IMS call.
     *
     * @param reason reason code to terminate a call
     * @throws ImsException if the IMS service fails to terminate the call
     */
    public void terminate(int reason) throws ImsException {
        if (VDBG) {
            log("terminate :: ImsCall=" + this +" reason=" + reason);
        }

        synchronized(mLockObj) {
            mHold = false;
            mInCall = false;

            if (mSession != null) {
                // TODO: Fix the fact that user invoked call terminations during
                // the process of establishing a conference call needs to be handled
                // as a special case.
                // Currently, any terminations (both invoked by the user or
                // by the network results in a callSessionTerminated() callback
                // from the network.  When establishing a conference call we bury
                // these callbacks until we get closure on all participants of the
                // conference. In some situations, we will throw away the callback
                // (when the underlying session of the host of the new conference
                // is terminated) or will will unbury it when the conference has been
                // established, like when the peer of the new conference goes away
                // after the conference has been created.  The UI relies on the callback
                // to reflect the fact that the call is gone.
                // So if a user decides to terminated a call while it is merging, it
                // could take a long time to reflect in the UI due to the conference
                // processing but we should probably cancel that and just terminate
                // the call immediately and clean up.  This is not a huge issue right
                // now because we have not seen instances where establishing a
                // conference takes a long time (more than a second or two).
                mSession.terminate(reason);
            }
        }
    }


    /**
     * Puts a call on hold. When succeeds, {@link Listener#onCallHeld} is called.
     *
     * @see Listener#onCallHeld, Listener#onCallHoldFailed
     * @throws ImsException if the IMS service fails to hold the call
     */
    public void hold() throws ImsException {
        if (VDBG) {
            log("hold :: ImsCall=" + this);
        }

        if (isOnHold()) {
            if (DBG) {
                log("hold :: call is already on hold");
            }
            return;
        }

        synchronized(mLockObj) {
            if (mUpdateRequest != UPDATE_NONE) {
                loge("hold :: update is in progress; request=" +
                        updateRequestToString(mUpdateRequest));
                throw new ImsException("Call update is in progress",
                        ImsReasonInfo.CODE_LOCAL_ILLEGAL_STATE);
            }

            if (mSession == null) {
                loge("hold :: ");
                throw new ImsException("No call session",
                        ImsReasonInfo.CODE_LOCAL_CALL_TERMINATED);
            }

            mSession.hold(createHoldMediaProfile());
            // FIXME: update the state on the callback?
            mHold = true;
            mUpdateRequest = UPDATE_HOLD;
        }
    }

    /**
     * Continues a call that's on hold. When succeeds, {@link Listener#onCallResumed} is called.
     *
     * @see Listener#onCallResumed, Listener#onCallResumeFailed
     * @throws ImsException if the IMS service fails to resume the call
     */
    public void resume() throws ImsException {
        if (VDBG) {
            log("resume :: ImsCall=" + this);
        }

        if (!isOnHold()) {
            if (DBG) {
                log("resume :: call is in conversation");
            }
            return;
        }

        synchronized(mLockObj) {
            if (mUpdateRequest != UPDATE_NONE) {
                loge("resume :: update is in progress; request=" +
                        updateRequestToString(mUpdateRequest));
                throw new ImsException("Call update is in progress",
                        ImsReasonInfo.CODE_LOCAL_ILLEGAL_STATE);
            }

            if (mSession == null) {
                loge("resume :: ");
                throw new ImsException("No call session",
                        ImsReasonInfo.CODE_LOCAL_CALL_TERMINATED);
            }

            // mHold is set to false in confirmation callback that the
            // ImsCall was resumed.
            mUpdateRequest = UPDATE_RESUME;
            mSession.resume(createResumeMediaProfile());
        }
    }

    /**
     * Merges the active & hold call.
     *
     * @see Listener#onCallMerged, Listener#onCallMergeFailed
     * @throws ImsException if the IMS service fails to merge the call
     */
    private void merge() throws ImsException {
        if (VDBG) {
            log("merge :: ImsCall=" + this);
        }

        synchronized(mLockObj) {
            if (mUpdateRequest != UPDATE_NONE) {
                loge("merge :: update is in progress; request=" +
                        updateRequestToString(mUpdateRequest));
                throw new ImsException("Call update is in progress",
                        ImsReasonInfo.CODE_LOCAL_ILLEGAL_STATE);
            }

            if (mSession == null) {
                loge("merge :: no call session");
                throw new ImsException("No call session",
                        ImsReasonInfo.CODE_LOCAL_CALL_TERMINATED);
            }

            // if skipHoldBeforeMerge = true, IMS service implementation will
            // merge without explicitly holding the call.
            if (mHold || (mContext.getResources().getBoolean(
                    com.android.internal.R.bool.skipHoldBeforeMerge))) {

                if (mMergePeer != null && !mMergePeer.isMultiparty() && !isMultiparty()) {
                    // We only set UPDATE_MERGE when we are adding the first
                    // calls to the Conference.  If there is already a conference
                    // no special handling is needed. The existing conference
                    // session will just go active and any other sessions will be terminated
                    // if needed.  There will be no merge failed callback.
                    // Mark both the host and peer UPDATE_MERGE to ensure both are aware that a
                    // merge is pending.
                    mUpdateRequest = UPDATE_MERGE;
                    mMergePeer.mUpdateRequest = UPDATE_MERGE;
                }

                mSession.merge();
            } else {
                // This code basically says, we need to explicitly hold before requesting a merge
                // when we get the callback that the hold was successful (or failed), we should
                // automatically request a merge.
                mSession.hold(createHoldMediaProfile());
                mHold = true;
                mUpdateRequest = UPDATE_HOLD_MERGE;
            }
        }
    }

    /**
     * Merges the active & hold call.
     *
     * @param bgCall the background (holding) call
     * @see Listener#onCallMerged, Listener#onCallMergeFailed
     * @throws ImsException if the IMS service fails to merge the call
     */
    public void merge(ImsCall bgCall) throws ImsException {
        if (VDBG) {
            log("merge(1) :: bgImsCall=" + bgCall);
        }

        if (bgCall == null) {
            throw new ImsException("No background call",
                    ImsReasonInfo.CODE_LOCAL_ILLEGAL_ARGUMENT);
        }

        synchronized(mLockObj) {
            // Mark both sessions as pending merge.
            this.setCallSessionMergePending(true);
            bgCall.setCallSessionMergePending(true);

            if ((!isMultiparty() && !bgCall.isMultiparty()) || isMultiparty()) {
                // If neither call is multiparty, the current call is the merge host and the bg call
                // is the merge peer (ie we're starting a new conference).
                // OR
                // If this call is multiparty, it is the merge host and the other call is the merge
                // peer.
                setMergePeer(bgCall);
            } else {
                // If the bg call is multiparty, it is the merge host.
                setMergeHost(bgCall);
            }
        }
        merge();
    }

    /**
     * Updates the current call's properties (ex. call mode change: video upgrade / downgrade).
     */
    public void update(int callType, ImsStreamMediaProfile mediaProfile) throws ImsException {
        if (VDBG) {
            log("update ::");
        }

        if (isOnHold()) {
            if (DBG) {
                log("update :: call is on hold");
            }
            throw new ImsException("Not in a call to update call",
                    ImsReasonInfo.CODE_LOCAL_ILLEGAL_STATE);
        }

        synchronized(mLockObj) {
            if (mUpdateRequest != UPDATE_NONE) {
                if (DBG) {
                    log("update :: update is in progress; request=" +
                            updateRequestToString(mUpdateRequest));
                }
                throw new ImsException("Call update is in progress",
                        ImsReasonInfo.CODE_LOCAL_ILLEGAL_STATE);
            }

            if (mSession == null) {
                loge("update :: ");
                throw new ImsException("No call session",
                        ImsReasonInfo.CODE_LOCAL_CALL_TERMINATED);
            }

            mSession.update(callType, mediaProfile);
            mUpdateRequest = UPDATE_UNSPECIFIED;
        }
    }

    /**
     * Extends this call (1-to-1 call) to the conference call
     * inviting the specified participants to.
     *
     */
    public void extendToConference(String[] participants) throws ImsException {
        if (VDBG) {
            log("extendToConference ::");
        }

        if (isOnHold()) {
            if (DBG) {
                log("extendToConference :: call is on hold");
            }
            throw new ImsException("Not in a call to extend a call to conference",
                    ImsReasonInfo.CODE_LOCAL_ILLEGAL_STATE);
        }

        synchronized(mLockObj) {
            if (mUpdateRequest != UPDATE_NONE) {
                if (DBG) {
                    log("extendToConference :: update is in progress; request=" +
                            updateRequestToString(mUpdateRequest));
                }
                throw new ImsException("Call update is in progress",
                        ImsReasonInfo.CODE_LOCAL_ILLEGAL_STATE);
            }

            if (mSession == null) {
                loge("extendToConference :: ");
                throw new ImsException("No call session",
                        ImsReasonInfo.CODE_LOCAL_CALL_TERMINATED);
            }

            mSession.extendToConference(participants);
            mUpdateRequest = UPDATE_EXTEND_TO_CONFERENCE;
        }
    }

    /**
     * Requests the conference server to invite an additional participants to the conference.
     *
     */
    public void inviteParticipants(String[] participants) throws ImsException {
        if (VDBG) {
            log("inviteParticipants ::");
        }

        synchronized(mLockObj) {
            if (mSession == null) {
                loge("inviteParticipants :: ");
                throw new ImsException("No call session",
                        ImsReasonInfo.CODE_LOCAL_CALL_TERMINATED);
            }

            mSession.inviteParticipants(participants);
        }
    }

    /**
     * Requests the conference server to remove the specified participants from the conference.
     *
     */
    public void removeParticipants(String[] participants) throws ImsException {
        if (DBG) {
            log("removeParticipants ::");
        }

        synchronized(mLockObj) {
            if (mSession == null) {
                loge("removeParticipants :: ");
                throw new ImsException("No call session",
                        ImsReasonInfo.CODE_LOCAL_CALL_TERMINATED);
            }

            mSession.removeParticipants(participants);
        }
    }

    /**
     * Sends a DTMF code. According to <a href="http://tools.ietf.org/html/rfc2833">RFC 2833</a>,
     * event 0 ~ 9 maps to decimal value 0 ~ 9, '*' to 10, '#' to 11, event 'A' ~ 'D' to 12 ~ 15,
     * and event flash to 16. Currently, event flash is not supported.
     *
     * @param c that represents the DTMF to send. '0' ~ '9', 'A' ~ 'D', '*', '#' are valid inputs.
     * @param result the result message to send when done.
     */
    public void sendDtmf(char c, Message result) {
        if (VDBG) {
            log("sendDtmf :: code=" + c);
        }

        synchronized(mLockObj) {
            if (mSession != null) {
                mSession.sendDtmf(c, result);
            }
        }
    }

    /**
     * Start a DTMF code. According to <a href="http://tools.ietf.org/html/rfc2833">RFC 2833</a>,
     * event 0 ~ 9 maps to decimal value 0 ~ 9, '*' to 10, '#' to 11, event 'A' ~ 'D' to 12 ~ 15,
     * and event flash to 16. Currently, event flash is not supported.
     *
     * @param c that represents the DTMF to send. '0' ~ '9', 'A' ~ 'D', '*', '#' are valid inputs.
     */
    public void startDtmf(char c) {
        if (DBG) {
            log("startDtmf :: session=" + mSession + ", code=" + c);
        }

        synchronized(mLockObj) {
            if (mSession != null) {
                mSession.startDtmf(c);
            }
        }
    }

    /**
     * Stop a DTMF code.
     */
    public void stopDtmf() {
        if (DBG) {
            log("stopDtmf :: session=" + mSession);
        }

        synchronized(mLockObj) {
            if (mSession != null) {
                mSession.stopDtmf();
            }
        }
    }

    /**
     * Sends an USSD message.
     *
     * @param ussdMessage USSD message to send
     */
    public void sendUssd(String ussdMessage) throws ImsException {
        if (VDBG) {
            log("sendUssd :: ussdMessage=" + ussdMessage);
        }

        synchronized(mLockObj) {
            if (mSession == null) {
                loge("sendUssd :: ");
                throw new ImsException("No call session",
                        ImsReasonInfo.CODE_LOCAL_CALL_TERMINATED);
            }

            mSession.sendUssd(ussdMessage);
        }
    }

    private void clear(ImsReasonInfo lastReasonInfo) {
        mInCall = false;
        mHold = false;
        mUpdateRequest = UPDATE_NONE;
        mLastReasonInfo = lastReasonInfo;
    }

    /**
     * Creates an IMS call session listener.
     */
    private ImsCallSession.Listener createCallSessionListener() {
        return new ImsCallSessionListenerProxy();
    }

    private ImsCall createNewCall(ImsCallSession session, ImsCallProfile profile) {
        ImsCall call = new ImsCall(mContext, profile);

        try {
            call.attachSession(session);
        } catch (ImsException e) {
            if (call != null) {
                call.close();
                call = null;
            }
        }

        // Do additional operations...

        return call;
    }

    private ImsStreamMediaProfile createHoldMediaProfile() {
        ImsStreamMediaProfile mediaProfile = new ImsStreamMediaProfile();

        if (mCallProfile == null) {
            return mediaProfile;
        }

        mediaProfile.mAudioQuality = mCallProfile.mMediaProfile.mAudioQuality;
        mediaProfile.mVideoQuality = mCallProfile.mMediaProfile.mVideoQuality;
        mediaProfile.mAudioDirection = ImsStreamMediaProfile.DIRECTION_SEND;

        if (mediaProfile.mVideoQuality != ImsStreamMediaProfile.VIDEO_QUALITY_NONE) {
            mediaProfile.mVideoDirection = ImsStreamMediaProfile.DIRECTION_SEND;
        }

        return mediaProfile;
    }

    private ImsStreamMediaProfile createResumeMediaProfile() {
        ImsStreamMediaProfile mediaProfile = new ImsStreamMediaProfile();

        if (mCallProfile == null) {
            return mediaProfile;
        }

        mediaProfile.mAudioQuality = mCallProfile.mMediaProfile.mAudioQuality;
        mediaProfile.mVideoQuality = mCallProfile.mMediaProfile.mVideoQuality;
        mediaProfile.mAudioDirection = ImsStreamMediaProfile.DIRECTION_SEND_RECEIVE;

        if (mediaProfile.mVideoQuality != ImsStreamMediaProfile.VIDEO_QUALITY_NONE) {
            mediaProfile.mVideoDirection = ImsStreamMediaProfile.DIRECTION_SEND_RECEIVE;
        }

        return mediaProfile;
    }

    private void enforceConversationMode() {
        if (mInCall) {
            mHold = false;
            mUpdateRequest = UPDATE_NONE;
        }
    }

    private void mergeInternal() {
        if (VDBG) {
            log("mergeInternal :: ImsCall=" + this);
        }

        mSession.merge();
        mUpdateRequest = UPDATE_MERGE;
    }

    private void notifyConferenceSessionTerminated(ImsReasonInfo reasonInfo) {
        ImsCall.Listener listener = mListener;
        clear(reasonInfo);

        if (listener != null) {
            try {
                listener.onCallTerminated(this, reasonInfo);
            } catch (Throwable t) {
                loge("notifyConferenceSessionTerminated :: ", t);
            }
        }
    }

    private void notifyConferenceStateUpdated(ImsConferenceState state) {
        Set<Entry<String, Bundle>> participants = state.mParticipants.entrySet();

        if (participants == null) {
            return;
        }

        Iterator<Entry<String, Bundle>> iterator = participants.iterator();
        List<ConferenceParticipant> conferenceParticipants = new ArrayList<>(participants.size());
        while (iterator.hasNext()) {
            Entry<String, Bundle> entry = iterator.next();

            String key = entry.getKey();
            Bundle confInfo = entry.getValue();
            String status = confInfo.getString(ImsConferenceState.STATUS);
            String user = confInfo.getString(ImsConferenceState.USER);
            String displayName = confInfo.getString(ImsConferenceState.DISPLAY_TEXT);
            String endpoint = confInfo.getString(ImsConferenceState.ENDPOINT);

            if (DBG) {
                log("notifyConferenceStateUpdated :: key=" + key +
                        ", status=" + status +
                        ", user=" + user +
                        ", displayName= " + displayName +
                        ", endpoint=" + endpoint);
            }

            Uri handle = Uri.parse(user);
            Uri endpointUri = Uri.parse(endpoint);
            int connectionState = ImsConferenceState.getConnectionStateForStatus(status);

            ConferenceParticipant conferenceParticipant = new ConferenceParticipant(handle,
                    displayName, endpointUri, connectionState);
            conferenceParticipants.add(conferenceParticipant);
        }

        if (!conferenceParticipants.isEmpty() && mListener != null) {
            try {
                mListener.onConferenceParticipantsStateChanged(this, conferenceParticipants);
            } catch (Throwable t) {
                loge("notifyConferenceStateUpdated :: ", t);
            }
        }
    }

    /**
     * Perform all cleanup and notification around the termination of a session.
     * Note that there are 2 distinct modes of operation.  The first is when
     * we receive a session termination on the primary session when we are
     * in the processing of merging.  The second is when we are not merging anything
     * and the call is terminated.
     *
     * @param reasonInfo The reason for the session termination
     */
    private void processCallTerminated(ImsReasonInfo reasonInfo) {
        if (VDBG) {
            String reasonString = reasonInfo != null ? reasonInfo.toString() : "null";
            log("processCallTerminated :: ImsCall=" + this + " reason=" + reasonString);
        }

        ImsCall.Listener listener = null;

        synchronized(ImsCall.this) {
            // If we are in the midst of establishing a conference, we will bury the termination
            // until the merge has completed.  If necessary we can surface the termination at this
            // point.
            if (isCallSessionMergePending()) {
                // Since we are in the process of a merge, this trigger means something
                // else because it is probably due to the merge happening vs. the
                // session is really terminated. Let's flag this and revisit if
                // the merge() ends up failing because we will need to take action on the
                // mSession in that case since the termination was not due to the merge
                // succeeding.
                if (DBG) {
                    log("processCallTerminated :: burying termination during ongoing merge.");
                }
                mSessionEndDuringMerge = true;
                mSessionEndDuringMergeReasonInfo = reasonInfo;
                return;
            }

            // If we are terminating the conference call, notify using conference listeners.
            if (isMultiparty()) {
                notifyConferenceSessionTerminated(reasonInfo);
                return;
            } else {
                listener = mListener;
                clear(reasonInfo);
            }
        }

        if (listener != null) {
            try {
                listener.onCallTerminated(ImsCall.this, reasonInfo);
            } catch (Throwable t) {
                loge("processCallTerminated :: ", t);
            }
        }
    }

    /**
     * This function determines if the ImsCallSession is our actual ImsCallSession or if is
     * the transient session used in the process of creating a conference. This function should only
     * be called within  callbacks that are not directly related to conference merging but might
     * potentially still be called on the transient ImsCallSession sent to us from
     * callSessionMergeStarted() when we don't really care. In those situations, we probably don't
     * want to take any action so we need to know that we can return early.
     *
     * @param session - The {@link ImsCallSession} that the function needs to analyze
     * @return true if this is the transient {@link ImsCallSession}, false otherwise.
     */
    private boolean isTransientConferenceSession(ImsCallSession session) {
        if (session != null && session != mSession && session == mTransientConferenceSession) {
            return true;
        }
        return false;
    }

    private void setTransientSessionAsPrimary(ImsCallSession transientSession) {
        synchronized (ImsCall.this) {
            mSession.setListener(null);
            mSession = transientSession;
            mSession.setListener(createCallSessionListener());
        }
    }

    /**
     * This function will determine if there is a pending conference and if
     * we are ready to finalize processing it.
     */
    private void tryProcessConferenceResult() {
        if (shouldProcessConferenceResult()) {
            if (isMergeHost()) {
                processMergeComplete();
            } else if (mMergeHost != null) {
                mMergeHost.processMergeComplete();
            } else {
                // There must be a merge host at this point.
                loge("tryProcessConferenceResult :: No merge host for this conference!");
            }
        }
    }

    /**
     * We have detected that a initial conference call has been fully configured. The internal
     * state of both {@code ImsCall} objects need to be cleaned up to reflect the new state.
     * This function should only be called in the context of the merge host to simplify logic
     *
     */
    private void processMergeComplete() {
        if (VDBG) {
            log("processMergeComplete :: ImsCall=" + this);
        }

        // The logic simplifies if we can assume that this function is only called on
        // the merge host.
        if (!isMergeHost()) {
            loge("processMergeComplete :: We are not the merge host!");
            return;
        }

        ImsCall.Listener listener;
        boolean swapRequired = false;
        synchronized(ImsCall.this) {
            ImsCall finalHostCall = this;
            ImsCall finalPeerCall = mMergePeer;

            if (isMultiparty()) {
                // The only clean up that we need to do for a merge into an existing conference
                // is to deal with the disconnect of the peer if it was successfully added to
                // the conference.
                setIsMerged(false);
                if (!isSessionAlive(mMergePeer.mSession)) {
                    // If the peer is dead, let's not play a disconnect sound for it when we
                    // unbury the termination callback.
                    mMergePeer.setIsMerged(true);
                } else {
                    mMergePeer.setIsMerged(false);
                }
            } else {
                // If we are here, we are not trying to merge a new call into an existing
                // conference.  That means that there is a transient session on the merge
                // host that represents the future conference once all the parties
                // have been added to it.  So make sure that it exists or else something
                // very wrong is going on.
                if (mTransientConferenceSession == null) {
                    loge("processMergeComplete :: No transient session!");
                    return;
                }
                if (mMergePeer == null) {
                    loge("processMergeComplete :: No merge peer!");
                    return;
                }

                // Since we are the host, we have the transient session attached to us. Let's detach
                // it and figure out where we need to set it for the final conference configuration.
                ImsCallSession transientConferenceSession = mTransientConferenceSession;
                mTransientConferenceSession = null;

                // Clear the listener for this transient session, we'll create a new listener
                // when it is attached to the final ImsCall that it should live on.
                transientConferenceSession.setListener(null);

                // Determine which call the transient session should be moved to.  If the current
                // call session is still alive and the merge peer's session is not, we have a
                // situation where the current call failed to merge into the conference but the
                // merge peer did merge in to the conference.  In this type of scenario the current
                // call will continue as a single party call, yet the background call will become
                // the conference.

                if (isSessionAlive(mSession) && !isSessionAlive(mMergePeer.getCallSession())) {
                    // I'm the host but we are moving the transient session to the peer since its
                    // session was disconnected and my session is still alive.  This signifies that
                    // their session was properly added to the conference but mine was not because
                    // it is probably in the held state as opposed to part of the final conference.
                    // In this case, we need to set isMerged to false on both calls so the
                    // disconnect sound is called when either call disconnects.
                    // Note that this case is only valid if this is an initial conference being
                    // brought up.
                    finalHostCall = mMergePeer;
                    finalPeerCall = this;
                    swapRequired = true;
                    setIsMerged(false);
                    mMergePeer.setIsMerged(false);
                    if (VDBG) {
                        log("processMergeComplete :: transient will transfer to merge peer");
                    }
                } else if (!isSessionAlive(mSession) && isSessionAlive(mMergePeer.getCallSession())) {
                    // The transient session stays with us and the disconnect sound should be played
                    // when the merge peer eventually disconnects since it was not actually added to
                    // the conference and is probably sitting in the held state.
                    finalHostCall = this;
                    finalPeerCall = mMergePeer;
                    swapRequired = false;
                    setIsMerged(false);
                    mMergePeer.setIsMerged(false); // Play the disconnect sound
                    if (VDBG) {
                        log("processMergeComplete :: transient will stay with the merge host");
                    }
                } else {
                    // The transient session stays with us and the disconnect sound should not be
                    // played when we ripple up the disconnect for the merge peer because it was
                    // only disconnected to be added to the conference.
                    finalHostCall = this;
                    finalPeerCall = mMergePeer;
                    swapRequired = false;
                    setIsMerged(false);
                    mMergePeer.setIsMerged(true);
                    if (VDBG) {
                        log("processMergeComplete :: transient will stay with us (I'm the host).");
                    }
                }

                if (VDBG) {
                    log("processMergeComplete :: call=" + finalHostCall + " is the final host");
                }

                // Add the transient session to the ImsCall that ended up being the host for the
                // conference.
                finalHostCall.setTransientSessionAsPrimary(transientConferenceSession);
            }

            listener = finalHostCall.mListener;

            // Clear all the merge related flags.
            clearMergeInfo();

            // For the final peer...let's bubble up any possible disconnects that we had
            // during the merge process
            finalPeerCall.notifySessionTerminatedDuringMerge();
            // For the final host, let's just bury the disconnects that we my have received
            // during the merge process since we are now the host of the conference call.
            finalHostCall.clearSessionTerminationFlags();
        }
        if (listener != null) {
            try {
                listener.onCallMerged(ImsCall.this, swapRequired);
            } catch (Throwable t) {
                loge("processMergeComplete :: ", t);
            }
        }
        return;
    }

    /**
     * Handles the case where the session has ended during a merge by reporting the termination
     * reason to listeners.
     */
    private void notifySessionTerminatedDuringMerge() {
        ImsCall.Listener listener;
        boolean notifyFailure = false;
        ImsReasonInfo notifyFailureReasonInfo = null;

        synchronized(ImsCall.this) {
            listener = mListener;
            if (mSessionEndDuringMerge) {
                // Set some local variables that will send out a notification about a
                // previously buried termination callback for our primary session now that
                // we know that this is not due to the conference call merging successfully.
                if (DBG) {
                    log("notifySessionTerminatedDuringMerge ::reporting terminate during merge");
                }
                notifyFailure = true;
                notifyFailureReasonInfo = mSessionEndDuringMergeReasonInfo;
            }
            clearSessionTerminationFlags();
        }

        if (listener != null && notifyFailure) {
            try {
                processCallTerminated(notifyFailureReasonInfo);
            } catch (Throwable t) {
                loge("notifySessionTerminatedDuringMerge :: ", t);
            }
        }
    }

    private void clearSessionTerminationFlags() {
        mSessionEndDuringMerge = false;
        mSessionEndDuringMergeReasonInfo = null;
    }

    /**
     * We received a callback from ImsCallSession that a merge failed. Clean up all
     * internal state to represent this state change.  The calling function is a callback
     * and should have been called on the session that was in the foreground
     * when merge() was originally called.  It is assumed that this function will be called
     * on the merge host.
     *
     * @param reasonInfo The {@link ImsReasonInfo} why the merge failed.
     */
    private void processMergeFailed(ImsReasonInfo reasonInfo) {
        if (VDBG) {
            log("processMergeFailed :: this=" + this + "reason=" + reasonInfo);
        }

        ImsCall.Listener listener;
        synchronized(ImsCall.this) {
            // The logic simplifies if we can assume that this function is only called on
            // the merge host.
            if (!isMergeHost()) {
                loge("processMergeFailed :: We are not the merge host!");
                return;
            }

            if (mMergePeer == null) {
                loge("processMergeFailed :: No merge peer!");
                return;
            }

            if (!isMultiparty()) {
                if (mTransientConferenceSession == null) {
                    loge("processMergeFailed :: No transient session!");
                    return;
                }
                // Clean up any work that we performed on the transient session.
                mTransientConferenceSession.setListener(null);
                mTransientConferenceSession = null;
            }

            // Ensure the calls being conferenced into the conference has isMerged = false.
            setIsMerged(false);
            mMergePeer.setIsMerged(false);

            listener = mListener;

            // Ensure any terminations are surfaced from this session.
            notifySessionTerminatedDuringMerge();
            mMergePeer.notifySessionTerminatedDuringMerge();

            // Clear all the various flags around coordinating this merge.
            clearMergeInfo();
        }
        if (listener != null) {
            try {
                listener.onCallMergeFailed(ImsCall.this, reasonInfo);
            } catch (Throwable t) {
                loge("processMergeFailed :: ", t);
            }
        }

        return;
    }

    private void notifyError(int reason, int statusCode, String message) {
    }

    private void throwImsException(Throwable t, int code) throws ImsException {
        if (t instanceof ImsException) {
            throw (ImsException) t;
        } else {
            throw new ImsException(String.valueOf(code), t, code);
        }
    }

    private void log(String s) {
        Rlog.d(TAG, s);
    }

    /**
     * Logs the specified message, as well as the current instance of {@link ImsCall}.
     *
     * @param s The message to log.
     */
    private void logv(String s) {
        StringBuilder sb = new StringBuilder();
        sb.append(s);
        sb.append(" imsCall=");
        sb.append(ImsCall.this);
        Rlog.v(TAG, sb.toString());
    }

    private void loge(String s) {
        Rlog.e(TAG, s);
    }

    private void loge(String s, Throwable t) {
        Rlog.e(TAG, s, t);
    }

    private class ImsCallSessionListenerProxy extends ImsCallSession.Listener {
        @Override
        public void callSessionProgressing(ImsCallSession session, ImsStreamMediaProfile profile) {
            if (isTransientConferenceSession(session)) {
                // If it is a transient (conference) session, there is no action for this signal.
                log("callSessionProgressing :: not supported for transient conference session=" +
                        session);
                return;
            }

            if (VDBG) {
                log("callSessionProgressing :: session=" + session + " profile=" + profile);
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
                mCallProfile.mMediaProfile.copyFrom(profile);
            }

            if (listener != null) {
                try {
                    listener.onCallProgressing(ImsCall.this);
                } catch (Throwable t) {
                    loge("callSessionProgressing :: ", t);
                }
            }
        }

        @Override
        public void callSessionStarted(ImsCallSession session, ImsCallProfile profile) {
            if (VDBG) {
                log("callSessionStarted :: session=" + session + " profile=" + profile);
            }

            if (!isTransientConferenceSession(session)) {
                // In the case that we are in the middle of a merge (either host or peer), we have
                // closure as far as this call's primary session is concerned.  If we are not
                // merging...its a NOOP.
                setCallSessionMergePending(false);
            } else {
                if (VDBG) {
                    log("callSessionStarted :: on transient session=" + session);
                }
                return;
            }

            // Check if there is an ongoing conference merge which has completed.  If there is
            // we can process the merge completion now.
            tryProcessConferenceResult();

            if (isTransientConferenceSession(session)) {
                // No further processing is needed if this is the transient session.
                return;
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
                mCallProfile = profile;
            }

            if (listener != null) {
                try {
                    listener.onCallStarted(ImsCall.this);
                } catch (Throwable t) {
                    loge("callSessionStarted :: ", t);
                }
            }
        }

        @Override
        public void callSessionStartFailed(ImsCallSession session, ImsReasonInfo reasonInfo) {
            if (isTransientConferenceSession(session)) {
                // We should not get this callback for a transient session.
                log("callSessionStartFailed :: not supported for transient conference session=" +
                        session);
                return;
            }

            if (VDBG) {
                log("callSessionStartFailed :: session=" + session + " reasonInfo=" + reasonInfo);
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
                mLastReasonInfo = reasonInfo;
            }

            if (listener != null) {
                try {
                    listener.onCallStartFailed(ImsCall.this, reasonInfo);
                } catch (Throwable t) {
                    loge("callSessionStarted :: ", t);
                }
            }
        }

        @Override
        public void callSessionTerminated(ImsCallSession session, ImsReasonInfo reasonInfo) {
            if (isTransientConferenceSession(session)) {
                log("callSessionTerminated :: on transient session=" + session);
                // This is bad, it should be treated much a callSessionMergeFailed since the
                // transient session only exists when in the process of a merge and the
                // termination of this session is effectively the end of the merge.
                processMergeFailed(reasonInfo);
                return;
            }

            if (VDBG) {
                log("callSessionTerminated :: session=" + session + " reasonInfo=" + reasonInfo);
            }

            // Process the termination first.  If we are in the midst of establishing a conference
            // call, we may bury this callback until we are done.  If there so no conference
            // call, the code after this function will be a NOOP.
            processCallTerminated(reasonInfo);

            // If session has terminated, it is no longer pending merge.
            setCallSessionMergePending(false);

            // Check if there is an ongoing conference merge which has completed.  If there is
            // we can process the merge completion now.
            tryProcessConferenceResult();
        }

        @Override
        public void callSessionHeld(ImsCallSession session, ImsCallProfile profile) {
            if (isTransientConferenceSession(session)) {
                // We should not get this callback for a transient session.
                log("callSessionHeld :: not supported for transient conference session=" + session);
                return;
            }

            if (VDBG) {
                log("callSessionHeld :: session=" + session + "profile=" + profile);
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                // If the session was held, it is no longer pending a merge -- this means it could
                // not be merged into the conference and was held instead.
                setCallSessionMergePending(false);

                mCallProfile = profile;

                if (mUpdateRequest == UPDATE_HOLD_MERGE) {
                    // This hold request was made to set the stage for a merge.
                    mergeInternal();
                    return;
                }

                // Check if there is an ongoing conference merge which has completed.  If there is
                // we can process the merge completion now.  processMergeComplete needs to be
                // called on the merge host.
                tryProcessConferenceResult();

                mUpdateRequest = UPDATE_NONE;
                listener = mListener;
            }

            if (listener != null) {
                try {
                    listener.onCallHeld(ImsCall.this);
                } catch (Throwable t) {
                    loge("callSessionHeld :: ", t);
                }
            }
        }

        @Override
        public void callSessionHoldFailed(ImsCallSession session, ImsReasonInfo reasonInfo) {
            if (isTransientConferenceSession(session)) {
                // We should not get this callback for a transient session.
                log("callSessionHoldFailed :: not supported for transient conference session=" +
                        session);
                return;
            }

            if (VDBG) {
                log("callSessionHoldFailed :: session" + session + "reasonInfo=" + reasonInfo);
            }

            boolean isHoldForMerge = false;
            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                if (mUpdateRequest == UPDATE_HOLD_MERGE) {
                    isHoldForMerge = true;
                }

                mUpdateRequest = UPDATE_NONE;
                listener = mListener;
            }

            if (listener != null) {
                try {
                    listener.onCallHoldFailed(ImsCall.this, reasonInfo);
                } catch (Throwable t) {
                    loge("callSessionHoldFailed :: ", t);
                }
            }
        }

        @Override
        public void callSessionHoldReceived(ImsCallSession session, ImsCallProfile profile) {
            if (isTransientConferenceSession(session)) {
                // We should not get this callback for a transient session.
                log("callSessionHoldReceived :: not supported for transient conference session=" +
                        session);
                return;
            }

            if (VDBG) {
                log("callSessionHoldReceived :: session=" + session + "profile=" + profile);
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
                mCallProfile = profile;
            }

            if (listener != null) {
                try {
                    listener.onCallHoldReceived(ImsCall.this);
                } catch (Throwable t) {
                    loge("callSessionHoldReceived :: ", t);
                }
            }
        }

        @Override
        public void callSessionResumed(ImsCallSession session, ImsCallProfile profile) {
            if (isTransientConferenceSession(session)) {
                log("callSessionResumed :: not supported for transient conference session=" +
                        session);
                return;
            }

            if (VDBG) {
                log("callSessionResumed :: session=" + session + "profile=" + profile);
            }

            // If this call was pending a merge, it is not anymore. This is the case when we
            // are merging in a new call into an existing conference.
            setCallSessionMergePending(false);

            // Check if there is an ongoing conference merge which has completed.  If there is
            // we can process the merge completion now.
            tryProcessConferenceResult();

            // TOOD: When we are merging a new call into an existing conference we are waiting
            // for 2 triggers to let us know that the conference has been established, the first
            // is a termination for the new calls (since it is added to the conference) the second
            // would be a resume on the existing conference.  If the resume comes first, then
            // we will make the onCallResumed() callback and its unclear how this will behave if
            // the termination has not come yet.

            ImsCall.Listener listener;
            synchronized(ImsCall.this) {
                listener = mListener;
                mCallProfile = profile;
                mUpdateRequest = UPDATE_NONE;
                mHold = false;
            }

            if (listener != null) {
                try {
                    listener.onCallResumed(ImsCall.this);
                } catch (Throwable t) {
                    loge("callSessionResumed :: ", t);
                }
            }
        }

        @Override
        public void callSessionResumeFailed(ImsCallSession session, ImsReasonInfo reasonInfo) {
            if (isTransientConferenceSession(session)) {
                log("callSessionResumeFailed :: not supported for transient conference session=" +
                        session);
                return;
            }

            if (VDBG) {
                log("callSessionResumeFailed :: session=" + session + "reasonInfo=" + reasonInfo);
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
                mUpdateRequest = UPDATE_NONE;
            }

            if (listener != null) {
                try {
                    listener.onCallResumeFailed(ImsCall.this, reasonInfo);
                } catch (Throwable t) {
                    loge("callSessionResumeFailed :: ", t);
                }
            }
        }

        @Override
        public void callSessionResumeReceived(ImsCallSession session, ImsCallProfile profile) {
            if (isTransientConferenceSession(session)) {
                log("callSessionResumeReceived :: not supported for transient conference session=" +
                        session);
                return;
            }

            if (VDBG) {
                log("callSessionResumeReceived :: session=" + session + "profile=" + profile);
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
                mCallProfile = profile;
            }

            if (listener != null) {
                try {
                    listener.onCallResumeReceived(ImsCall.this);
                } catch (Throwable t) {
                    loge("callSessionResumeReceived :: ", t);
                }
            }
        }

        @Override
        public void callSessionMergeStarted(ImsCallSession session,
                ImsCallSession newSession, ImsCallProfile profile) {
            if (VDBG) {
                log("callSessionMergeStarted :: session=" + session + " newSession=" + newSession +
                        ", profile=" + profile);
            }

            if (!isCallSessionMergePending()) {
                // Odd, we are not in the midst of merging anything.
                log("callSessionMergeStarted :: no merge in progress.");
                return;
            }

            // There are 2 ways that we can go here.  If the session that supplied the params
            // is not null, then it is the new session that represents the new conference
            // if the merge succeeds. If it is null, the merge is happening on our current
            // ImsCallSession.
            if (session == null) {
                // Everything is already set up and we just need to make sure
                // that we properly respond to all the future callbacks about
                // this merge.
                if (DBG) {
                    log("callSessionMergeStarted :: merging into existing ImsCallSession");
                }
                return;
            }

            if (DBG) {
                log("callSessionMergeStarted ::  setting our transient ImsCallSession");
            }

            // If we are here, this means that we are creating a new conference and
            // we need to do some extra work around managing a new ImsCallSession that
            // could represent our new ImsCallSession if the merge succeeds.
            synchronized(ImsCall.this) {
                // Keep track of this session for future callbacks to indicate success
                // or failure of this merge.
                mTransientConferenceSession = newSession;
                mTransientConferenceSession.setListener(createCallSessionListener());
            }

            return;
        }

        @Override
        public void callSessionMergeComplete(ImsCallSession session) {
            if (VDBG) {
                log("callSessionMergeComplete :: session=" + session);
            }

            setCallSessionMergePending(false);

            // Check if there is an ongoing conference merge which has completed.  If there is
            // we can process the merge completion now.
            tryProcessConferenceResult();
        }

        @Override
        public void callSessionMergeFailed(ImsCallSession session, ImsReasonInfo reasonInfo) {
            if (VDBG) {
                log("callSessionMergeFailed :: session=" + session + "reasonInfo=" + reasonInfo);
            }

            // Its possible that there could be threading issues with the other thread handling
            // the other call. This could affect our state.
            synchronized (ImsCall.this) {
                if (!isCallSessionMergePending()) {
                    // Odd, we are not in the midst of merging anything.
                    log("callSessionMergeFailed :: no merge in progress.");
                    return;
                }
                // Let's tell our parent ImsCall that the merge has failed and we need to clean
                // up any temporary, transient state.  Note this only gets called for an initial
                // conference.  If a merge into an existing conference fails, the two sessions will
                // just go back to their original state (ACTIVE or HELD).
                if (isMergeHost()) {
                    processMergeFailed(reasonInfo);
                } else if (mMergeHost != null) {
                    mMergeHost.processMergeFailed(reasonInfo);
                } else {
                    loge("callSessionMergeFailed :: No merge host for this conference!");
                }
            }
        }

        @Override
        public void callSessionUpdated(ImsCallSession session, ImsCallProfile profile) {
            if (isTransientConferenceSession(session)) {
                log("callSessionUpdated :: not supported for transient conference session=" +
                        session);
                return;
            }

            if (VDBG) {
                log("callSessionUpdated :: session=" + session + " profile=" + profile);
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
                mCallProfile = profile;
                mUpdateRequest = UPDATE_NONE;
            }

            if (listener != null) {
                try {
                    listener.onCallUpdated(ImsCall.this);
                } catch (Throwable t) {
                    loge("callSessionUpdated :: ", t);
                }
            }
        }

        @Override
        public void callSessionUpdateFailed(ImsCallSession session, ImsReasonInfo reasonInfo) {
            if (isTransientConferenceSession(session)) {
                log("callSessionUpdateFailed :: not supported for transient conference session=" +
                        session);
                return;
            }

            if (VDBG) {
                log("callSessionUpdateFailed :: session=" + session + " reasonInfo=" + reasonInfo);
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
                mUpdateRequest = UPDATE_NONE;
            }

            if (listener != null) {
                try {
                    listener.onCallUpdateFailed(ImsCall.this, reasonInfo);
                } catch (Throwable t) {
                    loge("callSessionUpdateFailed :: ", t);
                }
            }
        }

        @Override
        public void callSessionUpdateReceived(ImsCallSession session, ImsCallProfile profile) {
            if (isTransientConferenceSession(session)) {
                log("callSessionUpdateReceived :: not supported for transient conference " +
                        "session=" + session);
                return;
            }

            if (VDBG) {
                log("callSessionUpdateReceived :: session=" + session + " profile=" + profile);
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
                mProposedCallProfile = profile;
                mUpdateRequest = UPDATE_UNSPECIFIED;
            }

            if (listener != null) {
                try {
                    listener.onCallUpdateReceived(ImsCall.this);
                } catch (Throwable t) {
                    loge("callSessionUpdateReceived :: ", t);
                }
            }
        }

        @Override
        public void callSessionConferenceExtended(ImsCallSession session, ImsCallSession newSession,
                ImsCallProfile profile) {
            if (isTransientConferenceSession(session)) {
                log("callSessionConferenceExtended :: not supported for transient conference " +
                        "session=" + session);
                return;
            }

            if (VDBG) {
                log("callSessionConferenceExtended :: session=" + session  + " newSession=" +
                        newSession + ", profile=" + profile);
            }

            ImsCall newCall = createNewCall(newSession, profile);

            if (newCall == null) {
                callSessionConferenceExtendFailed(session, new ImsReasonInfo());
                return;
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
                mUpdateRequest = UPDATE_NONE;
            }

            if (listener != null) {
                try {
                    listener.onCallConferenceExtended(ImsCall.this, newCall);
                } catch (Throwable t) {
                    loge("callSessionConferenceExtended :: ", t);
                }
            }
        }

        @Override
        public void callSessionConferenceExtendFailed(ImsCallSession session,
                ImsReasonInfo reasonInfo) {
            if (isTransientConferenceSession(session)) {
                log("callSessionConferenceExtendFailed :: not supported for transient " +
                        "conference session=" + session);
                return;
            }

            if (DBG) {
                log("callSessionConferenceExtendFailed :: imsCall=" + ImsCall.this +
                        ", reasonInfo=" + reasonInfo);
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
                mUpdateRequest = UPDATE_NONE;
            }

            if (listener != null) {
                try {
                    listener.onCallConferenceExtendFailed(ImsCall.this, reasonInfo);
                } catch (Throwable t) {
                    loge("callSessionConferenceExtendFailed :: ", t);
                }
            }
        }

        @Override
        public void callSessionConferenceExtendReceived(ImsCallSession session,
                ImsCallSession newSession, ImsCallProfile profile) {
            if (isTransientConferenceSession(session)) {
                log("callSessionConferenceExtendReceived :: not supported for transient " +
                        "conference session" + session);
                return;
            }

            if (VDBG) {
                log("callSessionConferenceExtendReceived :: newSession=" + newSession +
                        ", profile=" + profile);
            }

            ImsCall newCall = createNewCall(newSession, profile);

            if (newCall == null) {
                // Should all the calls be terminated...???
                return;
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
            }

            if (listener != null) {
                try {
                    listener.onCallConferenceExtendReceived(ImsCall.this, newCall);
                } catch (Throwable t) {
                    loge("callSessionConferenceExtendReceived :: ", t);
                }
            }
        }

        @Override
        public void callSessionInviteParticipantsRequestDelivered(ImsCallSession session) {
            if (isTransientConferenceSession(session)) {
                log("callSessionInviteParticipantsRequestDelivered :: not supported for " +
                        "conference session=" + session);
                return;
            }

            if (VDBG) {
                log("callSessionInviteParticipantsRequestDelivered ::");
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
            }

            if (listener != null) {
                try {
                    listener.onCallInviteParticipantsRequestDelivered(ImsCall.this);
                } catch (Throwable t) {
                    loge("callSessionInviteParticipantsRequestDelivered :: ", t);
                }
            }
        }

        @Override
        public void callSessionInviteParticipantsRequestFailed(ImsCallSession session,
                ImsReasonInfo reasonInfo) {
            if (isTransientConferenceSession(session)) {
                log("callSessionInviteParticipantsRequestFailed :: not supported for " +
                        "conference session=" + session);
                return;
            }

            if (VDBG) {
                log("callSessionInviteParticipantsRequestFailed :: reasonInfo=" + reasonInfo);
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
            }

            if (listener != null) {
                try {
                    listener.onCallInviteParticipantsRequestFailed(ImsCall.this, reasonInfo);
                } catch (Throwable t) {
                    loge("callSessionInviteParticipantsRequestFailed :: ", t);
                }
            }
        }

        @Override
        public void callSessionRemoveParticipantsRequestDelivered(ImsCallSession session) {
            if (isTransientConferenceSession(session)) {
                log("callSessionRemoveParticipantsRequestDelivered :: not supported for " +
                        "conference session=" + session);
                return;
            }

            if (VDBG) {
                log("callSessionRemoveParticipantsRequestDelivered ::");
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
            }

            if (listener != null) {
                try {
                    listener.onCallRemoveParticipantsRequestDelivered(ImsCall.this);
                } catch (Throwable t) {
                    loge("callSessionRemoveParticipantsRequestDelivered :: ", t);
                }
            }
        }

        @Override
        public void callSessionRemoveParticipantsRequestFailed(ImsCallSession session,
                ImsReasonInfo reasonInfo) {
            if (isTransientConferenceSession(session)) {
                log("callSessionRemoveParticipantsRequestFailed :: not supported for " +
                        "conference session=" + session);
                return;
            }

            if (VDBG) {
                log("callSessionRemoveParticipantsRequestFailed :: reasonInfo=" + reasonInfo);
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
            }

            if (listener != null) {
                try {
                    listener.onCallRemoveParticipantsRequestFailed(ImsCall.this, reasonInfo);
                } catch (Throwable t) {
                    loge("callSessionRemoveParticipantsRequestFailed :: ", t);
                }
            }
        }

        @Override
        public void callSessionConferenceStateUpdated(ImsCallSession session,
                ImsConferenceState state) {
            if (isTransientConferenceSession(session)) {
                log("callSessionConferenceStateUpdated :: not supported for transient " +
                        "conference session=" + session);
                return;
            }

            if (VDBG) {
                log("callSessionConferenceStateUpdated :: state=" + state);
            }

            conferenceStateUpdated(state);
        }

        @Override
        public void callSessionUssdMessageReceived(ImsCallSession session, int mode,
                String ussdMessage) {
            if (isTransientConferenceSession(session)) {
                log("callSessionUssdMessageReceived :: not supported for transient " +
                        "conference session=" + session);
                return;
            }

            if (VDBG) {
                log("callSessionUssdMessageReceived :: mode=" + mode + ", ussdMessage=" +
                        ussdMessage);
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
            }

            if (listener != null) {
                try {
                    listener.onCallUssdMessageReceived(ImsCall.this, mode, ussdMessage);
                } catch (Throwable t) {
                    loge("callSessionUssdMessageReceived :: ", t);
                }
            }
        }

        @Override
        public void callSessionTtyModeReceived(ImsCallSession session, int mode) {
            if (VDBG) {
                log("callSessionTtyModeReceived :: mode=" + mode);
            }

            ImsCall.Listener listener;

            synchronized(ImsCall.this) {
                listener = mListener;
            }

            if (listener != null) {
                try {
                    listener.onCallSessionTtyModeReceived(ImsCall.this, mode);
                } catch (Throwable t) {
                    loge("callSessionTtyModeReceived :: ", t);
                }
            }
        }
    }

    /**
     * Report a new conference state to the current {@link ImsCall} and inform listeners of the
     * change.  Marked as {@code VisibleForTesting} so that the
     * {@code com.android.internal.telephony.TelephonyTester} class can inject a test conference
     * event package into a regular ongoing IMS call.
     *
     * @param state The {@link ImsConferenceState}.
     */
    @VisibleForTesting
    public void conferenceStateUpdated(ImsConferenceState state) {
        Listener listener;

        synchronized(this) {
            notifyConferenceStateUpdated(state);
            listener = mListener;
        }

        if (listener != null) {
            try {
                listener.onCallConferenceStateUpdated(this, state);
            } catch (Throwable t) {
                loge("callSessionConferenceStateUpdated :: ", t);
            }
        }
    }

    /**
     * Provides a human-readable string representation of an update request.
     *
     * @param updateRequest The update request.
     * @return The string representation.
     */
    private String updateRequestToString(int updateRequest) {
        switch (updateRequest) {
            case UPDATE_NONE:
                return "NONE";
            case UPDATE_HOLD:
                return "HOLD";
            case UPDATE_HOLD_MERGE:
                return "HOLD_MERGE";
            case UPDATE_RESUME:
                return "RESUME";
            case UPDATE_MERGE:
                return "MERGE";
            case UPDATE_EXTEND_TO_CONFERENCE:
                return "EXTEND_TO_CONFERENCE";
            case UPDATE_UNSPECIFIED:
                return "UNSPECIFIED";
            default:
                return "UNKNOWN";
        }
    }

    /**
     * Clears the merge peer for this call, ensuring that the peer's connection to this call is also
     * severed at the same time.
     */
    private void clearMergeInfo() {
        if (VDBG) {
            log("clearMergeInfo :: clearing all merge info");
        }

        // First clear out the merge partner then clear ourselves out.
        if (mMergeHost != null) {
            mMergeHost.mMergePeer = null;
            mMergeHost.mUpdateRequest = UPDATE_NONE;
            mMergeHost.mCallSessionMergePending = false;
        }
        if (mMergePeer != null) {
            mMergePeer.mMergeHost = null;
            mMergePeer.mUpdateRequest = UPDATE_NONE;
            mMergePeer.mCallSessionMergePending = false;
        }
        mMergeHost = null;
        mMergePeer = null;
        mUpdateRequest = UPDATE_NONE;
        mCallSessionMergePending = false;
    }

    /**
     * Sets the merge peer for the current call.  The merge peer is the background call that will be
     * merged into this call.  On the merge peer, sets the merge host to be this call.
     *
     * @param mergePeer The peer call to be merged into this one.
     */
    private void setMergePeer(ImsCall mergePeer) {
        mMergePeer = mergePeer;
        mMergeHost = null;

        mergePeer.mMergeHost = ImsCall.this;
        mergePeer.mMergePeer = null;
    }

    /**
     * Sets the merge hody for the current call.  The merge host is the foreground call this call
     * will be merged into.  On the merge host, sets the merge peer to be this call.
     *
     * @param mergeHost The merge host this call will be merged into.
     */
    public void setMergeHost(ImsCall mergeHost) {
        mMergeHost = mergeHost;
        mMergePeer = null;

        mergeHost.mMergeHost = null;
        mergeHost.mMergePeer = ImsCall.this;
    }

    /**
     * Determines if the current call is in the process of merging with another call or conference.
     *
     * @return {@code true} if in the process of merging.
     */
    private boolean isMerging() {
        return mMergePeer != null || mMergeHost != null;
    }

    /**
     * Determines if the current call is the host of the merge.
     *
     * @return {@code true} if the call is the merge host.
     */
    private boolean isMergeHost() {
        return mMergePeer != null && mMergeHost == null;
    }

    /**
     * Determines if the current call is the peer of the merge.
     *
     * @return {@code true} if the call is the merge peer.
     */
    private boolean isMergePeer() {
        return mMergePeer == null && mMergeHost != null;
    }

    /**
     * Determines if the call session is pending merge into a conference or not.
     *
     * @return {@code true} if a merge into a conference is pending, {@code false} otherwise.
     */
    private boolean isCallSessionMergePending() {
        return mCallSessionMergePending;
    }

    /**
     * Sets flag indicating whether the call session is pending merge into a conference or not.
     *
     * @param callSessionMergePending {@code true} if a merge into the conference is pending,
     *      {@code false} otherwise.
     */
    private void setCallSessionMergePending(boolean callSessionMergePending) {
        mCallSessionMergePending = callSessionMergePending;
    }

    /**
     * Determines if there is a conference merge in process.  If there is a merge in process,
     * determines if both the merge host and peer sessions have completed the merge process.  This
     * means that we have received terminate or hold signals for the sessions, indicating that they
     * are no longer in the process of being merged into the conference.
     * <p>
     * The sessions are considered to have merged if: both calls still have merge peer/host
     * relationships configured,  both sessions are not waiting to be merged into the conference,
     * and the transient conference session is alive in the case of an initial conference.
     *
     * @return {@code true} where the host and peer sessions have finished merging into the
     *      conference, {@code false} if the merge has not yet completed, and {@code false} if there
     *      is no conference merge in progress.
     */
    private boolean shouldProcessConferenceResult() {
        boolean areMergeTriggersDone = false;

        synchronized (ImsCall.this) {
            // if there is a merge going on, then the merge host/peer relationships should have been
            // set up.  This works for both the initial conference or merging a call into an
            // existing conference.
            if (!isMergeHost() && !isMergePeer()) {
                if (VDBG) {
                    log("shouldProcessConferenceResult :: no merge in progress");
                }
                return false;
            }

            // There is a merge in progress, so check the sessions to ensure:
            // 1. Both calls have completed being merged (or failing to merge) into the conference.
            // 2. The transient conference session is alive.
            if (isMergeHost()) {
                if (VDBG) {
                    log("shouldProcessConferenceResult :: We are a merge host=" + this);
                    log("shouldProcessConferenceResult :: Here is the merge peer=" + mMergePeer);
                }
                areMergeTriggersDone = !isCallSessionMergePending() &&
                        !mMergePeer.isCallSessionMergePending();
                if (!isMultiparty()) {
                    // Only check the transient session when there is no existing conference
                    areMergeTriggersDone &= isSessionAlive(mTransientConferenceSession);
                }
            } else if (isMergePeer()) {
                if (VDBG) {
                    log("shouldProcessConferenceResult :: We are a merge peer=" + this);
                    log("shouldProcessConferenceResult :: Here is the merge host=" + mMergeHost);
                }
                areMergeTriggersDone = !isCallSessionMergePending() &&
                        !mMergeHost.isCallSessionMergePending();
                if (!mMergeHost.isMultiparty()) {
                    // Only check the transient session when there is no existing conference
                    areMergeTriggersDone &= isSessionAlive(mMergeHost.mTransientConferenceSession);
                } else {
                    // This else block is a special case for Verizon to handle these steps
                    // 1. Establish a conference call.
                    // 2. Add a new call (conference in in BG)
                    // 3. Swap (conference active on FG)
                    // 4. Merge
                    // What happens here is that the BG call gets a terminated callback
                    // because it was added to the conference. I've seen where
                    // the FG gets no callback at all because its already active.
                    // So if we continue to wait for it to set its isCallSessionMerging
                    // flag to false...we'll be waiting forever.
                    areMergeTriggersDone = !isCallSessionMergePending();
                }
            } else {
                // Realistically this shouldn't happen, but best to be safe.
                loge("shouldProcessConferenceResult : merge in progress but call is neither" +
                        "host nor peer.");
            }
            if (VDBG) {
                log("shouldProcessConferenceResult :: returning:" +
                        (areMergeTriggersDone ? "true" : "false"));
            }
        }
        return areMergeTriggersDone;
    }

    /**
     * Provides a string representation of the {@link ImsCall}.  Primarily intended for use in log
     * statements.
     *
     * @return String representation of call.
     */
    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("[ImsCall objId:");
        sb.append(System.identityHashCode(this));
        sb.append(" onHold:");
        sb.append(isOnHold() ? "Y" : "N");
        sb.append(" mute:");
        sb.append(isMuted() ? "Y" : "N");
        sb.append(" updateRequest:");
        sb.append(updateRequestToString(mUpdateRequest));
        sb.append(" merging:");
        sb.append(isMerging() ? "Y" : "N");
        if (isMerging()) {
            if (isMergePeer()) {
                sb.append("P");
            } else {
                sb.append("H");
            }
        }
        sb.append(" merge action pending:");
        sb.append(isCallSessionMergePending() ? "Y" : "N");
        sb.append(" merged:");
        sb.append(isMerged() ? "Y" : "N");
        sb.append(" multiParty:");
        sb.append(isMultiparty() ? "Y" : "N");
        sb.append(" buried term:");
        sb.append(mSessionEndDuringMerge ? "Y" : "N");
        sb.append(" session:");
        sb.append(mSession);
        sb.append(" transientSession:");
        sb.append(mTransientConferenceSession);
        sb.append("]");
        return sb.toString();
    }
}
