/*
 * Copyright (C) 2010 The Android Open Source Project
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

#include "math.h"

#include <system/audio.h>

//-----------------------------------------------------------------------------
// Android to OpenSL ES
//----------------------
static inline SLuint32 android_to_sles_streamType(int type) {
    return (SLuint32) type;
}


static inline SLuint32 android_to_sles_sampleRate(uint32_t srHz) {
    // convert to milliHertz
    return (SLuint32) srHz*1000;
}


//-----------------------------------------------------------------------------
// OpenSL ES to Android
//----------------------
static inline int sles_to_android_streamType(SLuint32 type) {
    return (int)type;
}


static inline uint32_t sles_to_android_sampleRate(SLuint32 sampleRateMilliHertz) {
    return (uint32_t)(sampleRateMilliHertz / 1000);
}

static inline audio_format_t sles_to_android_sampleFormat(const SLDataFormat_PCM *df_pcm) {
    switch (df_pcm->formatType) {
    case SL_DATAFORMAT_PCM:
        switch (df_pcm->containerSize) {
            case 8:
                return AUDIO_FORMAT_PCM_8_BIT;
            case 16:
                return AUDIO_FORMAT_PCM_16_BIT;
            case 24:
                return AUDIO_FORMAT_PCM_24_BIT_PACKED;
            case 32:
                return AUDIO_FORMAT_PCM_32_BIT;
            default:
                return AUDIO_FORMAT_INVALID;
        }
    case SL_ANDROID_DATAFORMAT_PCM_EX:
        switch (((SLAndroidDataFormat_PCM_EX *)df_pcm)->representation) {
        case SL_ANDROID_PCM_REPRESENTATION_UNSIGNED_INT:
            switch (df_pcm->containerSize) {
            case 8:
                return AUDIO_FORMAT_PCM_8_BIT;
            default:
                return AUDIO_FORMAT_INVALID;
            }
        case SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT:
            switch (df_pcm->containerSize) {
            case 16:
                return AUDIO_FORMAT_PCM_16_BIT;
            case 24:
                return AUDIO_FORMAT_PCM_24_BIT_PACKED;
            case 32:
                return AUDIO_FORMAT_PCM_32_BIT;
            default:
                return AUDIO_FORMAT_INVALID;
            }
        case SL_ANDROID_PCM_REPRESENTATION_FLOAT:
            switch (df_pcm->containerSize) {
            case 32:
                return AUDIO_FORMAT_PCM_FLOAT;
            default:
                return AUDIO_FORMAT_INVALID;
            }
        default:
            return AUDIO_FORMAT_INVALID;
        }
    default:
        return AUDIO_FORMAT_INVALID;
    }
}


static inline audio_channel_mask_t sles_to_android_channelMaskIn(SLuint32 nbChannels,
        SLuint32 channelMask) {
    // FIXME handle channel mask mapping between SL ES and Android
    return audio_channel_in_mask_from_count(nbChannels);
}


static inline audio_channel_mask_t sles_to_android_channelMaskOut(SLuint32 nbChannels,
        SLuint32 channelMask) {
    // FIXME handle channel mask mapping between SL ES and Android
    return audio_channel_out_mask_from_count(nbChannels);
}


static inline float sles_to_android_amplification(SLmillibel level) {
    // FIXME use the FX Framework conversions
    return pow(10, (float)level/2000);
}


static inline uint32_t channelCountToMask(uint32_t channelCount)
{
    // FIXME channel mask is not yet implemented by Stagefright, so use a reasonable default
    //       that is computed from the channel count
    uint32_t channelMask;
    switch (channelCount) {
    case 1:
        // see explanation in data.c re: default channel mask for mono
        return SL_SPEAKER_FRONT_LEFT;
    case 2:
        return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    // Android-specific
    case 4:
        return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT
               | SL_SPEAKER_BACK_LEFT | SL_SPEAKER_BACK_RIGHT;
    case 6:
        return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT| SL_SPEAKER_FRONT_CENTER
               | SL_SPEAKER_LOW_FREQUENCY
               | SL_SPEAKER_BACK_LEFT | SL_SPEAKER_BACK_RIGHT;
    case 8:
        return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT | SL_SPEAKER_FRONT_CENTER
               | SL_SPEAKER_LOW_FREQUENCY
               | SL_SPEAKER_BACK_LEFT | SL_SPEAKER_BACK_RIGHT
               | SL_SPEAKER_SIDE_LEFT |SL_SPEAKER_SIDE_RIGHT;
    default:
        return UNKNOWN_CHANNELMASK;
    }
}
