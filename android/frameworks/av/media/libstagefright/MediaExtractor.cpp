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

//#define LOG_NDEBUG 0
#define LOG_TAG "MediaExtractor"
#include <utils/Log.h>
#include "include/ActAudioExtractor.h"
#include "include/ActVideoExtractor.h"
#include "include/AMRExtractor.h"
#include "include/MP3Extractor.h"
#include "include/MPEG4Extractor.h"
#include "include/WAVExtractor.h"
#include "include/OggExtractor.h"
#include "include/MPEG2PSExtractor.h"
#include "include/MPEG2TSExtractor.h"
#include "include/DRMExtractor.h"
#include "include/WVMExtractor.h"
#include "include/FLACExtractor.h"
#include "include/AACExtractor.h"

#include "matroska/MatroskaExtractor.h"

#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/MetaData.h>
#include <utils/String8.h>

#include <cutils/properties.h>
namespace android {

sp<MetaData> MediaExtractor::getMetaData() {
    return new MetaData;
}

uint32_t MediaExtractor::flags() const {
    return CAN_SEEK_BACKWARD | CAN_SEEK_FORWARD | CAN_PAUSE | CAN_SEEK;
}

/*
  * audio use all google's extractor.
  ************************************
  *      
  *ActionsCode(author:jinsongxue, change_code)
  */
// static
sp<MediaExtractor> MediaExtractor::Create(
        const sp<DataSource> &source, const char *mime) {
    sp<AMessage> meta;

    String8 tmp;
	MediaExtractor *ret = NULL;
    bool isDrm = false;

    if (mime == NULL) {
        float confidence;
        if (!source->sniff(&tmp, &confidence, &meta)) {
            ALOGV("FAILED to autodetect media content.");

            goto actions_extractor;
        }

        mime = tmp.string();
        ALOGV("Autodetected media content as '%s' with confidence %.2f",
             mime, confidence);
    }

   
    // DRM MIME type syntax is "drm+type+original" where
    // type is "es_based" or "container_based" and
    // original is the content's cleartext MIME type
    if (!strncmp(mime, "drm+", 4)) {
        const char *originalMime = strchr(mime+4, '+');
        if (originalMime == NULL) {
            // second + not found
            return NULL;
        }
        ++originalMime;
        if (!strncmp(mime, "drm+es_based+", 13)) {
            // DRMExtractor sets container metadata kKeyIsDRM to 1
            return new DRMExtractor(source, originalMime);
        } else if (!strncmp(mime, "drm+container_based+", 20)) {
            mime = originalMime;
            isDrm = true;
        } else {
            return NULL;
        }
    }

 
    if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG4)
            || !strcasecmp(mime, "audio/mp4")) {
        ret = new MPEG4Extractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_NB)
            || !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_WB)) {
        ret = new AMRExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_OGG)) {
        ret = new OggExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MATROSKA)) {
        ret = new MatroskaExtractor(source);
 
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WVM)) {
        // Return now.  WVExtractor should not have the DrmFlag set in the block below.
        return new WVMExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC_ADTS)) {
        ret = new AACExtractor(source, meta);
    }else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MPEG)) {
        ret = new MP3Extractor(source, meta);
    }else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_FLAC)) {
        ret = new FLACExtractor(source);
    }else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WAV)) {
        ret = new WAVExtractor(source);
    }
    if (ret != NULL) {
       if (isDrm) {
           ret->setDrmFlag(true);
       } else {
           ret->setDrmFlag(false);
       }
      return ret;
    }

actions_extractor:
	

  	off64_t filelen=0;
  	int rt = 0;
  	storage_io_t *storage_io = NULL;
  	char mime_type[32] = "";   	
  	storage_io = create_storage_io();
		if (storage_io == NULL)	{
			return NULL;
		}
		filelen = init_storage_io(storage_io, source);
		rt = format_check(storage_io, mime_type);
		if (mime_type[0] == '\0' || rt != 0) {
			dispose_storage_io(storage_io);
			ALOGE("no media content detected error !!!");
			return NULL;
		}
	
	 dispose_storage_io(storage_io);
	 
   /*ActionsCode(author:sunchengzhi,BUG00282856 shield format according to ro.owlplayer.ext)*/
   char demuxer_shield[PROPERTY_VALUE_MAX] = "";	 
   property_get("ro.owlplayer.ext", demuxer_shield, "unknown");
   if(strstr(demuxer_shield,mime_type)!=NULL){
   	ALOGD("%s not supported\n",mime_type);
   	return NULL;
   }
   /*ActionsCode(author:sunchengzhi,BUG00282856 end)*/
   
   ALOGI("Create: format mime_type: %s", mime_type);
     
     
	 if ((mime_type[0] >= 'a') && (mime_type[0] <= 'z')) {/* video */
		 ALOGV("Create()->new ActVideoExtractor!!!");
		 ret = new ActVideoExtractor(source, mime_type, NULL);
	 }else if ( (mime_type[0] >= 'A') && (mime_type[0] <= 'Z')) {
		 ret = new ActAudioExtractor(source, mime_type);
	 }else {
		 ALOGE("Creat: find extractor meet error!!!");
		 ret = NULL;
	 }
	
	if (ret != NULL) {
		isDrm ? ret->setDrmFlag(true) : ret->setDrmFlag(false);
		return ret;
	}
    return NULL;
}

}  // namespace android
