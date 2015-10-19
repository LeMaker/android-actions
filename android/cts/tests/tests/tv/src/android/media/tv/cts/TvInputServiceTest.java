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

package android.media.tv.cts;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Context;
import android.cts.util.PollingCheck;
import android.media.tv.TvContentRating;
import android.media.tv.TvContract;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.media.tv.TvTrackInfo;
import android.media.tv.TvView;
import android.media.tv.cts.TvInputServiceTest.CountingTvInputService.CountingSession;
import android.net.Uri;
import android.test.ActivityInstrumentationTestCase2;
import android.view.KeyEvent;
import android.view.Surface;

import com.android.cts.tv.R;

import java.util.ArrayList;
import java.util.List;


/**
 * Test {@link android.media.tv.TvInputService}.
 */
public class TvInputServiceTest extends ActivityInstrumentationTestCase2<TvViewStubActivity> {
    /** The maximum time to wait for an operation. */
    private static final long TIME_OUT = 15000L;
    private static final String mDummyTrackId = "dummyTrackId";
    private static final TvTrackInfo mDummyTrack =
            new TvTrackInfo.Builder(TvTrackInfo.TYPE_SUBTITLE, mDummyTrackId)
            .setLanguage("und").build();

    private TvView mTvView;
    private Activity mActivity;
    private Instrumentation mInstrumentation;
    private TvInputManager mManager;
    private TvInputInfo mStubInfo;
    private final StubCallback mCallback = new StubCallback();

    private static class StubCallback extends TvView.TvInputCallback {
        private int mChannelRetunedCount;
        private int mVideoAvailableCount;
        private int mVideoUnavailableCount;
        private int mTrackSelectedCount;
        private int mTrackChangedCount;
        private int mContentAllowedCount;
        private int mContentBlockedCount;

        @Override
        public void onChannelRetuned(String inputId, Uri channelUri) {
            mChannelRetunedCount++;
        }

        @Override
        public void onVideoAvailable(String inputId) {
            mVideoAvailableCount++;
        }

        @Override
        public void onVideoUnavailable(String inputId, int reason) {
            mVideoUnavailableCount++;
        }

        @Override
        public void onTrackSelected(String inputId, int type, String trackId) {
            mTrackSelectedCount++;
        }

        @Override
        public void onTracksChanged(String inputId, List<TvTrackInfo> trackList) {
            mTrackChangedCount++;
        }

        @Override
        public void onContentAllowed(String inputId) {
            mContentAllowedCount++;
        }

        @Override
        public void onContentBlocked(String inputId, TvContentRating rating) {
            mContentBlockedCount++;
        }
    }

    public TvInputServiceTest() {
        super(TvViewStubActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        if (!Utils.hasTvInputFramework(getActivity())) {
            return;
        }
        mActivity = getActivity();
        mInstrumentation = getInstrumentation();
        mTvView = (TvView) mActivity.findViewById(R.id.tvview);
        mManager = (TvInputManager) mActivity.getSystemService(Context.TV_INPUT_SERVICE);
        for (TvInputInfo info : mManager.getTvInputList()) {
            if (info.getServiceInfo().name.equals(CountingTvInputService.class.getName())) {
                mStubInfo = info;
                break;
            }
        }
        assertNotNull(mStubInfo);
        mTvView.setCallback(mCallback);

        CountingTvInputService.sSession = null;
    }

    public void testTvInputService() throws Throwable {
        if (!Utils.hasTvInputFramework(getActivity())) {
            return;
        }
        verifyCommandTune();
        verifyCommandSetStreamVolume();
        verifyCommandSetCaptionEnabled();
        verifyCommandSelectTrack();
        verifyCommandDispatchKeyEvent();
        verifyCallbackChannelRetuned();
        verifyCallbackVideoAvailable();
        verifyCallbackVideoUnavailable();
        verifyCallbackTracksChanged();
        verifyCallbackTrackSelected();
        verifyCallbackContentAllowed();
        verifyCallbackContentBlocked();

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mTvView.reset();
            }
        });
        mInstrumentation.waitForIdleSync();
    }

    public void verifyCommandTune() {
        Uri fakeChannelUri = TvContract.buildChannelUri(0);
        mTvView.tune(mStubInfo.getId(), fakeChannelUri);
        mInstrumentation.waitForIdleSync();
        new PollingCheck(TIME_OUT) {
            @Override
            protected boolean check() {
                CountingSession session = CountingTvInputService.sSession;
                return session != null && session.mTuneCount > 0;
            }
        }.run();
    }

    public void verifyCommandSetStreamVolume() {
        mTvView.setStreamVolume(1.0f);
        mInstrumentation.waitForIdleSync();
        new PollingCheck(TIME_OUT) {
            @Override
            protected boolean check() {
                CountingSession session = CountingTvInputService.sSession;
                return session != null && session.mSetStreamVolumeCount > 0;
            }
        }.run();
    }

    public void verifyCommandSetCaptionEnabled() {
        mTvView.setCaptionEnabled(true);
        mInstrumentation.waitForIdleSync();
        new PollingCheck(TIME_OUT) {
            @Override
            protected boolean check() {
                CountingSession session = CountingTvInputService.sSession;
                return session != null && session.mSetCaptionEnabledCount > 0;
            }
        }.run();
    }

    public void verifyCommandSelectTrack() {
        mTvView.selectTrack(TvTrackInfo.TYPE_AUDIO, "dummyTrackId");
        mInstrumentation.waitForIdleSync();
        new PollingCheck(TIME_OUT) {
            @Override
            protected boolean check() {
                CountingSession session = CountingTvInputService.sSession;
                return session != null && session.mSetStreamVolumeCount > 0;
            }
        }.run();
    }

    public void verifyCommandDispatchKeyEvent() {
        mTvView.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_K));
        mInstrumentation.waitForIdleSync();
        new PollingCheck(TIME_OUT) {
            @Override
            protected boolean check() {
                CountingSession session = CountingTvInputService.sSession;
                return session != null && session.mKeyDownCount > 0;
            }
        }.run();
    }

    public void verifyCallbackChannelRetuned() {
        CountingSession session = CountingTvInputService.sSession;
        assertNotNull(session);
        Uri fakeChannelUri = TvContract.buildChannelUri(0);
        session.notifyChannelRetuned(fakeChannelUri);
        new PollingCheck(TIME_OUT) {
            @Override
            protected boolean check() {
                return mCallback.mChannelRetunedCount > 0;
            }
        }.run();
    }

    public void verifyCallbackVideoAvailable() {
        CountingSession session = CountingTvInputService.sSession;
        assertNotNull(session);
        session.notifyVideoAvailable();
        new PollingCheck(TIME_OUT) {
            @Override
            protected boolean check() {
                return mCallback.mVideoAvailableCount > 0;
            }
        }.run();
    }

    public void verifyCallbackVideoUnavailable() {
        CountingSession session = CountingTvInputService.sSession;
        assertNotNull(session);
        session.notifyVideoUnavailable(TvInputManager.VIDEO_UNAVAILABLE_REASON_TUNING);
        new PollingCheck(TIME_OUT) {
            @Override
            protected boolean check() {
                return mCallback.mVideoUnavailableCount > 0;
            }
        }.run();
    }

    public void verifyCallbackTracksChanged() {
        CountingSession session = CountingTvInputService.sSession;
        assertNotNull(session);
        ArrayList<TvTrackInfo> tracks = new ArrayList<>();
        tracks.add(mDummyTrack);
        session.notifyTracksChanged(tracks);
        new PollingCheck(TIME_OUT) {
            @Override
            protected boolean check() {
                return mCallback.mTrackChangedCount > 0;
            }
        }.run();
    }

    public void verifyCallbackTrackSelected() {
        CountingSession session = CountingTvInputService.sSession;
        assertNotNull(session);
        session.notifyTrackSelected(mDummyTrack.getType(), mDummyTrack.getId());
        new PollingCheck(TIME_OUT) {
            @Override
            protected boolean check() {
                return mCallback.mTrackSelectedCount > 0;
            }
        }.run();
    }

    public void verifyCallbackContentAllowed() {
        CountingSession session = CountingTvInputService.sSession;
        assertNotNull(session);
        session.notifyContentAllowed();
        new PollingCheck(TIME_OUT) {
            @Override
            protected boolean check() {
                return mCallback.mContentAllowedCount > 0;
            }
        }.run();
    }

    public void verifyCallbackContentBlocked() {
        CountingSession session = CountingTvInputService.sSession;
        assertNotNull(session);
        TvContentRating rating = TvContentRating.createRating("android.media.tv", "US_TVPG",
                "US_TVPG_TV_MA", "US_TVPG_S", "US_TVPG_V");
        session.notifyContentBlocked(rating);
        new PollingCheck(TIME_OUT) {
            @Override
            protected boolean check() {
                return mCallback.mContentBlockedCount > 0;
            }
        }.run();
    }

    public static class CountingTvInputService extends StubTvInputService {
        static CountingTvInputService sInstance;
        static CountingSession sSession;

        @Override
        public Session onCreateSession(String inputId) {
            sSession = new CountingSession(this);
            return sSession;
        }

        public static class CountingSession extends Session {
            public volatile int mTuneCount;
            public volatile int mSetStreamVolumeCount;
            public volatile int mSetCaptionEnabledCount;
            public volatile int mSelectTrackCount;
            public volatile int mKeyDownCount;

            CountingSession(Context context) {
                super(context);
            }

            @Override
            public void onRelease() {
            }

            @Override
            public boolean onSetSurface(Surface surface) {
                return false;
            }

            @Override
            public boolean onTune(Uri channelUri) {
                mTuneCount++;
                return false;
            }

            @Override
            public void onSetStreamVolume(float volume) {
                mSetStreamVolumeCount++;
            }

            @Override
            public void onSetCaptionEnabled(boolean enabled) {
                mSetCaptionEnabledCount++;
            }

            @Override
            public boolean onSelectTrack(int type, String id) {
                mSelectTrackCount++;
                return false;
            }

            @Override
            public boolean onKeyDown(int keyCode, KeyEvent event) {
                mKeyDownCount++;
                return false;
            }
        }
    }
}
