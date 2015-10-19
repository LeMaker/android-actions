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
* @file OMXExif.cpp
*
* This file contains functionality for handling EXIF insertion.
*
*/

#include "CameraHalDebug.h"

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include <math.h>
#include "OMXVce.h"


namespace android
{

status_t OMXCameraAdapter::setParametersEXIF(const CameraParameters &params,
        BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;
    const char *valstr = NULL;
    double gpsPos;

    LOG_FUNCTION_NAME;

    if( ( valstr = params.get(CameraParameters::KEY_GPS_LATITUDE) ) != NULL )
    {
        gpsPos = strtod(valstr, NULL);

        if ( convertGPSCoord(gpsPos,
                             mEXIFData.mGPSData.mLatDeg,
                             mEXIFData.mGPSData.mLatMin,
                             mEXIFData.mGPSData.mLatSec,
                             mEXIFData.mGPSData.mLatSecDiv ) == NO_ERROR )
        {

            if ( 0 < gpsPos )
            {
                strncpy(mEXIFData.mGPSData.mLatRef, GPS_NORTH_REF, GPS_REF_SIZE);
            }
            else
            {
                strncpy(mEXIFData.mGPSData.mLatRef, GPS_SOUTH_REF, GPS_REF_SIZE);
            }

            mEXIFData.mGPSData.mLatValid = true;
        }
        else
        {
            mEXIFData.mGPSData.mLatValid = false;
        }
    }
    else
    {
        mEXIFData.mGPSData.mLatValid = false;
    }

    if( ( valstr = params.get(CameraParameters::KEY_GPS_LONGITUDE) ) != NULL )
    {
        gpsPos = strtod(valstr, NULL);

        if ( convertGPSCoord(gpsPos,
                             mEXIFData.mGPSData.mLongDeg,
                             mEXIFData.mGPSData.mLongMin,
                             mEXIFData.mGPSData.mLongSec,
                             mEXIFData.mGPSData.mLongSecDiv) == NO_ERROR )
        {

            if ( 0 < gpsPos )
            {
                strncpy(mEXIFData.mGPSData.mLongRef, GPS_EAST_REF, GPS_REF_SIZE);
            }
            else
            {
                strncpy(mEXIFData.mGPSData.mLongRef, GPS_WEST_REF, GPS_REF_SIZE);
            }

            mEXIFData.mGPSData.mLongValid= true;
        }
        else
        {
            mEXIFData.mGPSData.mLongValid = false;
        }
    }
    else
    {
        mEXIFData.mGPSData.mLongValid = false;
    }

    if( ( valstr = params.get(CameraParameters::KEY_GPS_ALTITUDE) ) != NULL )
    {
        gpsPos = strtod(valstr, NULL);
        mEXIFData.mGPSData.mAltitude = floor(fabs(gpsPos));
        if (gpsPos < 0)
        {
            mEXIFData.mGPSData.mAltitudeRef = 1;
        }
        else
        {
            mEXIFData.mGPSData.mAltitudeRef = 0;
        }
        mEXIFData.mGPSData.mAltitudeValid = true;
    }
    else
    {
        mEXIFData.mGPSData.mAltitudeValid= false;
    }

    if( (valstr = params.get(CameraParameters::KEY_GPS_TIMESTAMP)) != NULL )
    {
        long gpsTimestamp = strtol(valstr, NULL, 10);
        struct tm *timeinfo = gmtime( ( time_t * ) & (gpsTimestamp) );
        if ( NULL != timeinfo )
        {
            mEXIFData.mGPSData.mTimeStampHour = timeinfo->tm_hour;
            mEXIFData.mGPSData.mTimeStampMin = timeinfo->tm_min;
            mEXIFData.mGPSData.mTimeStampSec = timeinfo->tm_sec;
            mEXIFData.mGPSData.mTimeStampValid = true;
        }
        else
        {
            mEXIFData.mGPSData.mTimeStampValid = false;
        }
    }
    else
    {
        mEXIFData.mGPSData.mTimeStampValid = false;
    }
    if( ( valstr = params.get(CameraParameters::KEY_GPS_TIMESTAMP) ) != NULL )
    {
        long gpsDatestamp = strtol(valstr, NULL, 10);
        struct tm *timeinfo = gmtime( ( time_t * ) & (gpsDatestamp) );
        if ( NULL != timeinfo )
        {
            strftime(mEXIFData.mGPSData.mDatestamp, GPS_DATESTAMP_SIZE, "%Y:%m:%d", timeinfo);
            mEXIFData.mGPSData.mDatestampValid = true;
        }
        else
        {
            mEXIFData.mGPSData.mDatestampValid = false;
        }
    }
    else
    {
        mEXIFData.mGPSData.mDatestampValid = false;
    }

    if( ( valstr = params.get(CameraParameters::KEY_GPS_PROCESSING_METHOD) ) != NULL )
    {
        strncpy(mEXIFData.mGPSData.mProcMethod, valstr, GPS_PROCESSING_SIZE-1);
        mEXIFData.mGPSData.mProcMethodValid = true;
    }
    else
    {
        mEXIFData.mGPSData.mProcMethodValid = false;
    }

    if( ( valstr = params.get(ActCameraParameters::KEY_GPS_MAPDATUM) ) != NULL )
    {
        strncpy(mEXIFData.mGPSData.mMapDatum, valstr, GPS_MAPDATUM_SIZE-1);
        mEXIFData.mGPSData.mMapDatumValid = true;
    }
    else
    {
        mEXIFData.mGPSData.mMapDatumValid = false;
    }

    if( ( valstr = params.get(ActCameraParameters::KEY_GPS_VERSION) ) != NULL )
    {
        strncpy(mEXIFData.mGPSData.mVersionId, valstr, GPS_VERSION_SIZE-1);
        mEXIFData.mGPSData.mVersionIdValid = true;
    }
    else
    {
        mEXIFData.mGPSData.mVersionIdValid = false;
    }

    if( ( valstr = params.get(ActCameraParameters::KEY_EXIF_MODEL ) ) != NULL )
    {
        CAMHAL_LOGVB("EXIF Model: %s", valstr);
        mEXIFData.mModelValid= true;
    }
    else
    {
        mEXIFData.mModelValid= false;
    }

    if( ( valstr = params.get(ActCameraParameters::KEY_EXIF_MAKE ) ) != NULL )
    {
        CAMHAL_LOGVB("EXIF Make: %s", valstr);
        mEXIFData.mMakeValid = true;
    }
    else
    {
        mEXIFData.mMakeValid= false;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setupEXIF_vce(CameraHalExif *exif)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    struct timeval sTv;
    struct tm *pTime;
    OMXCameraPortParameters * capData = NULL;


    LOG_FUNCTION_NAME;
    memset(exif, 0, sizeof(CameraHalExif));

    capData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    exif->bExifEnable = true;
    if (mEXIFData.mModelValid)
    {
        strlcpy((char *)&(exif->exifmodel), mParams.get(ActCameraParameters::KEY_EXIF_MODEL),EXIF_MODEL_SIZE);
    }

    if (mEXIFData.mMakeValid)
    {
        strlcpy((char *)&(exif->exifmake), mParams.get(ActCameraParameters::KEY_EXIF_MAKE),EXIF_MAKE_SIZE);
    }

    unsigned int numerator = 0, denominator = 0;
    ExifElementsTable::stringToRational(mParams.get(CameraParameters::KEY_FOCAL_LENGTH),
                                        &numerator, &denominator);
    if (numerator || denominator)
    {
        exif->focalLengthH = numerator;
        exif->focalLengthL = denominator;
    }

    if ((NO_ERROR == ret))
    {
        int status = gettimeofday (&sTv, NULL);
        pTime = localtime (&sTv.tv_sec);
        char temp_value[EXIF_DATE_TIME_SIZE + 1];
        if ((0 == status) && (NULL != pTime))
        {
            snprintf(temp_value, EXIF_DATE_TIME_SIZE,
                     "%04d:%02d:%02d %02d:%02d:%02d",
                     pTime->tm_year + 1900,
                     pTime->tm_mon + 1,
                     pTime->tm_mday,
                     pTime->tm_hour,
                     pTime->tm_min,
                     pTime->tm_sec );
            strlcpy((char *)&exif->dateTime, temp_value, EXIF_DATE_TIME_SIZE);
        }
    }

    if ((NO_ERROR == ret))
    {
        exif->width= capData->mEncodeWidth;
        exif->height= capData->mEncodeHeight;
    }

    exif->bGPS = true;
    if (mEXIFData.mGPSData.mLatValid)
    {
        exif->gpsLATH[0] = abs(mEXIFData.mGPSData.mLatDeg);
        exif->gpsLATH[1] = abs(mEXIFData.mGPSData.mLatMin);
        exif->gpsLATH[2] = abs(mEXIFData.mGPSData.mLatSec);

        exif->gpsLATL[0] = 1;
        exif->gpsLATL[1] = 1;
        exif->gpsLATL[2] = abs(mEXIFData.mGPSData.mLatSecDiv);

        exif->gpsLATREF = 0;
        if(strcmp(mEXIFData.mGPSData.mLatRef, GPS_SOUTH_REF) == 0)
        {
            exif->gpsLATREF = 1;
        }
    }
    else
    {
        exif->gpsLATH[0] = 0;
        exif->gpsLATH[1] = 0;
        exif->gpsLATH[2] = 0;

        exif->gpsLATL[0] = 1;
        exif->gpsLATL[1] = 1;
        exif->gpsLATL[2] = 1;

        exif->gpsLATREF = 0;
        exif->bGPS = false;
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mLongValid))
    {
        exif->gpsLONGH[0] = abs(mEXIFData.mGPSData.mLongDeg);
        exif->gpsLONGH[1] = abs(mEXIFData.mGPSData.mLongMin);
        exif->gpsLONGH[2] = abs(mEXIFData.mGPSData.mLongSec);

        exif->gpsLONGL[0] = 1;
        exif->gpsLONGL[1] = 1;
        exif->gpsLONGL[2] = abs(mEXIFData.mGPSData.mLongSecDiv);

        exif->gpsLONGREF = 0;
        if(strcmp(mEXIFData.mGPSData.mLongRef, GPS_WEST_REF) == 0)
        {
            exif->gpsLONGREF = 1;
        }
    }
    else
    {
        exif->gpsLONGH[0] = 0;
        exif->gpsLONGH[1] = 0;
        exif->gpsLONGH[2] = 0;

        exif->gpsLONGL[0] = 1;
        exif->gpsLONGL[1] = 1;
        exif->gpsLONGL[2] = 1;

        exif->gpsLONGREF = 0;

        exif->bGPS = false;
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mAltitudeValid))
    {
        exif->gpsALTH = abs(mEXIFData.mGPSData.mAltitude);
        exif->gpsALTL = 1;
        exif->gpsALTREF = mEXIFData.mGPSData.mAltitudeRef;
    }
    else
    {
        exif->gpsALTH = 0;
        exif->gpsALTL = 1;
        exif->gpsALTREF = 0;
    }

    if (mEXIFData.mGPSData.mProcMethodValid)
    {
        strlcpy((char *)&(exif->gpsProcessMethod),
                mParams.get(CameraParameters::KEY_GPS_PROCESSING_METHOD),
                GPS_PROCESSING_SIZE);
    }
    else
    {   strlcpy((char *)&(exif->gpsProcessMethod),
                "",
                GPS_PROCESSING_SIZE);
    }

    if (mEXIFData.mGPSData.mTimeStampValid)
    {
        exif->gpsTimeH[0] = mEXIFData.mGPSData.mTimeStampHour;
        exif->gpsTimeH[1] = mEXIFData.mGPSData.mTimeStampMin;
        exif->gpsTimeH[2] = mEXIFData.mGPSData.mTimeStampSec;

        exif->gpsTimeL[0] = 1;
        exif->gpsTimeL[1] = 1;
        exif->gpsTimeL[2] = 1;
    }
    else
    {
        exif->gpsTimeH[0] = 0;
        exif->gpsTimeH[1] = 0;
        exif->gpsTimeH[2] = 0;

        exif->gpsTimeL[0] = 1;
        exif->gpsTimeL[1] = 1;
        exif->gpsTimeL[2] = 1;    
    }

    if (mEXIFData.mGPSData.mDatestampValid )
    {
        strlcpy((char *)&(exif->gpsDatestamp), mEXIFData.mGPSData.mDatestamp, GPS_DATESTAMP_SIZE);
    }

    if (mParams.get(CameraParameters::KEY_ROTATION) )
    {
        int exif_orient =
            strtol(mParams.get(CameraParameters::KEY_ROTATION),NULL,10);


        exif->imageOri = exif_orient;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setupEXIF_libjpeg(ExifElementsTable* exifTable)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    struct timeval sTv;
    struct tm *pTime;
    OMXCameraPortParameters * capData = NULL;

    LOG_FUNCTION_NAME;

    capData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    if ((NO_ERROR == ret) && (mEXIFData.mModelValid))
    {
        ret = exifTable->insertElement(TAG_MODEL, mParams.get(ActCameraParameters::KEY_EXIF_MODEL));
    }

    if ((NO_ERROR == ret) && (mEXIFData.mMakeValid))
    {
        ret = exifTable->insertElement(TAG_MAKE, mParams.get(ActCameraParameters::KEY_EXIF_MAKE));
    }

    if ((NO_ERROR == ret))
    {
        unsigned int numerator = 0, denominator = 0;
        ExifElementsTable::stringToRational(mParams.get(CameraParameters::KEY_FOCAL_LENGTH),
                                            &numerator, &denominator);
        if (numerator || denominator)
        {
            char temp_value[256]; // arbitrarily long string
            snprintf(temp_value,
                     sizeof(temp_value)/sizeof(char),
                     "%u/%u", numerator, denominator);
            ret = exifTable->insertElement(TAG_FOCALLENGTH, temp_value);

        }
    }

    if ((NO_ERROR == ret))
    {
        int status = gettimeofday (&sTv, NULL);
        pTime = localtime (&sTv.tv_sec);
        char temp_value[EXIF_DATE_TIME_SIZE + 1];
        if ((0 == status) && (NULL != pTime))
        {
            snprintf(temp_value, EXIF_DATE_TIME_SIZE,
                     "%04d:%02d:%02d %02d:%02d:%02d",
                     pTime->tm_year + 1900,
                     pTime->tm_mon + 1,
                     pTime->tm_mday,
                     pTime->tm_hour,
                     pTime->tm_min,
                     pTime->tm_sec );

            ret = exifTable->insertElement(TAG_DATETIME, temp_value);
        }
    }

    if ((NO_ERROR == ret))
    {
        char temp_value[5];
        snprintf(temp_value, sizeof(temp_value)/sizeof(char), "%lu", capData->mEncodeWidth);
        ret = exifTable->insertElement(TAG_IMAGE_WIDTH, temp_value);
    }

    if ((NO_ERROR == ret))
    {
        char temp_value[5];
        snprintf(temp_value, sizeof(temp_value)/sizeof(char), "%lu", capData->mEncodeHeight);
        ret = exifTable->insertElement(TAG_IMAGE_LENGTH, temp_value);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mLatValid))
    {
        char temp_value[256]; // arbitrarily long string
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char) - 1,
                 "%d/%d,%d/%d,%d/%d",
                 abs(mEXIFData.mGPSData.mLatDeg), 1,
                 abs(mEXIFData.mGPSData.mLatMin), 1,
                 abs(mEXIFData.mGPSData.mLatSec), abs(mEXIFData.mGPSData.mLatSecDiv));
        ret = exifTable->insertElement(TAG_GPS_LAT, temp_value);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mLatValid))
    {
        ret = exifTable->insertElement(TAG_GPS_LAT_REF, mEXIFData.mGPSData.mLatRef);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mLongValid))
    {
        char temp_value[256]; // arbitrarily long string
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char) - 1,
                 "%d/%d,%d/%d,%d/%d",
                 abs(mEXIFData.mGPSData.mLongDeg), 1,
                 abs(mEXIFData.mGPSData.mLongMin), 1,
                 abs(mEXIFData.mGPSData.mLongSec), abs(mEXIFData.mGPSData.mLongSecDiv));
        ret = exifTable->insertElement(TAG_GPS_LONG, temp_value);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mLongValid))
    {
        ret = exifTable->insertElement(TAG_GPS_LONG_REF, mEXIFData.mGPSData.mLongRef);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mAltitudeValid))
    {
        char temp_value[256]; // arbitrarily long string
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char) - 1,
                 "%d/%d",
                 abs( mEXIFData.mGPSData.mAltitude), 1);
        ret = exifTable->insertElement(TAG_GPS_ALT, temp_value);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mAltitudeValid))
    {
        char temp_value[5];
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char) - 1,
                 "%d", mEXIFData.mGPSData.mAltitudeRef);
        ret = exifTable->insertElement(TAG_GPS_ALT_REF, temp_value);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mMapDatumValid))
    {
        ret = exifTable->insertElement(TAG_GPS_MAP_DATUM, mEXIFData.mGPSData.mMapDatum);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mProcMethodValid))
    {
        char temp_value[GPS_PROCESSING_SIZE];

        memcpy(temp_value, ExifAsciiPrefix, sizeof(ExifAsciiPrefix));
        memcpy(temp_value + sizeof(ExifAsciiPrefix),
               mParams.get(CameraParameters::KEY_GPS_PROCESSING_METHOD),
               (GPS_PROCESSING_SIZE - sizeof(ExifAsciiPrefix)));
        ret = exifTable->insertElement(TAG_GPS_PROCESSING_METHOD, temp_value);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mVersionIdValid))
    {
        char temp_value[256]; // arbitrarily long string
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char) - 1,
                 "%d,%d,%d,%d",
                 mEXIFData.mGPSData.mVersionId[0],
                 mEXIFData.mGPSData.mVersionId[1],
                 mEXIFData.mGPSData.mVersionId[2],
                 mEXIFData.mGPSData.mVersionId[3]);
        ret = exifTable->insertElement(TAG_GPS_VERSION_ID, temp_value);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mTimeStampValid))
    {
        char temp_value[256]; // arbitrarily long string
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char) - 1,
                 "%d/%d,%d/%d,%d/%d",
                 mEXIFData.mGPSData.mTimeStampHour, 1,
                 mEXIFData.mGPSData.mTimeStampMin, 1,
                 mEXIFData.mGPSData.mTimeStampSec, 1);
        ret = exifTable->insertElement(TAG_GPS_TIMESTAMP, temp_value);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mDatestampValid) )
    {
        ret = exifTable->insertElement(TAG_GPS_DATESTAMP, mEXIFData.mGPSData.mDatestamp);
    }

    if ((NO_ERROR == ret) && mParams.get(CameraParameters::KEY_ROTATION) )
    {
        const char* exif_orient =
            ExifElementsTable::degreesToExifOrientation(mParams.getInt(CameraParameters::KEY_ROTATION));

        if (exif_orient)
        {
            ret = exifTable->insertElement(TAG_ORIENTATION, exif_orient);
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::convertGPSCoord(double coord,
        int &deg,
        int &min,
        int &sec,
        int &secDivisor)
{
    double tmp;

    LOG_FUNCTION_NAME;

    if ( coord == 0 )
    {

        ALOGE("Invalid GPS coordinate");

        return -EINVAL;
    }

    deg = (int) floor(fabs(coord));
    tmp = ( fabs(coord) - floor(fabs(coord)) ) * GPS_MIN_DIV;
    min = (int) floor(tmp);
    tmp = ( tmp - floor(tmp) ) * ( GPS_SEC_DIV * GPS_SEC_ACCURACY );
    sec = (int) floor(tmp);
    secDivisor = GPS_SEC_ACCURACY;

    if( sec >= ( GPS_SEC_DIV * GPS_SEC_ACCURACY ) )
    {
        sec = 0;
        min += 1;
    }

    if( min >= 60 )
    {
        min = 0;
        deg += 1;
    }

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}


status_t OMXCameraAdapter::allocExifObject(void *key, void **exif)
{
    LOG_FUNCTION_NAME;
    void *p;
    status_t ret = NO_ERROR;

    p = new CameraHalExif();


    mExifQueue.add(key, p);
    *exif = p;
    LOG_FUNCTION_NAME_EXIT;
    return ret;

}

status_t OMXCameraAdapter::getExif(void *key,void **exif, int *type)
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;

    freeExifObject(key);

    Mutex::Autolock lock(mExifLock);

    allocExifObject(key,exif);

    setupEXIF_vce(reinterpret_cast<CameraHalExif*>(*exif));
    *type = CameraFrame::HAS_EXIF_DATA_VCE;

    LOG_FUNCTION_NAME_EXIT;
    return ret;

}

status_t OMXCameraAdapter::freeExifObject(void *key)
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;
    void *exif;
    ssize_t index;

    Mutex::Autolock lock(mExifLock);

    index = mExifQueue.indexOfKey(key);
    if(index < 0)
    {
        return UNKNOWN_ERROR;
    }
    exif = mExifQueue.valueAt(index);
    delete reinterpret_cast<CameraHalExif*>( exif);
    mExifQueue.removeItemsAt(index,1);
    LOG_FUNCTION_NAME_EXIT;
    return ret;

}

status_t OMXCameraAdapter::freeExifObjects()
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;
    uint32_t i;
    void *exif;
    Mutex::Autolock lock(mExifLock);

    for(i = 0; i< mExifQueue.size(); i++)
    {
        exif = mExifQueue.valueAt(i);
        delete reinterpret_cast<CameraHalExif*>( exif);
    }
    mExifQueue.clear();
    LOG_FUNCTION_NAME_EXIT;
    return ret;

}

};
