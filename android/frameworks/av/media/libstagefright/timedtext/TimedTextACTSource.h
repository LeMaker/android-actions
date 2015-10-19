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

#ifndef TIMED_TEXT_ACT_SOURCE_H_
#define TIMED_TEXT_ACT_SOURCE_H_

#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>

#include "TimedTextSource.h"

//bruce add
#include "subtitles_plugin.h"
#define MAX_SUBTITLE_DATA_LEN 2048
namespace android {
 
class MediaBuffer;
class Parcel;

class TimedTextACTSource : public TimedTextSource {
public:
    TimedTextACTSource(const sp<MediaSource>& mediaSource);
    virtual status_t start() { return mSource->start(); }
    virtual status_t stop() { return mSource->stop(); }
    virtual status_t read(
            int64_t *startTimeUs,
            int64_t *endTimeUs,
            Parcel *parcel,
            const MediaSource::ReadOptions *options = NULL);
    //virtual status_t extractGlobalDescriptions(Parcel *parcel);
    virtual sp<MetaData> getFormat();

protected:
    virtual ~TimedTextACTSource();

private:

	tSub *mactsub;
	bool minitsubflag;
	
	int32_t mLastStartTimeMs;
	int32_t mLastEndTimeMs;
	char pSubtitle_buf[MAX_SUBTITLE_DATA_LEN]; 
	
  sp<MediaSource> mSource;

	void *msub_handle;
	void *msub_initBuf;
	
	void *msub_so_handle;
	subdec_plugin_t *msub_interface; 

  status_t getText(
            const MediaSource::ReadOptions *options,
            AString *text, int64_t *startTimeUs, int64_t *endTimeUs);
	
	status_t extractAndAppendLocalDescriptions(
			int64_t timeUs, const AString &text, Parcel *parcel);
	
    DISALLOW_EVIL_CONSTRUCTORS(TimedTextACTSource);
};

}  // namespace android

#endif  // TIMED_TEXT_3GPP_SOURCE_H_
