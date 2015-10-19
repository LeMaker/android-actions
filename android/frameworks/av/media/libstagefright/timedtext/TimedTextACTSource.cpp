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
#define LOG_TAG "TimedTextACTSource"
#include <utils/Log.h>

#include <binder/Parcel.h>
#include <media/stagefright/foundation/ADebug.h>  // CHECK_XX macro
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaDefs.h>  // for MEDIA_MIMETYPE_xxx
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>

#include "TimedTextACTSource.h"
#include "TextDescriptions.h"
#include "SubtitleEncodingConvert.h"
#include <dlfcn.h> 

namespace android {

TimedTextACTSource::TimedTextACTSource(const sp<MediaSource>& mediaSource)
    : mSource(mediaSource),
      minitsubflag(false),
      mLastStartTimeMs(-1),
      mLastEndTimeMs(-1),
      mactsub(NULL),
      msub_so_handle(NULL),
      msub_interface(NULL),
      msub_initBuf(NULL),
      msub_handle(NULL){
      ALOGD("####TimedTextACTSource create####\n");
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
      
}

TimedTextACTSource::~TimedTextACTSource() {
	if (minitsubflag){
		minitsubflag=false;
	}
	if(msub_handle != NULL) {
		msub_interface->subtitle_close_buf(msub_handle);
		free(msub_handle);
		msub_handle=0;
	}
	if(msub_initBuf != NULL) {
		free(msub_initBuf);
		msub_initBuf=0;
	}
	if(msub_so_handle!=NULL){
		dlclose(msub_so_handle);
		msub_so_handle = NULL;
	}

}
status_t TimedTextACTSource::read(
        int64_t *startTimeUs,
        int64_t *endTimeUs,
        Parcel *parcel,
        const MediaSource::ReadOptions *options) {

	  AString text;
	  int encoding;
    char mLocaleEncoding[MAX_SUB_LINE_LEN];
    char textbuf[MAX_SUB_LINE_LEN];
    status_t err = getText(options, &text, startTimeUs, endTimeUs);
    if (err != OK) {
    	  ALOGD("###TimedTextACTSource read err###\n");
        return err;
    }
    encoding= subEncodingDetect(text.c_str());
    encodingEnumToString(encoding,mLocaleEncoding);
    if (convertValues(text.c_str(),mLocaleEncoding,textbuf) < 0) {
		    return ERROR_MALFORMED;
	  }
	  text = textbuf;
   
    CHECK_GE(*startTimeUs, 0);
    ALOGD("###extractAndAppendLocalDescriptions,sub is %s,startTimeUs is %lld,endTimeUs is %lld###\n",text.c_str(),*startTimeUs,*endTimeUs);
    extractAndAppendLocalDescriptions(*startTimeUs, text, parcel);
    return OK;
}

#if 0
text.clear();

text.append(mactsub.pSubtitle_buf, mactsub.iText_Len);

*startTimeUs = text.iStartTime;
*endTimeUs	 = text.iEndTime;


/*decode a subtitle*/ 
tSub *decode_subtitles_buf(void *sub_handle,char *buf);

/*initial*/
int subtitle_open_buf(void *sub_handle,char *buf);

/*dispose*/
void *subtitle_close_buf(void *sub_handle);

#endif



status_t TimedTextACTSource::getText(
        const MediaSource::ReadOptions *options,
        AString *text, int64_t *startTimeUs, int64_t *endTimeUs) {

	status_t err;
	MediaBuffer *srcBuffer;
  int64_t seekTimeUs;
  MediaSource::ReadOptions::SeekMode mode;
  bool seekto= false;

	text->clear();
	
  if (options != NULL && options->getSeekTo(&seekTimeUs, &mode) && mactsub !=NULL){
  	  mLastStartTimeMs = -1;
  	  ALOGD("####text seek#####\n");
  	  seekto = true;
  	  mactsub = NULL;
  }
  if(mactsub!=NULL){
  	if(mactsub->pSubtitle_buf !=NULL  && mactsub->iText_Len>0){
			text->append(mactsub->pSubtitle_buf, mactsub->iText_Len);
		}
  }
	

READ_AGAIN:
	if(seekto==true){
		err = mSource->read(&srcBuffer,options);
		seekto = false;
	}else{
		err = mSource->read(&srcBuffer);
	}
	
 ALOGD("%s,pktlen:%d\n",__FUNCTION__,srcBuffer->size());

	if(srcBuffer==0) {
		ALOGD("%s,%d,\n",__FUNCTION__,__LINE__,srcBuffer->size());
		//return ERROR_MALFORMED;
		if(err == ERROR_END_OF_STREAM)
			return OK;
		return ERROR_MALFORMED;
	}

	if (srcBuffer->size() == 0){
		ALOGD("%s,%d,\n",__FUNCTION__,__LINE__);
		srcBuffer->release();
		return WOULD_BLOCK;
	}


//	srcBuffer->release();
//	return OK;

 ALOGD("%s,%d\n",__FUNCTION__,__LINE__);
	int32_t isInit;
	srcBuffer->meta_data()->findInt32(kKeyIsCodecConfig, &isInit);

	if (isInit == 1){ //(minitsubflag == false){
		ALOGD("subtitle Decoder init %s,%d,\n",__FUNCTION__,__LINE__);
		{
			packet_header_t *pkt = (packet_header_t *)(srcBuffer->data());
			unsigned char *tp = (unsigned char *)(srcBuffer->data());
			ALOGD("subtitle Decoder init data:%x,%x,%x,%x %x %x,\n",tp[0], tp[1],tp[2],tp[3],pkt->header_type,pkt->block_len);

		}

		if(msub_handle != NULL){
			free(msub_handle);
		}
		msub_handle = (tSubParser*)malloc(sizeof(tSubParser));
	  if(msub_handle == NULL){
		    return ERROR_MALFORMED;
		}


		int rt = msub_interface->subtitle_open_buf(msub_handle, (char *)(srcBuffer->data()));
		srcBuffer->release();
		if(rt!=0){
			ALOGD("%s,%d,\n",__FUNCTION__,__LINE__);
			return ERROR_MALFORMED;
		}

		minitsubflag = true;


		goto READ_AGAIN;
	}
	
	

	if(0)
	{
		ALOGE("decode_subtitles_buf is called", decode_subtitles_buf);
		unsigned char *tp = (unsigned char *)(srcBuffer->data());
		ALOGD("subtext:%x,%x,%x,%x size:%d,\n",tp[0], tp[1],tp[2],tp[3], srcBuffer->size());
		int len = srcBuffer->size() - 28;
		int ctr=0;
		while(len-->0){
			ALOGW("0x%x ", tp[28]);
			tp++;
			if(ctr++>30) break;
		}
	}

  ALOGD("%s,%d\n",__FUNCTION__,__LINE__);
	mactsub = msub_interface->decode_subtitles_buf(msub_handle, (char *)(srcBuffer->data()));
  mactsub->iText_Len = strlen(mactsub->pSubtitle_buf);

	ALOGV("iStartTime: %d, iEndTime: %d\n", mactsub->iStartTime, mactsub->iEndTime);
	ALOGV("pSubtitle_buf: 0x%x, len:%d %d\n", mactsub->pSubtitle_buf, mactsub->iText_Len, \
				strlen(mactsub->pSubtitle_buf));
	ALOGV("srcBuffer->realease : %s\n", mactsub->pSubtitle_buf);

	srcBuffer->release();

	if (mactsub == NULL)
		return ERROR_MALFORMED;
		//return ERROR_MALFORMED;

	if (mactsub->iSubtitleType == SUB_IMAGE){
		return ERROR_MALFORMED;
	}
		
	
	
	

  if(mactsub->iStartTime==mLastStartTimeMs || mLastStartTimeMs==-1){
  	mLastStartTimeMs = mactsub->iStartTime;
  	mLastEndTimeMs = mactsub->iEndTime;
  	text->append(mactsub->pSubtitle_buf, mactsub->iText_Len);
  	goto READ_AGAIN;
  }
  *startTimeUs = (int64_t)mLastStartTimeMs*1000;
	*endTimeUs	 = (int64_t)mLastEndTimeMs*1000;
  mLastStartTimeMs = mactsub->iStartTime;
  mLastEndTimeMs = mactsub->iEndTime;
	
    return OK;	
}

status_t TimedTextACTSource::extractAndAppendLocalDescriptions(
        int64_t timeUs, const AString &text, Parcel *parcel) {
    const void *data = text.c_str();
    size_t size = text.size();
    int32_t flag = TextDescriptions::LOCAL_DESCRIPTIONS |
                   TextDescriptions::OUT_OF_BAND_TEXT_SRT;

    if (size > 0) {
        return TextDescriptions::getParcelOfDescriptions(
                (const uint8_t *)data, size, flag, timeUs / 1000, parcel);
    }
    return OK;
}

sp<MetaData> TimedTextACTSource::getFormat() {
    return mSource->getFormat();
}


}  // namespace android
