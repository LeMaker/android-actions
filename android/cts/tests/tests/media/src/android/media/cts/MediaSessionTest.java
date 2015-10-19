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

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.media.AudioAttributes;
import android.media.AudioManager;
import android.media.MediaDescription;
import android.media.MediaMetadata;
import android.media.Rating;
import android.media.VolumeProvider;
import android.media.session.MediaController;
import android.media.session.MediaSession;
import android.media.session.MediaSessionManager;
import android.media.session.PlaybackState;
import android.os.Bundle;
import android.test.AndroidTestCase;

import java.util.ArrayList;
import java.util.Set;

public class MediaSessionTest extends AndroidTestCase {
    private AudioManager mAudioManager;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
    }

    /**
     * Tests that a session can be created and that all the fields are
     * initialized correctly.
     */
    public void testCreateSession() throws Exception {
        String tag = "test session";
        MediaSession session = new MediaSession(getContext(), tag);
        assertNotNull(session.getSessionToken());
        assertFalse("New session should not be active", session.isActive());

        // Verify by getting the controller and checking all its fields
        MediaController controller = session.getController();
        assertNotNull(controller);
        verifyNewSession(controller, tag);
    }

    /**
     * Tests that the various configuration bits on a session get passed to the
     * controller.
     */
    public void testConfigureSession() throws Exception {
        String tag = "test session";
        String key = "test-key";
        String val = "test-val";
        MediaSession session = new MediaSession(getContext(), tag);
        MediaController controller = session.getController();

        // test setExtras
        Bundle extras = new Bundle();
        extras.putString(key, val);
        session.setExtras(extras);
        Bundle extrasOut = controller.getExtras();
        assertNotNull(extrasOut);
        assertEquals(val, extrasOut.get(key));

        // test setFlags
        session.setFlags(5);
        assertEquals(5, controller.getFlags());

        // test setMetadata
        MediaMetadata metadata = new MediaMetadata.Builder().putString(key, val).build();
        session.setMetadata(metadata);
        MediaMetadata metadataOut = controller.getMetadata();
        assertNotNull(metadataOut);
        assertEquals(val, metadataOut.getString(key));

        // test setPlaybackState
        PlaybackState state = new PlaybackState.Builder().setActions(55).build();
        session.setPlaybackState(state);
        PlaybackState stateOut = controller.getPlaybackState();
        assertNotNull(stateOut);
        assertEquals(55L, stateOut.getActions());

        // test setPlaybackToRemote, do this before testing setPlaybackToLocal
        // to ensure it switches correctly.
        try {
            session.setPlaybackToRemote(null);
            fail("Expected IAE for setPlaybackToRemote(null)");
        } catch (IllegalArgumentException e) {
            // expected
        }
        VolumeProvider vp = new VolumeProvider(VolumeProvider.VOLUME_CONTROL_FIXED, 11, 11) {};
        session.setPlaybackToRemote(vp);
        MediaController.PlaybackInfo info = controller.getPlaybackInfo();
        assertNotNull(info);
        assertEquals(MediaController.PlaybackInfo.PLAYBACK_TYPE_REMOTE, info.getPlaybackType());
        assertEquals(11, info.getMaxVolume());
        assertEquals(11, info.getCurrentVolume());
        assertEquals(VolumeProvider.VOLUME_CONTROL_FIXED, info.getVolumeControl());

        // test setPlaybackToLocal
        AudioAttributes attrs = new AudioAttributes.Builder().addTag(val).build();
        session.setPlaybackToLocal(attrs);
        info = controller.getPlaybackInfo();
        assertNotNull(info);
        assertEquals(MediaController.PlaybackInfo.PLAYBACK_TYPE_LOCAL, info.getPlaybackType());
        Set<String> tags = info.getAudioAttributes().getTags();
        assertNotNull(tags);
        assertTrue(tags.contains(val));

        // test setQueue and setQueueTitle
        ArrayList<MediaSession.QueueItem> queue = new ArrayList<MediaSession.QueueItem>();
        MediaSession.QueueItem item = new MediaSession.QueueItem(new MediaDescription.Builder()
                .setMediaId(val).setTitle("title").build(), 11);
        queue.add(item);
        session.setQueue(queue);
        session.setQueueTitle(val);

        assertEquals(val, controller.getQueueTitle());
        assertEquals(1, controller.getQueue().size());
        assertEquals(11, controller.getQueue().get(0).getQueueId());
        assertEquals(val, controller.getQueue().get(0).getDescription().getMediaId());

        session.setQueue(null);
        session.setQueueTitle(null);

        assertNull(controller.getQueueTitle());
        assertNull(controller.getQueue());

        // test setSessionActivity
        Intent intent = new Intent("cts.MEDIA_SESSION_ACTION");
        PendingIntent pi = PendingIntent.getActivity(getContext(), 555, intent, 0);
        session.setSessionActivity(pi);
        assertEquals(pi, controller.getSessionActivity());
    }

    /**
     * Verifies that a new session hasn't had any configuration bits set yet.
     *
     * @param controller The controller for the session
     */
    private void verifyNewSession(MediaController controller, String tag) {
        assertEquals("New session has unexpected configuration", 0L, controller.getFlags());
        assertNull("New session has unexpected configuration", controller.getExtras());
        assertNull("New session has unexpected configuration", controller.getMetadata());
        assertEquals("New session has unexpected configuration",
                getContext().getPackageName(), controller.getPackageName());
        assertNull("New session has unexpected configuration", controller.getPlaybackState());
        assertNull("New session has unexpected configuration", controller.getQueue());
        assertNull("New session has unexpected configuration", controller.getQueueTitle());
        assertEquals("New session has unexpected configuration", Rating.RATING_NONE,
                controller.getRatingType());
        assertNull("New session has unexpected configuration", controller.getSessionActivity());

        assertNotNull(controller.getSessionToken());
        assertNotNull(controller.getTransportControls());
        assertEquals(tag, controller.getTag());

        MediaController.PlaybackInfo info = controller.getPlaybackInfo();
        assertNotNull(info);
        assertEquals(MediaController.PlaybackInfo.PLAYBACK_TYPE_LOCAL, info.getPlaybackType());
        AudioAttributes attrs = info.getAudioAttributes();
        assertNotNull(attrs);
        assertEquals(AudioAttributes.USAGE_MEDIA, attrs.getUsage());
        assertEquals(mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC),
                info.getCurrentVolume());
    }
}
