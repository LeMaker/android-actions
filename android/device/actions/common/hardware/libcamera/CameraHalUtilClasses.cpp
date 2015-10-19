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
* @file CameraHalUtilClasses.cpp
*
* This file maps the CameraHardwareInterface to the Camera interfaces on OMAP4 (mainly OMX).
*
*/


#include "CameraHalDebug.h"

#include "CameraHal.h"
#include "Resize.h"

namespace android
{

/*--------------------FrameProvider Class STARTS here-----------------------------*/

int FrameProvider::enableFrameNotification(int32_t frameTypes)
{
    LOG_FUNCTION_NAME;
    status_t ret = NO_ERROR;

    ///Enable the frame notification to CameraAdapter (which implements FrameNotifier interface)
    mFrameNotifier->enableFrameMsgType(frameTypes<<FrameNotifier::FRAME_BIT_FIELD_POSITION
                                  , mFrameCallback
                                  , mCookie
                                 );

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

int FrameProvider::disableFrameNotification(int32_t frameTypes)
{
    LOG_FUNCTION_NAME;
    status_t ret = NO_ERROR;

    mFrameNotifier->disableFrameMsgType(frameTypes<<FrameNotifier::FRAME_BIT_FIELD_POSITION
                                   , mCookie
                                  );

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

int FrameProvider::returnFrame(void *frameBuf, CameraFrame::FrameType frameType, CameraFrame::StreamType streamType)
{
    status_t ret = NO_ERROR;

    mFrameNotifier->returnFrame(frameBuf, frameType, streamType);

    return ret;
}

/*--------------------FrameProvider Class ENDS here-----------------------------*/

/*--------------------EventProvider Class STARTS here-----------------------------*/

int EventProvider::enableEventNotification(int32_t frameTypes)
{
    LOG_FUNCTION_NAME;
    status_t ret = NO_ERROR;

    ///Enable the frame notification to CameraAdapter (which implements FrameNotifier interface)
    mEventNotifier->enableMsgType(frameTypes<<MessageNotifier::EVENT_BIT_FIELD_POSITION
                                  , mEventCallback
                                  , mCookie
                                 );

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

int EventProvider::disableEventNotification(int32_t frameTypes)
{
    LOG_FUNCTION_NAME;
    status_t ret = NO_ERROR;

    mEventNotifier->disableMsgType(frameTypes<<FrameNotifier::FRAME_BIT_FIELD_POSITION
                                   , mCookie
                                  );

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

/*--------------------EventProvider Class ENDS here-----------------------------*/

/*--------------------CameraArea Class STARTS here-----------------------------*/

status_t CameraArea::transfrom(size_t width,
                               size_t height,
                               size_t &top,
                               size_t &left,
                               size_t &areaWidth,
                               size_t &areaHeight)
{
    status_t ret = NO_ERROR;
    size_t hRange, vRange;
    double hScale, vScale;

    LOG_FUNCTION_NAME

    hRange = CameraArea::RIGHT - CameraArea::LEFT;
    vRange = CameraArea::BOTTOM - CameraArea::TOP;
    hScale = ( double ) width / ( double ) hRange;
    vScale = ( double ) height / ( double ) vRange;

    top = ( mTop + vRange / 2 ) * vScale;
    left = ( mLeft + hRange / 2 ) * hScale;
    areaHeight = ( mBottom + vRange / 2 ) * vScale;
    areaHeight -= top;
    areaWidth = ( mRight + hRange / 2) * hScale;
    areaWidth -= left;

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t CameraArea::checkArea(ssize_t top,
                               ssize_t left,
                               ssize_t bottom,
                               ssize_t right,
                               ssize_t weight)
{

    //Handles the invalid regin corner case.
    if ( ( 0 == top ) && ( 0 == left ) && ( 0 == bottom ) && ( 0 == right ) && ( 0 == weight ) )
    {
        return NO_ERROR;
    }

    if ( ( CameraArea::WEIGHT_MIN > weight ) ||  ( CameraArea::WEIGHT_MAX < weight ) )
    {
        CAMHAL_LOGEB("Camera area weight is invalid %d", static_cast<int>(weight));
        return -EINVAL;
    }

    if ( ( CameraArea::TOP > top ) || ( CameraArea::BOTTOM < top ) )
    {
        CAMHAL_LOGEB("Camera area top coordinate is invalid %d", static_cast<int>(top) );
        return -EINVAL;
    }

    if ( ( CameraArea::TOP > bottom ) || ( CameraArea::BOTTOM < bottom ) )
    {
        CAMHAL_LOGEB("Camera area bottom coordinate is invalid %d", static_cast<int>(bottom) );
        return -EINVAL;
    }

    if ( ( CameraArea::LEFT > left ) || ( CameraArea::RIGHT < left ) )
    {
        CAMHAL_LOGEB("Camera area left coordinate is invalid %d", static_cast<int>(left) );
        return -EINVAL;
    }

    if ( ( CameraArea::LEFT > right ) || ( CameraArea::RIGHT < right ) )
    {
        CAMHAL_LOGEB("Camera area right coordinate is invalid %d", static_cast<int>(right) );
        return -EINVAL;
    }

    if ( left >= right )
    {
        CAMHAL_LOGEA("Camera area left larger than right");
        return -EINVAL;
    }

    if ( top >= bottom )
    {
        CAMHAL_LOGEA("Camera area top larger than bottom");
        return -EINVAL;
    }

    return NO_ERROR;
}

status_t CameraArea::parseAreas(const char *area,
                                size_t areaLength,
                                Vector< sp<CameraArea> > &areas)
{
    status_t ret = NO_ERROR;
    char *ctx;
    char *pArea = NULL;
    char *pStart = NULL;
    char *pEnd = NULL;
    const char *startToken = "(";
    const char endToken = ')';
    const char sep = ',';
    ssize_t top, left, bottom, right, weight;
    char *tmpBuffer = NULL;
    sp<CameraArea> currentArea;

    LOG_FUNCTION_NAME

    if ( ( NULL == area ) ||
            ( 0 >= areaLength ) )
    {
        return -EINVAL;
    }

    tmpBuffer = ( char * ) malloc(areaLength);
    if ( NULL == tmpBuffer )
    {
        return -ENOMEM;
    }

    memcpy(tmpBuffer, area, areaLength);

    pArea = strtok_r(tmpBuffer, startToken, &ctx);

    do
    {

        pStart = pArea;
        if ( NULL == pStart )
        {
            CAMHAL_LOGEA("Parsing of the left area coordinate failed!");
            ret = -EINVAL;
            break;
        }
        else
        {
            left = static_cast<ssize_t>(strtol(pStart, &pEnd, 10));
        }

        if ( sep != *pEnd )
        {
            CAMHAL_LOGEA("Parsing of the top area coordinate failed!");
            ret = -EINVAL;
            break;
        }
        else
        {
            top = static_cast<ssize_t>(strtol(pEnd+1, &pEnd, 10));
        }

        if ( sep != *pEnd )
        {
            CAMHAL_LOGEA("Parsing of the right area coordinate failed!");
            ret = -EINVAL;
            break;
        }
        else
        {
            right = static_cast<ssize_t>(strtol(pEnd+1, &pEnd, 10));
        }

        if ( sep != *pEnd )
        {
            CAMHAL_LOGEA("Parsing of the bottom area coordinate failed!");
            ret = -EINVAL;
            break;
        }
        else
        {
            bottom = static_cast<ssize_t>(strtol(pEnd+1, &pEnd, 10));
        }

        if ( sep != *pEnd )
        {
            CAMHAL_LOGEA("Parsing of the weight area coordinate failed!");
            ret = -EINVAL;
            break;
        }
        else
        {
            weight = static_cast<ssize_t>(strtol(pEnd+1, &pEnd, 10));
        }

        if ( endToken != *pEnd )
        {
            CAMHAL_LOGEA("Malformed area!");
            ret = -EINVAL;
            break;
        }

        ret = checkArea(top, left, bottom, right, weight);
        if ( NO_ERROR != ret )
        {
            break;
        }

        currentArea = new CameraArea(top, left, bottom, right, weight);
        CAMHAL_LOGDB("Area parsed [%dx%d, %dx%d] %d",
                     ( int ) top,
                     ( int ) left,
                     ( int ) bottom,
                     ( int ) right,
                     ( int ) weight);
        if ( NULL != currentArea.get() )
        {
            areas.add(currentArea);
        }
        else
        {
            ret = -ENOMEM;
            break;
        }

        pArea = strtok_r(NULL, startToken, &ctx);

    }
    while ( NULL != pArea );

    if ( NULL != tmpBuffer )
    {
        free(tmpBuffer);
    }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

bool CameraArea::areAreasDifferent(Vector< sp<CameraArea> > &area1,
                                   Vector< sp<CameraArea> > &area2)
{
    if (area1.size() != area2.size())
    {
        return true;
    }

    // not going to care about sorting order for now
    for (unsigned int i = 0; i < area1.size(); i++)
    {
        if (!area1.itemAt(i)->compare(area2.itemAt(i)))
        {
            return true;
        }
    }

    return false;
}

bool CameraArea::compare(const sp<CameraArea> &area)
{
    return ((mTop == area->mTop) && (mLeft == area->mLeft) &&
            (mBottom == area->mBottom) && (mRight == area->mRight) &&
            (mWeight == area->mWeight));
}


/*--------------------CameraArea Class ENDS here-----------------------------*/


int CameraFormatConverter::halFormatForCameraFormat(const char *cameraformat)
{
    if(cameraformat == NULL)
    {
        return -1;
    }
    if(strcmp(cameraformat, CameraParameters::PIXEL_FORMAT_RGB565) == 0)
    {
        return HAL_PIXEL_FORMAT_RGB_565;
    }
    else if(strcmp(cameraformat, CameraParameters::PIXEL_FORMAT_RGBA8888) == 0)
    {
        return HAL_PIXEL_FORMAT_RGBA_8888;
    }
    else if(strcmp(cameraformat, CameraParameters::PIXEL_FORMAT_YUV422SP) == 0)
    {
        //TODO, FIX this
        return HAL_PIXEL_FORMAT_YCbCr_422_SP;
    }
    else if(strcmp(cameraformat, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0)
    {
        return HAL_PIXEL_FORMAT_ACT_NV12;
    }
    else if(strcmp(cameraformat, CameraParameters::PIXEL_FORMAT_YUV422I) == 0)
    {
        //TODO, FIX this
        return HAL_PIXEL_FORMAT_YCbCr_422_I;
    }
    else if(strcmp(cameraformat, CameraParameters::PIXEL_FORMAT_YUV420P) == 0)
    {
        return HAL_PIXEL_FORMAT_ACT_YU12;
    }
    else if(strcmp(cameraformat, CameraParameters::PIXEL_FORMAT_JPEG) == 0)
    {
        return -1;
    }
    else
    {
        return -1;
    }
    return -1;

}




};
