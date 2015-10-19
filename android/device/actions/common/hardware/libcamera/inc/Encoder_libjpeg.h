/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
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

/**
* @file Encoder_libjpeg.h
*
* This defines API for camerahal to encode YUV using libjpeg
*
*/

#ifndef ANDROID_CAMERA_HARDWARE_ENCODER_LIBJPEG_H
#define ANDROID_CAMERA_HARDWARE_ENCODER_LIBJPEG_H

#include <utils/threads.h>
#include <utils/RefBase.h>

extern "C" {
#include "jhead.h"
}
namespace android
{
/**
 * libjpeg encoder class - uses libjpeg to encode yuv
 */

#define MAX_EXIF_TAGS_SUPPORTED 30

static const char TAG_MODEL[] = "Model";
static const char TAG_MAKE[] = "Make";
static const char TAG_FOCALLENGTH[] = "FocalLength";
static const char TAG_DATETIME[] = "DateTime";
static const char TAG_IMAGE_WIDTH[] = "ImageWidth";
static const char TAG_IMAGE_LENGTH[] = "ImageLength";
static const char TAG_GPS_LAT[] = "GPSLatitude";
static const char TAG_GPS_LAT_REF[] = "GPSLatitudeRef";
static const char TAG_GPS_LONG[] = "GPSLongitude";
static const char TAG_GPS_LONG_REF[] = "GPSLongitudeRef";
static const char TAG_GPS_ALT[] = "GPSAltitude";
static const char TAG_GPS_ALT_REF[] = "GPSAltitudeRef";
static const char TAG_GPS_MAP_DATUM[] = "GPSMapDatum";
static const char TAG_GPS_PROCESSING_METHOD[] = "GPSProcessingMethod";
static const char TAG_GPS_VERSION_ID[] = "GPSVersionID";
static const char TAG_GPS_TIMESTAMP[] = "GPSTimeStamp";
static const char TAG_GPS_DATESTAMP[] = "GPSDateStamp";
static const char TAG_ORIENTATION[] = "Orientation";

class ExifElementsTable
{
public:
    ExifElementsTable() :
        gps_tag_count(0), exif_tag_count(0), position(0),
        jpeg_opened(false), has_datetime_tag(false), has_focal_length_tag(false) {
            //Init table value
			for (int i = 0; i < MAX_EXIF_TAGS_SUPPORTED; i++) {
				memset(&table[i], 0, sizeof(ExifElement_t));
			}
		}
    ~ExifElementsTable();

    status_t insertElement(const char* tag, const char* value);
    void insertExifToJpeg(unsigned char* jpeg, size_t jpeg_size);
    status_t insertExifThumbnailImage(const char*, int);
    void saveJpeg(unsigned char* picture, size_t jpeg_size);
    static const char* degreesToExifOrientation(unsigned int);
    static void stringToRational(const char*, unsigned int*, unsigned int*);
    static bool isAsciiTag(const char* tag);
private:
    ExifElement_t table[MAX_EXIF_TAGS_SUPPORTED];
    unsigned int gps_tag_count;
    unsigned int exif_tag_count;
    unsigned int position;
    bool jpeg_opened;
    bool has_datetime_tag;
    bool has_focal_length_tag;
};

class Encoder_libjpeg: public RefBase 
{
    /* public member types and variables */
public:
    struct params
    {
        uint8_t* src;
        int src_size;
        int src_offset;
        uint8_t* dst;
        int dst_size;
        int quality;
        int left_crop;
        int top_crop;
        int orig_width;
        int orig_height;
        int in_width;
        int in_height;
        int out_width;
        int out_height;
        int start_offset;
        int format;
        size_t jpeg_size;
    };
    /* public member functions */
public:
    Encoder_libjpeg(params* main_jpeg,
                    params* tn_jpeg,
                    void* exif)
        :  mMainInput(main_jpeg), mThumbnailInput(tn_jpeg), 
          mExif(exif),  mThumb(NULL)
    {
    }

    ~Encoder_libjpeg()
    {
    }

    bool encode()
    {
        size_t size = 0;
        sp<Encoder_libjpeg> tn = NULL;
        if (mThumbnailInput)
        {
            // start thread to encode thumbnail
            mThumb = new Encoder_libjpeg(mThumbnailInput, NULL, NULL);
            mThumb->encode();
        }

        size = encode(mMainInput);
        if(size > 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

private:
    params* mMainInput;
    params* mThumbnailInput;
    void* mExif;
    sp<Encoder_libjpeg> mThumb;
    size_t encode(params*);
};

}

#endif
