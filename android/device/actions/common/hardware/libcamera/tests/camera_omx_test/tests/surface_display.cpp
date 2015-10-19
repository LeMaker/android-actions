#include <cutils/memory.h>
#include <linux/videodev2.h>
#include <utils/Log.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <gui/Surface.h>
#include <gui/ISurface.h>
#include <gui/SurfaceComposerClient.h>

#include "omxcameratest.h"
void yuv420_to_rgb565(const unsigned char *src, unsigned short *dst, int width, int height);

using namespace android;


#ifdef __cplusplus
extern "C" {
#endif

static sp<SurfaceComposerClient> client;
static sp<Surface> surface;
static  sp<SurfaceControl> surfaceControl ;

static int disp_w, disp_h;


int surface_display_main_init(int w, int h)
{
    // set up the thread-pool
    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();

    // create a client to surfaceflinger
    client = new SurfaceComposerClient();
    
    // create pushbuffer surface
    surfaceControl = client->createSurface( String8("camera_omx_tests"), 160, 240, PIXEL_FORMAT_RGB_565,0);

             
    disp_w=w;
    disp_h=h;
    SurfaceComposerClient::openGlobalTransaction();
    surfaceControl->setLayer(100000);
    surfaceControl->setSize(disp_w, disp_h);
    SurfaceComposerClient::closeGlobalTransaction();

    // pretend it went cross-process
    Parcel parcel;
    SurfaceControl::writeSurfaceToParcel(surfaceControl, &parcel);
    parcel.setDataPosition(0);
    surface = Surface::readFromParcel(parcel);

    return 0;
}
//注意，默认buffer格式为rgb565
int surface_display_main_show(const void *buf, int width, int height, int format)
{

   if(width!=disp_w || height!=disp_h){
    printf("error, wrong buffer dimension width=%d height=%d \
    request width=%d height=%d ", width, height, disp_w, disp_h);
    return -1;
   }
    Surface::SurfaceInfo info;
    surface->lock(&info);
    ssize_t bpr = info.s * bytesPerPixel(info.format);
      //covert yuv to rgb when necessary
   if(format !=DEFAULT_CAMERA_COLOR_FORMAT){
                yuv420_to_rgb565((const unsigned char *)buf,(unsigned short *)info.bits, width, height);
   }else{
        memcpy(info.bits, buf, disp_w*disp_h*2);
    }
    surface->unlockAndPost();
    return 0;
}

#ifdef __cplusplus
}
#endif
