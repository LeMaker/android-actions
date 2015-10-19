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

#ifndef ActAudio_EXTRACTOR_H_
#define ActAudio_EXTRACTOR_H_

#include <media/stagefright/MediaExtractor.h>
#include "actal_posix_dev.h"
#include "format_dev.h"
#include "id3parse.h"
#include "ActDataSource.h"
#include "music_parser_lib_dev.h"

namespace android {

struct AMessage;
class String8;

class ActAudioExtractor : public MediaExtractor {
public:
    ActAudioExtractor(const sp<DataSource>& source, const char *mime);

    virtual size_t countTracks();
    virtual sp<MediaSource> getTrack(size_t index);
    virtual sp<MetaData> getTrackMetaData(size_t index, uint32_t flags);

    virtual sp<MetaData> getMetaData();

protected:
    virtual ~ActAudioExtractor();

private:
    friend class ActAudioSource;

    sp<DataSource> mDataSource;
    sp<MetaData> mMeta;
    status_t mInitCheck;
    size_t mFrameSize;

    void * mLib_handle;
    void * mPlugin_handle;
    storage_io_t  *mInput;
    music_parser_plugin_t *mPlugin_info; /* 解码插件信息 */
	music_info_t m_ai;

    ActAudioExtractor(const ActAudioExtractor &);
    ActAudioExtractor &operator=(const ActAudioExtractor &);
};

//bool SniffActAudio(
//        const sp<DataSource> &source, String8 *mimeType, float *confidence,
//        sp<AMessage> *);
}  // namespace android

#endif  // AMR_EXTRACTOR_H_
