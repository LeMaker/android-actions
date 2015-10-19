#include "CameraHWCaps.h"
#include "CameraHal.h"

#include "CameraHalDebug.h"

namespace android
{
//ActionsCode(author:liuyiguang, notice: vce already support nv12, nv21, yv12, yu12. So Format Type here is no more use!)
int getHwVceFormatType()
{
#ifdef CAMERA_FORMAT_NV12YU12
    return FORMAT_NV12YU12_TYPE;
#else
    return FORMAT_NV21YV12_TYPE;
#endif
}

int getHwVideoFormatType()
{
    return getHwVceFormatType();
}

int getHwJpegFormatType()
{
    return getHwVceFormatType();
}

int getHwZoomFormatType()
{
    return getHwVceFormatType();
}

int getHwVideoEncodeFormatPreferred(int format)
{
    if(getHwVideoFormatType() == FORMAT_NV12YU12_TYPE)
    {
        CAMHAL_LOGDB("format:%d", format);
        switch(format)
        {
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV422I:
            return CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12;

            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            return CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12;


            default:
            return CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12;
        }

    }
    else
    {
        switch(format)
        {
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV422I:
            return CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP;

            //nv12 was supportted by encoder
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            return CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12;

            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            return CameraFrame::CAMERA_FRAME_FORMAT_YUV420P;


            default:
            return CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP;
        }
    }
    return CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP;
}

//ActionsCode(author:liuyiguang, change_code, relax the solution of encode)
//int isHwVceSupport(int sw, int sh, int sf, int dw, int dh, int df)
#if 0
int isHwVceSupport(int sw, int sh, int sf, int dw, int dh, int df, int stride)
{
    if(getHwZoomFormatType() == FORMAT_NV12YU12_TYPE)
    {
        CAMHAL_LOGDB("sw:%d, sh:%d, sf:%d, dw:%d, dh:%d, df:%d",
                sw, sh, sf, dw, dh, df);
        if ((sf != CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12
            && sf != CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12)
            || (df != CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12
            && df != CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12))
        {
            return false;
        }
        //ActionsCode(author:liuyiguang, change_code, relax the solution of encode)
        //if((sf == CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12 
        //    || df == CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12)
        //    && (dw%32 || sw %32))
        if((sf == CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12 
            || df == CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12)
            && (dw % 16 || sw % 16 || stride % 32))
        {
            return false;
        }
		
    }else{//here
        if ((sf != CameraFrame::CAMERA_FRAME_FORMAT_YUV420P
            && sf != CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP)
            || (df != CameraFrame::CAMERA_FRAME_FORMAT_YUV420P
            && df != CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP))
        {
            return false;
        }
        //ActionsCode(author:liuyiguang, change_code, relax the solution of encode)
        //if((sf == CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP 
        //    || df == CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP)
        //    && (dw%32 || sw % 32))
        if((sf == CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP 
            || df == CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP)
            && (dw % 16 || sw % 16 || stride % 32))
        {
            return false;
        }
    }
    //ActionsCode(author:liuyiguang, change_code, relax the solution of encode)
    //if((dw < sw)
    //    ||(dh < sh)
    //    ||((dw-1)/sw >= MAX_CAMERA_SCALE)
    //    ||((dh-1)/sh >= MAX_CAMERA_SCALE)
    //    )
    if((dw < sw / 2)
        ||(dh < sh / 2)
        ||((dw-1)/sw >= MAX_CAMERA_SCALE)
        ||((dh-1)/sh >= MAX_CAMERA_SCALE)
        )
    {
        return false;
    }
    return true;

}
#else
//ActionsCode(author:liuyiguang, change_code, vce support nv21, nv12, yv12, yu12, so rewrite this function!)
int isHwVceSupport(int sw, int sh, int sf, int dw, int dh, int df, int stride)
{
    CAMHAL_LOGEB("vce sw:%d, sh:%d, sf:%d, dw:%d, dh:%d, df:%d",
        sw, sh, sf, dw, dh, df);
        if ((sf != CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12
            && sf != CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12
            && sf != CameraFrame::CAMERA_FRAME_FORMAT_YUV420P
            && sf != CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP)
            || (df != CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12
            && df != CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12
            && df != CameraFrame::CAMERA_FRAME_FORMAT_YUV420P
            && df != CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP))
        {
            return false;
        }
        if(dw % 16 || sw % 16 || stride % 32)
        {
            return false;
        }
        if((dw < sw / 2)
            ||(dh < sh / 2)
            ||((dw-1)/sw >= MAX_CAMERA_SCALE)
            ||((dh-1)/sh >= MAX_CAMERA_SCALE)
            )
        {
            return false;
        }

        return true;
}
#endif

//ActionsCode(author:liuyiguang, change_code, relax the solution of encode)
//int isHwZoomSupport(int sw, int sh, int sf, int dw, int dh, int df)
//{
//    return isHwVceSupport(sw, sh, sf, dw, dh, df);
//}
//
//int isHwJpegEncodeSupport(int sw, int sh, int sf, int dw, int dh, int df)
//{
//    return isHwVceSupport(sw, sh, sf, dw, dh, df);
//}

int isHwZoomSupport(int sw, int sh, int sf, int dw, int dh, int df, int stride)
{
    return isHwVceSupport(sw, sh, sf, dw, dh, df, stride);
}

int isHwJpegEncodeSupport(int sw, int sh, int sf, int dw, int dh, int df, int stride)
{
    return isHwVceSupport(sw, sh, sf, dw, dh, df, stride);
}

int getHwVideoFormatAligned()
{
    return false;
}
int getHwVceOutFormat(int format)
{
    if(getHwZoomFormatType() == FORMAT_NV12YU12_TYPE)
    {
        switch(format)
        {
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV422I:
            return CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12;

            default:
            return CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12;
        }

    }
    else
    {
        switch(format)
        {
            //ActionsCode(author:liuyiguang, change_code, vce output format use below!)
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV422I:
            return CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP;

            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            return CameraFrame::CAMERA_FRAME_FORMAT_YUV420P;

            default:
            return CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP;
        }
    }
    return CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP;
}

};
