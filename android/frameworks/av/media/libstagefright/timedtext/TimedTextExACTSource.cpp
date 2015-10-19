 /*
 * Copyright (C) 2012 The Android Open Source Project
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
#define LOG_TAG "TimedTextExACTSource"
#include <utils/Log.h>

#include <binder/Parcel.h>
#include <media/stagefright/foundation/ADebug.h>  // CHECK_XX macro
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaDefs.h>  // for MEDIA_MIMETYPE_xxx
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/DataSource.h>

#include "TimedTextExACTSource.h"
#include "TextDescriptions.h"
#include "SubtitleEncodingConvert.h"
#include <dlfcn.h> 

namespace android {

TimedTextExACTSource::TimedTextExACTSource(const sp<DataSource>& dataSource,const char *mimeType)
    : mSource(dataSource),
      mIndex(0),
      msub_handle(NULL),
      msub_so_handle(NULL),
      msub_interface(NULL),
      mMimeType(strdup(mimeType)),
      mMetaData(new MetaData){
   		if(mMimeType==NULL){
  			ALOGD("###unsupport mimeType### \n");
  			return ;
   		}
   		typedef void*(*FuncPtr)(void);
   		FuncPtr func_handle;
   		msub_so_handle = dlopen("libsub.so", RTLD_NOW);
   		if(msub_so_handle==NULL){
   			ALOGE("In %s dlopen Error\n",__func__);
   			return;
   		}
   		func_handle = (FuncPtr)dlsym(msub_so_handle ,"get_plugin_info");
   		if(func_handle==NULL){
   			ALOGE("In %s dlsym Error\n",__func__);
   			dlclose(msub_so_handle);
   			return;
   		}
   		msub_interface =(subdec_plugin_t*)func_handle();
   		if(msub_interface == NULL) {
   			ALOGE("In %s get func_handle Error\n",__func__);
        dlclose(msub_so_handle);
        msub_so_handle = NULL;
        return;
   		}
   		ALOGD("mMimeType is %s \n",mMimeType);
   		mMetaData->setCString(kKeyMediaLanguage, "und");	
}


TimedTextExACTSource::~TimedTextExACTSource() {
	
		ALOGD("#####TimedTextExACTSource destroyed####\n");
	if(msub_handle != NULL) {
		ALOGD("#####subtitle_close start####\n");
		msub_interface->subtitle_close(msub_handle);
		ALOGD("#####subtitle_close end####\n");
		free(msub_handle);
		msub_handle=NULL;
	}
	if(msub_so_handle!=NULL){
		dlclose(msub_so_handle);
		msub_so_handle = NULL;
	}

	if(mMimeType!=NULL){
		free(mMimeType);
		mMimeType=NULL;
	}
}
status_t TimedTextExACTSource::getNextSubtitleInfo(
            int64_t *startTimeUs, ExActTextInfo *info,int32_t index){
            if(msub_handle!=NULL){
            	ALOGD("get_subtiles \n");
            	mactsub = msub_interface->get_subtitles(msub_handle,index);
            	if(mactsub!=NULL){
            		*startTimeUs =  (int64_t)mactsub->iStartTime*1000;
            		if(mactsub->iEndTime<=mactsub->iStartTime){
            			mactsub->iEndTime = mactsub->iStartTime + 1000;
            		}
            		info->endTimeUs = (int64_t)mactsub->iEndTime*1000;
	              info->textLen = mactsub->iText_Len;
	              info->textbuf = mactsub->pSubtitle_buf;
	              ALOGD("index:%d,textbuf:%x\n",index,info->textbuf);
	              
            	}else{
            		return ERROR_END_OF_STREAM;
            	}
            }
            return OK;
}
status_t TimedTextExACTSource::scanFile(){
   int64_t startTimeUs;
   bool endOfFile = false;
   int32_t Index = 0;
	 while (!endOfFile) {
        ExActTextInfo info;
        status_t err = getNextSubtitleInfo(&startTimeUs, &info,Index);
        switch (err) {
            case OK:
            	  ALOGD("###startTimeUs:%lld,endTimesUs:%lld,index:%d####\n",startTimeUs,info.endTimeUs,Index);
                mTextVector.add(startTimeUs, info);
                Index++;
                break;
            case ERROR_END_OF_STREAM:
                endOfFile = true;
                break;
            default:
                return err;
        }
    }
    if (mTextVector.isEmpty()) {
        return ERROR_MALFORMED;
    }
	return OK;
}
void TimedTextExACTSource::reset() {
    mTextVector.clear();
    mIndex = 0;
}
status_t TimedTextExACTSource::stop(){
	reset();
	return OK;
}

status_t TimedTextExACTSource::start()
{
	status_t err;
	off64_t filelen;
	unsigned char *textbuf = NULL;
	if(msub_handle != NULL){
		msub_interface->subtitle_close(msub_handle);
		free(msub_handle);
	}
 
	msub_handle = (tSubParser*)malloc(sizeof(tSubParser));
	if (msub_handle == NULL)
		return ERROR_MALFORMED;
  
  ALOGD("####subtitle_open start######\n");
  if( mSource->getSize(&filelen)!=OK ){
  	ALOGD("####getSize err####\n");
  }
  if(filelen>0){
  	textbuf=(unsigned char *)malloc(filelen);
  	if(textbuf==NULL){
  		ALOGD("####malloc err,%s######\n",strerror(errno));
  		return ERROR_MALFORMED;
  	}
  	if(mSource->readAt(0,textbuf,filelen)!=filelen){
  		ALOGD("####read sub file error######\n");
		if(textbuf!=NULL){
  			free(textbuf);
  			textbuf = NULL;
  		}
  		return ERROR_MALFORMED;
  	}
  }
	if(msub_interface->subtitle_open(msub_handle,textbuf,(int)filelen,mMimeType) != 0) {
		ALOGD("####subtitle_open ERROR_MALFORMED######\n");
		if(textbuf!=NULL){
  			free(textbuf);
  			textbuf = NULL;
  		}
		return ERROR_MALFORMED;
	}
	ALOGD("####scanFile start######\n");
	err = scanFile();
	ALOGD("####scanFile end######\n");
  if (err != OK) {
  	ALOGD("####scanFile err######\n");
  	reset();
  }
  if(textbuf!=NULL){
  	free(textbuf);
  	textbuf = NULL;
  }
  return OK;
}


status_t TimedTextExACTSource::read(
        int64_t *startTimeUs,
        int64_t *endTimeUs,
        Parcel *parcel,
        const MediaSource::ReadOptions *options) {

	   AString text;
     int encoding;
     char mLocaleEncoding[MAX_SUB_LINE_LEN];
     char textbuf[MAX_SUB_LINE_LEN];
     ALOGV("###read text###\n");
     status_t err = getText(options, &text, startTimeUs, endTimeUs);
     if (err != OK) {
        return err;
     }
     encoding= subEncodingDetect(text.c_str());
     encodingEnumToString(encoding,mLocaleEncoding);
     if (convertValues(text.c_str(),mLocaleEncoding,textbuf) < 0) {
		    return ERROR_MALFORMED;
	   }
	   text = textbuf;
     CHECK_GE(*startTimeUs, 0);
     extractAndAppendLocalDescriptions(*startTimeUs, text, parcel);
     return OK;
}

int TimedTextExACTSource::compareExtendedRangeAndTime(size_t index, int64_t timeUs) {
    CHECK_LT(index, mTextVector.size());
    int64_t endTimeUs = mTextVector.valueAt(index).endTimeUs;
    int64_t startTimeUs = (index > 0) ?
            mTextVector.valueAt(index - 1).endTimeUs : 0;
    if (timeUs >= startTimeUs && timeUs < endTimeUs) {
        return 0;
    } else if (endTimeUs <= timeUs) {
        return -1;
    } else {
        return 1;
    }
}

status_t TimedTextExACTSource::getText(
        const MediaSource::ReadOptions *options,
        AString *text, int64_t *startTimeUs, int64_t *endTimeUs) {
    if (mTextVector.size() == 0) {
        return ERROR_END_OF_STREAM;
    }
    text->clear();
    int64_t seekTimeUs;
    MediaSource::ReadOptions::SeekMode mode;
    if (options != NULL && options->getSeekTo(&seekTimeUs, &mode)) {
        int64_t lastEndTimeUs =
                mTextVector.valueAt(mTextVector.size() - 1).endTimeUs;
        ALOGD("##lastEndTimes:%lld###\n",lastEndTimeUs);
        if (seekTimeUs < 0) {
            return ERROR_OUT_OF_RANGE;
        } else if (seekTimeUs >= lastEndTimeUs) {
            return ERROR_END_OF_STREAM;
        } else {
            // binary search
            size_t low = 0;
            size_t high = mTextVector.size() - 1;
            size_t mid = 0;

            while (low <= high) {
                mid = low + (high - low)/2;
                int diff = compareExtendedRangeAndTime(mid, seekTimeUs);
                if (diff == 0) {
                    break;
                } else if (diff < 0) {
                    low = mid + 1;
                } else {
                    high = mid - 1;
                }
            }
            mIndex = mid;
        }
    }
    ALOGV("mindex:%d,size:%d\n",mIndex,mTextVector.size());
    if (mIndex >= mTextVector.size()) {
        return ERROR_END_OF_STREAM;
    }

    const ExActTextInfo &info = mTextVector.valueAt(mIndex);
    *startTimeUs = mTextVector.keyAt(mIndex);
    *endTimeUs = info.endTimeUs;
    mIndex++;
    ALOGV("****startTimesUs:%lld,endTimeUs:%lld,textLen:%d,textbuf:%s\n",*startTimeUs,*endTimeUs,info.textLen,info.textbuf);
    text->append(info.textbuf,info.textLen);
    return OK;
	
}

status_t TimedTextExACTSource::extractAndAppendLocalDescriptions(
        int64_t timeUs, const AString &text, Parcel *parcel) {
    const void *data = text.c_str();
    size_t size = text.size();
    ALOGV("##size is %d##\n",size);
    int32_t flag = TextDescriptions::LOCAL_DESCRIPTIONS |
                   TextDescriptions::OUT_OF_BAND_TEXT_SRT;

    if (size > 0) {
        return TextDescriptions::getParcelOfDescriptions(
                (const uint8_t *)data, size, flag, timeUs / 1000, parcel);
    }
    return OK;
}

sp<MetaData> TimedTextExACTSource::getFormat() {
	return mMetaData;
}

}  // namespace android
