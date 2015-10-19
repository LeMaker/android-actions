#ifndef __CAMERA_RES_TABLE_H__
#define __CAMERA_RES_TABLE_H__

namespace android
{
typedef struct
{
    const char *name;
    int width;
    int height;
}CameraResItem;

class CameraResTable
{
public:
    static const CameraResItem *getResByName(const char *name);
    static const CameraResItem* getResBySize(int w, int h);
private:
    static const CameraResItem sCameraResNameTable[];
};

}

#endif

