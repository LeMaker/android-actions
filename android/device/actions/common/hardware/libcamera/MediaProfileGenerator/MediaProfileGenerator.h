#ifndef __CAMERA_MEDIA_PROFILE_GENARATOR_H__
#define __CAMERA_MEDIA_PROFILE_GENARATOR_H__

#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <utils/Vector.h>
#include <utils/threads.h>

#define _MAX_CAMERA_NUM (2) 
#define _MAX_SENSOR_NAME_SIZE (32) 

namespace android
{


struct CameraFrameSize
{
    int width;
    int height;

    CameraFrameSize() {
        width = 0;
        height = 0;
    }

    CameraFrameSize(int w, int h) {
        width = w;
        height = h;
    }
};

class MediaProfileGenerator{
public:
    MediaProfileGenerator();
    virtual ~MediaProfileGenerator() ;
    status_t loadTemplate(const char * temppath);
    status_t setCameraNum(int num);
    status_t addVideoSize(int id, int width, int height);
    status_t setSensorName(int id, char *name);
    status_t genMediaProfile(const char *savepath);

    static bool checkMediaProfile(const char *path, const char *sensor0, const char *sensor1);

private:
    status_t getTagValue(char *str, const char *tag, char **start, char **end );

    int getMaxSize(int id);
    int getMinSize(int id);

    void genVideoSizeInfo(char *buf, int bufsize, const char *quality, int w, int h);
    void genSizes(int id, char *buf, int bufsize);

    int mCameraNum;
    char *mHeader;
    char *mFooter;
    char *mProfileTemplate;
    char *mVideoSizeTemplate;
    char *mTemplate;
    unsigned long mHeaderLen;
    unsigned long mFooterLen;

    Vector<CameraFrameSize> mFrameSizes[_MAX_CAMERA_NUM];
    char mSensorNames[_MAX_CAMERA_NUM][_MAX_SENSOR_NAME_SIZE];
};
};

#endif
