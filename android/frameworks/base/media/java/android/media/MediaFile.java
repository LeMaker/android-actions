/*
 * Copyright (C) 2007 The Android Open Source Project
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

package android.media;

import android.media.DecoderCapabilities;
import android.media.DecoderCapabilities.VideoDecoder;
import android.media.DecoderCapabilities.AudioDecoder;
import android.mtp.MtpConstants;

import java.util.HashMap;
import java.util.List;
import java.util.Locale;

/**
 * MediaScanner helper class.
 *
 * {@hide}
 */
public class MediaFile {
    //ActionsCode(authro:lishiyuan, comment: support more media format)
	private static final boolean use_actions_code = true;

    // Audio file types
    public static final int FILE_TYPE_MP3     = 1;
    public static final int FILE_TYPE_M4A     = 2;
    public static final int FILE_TYPE_WAV     = 3;
    public static final int FILE_TYPE_AMR     = 4;
    public static final int FILE_TYPE_AWB     = 5;
    public static final int FILE_TYPE_WMA     = 6;
    public static final int FILE_TYPE_OGG     = 7;
    public static final int FILE_TYPE_AAC     = 8;
    public static final int FILE_TYPE_MKA     = 9;
    public static final int FILE_TYPE_RM      = 10;
    public static final int FILE_TYPE_DTS     = 11;
    public static final int FILE_TYPE_AC3     = 12;    
    public static final int FILE_TYPE_AA      = 13;
    public static final int FILE_TYPE_AIFF    = 14;
    public static final int FILE_TYPE_MPC     = 15;    
    public static final int FILE_TYPE_ALAC    = 16;
    public static final int FILE_TYPE_APE     = 17;
    public static final int FILE_TYPE_PCM     = 18;
    public static final int FILE_TYPE_FLAC    = 19;
    private static final int FIRST_AUDIO_FILE_TYPE = FILE_TYPE_MP3;
    private static final int LAST_AUDIO_FILE_TYPE = FILE_TYPE_FLAC;

    // MIDI file types
    public static final int FILE_TYPE_MID     = 21;
    public static final int FILE_TYPE_SMF     = 22;
    public static final int FILE_TYPE_IMY     = 23;
    private static final int FIRST_MIDI_FILE_TYPE = FILE_TYPE_MID;
    private static final int LAST_MIDI_FILE_TYPE = FILE_TYPE_IMY;
   
    // Video file types
    public static final int FILE_TYPE_MP4     = 31;
    public static final int FILE_TYPE_M4V     = 32;
    public static final int FILE_TYPE_3GPP    = 33;
    public static final int FILE_TYPE_3GPP2   = 34;
    public static final int FILE_TYPE_WMV     = 35;
    public static final int FILE_TYPE_ASF     = 36;
    public static final int FILE_TYPE_MKV     = 37;
    public static final int FILE_TYPE_MP2TS   = 38;
    public static final int FILE_TYPE_AVI     = 39;
    public static final int FILE_TYPE_WEBM    = 40;
    public static final int FILE_TYPE_FLV     = 41;
    public static final int FILE_TYPE_MPG     = 42;
    public static final int FILE_TYPE_DIVX    = 43;    
    public static final int FILE_TYPE_OGM     = 44;    
    public static final int FILE_TYPE_RMVB    = 45;
    private static final int FIRST_VIDEO_FILE_TYPE = FILE_TYPE_MP4;
    private static final int LAST_VIDEO_FILE_TYPE = FILE_TYPE_RMVB;
    
    // More video file types
    public static final int FILE_TYPE_MP2PS   = 200;
    private static final int FIRST_VIDEO_FILE_TYPE2 = FILE_TYPE_MP2PS;
    private static final int LAST_VIDEO_FILE_TYPE2 = FILE_TYPE_MP2PS;

    // Image file types
    public static final int FILE_TYPE_JPEG    = 51;
    public static final int FILE_TYPE_GIF     = 52;
    public static final int FILE_TYPE_PNG     = 53;
    public static final int FILE_TYPE_BMP     = 54;
    public static final int FILE_TYPE_TIFF    = 55;
    public static final int FILE_TYPE_WBMP    = 56;
    public static final int FILE_TYPE_WEBP    = 57;
    private static final int FIRST_IMAGE_FILE_TYPE = FILE_TYPE_JPEG;
    private static final int LAST_IMAGE_FILE_TYPE = FILE_TYPE_WEBP;
   
    // Playlist file types
    public static final int FILE_TYPE_M3U      = 61;
    public static final int FILE_TYPE_PLS      = 62;
    public static final int FILE_TYPE_WPL      = 63;
    public static final int FILE_TYPE_HTTPLIVE = 64;

    private static final int FIRST_PLAYLIST_FILE_TYPE = FILE_TYPE_M3U;
    private static final int LAST_PLAYLIST_FILE_TYPE = FILE_TYPE_HTTPLIVE;

    // Drm file types
    public static final int FILE_TYPE_FL      = 71;
    private static final int FIRST_DRM_FILE_TYPE = FILE_TYPE_FL;
    private static final int LAST_DRM_FILE_TYPE = FILE_TYPE_FL;

    // Other popular file types
    public static final int FILE_TYPE_TEXT          = 100;
    public static final int FILE_TYPE_HTML          = 101;
    public static final int FILE_TYPE_PDF           = 102;
    public static final int FILE_TYPE_XML           = 103;
    public static final int FILE_TYPE_MS_WORD       = 104;
    public static final int FILE_TYPE_MS_EXCEL      = 105;
    public static final int FILE_TYPE_MS_POWERPOINT = 106;
    public static final int FILE_TYPE_ZIP           = 107;
    
    public static class MediaFileType {
        public final int fileType;
        public final String mimeType;
        
        MediaFileType(int fileType, String mimeType) {
            this.fileType = fileType;
            this.mimeType = mimeType;
        }
    }
    
    private static final HashMap<String, MediaFileType> sFileTypeMap
            = new HashMap<String, MediaFileType>();
    private static final HashMap<String, Integer> sMimeTypeMap
            = new HashMap<String, Integer>();
    // maps file extension to MTP format code
    private static final HashMap<String, Integer> sFileTypeToFormatMap
            = new HashMap<String, Integer>();
    // maps mime type to MTP format code
    private static final HashMap<String, Integer> sMimeTypeToFormatMap
            = new HashMap<String, Integer>();
    // maps MTP format code to mime type
    private static final HashMap<Integer, String> sFormatToMimeTypeMap
            = new HashMap<Integer, String>();

    static void addFileType(String extension, int fileType, String mimeType) {
        sFileTypeMap.put(extension, new MediaFileType(fileType, mimeType));
        sMimeTypeMap.put(mimeType, Integer.valueOf(fileType));
    }

    static void addFileType(String extension, int fileType, String mimeType, int mtpFormatCode) {
        addFileType(extension, fileType, mimeType);
        sFileTypeToFormatMap.put(extension, Integer.valueOf(mtpFormatCode));
        sMimeTypeToFormatMap.put(mimeType, Integer.valueOf(mtpFormatCode));
        sFormatToMimeTypeMap.put(mtpFormatCode, mimeType);
    }

    private static boolean isWMAEnabled() {
        List<AudioDecoder> decoders = DecoderCapabilities.getAudioDecoders();
        int count = decoders.size();
        for (int i = 0; i < count; i++) {
            AudioDecoder decoder = decoders.get(i);
            if (decoder == AudioDecoder.AUDIO_DECODER_WMA) {
                return true;
            }
        }
        return false;
    }

    private static boolean isWMVEnabled() {
        List<VideoDecoder> decoders = DecoderCapabilities.getVideoDecoders();
        int count = decoders.size();
        for (int i = 0; i < count; i++) {
            VideoDecoder decoder = decoders.get(i);
            if (decoder == VideoDecoder.VIDEO_DECODER_WMV) {
                return true;
            }
        }
        return false;
    }
	/*
	* 2014-12-23 fix bug BUG00234495, add new 
	*                   media format xv
 	************************************
  	*      
  	*ActionsCode(rongxing , change_code)
  	*/
    static {
        addFileType("MP3", FILE_TYPE_MP3, "audio/mpeg", MtpConstants.FORMAT_MP3);
        if (use_actions_code){
            addFileType("MP2", FILE_TYPE_MP3, "audio/mpeg", MtpConstants.FORMAT_MP3);
            addFileType("MP1", FILE_TYPE_MP3, "audio/mpeg", MtpConstants.FORMAT_MP3);
        }
		
        addFileType("MPGA", FILE_TYPE_MP3, "audio/mpeg", MtpConstants.FORMAT_MP3);
        addFileType("M4A", FILE_TYPE_M4A, "audio/mp4", MtpConstants.FORMAT_MPEG);
        addFileType("WAV", FILE_TYPE_WAV, "audio/x-wav", MtpConstants.FORMAT_WAV);
        addFileType("AMR", FILE_TYPE_AMR, "audio/amr");
        addFileType("AWB", FILE_TYPE_AWB, "audio/amr-wb");
        if (isWMAEnabled()) {
            addFileType("WMA", FILE_TYPE_WMA, "audio/x-ms-wma", MtpConstants.FORMAT_WMA);
        }
        addFileType("OGG", FILE_TYPE_OGG, "audio/ogg", MtpConstants.FORMAT_OGG);
        addFileType("OGG", FILE_TYPE_OGG, "application/ogg", MtpConstants.FORMAT_OGG);
        addFileType("OGA", FILE_TYPE_OGG, "application/ogg", MtpConstants.FORMAT_OGG);
        addFileType("AAC", FILE_TYPE_AAC, "audio/aac", MtpConstants.FORMAT_AAC);
        addFileType("AAC", FILE_TYPE_AAC, "audio/aac-adts", MtpConstants.FORMAT_AAC);
        addFileType("MKA", FILE_TYPE_MKA, "audio/x-matroska");
        if (use_actions_code){
		    addFileType("RM", FILE_TYPE_RM, "audio/x-pn-realaudio");
			addFileType("RAM", FILE_TYPE_RM, "audio/x-pn-realaudio");
        	addFileType("DTS", FILE_TYPE_DTS, "audio/x-dts");
        	addFileType("AC3", FILE_TYPE_AC3, "audio/x-ac3");
			addFileType("AA", FILE_TYPE_AA, "audio/audible");
			addFileType("AAX", FILE_TYPE_AA, "audio/audible");
			addFileType("FLAC", FILE_TYPE_FLAC, "audio/x-flac");
        	addFileType("APE", FILE_TYPE_APE, "audio/x-ape");
 
			addFileType("DTS", FILE_TYPE_DTS, "audio/DTS");
			addFileType("AC3", FILE_TYPE_AC3, "audio/AC3");
			addFileType("MP3", FILE_TYPE_MP3, "audio/MP3");
			addFileType("AAC", FILE_TYPE_AAC, "audio/AAC");
			addFileType("AMR", FILE_TYPE_AMR, "audio/AMR");
			addFileType("WMA", FILE_TYPE_WMA, "audio/WMASTD");
			addFileType("WMA", FILE_TYPE_WMA, "audio/WMALSL");
			addFileType("WMA", FILE_TYPE_WMA, "audio/WMAPRO");
			addFileType("OGG", FILE_TYPE_OGG, "audio/OGG");
			addFileType("APE", FILE_TYPE_APE, "audio/APE");
			addFileType("FLAC", FILE_TYPE_FLAC, "audio/FLAC");
			addFileType("RM", FILE_TYPE_RM, "audio/COOK");
        	addFileType("RA", FILE_TYPE_RM, "audio/COOK");
			addFileType("PCM", FILE_TYPE_PCM, "audio/PCM");
			addFileType("PCM", FILE_TYPE_PCM, "audio/ADPCM");
        	addFileType("AWB", FILE_TYPE_AWB, "audio/AWB");
        }
        addFileType("MID", FILE_TYPE_MID, "audio/midi");
        addFileType("MIDI", FILE_TYPE_MID, "audio/midi");
        addFileType("XMF", FILE_TYPE_MID, "audio/midi");
        addFileType("RTTTL", FILE_TYPE_MID, "audio/midi");
        addFileType("SMF", FILE_TYPE_SMF, "audio/sp-midi");
        addFileType("IMY", FILE_TYPE_IMY, "audio/imelody");
        addFileType("RTX", FILE_TYPE_MID, "audio/midi");
        addFileType("OTA", FILE_TYPE_MID, "audio/midi");
        addFileType("MXMF", FILE_TYPE_MID, "audio/midi");
        
        addFileType("MPEG", FILE_TYPE_MP4, "video/mpeg", MtpConstants.FORMAT_MPEG);
        addFileType("MPG", FILE_TYPE_MP4, "video/mpeg", MtpConstants.FORMAT_MPEG);
        addFileType("MP4", FILE_TYPE_MP4, "video/mp4", MtpConstants.FORMAT_MPEG);
        addFileType("M4V", FILE_TYPE_M4V, "video/mp4", MtpConstants.FORMAT_MPEG);
		if (use_actions_code){
		    addFileType("MOV", FILE_TYPE_M4V, "video/mp4", MtpConstants.FORMAT_MPEG);//add by 3307
		}
        addFileType("3GP", FILE_TYPE_3GPP, "video/3gpp",  MtpConstants.FORMAT_3GP_CONTAINER);
        addFileType("3GPP", FILE_TYPE_3GPP, "video/3gpp", MtpConstants.FORMAT_3GP_CONTAINER);
        addFileType("3G2", FILE_TYPE_3GPP2, "video/3gpp2", MtpConstants.FORMAT_3GP_CONTAINER);
        addFileType("3GPP2", FILE_TYPE_3GPP2, "video/3gpp2", MtpConstants.FORMAT_3GP_CONTAINER);
        addFileType("MKV", FILE_TYPE_MKV, "video/x-matroska");
        addFileType("WEBM", FILE_TYPE_WEBM, "video/webm");
        addFileType("TS", FILE_TYPE_MP2TS, "video/mp2ts");
        addFileType("AVI", FILE_TYPE_AVI, "video/avi");

        if (use_actions_code){
			addFileType("TS", FILE_TYPE_MP2TS, "video/ts");
        	addFileType("M2TS", FILE_TYPE_MP2TS, "video/ts");
        	addFileType("MTS", FILE_TYPE_MP2TS, "video/ts");
        	addFileType("TP", FILE_TYPE_MP2TS, "video/ts");
        	addFileType("F4V", FILE_TYPE_FLV, "video/flv");
        	addFileType("FLV", FILE_TYPE_FLV, "video/flv");
			//ActionsCode(author:rongxing, fix BUG00234495: add new media format xv)
			addFileType("XV", FILE_TYPE_FLV, "video/flv");
        	addFileType("RMVB", FILE_TYPE_RMVB, "video/rm");
        	addFileType("RM", FILE_TYPE_RMVB, "video/rm");
        	addFileType("DAT", FILE_TYPE_MPG, "video/mpg");
	        addFileType("VOB", FILE_TYPE_MPG, "video/mpg");
    	    addFileType("EVO", FILE_TYPE_MPG, "video/mpg");
        	addFileType("MKV", FILE_TYPE_MKV, "video/mkv");
        	addFileType("WEBM", FILE_TYPE_WEBM, "video/mkv");
        	addFileType("WMV", FILE_TYPE_WMV, "video/wmv");
        	addFileType("ASF", FILE_TYPE_ASF, "video/wmv");
        	addFileType("DIVX", FILE_TYPE_DIVX, "video/avi");
        	addFileType("OGG", FILE_TYPE_OGM, "video/ogm");
        	addFileType("OGM", FILE_TYPE_OGM, "video/ogm");
        	addFileType("3GP", FILE_TYPE_3GPP, "video/mp4");
        	addFileType("3GPP", FILE_TYPE_3GPP, "video/mp4");
        	addFileType("3G2", FILE_TYPE_3GPP2, "video/mp4");
        	addFileType("3GPP2", FILE_TYPE_3GPP2, "video/mp4");	
		}
        if (isWMVEnabled()) {
            addFileType("WMV", FILE_TYPE_WMV, "video/x-ms-wmv", MtpConstants.FORMAT_WMV);
			if (use_actions_code){
				addFileType("WMV", FILE_TYPE_WMV, "video/wmv");
			}
            addFileType("ASF", FILE_TYPE_ASF, "video/x-ms-asf");
        }
        //images
        addFileType("JPG", FILE_TYPE_JPEG, "image/jpeg", MtpConstants.FORMAT_EXIF_JPEG);
        addFileType("JPEG", FILE_TYPE_JPEG, "image/jpeg", MtpConstants.FORMAT_EXIF_JPEG);
        addFileType("GIF", FILE_TYPE_GIF, "image/gif", MtpConstants.FORMAT_GIF);
        addFileType("PNG", FILE_TYPE_PNG, "image/png", MtpConstants.FORMAT_PNG);
        addFileType("BMP", FILE_TYPE_BMP, "image/x-ms-bmp", MtpConstants.FORMAT_BMP);
		if (use_actions_code){
		    addFileType("TIFF", FILE_TYPE_TIFF, "image/tiff");
            addFileType("TIF", FILE_TYPE_TIFF, "image/tif");
		}
        addFileType("WBMP", FILE_TYPE_WBMP, "image/vnd.wap.wbmp");
        addFileType("WEBP", FILE_TYPE_WEBP, "image/webp");
 
        addFileType("M3U", FILE_TYPE_M3U, "audio/x-mpegurl", MtpConstants.FORMAT_M3U_PLAYLIST);
        addFileType("M3U", FILE_TYPE_M3U, "application/x-mpegurl", MtpConstants.FORMAT_M3U_PLAYLIST);
        addFileType("PLS", FILE_TYPE_PLS, "audio/x-scpls", MtpConstants.FORMAT_PLS_PLAYLIST);
        addFileType("WPL", FILE_TYPE_WPL, "application/vnd.ms-wpl", MtpConstants.FORMAT_WPL_PLAYLIST);
        addFileType("M3U8", FILE_TYPE_HTTPLIVE, "application/vnd.apple.mpegurl");
        addFileType("M3U8", FILE_TYPE_HTTPLIVE, "audio/mpegurl");
        addFileType("M3U8", FILE_TYPE_HTTPLIVE, "audio/x-mpegurl");

        addFileType("FL", FILE_TYPE_FL, "application/x-android-drm-fl");

        addFileType("TXT", FILE_TYPE_TEXT, "text/plain", MtpConstants.FORMAT_TEXT);
        addFileType("HTM", FILE_TYPE_HTML, "text/html", MtpConstants.FORMAT_HTML);
        addFileType("HTML", FILE_TYPE_HTML, "text/html", MtpConstants.FORMAT_HTML);
        addFileType("PDF", FILE_TYPE_PDF, "application/pdf");
        addFileType("DOC", FILE_TYPE_MS_WORD, "application/msword", MtpConstants.FORMAT_MS_WORD_DOCUMENT);
        addFileType("XLS", FILE_TYPE_MS_EXCEL, "application/vnd.ms-excel", MtpConstants.FORMAT_MS_EXCEL_SPREADSHEET);
        addFileType("PPT", FILE_TYPE_MS_POWERPOINT, "application/mspowerpoint", MtpConstants.FORMAT_MS_POWERPOINT_PRESENTATION);
        addFileType("FLAC", FILE_TYPE_FLAC, "audio/flac", MtpConstants.FORMAT_FLAC);
        addFileType("ZIP", FILE_TYPE_ZIP, "application/zip");
        addFileType("MPG", FILE_TYPE_MP2PS, "video/mp2p");
        addFileType("MPEG", FILE_TYPE_MP2PS, "video/mp2p");
    }

    public static boolean isAudioFileType(int fileType) {
        return ((fileType >= FIRST_AUDIO_FILE_TYPE &&
                fileType <= LAST_AUDIO_FILE_TYPE) ||
                (fileType >= FIRST_MIDI_FILE_TYPE &&
                fileType <= LAST_MIDI_FILE_TYPE));
    }

    public static boolean isVideoFileType(int fileType) {
        return (fileType >= FIRST_VIDEO_FILE_TYPE &&
                fileType <= LAST_VIDEO_FILE_TYPE)
            || (fileType >= FIRST_VIDEO_FILE_TYPE2 &&
                fileType <= LAST_VIDEO_FILE_TYPE2);
    }

    public static boolean isImageFileType(int fileType) {
        return (fileType >= FIRST_IMAGE_FILE_TYPE &&
                fileType <= LAST_IMAGE_FILE_TYPE);
    }

    public static boolean isPlayListFileType(int fileType) {
        return (fileType >= FIRST_PLAYLIST_FILE_TYPE &&
                fileType <= LAST_PLAYLIST_FILE_TYPE);
    }

    public static boolean isDrmFileType(int fileType) {
        return (fileType >= FIRST_DRM_FILE_TYPE &&
                fileType <= LAST_DRM_FILE_TYPE);
    }

    public static MediaFileType getFileType(String path) {
        int lastDot = path.lastIndexOf('.');
        if (lastDot < 0)
            return null;
        return sFileTypeMap.get(path.substring(lastDot + 1).toUpperCase(Locale.ROOT));
    }

    public static boolean isMimeTypeMedia(String mimeType) {
        int fileType = getFileTypeForMimeType(mimeType);
        return isAudioFileType(fileType) || isVideoFileType(fileType)
                || isImageFileType(fileType) || isPlayListFileType(fileType);
    }

    // generates a title based on file name
    public static String getFileTitle(String path) {
        // extract file name after last slash
        int lastSlash = path.lastIndexOf('/');
        if (lastSlash >= 0) {
            lastSlash++;
            if (lastSlash < path.length()) {
                path = path.substring(lastSlash);
            }
        }
        // truncate the file extension (if any)
        int lastDot = path.lastIndexOf('.');
        if (lastDot > 0) {
            path = path.substring(0, lastDot);
        }
        return path;
    }

    public static int getFileTypeForMimeType(String mimeType) {
        Integer value = sMimeTypeMap.get(mimeType);
        return (value == null ? 0 : value.intValue());
    }

    public static String getMimeTypeForFile(String path) {
        MediaFileType mediaFileType = getFileType(path);
        return (mediaFileType == null ? null : mediaFileType.mimeType);
    }

    public static int getFormatCode(String fileName, String mimeType) {
        if (mimeType != null) {
            Integer value = sMimeTypeToFormatMap.get(mimeType);
            if (value != null) {
                return value.intValue();
            }
        }
        int lastDot = fileName.lastIndexOf('.');
        if (lastDot > 0) {
            String extension = fileName.substring(lastDot + 1).toUpperCase(Locale.ROOT);
            Integer value = sFileTypeToFormatMap.get(extension);
            if (value != null) {
                return value.intValue();
            }
        }
        return MtpConstants.FORMAT_UNDEFINED;
    }

    public static String getMimeTypeForFormatCode(int formatCode) {
        return sFormatToMimeTypeMap.get(formatCode);
    }
}
