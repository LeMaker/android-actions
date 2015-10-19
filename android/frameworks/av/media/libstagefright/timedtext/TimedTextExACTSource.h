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

#ifndef TIMED_TEXT_EXACT_SOURCE_H_
#define TIMED_TEXT_EXACT_SOURCE_H_

#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>

#include "TimedTextSource.h"

//bruce add
#include "subtitles_plugin.h"

namespace android {

class AString;
class DataSource;
class MediaBuffer;
class Parcel;


class TimedTextExACTSource : public TimedTextSource {
public:
	  TimedTextExACTSource(const sp<DataSource>& dataSource,const char *mimeType);
    virtual status_t start();
    virtual status_t stop();
    virtual status_t read(
            int64_t *startTimeUs,
            int64_t *endTimeUs,
            Parcel *parcel,
            const MediaSource::ReadOptions *options = NULL);
    //virtual status_t extractGlobalDescriptions(Parcel *parcel);
    virtual sp<MetaData> getFormat();

protected:
    virtual ~TimedTextExACTSource();

private:
  struct ExActTextInfo {
        int64_t endTimeUs;
        // The offset of the text in the original file.
        char *textbuf;
        int textLen;
  };
  
	sp<MetaData> mMetaData;
  sp<DataSource> mSource;

	int32_t mIndex;
	char*mMimeType;
	tSub *mactsub;
	void *msub_handle;
	void *msub_initBuf;
	
	void *msub_so_handle;
	subdec_plugin_t *msub_interface; 

  void reset();
  status_t scanFile();
  
  status_t getText(
            const MediaSource::ReadOptions *options,
            AString *text, int64_t *startTimeUs, int64_t *endTimeUs);
	
	status_t extractAndAppendLocalDescriptions(
			int64_t timeUs, const AString &text, Parcel *parcel);
  status_t getNextSubtitleInfo(
           int64_t *startTimeUs, ExActTextInfo *info,int32_t index);
  int compareExtendedRangeAndTime(size_t index, int64_t timeUs);
  

    
  KeyedVector<int64_t, ExActTextInfo> mTextVector;
	
  DISALLOW_EVIL_CONSTRUCTORS(TimedTextExACTSource);
};

}  // namespace android

#endif  // TIMED_TEXT_3GPP_SOURCE_H_
