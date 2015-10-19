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
package android.media.cts;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.AudioAttributes;
import android.util.Log;

import java.util.LinkedList;

/**
 * Class for playing audio by using audio track.
 * {@link #write(byte[], int, int)} and {@link #write(short[], int, int)} methods will
 * block until all data has been written to system. In order to avoid blocking, this class
 * caculates available buffer size first then writes to audio sink.
 */
public class NonBlockingAudioTrack {
    private static final String TAG = NonBlockingAudioTrack.class.getSimpleName();

    class QueueElem {
        byte[] data;
        int offset;
        int size;
    }

    private AudioTrack mAudioTrack;
    private boolean mWriteMorePending = false;
    private int mSampleRate;
    private int mFrameSize;
    private int mBufferSizeInFrames;
    private int mNumFramesSubmitted = 0;
    private int mNumBytesQueued = 0;
    private LinkedList<QueueElem> mQueue = new LinkedList<QueueElem>();

    public NonBlockingAudioTrack(int sampleRate, int channelCount, boolean hwAvSync,
                    int audioSessionId) {
        int channelConfig;
        switch (channelCount) {
            case 1:
                channelConfig = AudioFormat.CHANNEL_OUT_MONO;
                break;
            case 2:
                channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
                break;
            case 6:
                channelConfig = AudioFormat.CHANNEL_OUT_5POINT1;
                break;
            default:
                throw new IllegalArgumentException();
        }

        int minBufferSize =
            AudioTrack.getMinBufferSize(
                    sampleRate,
                    channelConfig,
                    AudioFormat.ENCODING_PCM_16BIT);

        int bufferSize = 2 * minBufferSize;

        if (!hwAvSync) {
            mAudioTrack = new AudioTrack(
                    AudioManager.STREAM_MUSIC,
                    sampleRate,
                    channelConfig,
                    AudioFormat.ENCODING_PCM_16BIT,
                    bufferSize,
                    AudioTrack.MODE_STREAM);
        }
        else {
            // build AudioTrack using Audio Attributes and FLAG_HW_AV_SYNC
            AudioAttributes audioAttributes = (new AudioAttributes.Builder())
                            .setLegacyStreamType(AudioManager.STREAM_MUSIC)
                            .setFlags(AudioAttributes.FLAG_HW_AV_SYNC)
                            .build();
            AudioFormat audioFormat = (new AudioFormat.Builder())
                            .setChannelMask(channelConfig)
                            .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                            .setSampleRate(sampleRate)
                            .build();
             mAudioTrack = new AudioTrack(audioAttributes, audioFormat, bufferSize,
                                    AudioTrack.MODE_STREAM, audioSessionId);
        }

        mSampleRate = sampleRate;
        mFrameSize = 2 * channelCount;
        mBufferSizeInFrames = bufferSize / mFrameSize;
    }

    public long getAudioTimeUs() {
        int numFramesPlayed = mAudioTrack.getPlaybackHeadPosition();

        return (numFramesPlayed * 1000000L) / mSampleRate;
    }

    public int getNumBytesQueued() {
        return mNumBytesQueued;
    }

    public void play() {
        mAudioTrack.play();
    }

    public void stop() {
        cancelWriteMore();

        mAudioTrack.stop();

        mNumFramesSubmitted = 0;
        mQueue.clear();
        mNumBytesQueued = 0;
    }

    public void pause() {
        cancelWriteMore();

        mAudioTrack.pause();
    }

    public void flush() {
        if (mAudioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
            return;
        }
        mAudioTrack.flush();
        mNumFramesSubmitted = 0;
        mQueue.clear();
        mNumBytesQueued = 0;
    }

    public void release() {
        cancelWriteMore();

        mAudioTrack.release();
        mAudioTrack = null;
    }

    public void process() {
        mWriteMorePending = false;
        writeMore();
    }

    public int getPlayState() {
        return mAudioTrack.getPlayState();
    }

    private void writeMore() {
        if (mQueue.isEmpty()) {
            return;
        }

        int numFramesPlayed = mAudioTrack.getPlaybackHeadPosition();
        int numFramesPending = mNumFramesSubmitted - numFramesPlayed;
        int numFramesAvailableToWrite = mBufferSizeInFrames - numFramesPending;
        int numBytesAvailableToWrite = numFramesAvailableToWrite * mFrameSize;

        while (numBytesAvailableToWrite > 0) {
            QueueElem elem = mQueue.peekFirst();

            int numBytes = elem.size;
            if (numBytes > numBytesAvailableToWrite) {
                numBytes = numBytesAvailableToWrite;
            }

            int written = mAudioTrack.write(elem.data, elem.offset, numBytes);
            assert(written == numBytes);

            mNumFramesSubmitted += written / mFrameSize;

            elem.size -= numBytes;
            numBytesAvailableToWrite -= numBytes;
            mNumBytesQueued -= numBytes;

            if (elem.size == 0) {
                mQueue.removeFirst();

                if (mQueue.isEmpty()) {
                    break;
                }
            } else {
                elem.offset += numBytes;
            }
        }

        if (!mQueue.isEmpty()) {
            scheduleWriteMore();
        }
    }

    private void scheduleWriteMore() {
        if (mWriteMorePending) {
            return;
        }

        int numFramesPlayed = mAudioTrack.getPlaybackHeadPosition();
        int numFramesPending = mNumFramesSubmitted - numFramesPlayed;
        int pendingDurationMs = 1000 * numFramesPending / mSampleRate;

        mWriteMorePending = true;
    }

    private void cancelWriteMore() {
        mWriteMorePending = false;
    }

    public void write(byte[] data, int size) {
        QueueElem elem = new QueueElem();
        elem.data = data;
        elem.offset = 0;
        elem.size = size;

        // accumulate size written to queue
        mNumBytesQueued += size;
        mQueue.add(elem);
    }
}

