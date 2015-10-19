/*
 * Copyright (C) 2009 The Android Open Source Project
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

package android.media.cts;


import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.AudioTrack.OnPlaybackPositionUpdateListener;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.test.AndroidTestCase;

public class AudioTrack_ListenerTest extends AndroidTestCase {
    private boolean mOnMarkerReachedCalled;
    private boolean mOnPeriodicNotificationCalled;
    private boolean mIsHandleMessageCalled;
    private final int TEST_SR = 11025;
    private final int TEST_CONF = AudioFormat.CHANNEL_CONFIGURATION_MONO;
    private final int TEST_FORMAT = AudioFormat.ENCODING_PCM_8BIT;
    private final int TEST_MODE = AudioTrack.MODE_STREAM;
    private final int TEST_STREAM_TYPE1 = AudioManager.STREAM_MUSIC;
    private Handler mHandler = new Handler(Looper.getMainLooper()) {
        @Override
        public void handleMessage(Message msg) {
            mIsHandleMessageCalled = true;
            super.handleMessage(msg);
        }
    };
    private final int mMinBuffSize = AudioTrack.getMinBufferSize(TEST_SR, TEST_CONF, TEST_FORMAT);
    private AudioTrack mAudioTrack;
    private OnPlaybackPositionUpdateListener mListener =
                                new MockOnPlaybackPositionUpdateListener();
    private MakeSomethingAsynchronouslyAndLoop<AudioTrack> mMakeSomething;

    @Override
    protected void setUp() throws Exception
    {
        super.setUp();
        if (mAudioTrack == null) {
            mMakeSomething = new MakeSomethingAsynchronouslyAndLoop<AudioTrack>(
                new MakesSomething<AudioTrack>() {
                    @Override
                    public AudioTrack makeSomething()
                    {
                        return new AudioTrack(TEST_STREAM_TYPE1, TEST_SR, TEST_CONF,
                            TEST_FORMAT, 2 * mMinBuffSize, TEST_MODE);
                    }
                }
            );
            mAudioTrack = mMakeSomething.make();
        }
    }

    public void testAudioTrackCallback() throws Exception {
        mAudioTrack.setPlaybackPositionUpdateListener(mListener);
        doTest(false /*customHandler*/);
    }

    public void testAudioTrackCallbackWithHandler() throws Exception {
        mAudioTrack.setPlaybackPositionUpdateListener(mListener, mHandler);
        doTest(true /*customHandler*/);
        // ToBeFixed: Handler#handleMessage() is never called
        // FIXME possibly because the new Handler() is missing the Looper parameter
        assertFalse(mIsHandleMessageCalled);
    }

    private void doTest(boolean customHandler) throws Exception {
        mOnMarkerReachedCalled = false;
        mOnPeriodicNotificationCalled = false;
        byte[] vai = AudioTrackTest.createSoundDataInByteArray(2 * mMinBuffSize, TEST_SR, 1024);
        int markerInFrames = vai.length / 4;
        assertEquals(AudioTrack.SUCCESS, mAudioTrack.setNotificationMarkerPosition(markerInFrames));
        int periodInFrames = vai.length / 2;
        assertEquals(AudioTrack.SUCCESS, mAudioTrack.setPositionNotificationPeriod(periodInFrames));

        boolean hasPlayed = false;
        int written = 0;
        while (written < vai.length) {
            written += mAudioTrack.write(vai, written, vai.length - written);
            if (!hasPlayed) {
                mAudioTrack.play();
                hasPlayed = true;
            }
        }

        final int numChannels = (TEST_CONF == AudioFormat.CHANNEL_CONFIGURATION_STEREO) ? 2 : 1;
        final int bytesPerSample = (TEST_FORMAT == AudioFormat.ENCODING_PCM_16BIT) ? 2 : 1;
        final int bytesPerFrame = numChannels * bytesPerSample;
        final int sampleLengthMs = (int)(1000 * ((float)vai.length / TEST_SR / bytesPerFrame));
        Thread.sleep(sampleLengthMs + 1000);
        if (!customHandler) {
            assertTrue(mOnMarkerReachedCalled);
            assertTrue(mOnPeriodicNotificationCalled);
        }
        mAudioTrack.stop();
    }

    // lightweight java.util.concurrent.Future*
    private static class FutureLatch<T>
    {
        private T mValue;
        private boolean mSet;
        public void set(T value)
        {
            synchronized (this) {
                assert !mSet;
                mValue = value;
                mSet = true;
                notify();
            }
        }
        public T get()
        {
            T value;
            synchronized (this) {
                while (!mSet) {
                    try {
                        wait();
                    } catch (InterruptedException e) {
                        ;
                    }
                }
                value = mValue;
            }
            return value;
        }
    }

    // represents a factory for T
    private interface MakesSomething<T>
    {
        T makeSomething();
    }

    // used to construct an object in the context of an asynchronous thread with looper
    private static class MakeSomethingAsynchronouslyAndLoop<T>
    {
        private Thread mThread;
        volatile private Looper mLooper;
        private final MakesSomething<T> mWhatToMake;

        public MakeSomethingAsynchronouslyAndLoop(MakesSomething<T> whatToMake)
        {
            assert whatToMake != null;
            mWhatToMake = whatToMake;
        }

        public T make()
        {
            final FutureLatch<T> futureLatch = new FutureLatch<T>();
            mThread = new Thread()
            {
                @Override
                public void run()
                {
                    Looper.prepare();
                    mLooper = Looper.myLooper();
                    T something = mWhatToMake.makeSomething();
                    futureLatch.set(something);
                    Looper.loop();
                }
            };
            mThread.start();
            return futureLatch.get();
        }
        public void join()
        {
            mLooper.quit();
            try {
                mThread.join();
            } catch (InterruptedException e) {
                ;
            }
            // avoid dangling references
            mLooper = null;
            mThread = null;
        }
    }

    private class MockOnPlaybackPositionUpdateListener
                                        implements OnPlaybackPositionUpdateListener {

        public void onMarkerReached(AudioTrack track) {
            mOnMarkerReachedCalled = true;
        }

        public void onPeriodicNotification(AudioTrack track) {
            mOnPeriodicNotificationCalled = true;
        }

    }

    @Override
    protected void tearDown() throws Exception {
        if (mMakeSomething != null) {
            mMakeSomething.join();
        }
        if (mAudioTrack != null) {
            mAudioTrack.release();
            mAudioTrack = null;
        }
        super.tearDown();
    }

}
