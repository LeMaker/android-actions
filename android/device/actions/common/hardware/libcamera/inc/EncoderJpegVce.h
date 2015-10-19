
#ifndef CAMERA_HARDWARE_ENCODER_JPEG_VCE_H
#define CAMERA_HARDWARE_ENCODER_JPEG_VCE_H

#include <utils/threads.h>
#include <utils/RefBase.h>


#include "OMXVce.h"

namespace android
{


class EncoderJpegVce: public RefBase
{

    /* public member functions */
public:
    EncoderJpegVce(ImageJpegEncoderParam* param,
                   CameraHalExif *exif,
                   OMXVceJpegEncoder *vceEncoder
        )
        :  mInputParam(param),mExif(exif),mVceEncoder(vceEncoder)
    {
        mLastEncoderData = NULL;
    }

    ~EncoderJpegVce()
    {
        freeEncodeData();
    }

    bool encode()
    {
        size_t size = 0;
        // encode our main image
        size = encode(reinterpret_cast<ImageJpegEncoderParam *>(mInputParam), reinterpret_cast<CameraHalExif *>(mExif));
        if(size > 0)
        {
            mLastEncoderData = mInputParam->outData;
            return true;
        }

        return false;
    }


private:
    ImageJpegEncoderParam* mInputParam;
    CameraHalExif *mExif;
    OMXVceJpegEncoder *mVceEncoder;//libcamera/vce/??
    void* mLastEncoderData;

    size_t encode(ImageJpegEncoderParam *param, CameraHalExif *exifParam)
    {
        param->outDataSize = 0;
        if(mVceEncoder)
        {
            mVceEncoder->encode(param, exifParam);
        }
        return param->outDataSize;
    };
    void freeEncodeData()
    {
        if(mVceEncoder && (mLastEncoderData != NULL))
        {
            mVceEncoder->freeEncodeData(mLastEncoderData);
        }
        mLastEncoderData = NULL;

    };
};

}

#endif

