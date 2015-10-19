

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>

#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>

#include <camera/Camera.h>
#include <camera/ICamera.h>
#include <media/mediarecorder.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <camera/CameraParameters.h>
#include <system/audio.h>
#include <system/camera.h>

#include <cutils/memory.h>
#include <utils/Log.h>

#include <sys/wait.h>

#include "camera_test.h" 


#define UNUSED(x) (void)(x)


int camera_index = 0;
sp<Camera> camera;
sp<SurfaceComposerClient> client;
sp<SurfaceControl> surfaceControl;
sp<Surface> previewSurface;
CameraParameters params; 


bool reSizePreview = true;
bool hardwareActive = false;
bool recordingMode = false;
bool previewRunning = false;
int previewFormat = 0;


int openCamera() {
    printf("openCamera(camera_index=%d)\n", camera_index);
    camera = Camera::connect(camera_index);

    if ( NULL == camera.get() ) {
        printf("Unable to connect to CameraService\n");
        printf("Retrying... \n");
        sleep(1);
        camera = Camera::connect(camera_index);

        if ( NULL == camera.get() ) {
            printf("Giving up!! \n");
            return -1;
        }
    }

    params = camera->getParameters();
    camera->setParameters(params.flatten());

    camera->setListener(new CameraHandler());

    hardwareActive = true;

    return 0;
}                




int closeCamera() {
    if ( NULL == camera.get() ) {
        printf("invalid camera reference\n");

        return -1;
    }

    camera->disconnect();
    camera.clear();

    hardwareActive = false;

    return 0;
}  


int startPreview() {
    int previewWidth, previewHeight;
    int pictureWidth, pictureHeight;

    previewWidth = 800;
    previewHeight = 600;

    pictureWidth = 1280;
    pictureHeight=720;


    if (reSizePreview) {

        

#if 1
        if ( createPreviewSurface(800,
                                  480,
                                  HAL_PIXEL_FORMAT_YV12) < 0 ) {
#else
        if ( createPreviewSurface(800,
                                  480,
                                  HAL_PIXEL_FORMAT_RGB_565) < 0 ) {
#endif
            printf("Error while creating preview surface\n");
            return -1;
        }

        if ( !hardwareActive ) {
            openCamera();
        }

        params.setPreviewSize(previewWidth, previewHeight);
        params.setPictureSize(pictureWidth, pictureHeight);

        camera->setParameters(params.flatten());
        camera->setPreviewDisplay(previewSurface);

        if(!hardwareActive) 
        {
        }
        camera->startPreview();

        previewRunning = true;
        reSizePreview = false;

    }

    return 0;
}            


void stopPreview() {
    if ( hardwareActive ) {
        camera->stopPreview();

        destroyPreviewSurface();

        previewRunning  = false;
        reSizePreview = true;
        closeCamera();
    }
}  



int createPreviewSurface(unsigned int width, unsigned int height, int32_t pixFormat) {
    unsigned int previewWidth, previewHeight;

    if ( MAX_PREVIEW_SURFACE_WIDTH < width ) {
        previewWidth = MAX_PREVIEW_SURFACE_WIDTH;
    } else {
        previewWidth = width;
    }

    if ( MAX_PREVIEW_SURFACE_HEIGHT < height ) {
        previewHeight = MAX_PREVIEW_SURFACE_HEIGHT;
    } else {
        previewHeight = height;
    }

    client = new SurfaceComposerClient();

    if ( NULL == client.get() ) {
        printf("Unable to establish connection to Surface Composer \n");

        return -1;
    }

    surfaceControl = client->createSurface(String8("camera_preview_test"),
                                           previewWidth,
                                           previewHeight,
                                           pixFormat, 0);

    previewSurface = surfaceControl->getSurface();

    client->openGlobalTransaction();
    surfaceControl->setLayer(0x7fffffff);
    surfaceControl->setPosition(0, 0);
    surfaceControl->setSize(previewWidth, previewHeight);
    surfaceControl->show();
    client->closeGlobalTransaction();

    return 0;
}

int destroyPreviewSurface() {

    if ( NULL != previewSurface.get() ) {
        printf("previewSurface.clear\n");
        previewSurface.clear();
    }

    if ( NULL != surfaceControl.get() ) {
        surfaceControl->clear();
        surfaceControl.clear();
    }

    if ( NULL != client.get() ) {
        client->dispose();
        client.clear();
    }

    return 0;
}  
void saveFile(char *filename, unsigned char *buff,  size_t len)
{
    int             fd = -1;

    if(filename == NULL || len <= 0)
    {
        printf("savefile error!\n");
        return;
    }
    fd = open(filename, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, 0611);
    if(fd < 0)
    {
        printf("Unable to open file %s: %s\n", filename, strerror(fd));
        return;
    }

    printf("Copying from 0x%p, size=%d \n", buff, len);

    write(fd, buff, len);

    close(fd);
    return;

}


void my_raw_callback(const sp<IMemory>& mem) 
{
    unsigned long long st;
    struct timeval current_time;

    char filename[1024];

    gettimeofday(&current_time, 0);

    st = current_time.tv_sec * 1000000 + current_time.tv_usec;

    snprintf((char *)&filename, 1023, "/data/camera_data/raw_%llu.raw", st);
    printf("saving raw file!!!!!!!!!!!!!!\n");
    saveFile((char *)&filename, (unsigned char *)mem->pointer(), mem->size());

}
void my_preview_callback(const sp<IMemory>& mem) 
{
    unsigned long long st;
    struct timeval current_time;

    char filename[1024];

    gettimeofday(&current_time, 0);

    st = current_time.tv_sec * 1000000 + current_time.tv_usec;

    snprintf((char *)&filename, 1023, "/data/camera_data/preview_%llu.raw", st);
    printf("saving preview file!!!!!!!!!!!!!!\n");
    saveFile((char *)&filename, (unsigned char *)mem->pointer(), mem->size());

}
void my_jpeg_callback(const sp<IMemory>& mem) 
{
    unsigned long long st;
    struct timeval current_time;

    char filename[1024];

    gettimeofday(&current_time, 0);

    st = current_time.tv_sec * 1000000 + current_time.tv_usec;

    snprintf((char *)&filename, 1023, "/data/camera_data/img_%llu.jpeg", st);
    printf("saving jpeg file!!!!!!!!!!!!!!\n");
    saveFile((char *)&filename, (unsigned char *)mem->pointer(), mem->size()); 
    return;

}
void my_face_callback(camera_frame_metadata_t *metadata) 
{
    UNUSED(metadata);
}

void CameraHandler::notify(int32_t msgType, int32_t ext1, int32_t ext2) {

    printf("Notify cb: %d %d %d\n", msgType, ext1, ext2);

    if ( msgType & CAMERA_MSG_FOCUS )
        printf("AutoFocus %s in us\n", (ext1) ? "OK" : "FAIL" );

    if ( msgType & CAMERA_MSG_SHUTTER )
        printf("Shutter done in  us\n");

    if ( msgType & CAMERA_MSG_ERROR && (ext1 == 1))
      {
        printf("Camera Test CAMERA_MSG_ERROR.....\n");
        
        stopPreview();

        if (recordingMode)
        {
            recordingMode = false;
        }
      }
}             



void CameraHandler::postData(int32_t msgType,
                             const sp<IMemory>& dataPtr,
                             camera_frame_metadata_t *metadata) {
    printf("Data cb: %d\n", msgType);

    if ( msgType & CAMERA_MSG_PREVIEW_FRAME )
        my_preview_callback(dataPtr);

    if ( msgType & CAMERA_MSG_RAW_IMAGE ) {
        printf("RAW done in  us\n");
        my_raw_callback(dataPtr);
    }

    if (msgType & CAMERA_MSG_POSTVIEW_FRAME) {
        printf("Postview frame  us\n");
    }

    if (msgType & CAMERA_MSG_COMPRESSED_IMAGE ) {
        printf("JPEG done in us\n");
        my_jpeg_callback(dataPtr);
    }

    if ( ( msgType & CAMERA_MSG_PREVIEW_METADATA ) &&
         ( NULL != metadata ) ) {
        printf("Face detected %d \n", metadata->number_of_faces);
        my_face_callback(metadata);
    }
}  


void CameraHandler::postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr)

{
    printf("Recording cb: %d %lld %p\n", msgType, timestamp, dataPtr.get());

    static uint32_t count = 0;

    count++;

    uint8_t *ptr = (uint8_t*) dataPtr->pointer();

    printf("VID_CB: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9]);

    camera->releaseRecordingFrame(dataPtr);
} 





int main(int argc, char *argv[]) {

    int cnt = 0;

    UNUSED(argc);
    UNUSED(argv);

    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();

    if ( openCamera() < 0 ) {
        printf("Camera initialization failed\n");
        system("echo camerahal_test > /sys/power/wake_unlock");



        return -1;
    } 

    params.set(CameraParameters::KEY_ZOOM, 20);
    if ( hardwareActive )
    {
        camera->setParameters(params.flatten());
    }

    camera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_ENABLE_MASK);
    startPreview();
    while( 1 )
    {
        printf("p=%d\n",cnt);
        if((++cnt)>15)
        {
            break;
        }
        if((cnt ==5))
        {
            if ( hardwareActive )
            {
                printf("taking picture!!!!!!!!!!!");
                fflush(stdout);
                camera->takePicture(CAMERA_MSG_COMPRESSED_IMAGE|CAMERA_MSG_RAW_IMAGE);
            }
        }
        
        sleep(3);
    }
    printf("stop preview!!!!!!!!!!!!!!\n");
    stopPreview();


    if ( hardwareActive ) {
        closeCamera();
    }   

    sleep(10);
    return 0;
}
