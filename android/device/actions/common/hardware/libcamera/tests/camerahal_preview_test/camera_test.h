#ifndef CAMERA_TEST_H
#define CAMERA_TEST_H

#define PRINTOVER(arg...)     ALOGD(#arg)
#define LOG_FUNCTION_NAME         ALOGD("%d: %s() ENTER", __LINE__, __FUNCTION__);
#define LOG_FUNCTION_NAME_EXIT    ALOGD("%d: %s() EXIT", __LINE__, __FUNCTION__);


#define MAX_PREVIEW_SURFACE_WIDTH   800
#define MAX_PREVIEW_SURFACE_HEIGHT  480

#define SDCARD_PATH "/sdcard/"


namespace android {
    class CameraHandler: public CameraListener {
        public:
            virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2);
            virtual void postData(int32_t msgType,
                                  const sp<IMemory>& dataPtr,
                                  camera_frame_metadata_t *metadata);

            virtual void postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr);
    };

};

using namespace android;


int openCamera();
int closeCamera();
int startPreview();
void stopPreview();

int createPreviewSurface(unsigned int width, unsigned int height, int32_t pixFormat) ;
int destroyPreviewSurface() ;

#endif
