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

import com.android.cts.media.R;

import android.content.Context;
import android.media.audiofx.AudioEffect;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.audiofx.LoudnessEnhancer;
import android.os.Looper;
import android.test.AndroidTestCase;
import android.util.Log;

public class LoudnessEnhancerTest extends PostProcTestBase {

    private String TAG = "LoudnessEnhancerTest";
    private LoudnessEnhancer mLE;

    //-----------------------------------------------------------------
    // LOUDNESS ENHANCER TESTS:
    //----------------------------------

    //-----------------------------------------------------------------
    // 0 - constructor
    //----------------------------------

    //Test case 0.0: test constructor and release
    public void test0_0ConstructorAndRelease() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        AudioManager am = (AudioManager) getContext().getSystemService(Context.AUDIO_SERVICE);
        assertNotNull("null AudioManager", am);
        getLoudnessEnhancer(0);
        releaseLoudnessEnhancer();

        int session = am.generateAudioSessionId();
        assertTrue("cannot generate new session", session != AudioManager.ERROR);
        getLoudnessEnhancer(session);
        releaseLoudnessEnhancer();
    }

    //-----------------------------------------------------------------
    // 1 - get/set parameters
    //----------------------------------

    //Test case 1.0: test set/get target gain
    public void test1_0TargetGain() throws Exception {
        if (!hasAudioOutput()) {
            return;
        }
        getLoudnessEnhancer(0);
        try {
            mLE.setTargetGain(0);
            assertEquals("target gain differs from value set", 0.0f, mLE.getTargetGain());
            mLE.setTargetGain(800);
            assertEquals("target gain differs from value set", 800.0f, mLE.getTargetGain());
        } catch (IllegalArgumentException e) {
            fail("target gain illegal argument");
        } catch (UnsupportedOperationException e) {
            fail("target gain unsupported operation");
        } catch (IllegalStateException e) {
            fail("target gain operation called in wrong state");
        } finally {
            releaseLoudnessEnhancer();
        }
    }

    //-----------------------------------------------------------------
    // private methods
    //----------------------------------
    private void getLoudnessEnhancer(int session) {
        releaseLoudnessEnhancer();
        try {
            mLE = new LoudnessEnhancer(session);
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "getLoudnessEnhancer() LoudnessEnhancer not found exception: ", e);
        } catch (UnsupportedOperationException e) {
            Log.e(TAG, "getLoudnessEnhancer() Effect library not loaded exception: ", e);
        }
        assertNotNull("could not create LoudnessEnhancer", mLE);
    }

    private void releaseLoudnessEnhancer() {
        if (mLE != null) {
            mLE.release();
            mLE = null;
        }
    }
}