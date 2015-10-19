/*
 * Copyright (C) 2013 The Android Open Source Project
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

import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;
import android.media.MediaMetadataRetriever;
import android.test.AndroidTestCase;

public class MediaMetadataRetrieverTest extends AndroidTestCase {

    @Override
    protected void setUp() throws Exception {
        super.setUp();
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
    }

    public void test3gppMetadata() {
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();

        try {
            Resources resources = getContext().getResources();
            AssetFileDescriptor afd = resources.openRawResourceFd(R.raw.testvideo);

            retriever.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());

            afd.close();
        } catch (Exception e) {
            fail("Unable to open file");
        }

        assertEquals("Title was other than expected",
                "Title", retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE));

        assertEquals("Artist was other than expected",
                "UTF16LE エンディアン ",
                retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ARTIST));

        assertEquals("Album was other than expected",
                "Test album",
                retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ALBUM));

        assertEquals("Track number was other than expected",
                "10",
                retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_CD_TRACK_NUMBER));

        assertEquals("Year was other than expected",
                "2013", retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_YEAR));

        assertNull("Writer was unexpected present",
                retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_WRITER));
    }

    public void testSetDataSourceNull() {
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();

        try {
            retriever.setDataSource((String)null);
            fail("Expected IllegalArgumentException.");
        } catch (IllegalArgumentException ex) {
            // Expected, test passed.
        }
    }

    public void testID3v2Metadata() {
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();

        try {
            Resources resources = getContext().getResources();
            AssetFileDescriptor afd = resources.openRawResourceFd(
                    R.raw.video_480x360_mp4_h264_500kbps_25fps_aac_stereo_128kbps_44100hz_id3v2);

            retriever.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());

            afd.close();
        } catch (Exception e) {
            fail("Unable to open file");
        }

        assertEquals("Title was other than expected",
                "Title", retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE));

        assertEquals("Artist was other than expected",
                "UTF16LE エンディアン ",
                retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ARTIST));

        assertEquals("Album was other than expected",
                "Test album",
                retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ALBUM));

        assertEquals("Track number was other than expected",
                "10",
                retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_CD_TRACK_NUMBER));

        assertEquals("Year was other than expected",
                "2013", retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_YEAR));

        assertNull("Writer was unexpectedly present",
                retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_WRITER));
    }
}
