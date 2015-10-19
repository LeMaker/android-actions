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

using namespace android;

int camera_index = 0;
int print_menu;
sp<Camera> camera;
sp<MediaRecorder> recorder;
sp<SurfaceComposerClient> client;
sp<SurfaceControl> surfaceControl;
sp<Surface> previewSurface;
CameraParameters params;

double latitude = 0.0;
double longitude = 0.0;
double degree_by_step = 17.5609756;//..0975609756097;
double altitude = 0.0;
int awb_mode = 0;
int effects_mode = 0;
int scene_mode = 0;

int rotation = 0;
bool reSizePreview = true;
bool hardwareActive = false;
bool recordingMode = false;
bool previewRunning = false;
int saturation = 0;
int zoomIDX = 0;
int videoCodecIDX = 0;
int audioCodecIDX = 0;
int outputFormatIDX = 0;
int contrast = 0;
int brightness = 0;
int sharpness = 0;
int iso_mode = 0;
int previewFormat = 0;
int pictureFormat = 0; 
int jpegQuality = 85;
int thumbQuality = 85;
int flashIdx = 0;
timeval autofocus_start, picture_start;
char script_name[80];
int prevcnt = 0;
int videoFd = -1;
int elockidx = 0;
int wblockidx = 0;

int focus_mode = 0;
int antibanding_mode = 0;
int compensation = 0;

char dir_path[80] = SDCARD_PATH;


int thumbSizeIDX =  0;
int previewSizeIDX = 0;
int captureSizeIDX = 0;
int frameRateIDX = 0;
int VcaptureSizeIDX = 0;
int VbitRateIDX = 0;

static unsigned int recording_counter = 1;

int dump_preview = 0;
int bufferStarvationTest = 0;
bool showfps = false;

bool bLogSysLinkTrace = true;
bool stressTest = false;
bool stopScript = false;
int restartCount = 0;



 

outformat outputFormat[] = {
//        { OUTPUT_FORMAT_THREE_GPP, "3gp" },
        { OUTPUT_FORMAT_MPEG_4, "mp4" },
    };

size_t length_outputFormat = ARRAY_SIZE(outputFormat);

video_Codecs videoCodecs[] = {
//  { VIDEO_ENCODER_H263, "H263" },
  { VIDEO_ENCODER_H264, "H264" },
  { VIDEO_ENCODER_MPEG_4_SP, "MPEG4"}
};

size_t length_videoCodecs = ARRAY_SIZE(videoCodecs);

audio_Codecs audioCodecs[] = {
  { AUDIO_ENCODER_AMR_NB, "AMR_NB" },
//  { AUDIO_ENCODER_AMR_WB, "AMR_WB" },
//  { AUDIO_ENCODER_AAC, "AAC" },
//  { AUDIO_ENCODER_HE_AAC, "AAC+" },
//  { AUDIO_ENCODER_LIST_END, "disabled"},
};
size_t length_audioCodecs = ARRAY_SIZE(audioCodecs);



V_bitRate VbitRate[] = {
  {    64000, "64K"  },
  {   128000, "128K" },
  {   192000, "192K" },
  {   240000, "240K" },
  {   320000, "320K" },
  {   360000, "360K" },
  {   384000, "384K" },
  {   420000, "420K" },
  {   768000, "768K" },
  {  1000000, "1M"   },
  {  1500000, "1.5M" },
  {  2000000, "2M"   },
  {  4000000, "4M"   },
  {  6000000, "6M"   },
  {  8000000, "8M"   },
  { 10000000, "10M"  },
};

size_t length_VbitRate = ARRAY_SIZE(VbitRate);

#define CAMERA_MODEL_DEF(num,back,front) \
    const char *cameras[] = {#back, #front};      \
    size_t length_cameras =  num;                 \
                                                  \
void initModelDefaults(int i)                     \
{                                                 \
    if(i==0)                                      \
    {                                             \
        initModelDefaults_##back();               \
    }                                             \
    else                                          \
    {                                             \
        initModelDefaults_##front();              \
    }                                             \
}                                                 \
                                                  \
void getSupported(int i, camera_supported_t *s)                        \
{                                                 \
    if(i==0)                                      \
    {                                             \
        getModelSupported_##back(s);               \
    }                                             \
    else                                          \
    {                                             \
        getModelSupported_##front(s);              \
    }                                             \
}


#if 0
#define MAX_CAMERA_NUM 2
#include "camera_model_gt2005.h"
#include "camera_model_gc0308.h"

CAMERA_MODEL_DEF(MAX_CAMERA_NUM,gt2005,gc0308);

#else

#define MAX_CAMERA_NUM 2
#include "camera_model_gc2035.h"
#include "camera_model_gc0308.h"

CAMERA_MODEL_DEF(MAX_CAMERA_NUM,gc2035,gc0308);

#endif

camera_supported_t camera_supporteds[MAX_CAMERA_NUM];
    

void init_supported()
{
    int i;
    for(i =0;i<MAX_CAMERA_NUM;i++)
    {
        getSupported(i,&camera_supporteds[i]);
    }
}



/** Calculate delay from a reference time */
unsigned long long timeval_delay(const timeval *ref) {
    unsigned long long st, end, delay;
    timeval current_time;

    gettimeofday(&current_time, 0);

    st = ref->tv_sec * 1000000 + ref->tv_usec;
    end = current_time.tv_sec * 1000000 + current_time.tv_usec;
    delay = end - st;

    return delay;
}

/** Callback for takePicture() */
void my_raw_callback(const sp<IMemory>& mem) {

    static int      counter = 1;
    unsigned char   *buff = NULL;
    int             size;
    int             fd = -1;
    char            fn[256];
    int width;
    int height;
    const char *format;

    LOG_FUNCTION_NAME;

    if (mem == NULL)
        goto out;

    //Start preview after capture.
    camera->startPreview();

    fn[0] = 0;
    params.getPictureSize(&width,&height);
    format = params.getPreviewFormat();
    sprintf(fn, SDCARD_PATH"img%03d_%04d_%04d_%s.raw", counter,width,height,format);
    fd = open(fn, O_CREAT | O_WRONLY | O_TRUNC, 0777);

    if (fd < 0)
        goto out;

    size = mem->size();

    if (size <= 0)
        goto out;

    buff = (unsigned char *)mem->pointer();

    if (!buff)
        goto out;

    if (size != write(fd, buff, size))
        printf("Bad Write int a %s error (%d)%s\n", fn, errno, strerror(errno));

    counter++;
    printf("%s: buffer=%08X, size=%d stored at %s\n",
           __FUNCTION__, (int)buff, size, fn);

out:

    if (fd >= 0)
        close(fd);

    LOG_FUNCTION_NAME_EXIT;
}

void saveFile(const sp<IMemory>& mem) {
    static int      counter = 1;
    unsigned char   *buff = NULL;
    int             size;
    int             fd = -1;
    char            fn[256];
    int width;
    int height;
    const char *format;

    LOG_FUNCTION_NAME;

    if (mem == NULL)
        goto out;

    fn[0] = 0;

    params.getPreviewSize(&width,&height);
    format = params.getPreviewFormat();

    sprintf(fn, SDCARD_PATH"preview%03d_%04d_%04d_%s.yuv", counter,width,height,format);
    fd = open(fn, O_CREAT | O_WRONLY | O_TRUNC, 0777);
    if(fd < 0) {
        ALOGE("Unable to open file %s: %s", fn, strerror(fd));
        goto out;
    }

    size = mem->size();
    if (size <= 0) {
        ALOGE("IMemory object is of zero size");
        goto out;
    }

    buff = (unsigned char *)mem->pointer();
    if (!buff) {
        ALOGE("Buffer pointer is invalid");
        goto out;
    }

    if (size != write(fd, buff, size))
        printf("Bad Write int a %s error (%d)%s\n", fn, errno, strerror(errno));

    counter++;
    printf("%s: buffer=%08X, size=%d\n",
           __FUNCTION__, (int)buff, size);

out:

    if (fd >= 0)
        close(fd);

    LOG_FUNCTION_NAME_EXIT;
}


void debugShowFPS()
{
    static int mFrameCount = 0;
    static int mLastFrameCount = 0;
    static nsecs_t mLastFpsTime = 0;
    static float mFps = 0;
    mFrameCount++;
    if ( ( mFrameCount % 30 ) == 0 ) {
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFpsTime;
        mFps =  ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFpsTime = now;
        mLastFrameCount = mFrameCount;
        printf("####### [%d] Frames, %f FPS", mFrameCount, mFps);
    }
}

/** Callback for startPreview() */
void my_preview_callback(const sp<IMemory>& mem) {

    printf("PREVIEW Callback 0x%x", ( unsigned int ) mem->pointer());
    if (dump_preview) {

        if(prevcnt==50)
        saveFile(mem);

        prevcnt++;

        uint8_t *ptr = (uint8_t*) mem->pointer();

        printf("PRV_CB: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9]);

    }

    debugShowFPS();
}

/** Callback for takePicture() */
void my_jpeg_callback(const sp<IMemory>& mem) {
    static int  counter = 1;
    unsigned char   *buff = NULL;
    int     size;
    int     fd = -1;
    char        fn[256];

    LOG_FUNCTION_NAME;

    //Start preview after capture.
    camera->startPreview();

    if (mem == NULL)
        goto out;

    fn[0] = 0;
    sprintf(fn, "%s/img%03d.jpg", dir_path,counter);
    fd = open(fn, O_CREAT | O_WRONLY | O_TRUNC, 0777);

    if(fd < 0) {
        ALOGE("Unable to open file %s: %s", fn, strerror(fd));
        goto out;
    }

    size = mem->size();
    if (size <= 0) {
        ALOGE("IMemory object is of zero size");
        goto out;
    }

    buff = (unsigned char *)mem->pointer();
    if (!buff) {
        ALOGE("Buffer pointer is invalid");
        goto out;
    }

    if (size != write(fd, buff, size))
        printf("Bad Write int a %s error (%d)%s\n", fn, errno, strerror(errno));

    counter++;
    printf("%s: buffer=%08X, size=%d stored at %s\n",
           __FUNCTION__, (int)buff, size, fn);

out:

    if (fd >= 0)
        close(fd);

    LOG_FUNCTION_NAME_EXIT;
}

void my_face_callback(camera_frame_metadata_t *metadata) {
    int idx;

    if ( NULL == metadata ) {
        return;
    }

    for ( idx = 0 ; idx < metadata->number_of_faces ; idx++ ) {
        printf("Face %d at %d,%d %d,%d \n",
               idx,
               metadata->faces[idx].rect[0],
               metadata->faces[idx].rect[1],
               metadata->faces[idx].rect[2],
               metadata->faces[idx].rect[3]);
    }

}

void CameraHandler::notify(int32_t msgType, int32_t ext1, int32_t ext2) {

    printf("Notify cb: %d %d %d\n", msgType, ext1, ext2);

    if ( msgType & CAMERA_MSG_FOCUS )
        printf("AutoFocus %s in %llu us\n", (ext1) ? "OK" : "FAIL", timeval_delay(&autofocus_start));

    if ( msgType & CAMERA_MSG_SHUTTER )
        printf("Shutter done in %llu us\n", timeval_delay(&picture_start));

    if ( msgType & CAMERA_MSG_ERROR && (ext1 == 1))
      {
        printf("Camera Test CAMERA_MSG_ERROR.....\n");
        if (stressTest)
          {
            printf("Camera Test Notified of Error Restarting.....\n");
            stopScript = true;
          }
        else
          {
            printf("Camera Test Notified of Error Stopping.....\n");
            stopScript =false;
            stopPreview();

            if (recordingMode)
              {
                stopRecording();
                closeRecorder();
                recordingMode = false;
              }
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
        printf("RAW done in %llu us\n", timeval_delay(&picture_start));
        my_raw_callback(dataPtr);
    }

    if (msgType & CAMERA_MSG_POSTVIEW_FRAME) {
        printf("Postview frame %llu us\n", timeval_delay(&picture_start));
    }

    if (msgType & CAMERA_MSG_COMPRESSED_IMAGE ) {
        printf("JPEG done in %llu us\n", timeval_delay(&picture_start));
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

    //if(count==100)
    //saveFile(dataPtr);

    count++;

    uint8_t *ptr = (uint8_t*) dataPtr->pointer();

    printf("VID_CB: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9]);

    camera->releaseRecordingFrame(dataPtr);
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

    surfaceControl = client->createSurface(String8("camera_test_menu"),
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

void printSupportedParams()
{
    printf("\n\r\tSupported Cameras: %s", params.get("camera-indexes"));
    printf("\n\r\tSupported Picture Sizes: %s", params.get(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES));
    printf("\n\r\tSupported Picture Formats: %s", params.get(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS));
    printf("\n\r\tSupported Preview Sizes: %s", params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES));
    printf("\n\r\tSupported Preview Formats: %s", params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS));
    printf("\n\r\tSupported Preview Frame Rates: %s", params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES));
    printf("\n\r\tSupported Thumbnail Sizes: %s", params.get(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES));
    printf("\n\r\tSupported Whitebalance Modes: %s", params.get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE));
    printf("\n\r\tSupported Effects: %s", params.get(CameraParameters::KEY_SUPPORTED_EFFECTS));
    printf("\n\r\tSupported Scene Modes: %s", params.get(CameraParameters::KEY_SUPPORTED_SCENE_MODES));
    printf("\n\r\tSupported Focus Modes: %s", params.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES));
    printf("\n\r\tSupported Antibanding Options: %s", params.get(CameraParameters::KEY_SUPPORTED_ANTIBANDING));
    printf("\n\r\tSupported Flash Modes: %s", params.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES));
    printf("\n\r\tSupported Focus Areas: %d", params.getInt(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS));

    if ( NULL != params.get(CameraParameters::KEY_FOCUS_DISTANCES) ) {
        printf("\n\r\tFocus Distances: %s \n", params.get(CameraParameters::KEY_FOCUS_DISTANCES));
    }

    return;
}


int destroyPreviewSurface() {

    if ( NULL != previewSurface.get() ) {
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

int openRecorder() {
    recorder = new MediaRecorder();

    if ( NULL == recorder.get() ) {
        printf("Error while creating MediaRecorder\n");

        return -1;
    }

    return 0;
}

int closeRecorder() {
    if ( NULL == recorder.get() ) {
        printf("invalid recorder reference\n");

        return -1;
    }

    if ( recorder->init() < 0 ) {
        printf("recorder failed to initialize\n");

        return -1;
    }

    if ( recorder->close() < 0 ) {
        printf("recorder failed to close\n");

        return -1;
    }

    if ( recorder->release() < 0 ) {
        printf("error while releasing recorder\n");

        return -1;
    }

    recorder.clear();

    return 0;
}

int configureRecorder() {

    char videoFile[256],vbit_string[50];
    videoFd = -1;

    if ( ( NULL == recorder.get() ) || ( NULL == camera.get() ) ) {
        printf("invalid recorder and/or camera references\n");

        return -1;
    }

    camera->unlock();

    sprintf(vbit_string,"video-param-encoding-bitrate=%u", VbitRate[VbitRateIDX].bit_rate);
    String8 bit_rate(vbit_string);
    if ( recorder->setParameters(bit_rate) < 0 ) {
        printf("error while configuring bit rate\n");

        return -1;
    }

    if ( recorder->setCamera(camera->remote(), camera->getRecordingProxy()) < 0 ) {
        printf("error while setting the camera\n");

        return -1;
    }

    if ( recorder->setVideoSource(VIDEO_SOURCE_CAMERA) < 0 ) {
        printf("error while configuring camera video source\n");

        return -1;
    }


    if ( AUDIO_ENCODER_LIST_END != audioCodecs[audioCodecIDX].type ) {
        if ( recorder->setAudioSource(AUDIO_SOURCE_DEFAULT) < 0 ) {
            printf("error while configuring camera audio source\n");

            return -1;
        }
    }

    if ( recorder->setOutputFormat(outputFormat[outputFormatIDX].type) < 0 ) {
        printf("error while configuring output format\n");

        return -1;
    }

    if(mkdir(SDCARD_PATH"videos",0777) == -1)
         printf("\n Directory --videos-- was not created \n");
    sprintf(videoFile, SDCARD_PATH"videos/video%d.%s", recording_counter,outputFormat[outputFormatIDX].desc);

    videoFd = open(videoFile, O_CREAT | O_RDWR,0777);

    if(videoFd < 0){
        printf("Error while creating video filename\n");

        return -1;
    }

    if ( recorder->setOutputFile(videoFd, 0, 0) < 0 ) {
        printf("error while configuring video filename\n");

        return -1;
    }

    recording_counter++;

    if ( recorder->setVideoFrameRate(GET_SUPPORTED(fpsConstRanges)[frameRateIDX].constFramerate) < 0 ) {
        printf("error while configuring video framerate\n");
        return -1;
    }
    

    if ( recorder->setVideoSize(GET_SUPPORTED(VcaptureSize)[VcaptureSizeIDX].width, GET_SUPPORTED(VcaptureSize)[VcaptureSizeIDX].height) < 0 ) {
        printf("error while configuring video size\n");

        return -1;
    }

    if ( recorder->setVideoEncoder(videoCodecs[videoCodecIDX].type) < 0 ) {
        printf("error while configuring video codec\n");

        return -1;
    }

    if ( AUDIO_ENCODER_LIST_END != audioCodecs[audioCodecIDX].type ) {
        if ( recorder->setAudioEncoder(audioCodecs[audioCodecIDX].type) < 0 ) {
            printf("error while configuring audio codec\n");

            return -1;
        }
    }

    //if ( recorder->setPreviewSurface( surfaceControl->getSurface() ) < 0 ) {
    if ( recorder->setPreviewSurface( surfaceControl->getSurface()->getIGraphicBufferProducer() ) < 0 ) {
        printf("error while configuring preview surface\n");

        return -1;
    }

    return 0;
}

int startRecording() {
    if ( ( NULL == recorder.get() ) || ( NULL == camera.get() ) ) {
        printf("invalid recorder and/or camera references\n");

        return -1;
    }

    camera->unlock();

    if ( recorder->prepare() < 0 ) {
        printf("recorder prepare failed\n");

        return -1;
    }

    if ( recorder->start() < 0 ) {
        printf("recorder start failed\n");

        return -1;
    }

    return 0;
}

int stopRecording() {
    if ( NULL == recorder.get() ) {
        printf("invalid recorder reference\n");

        return -1;
    }

    if ( recorder->stop() < 0 ) {
        printf("recorder failed to stop\n");

        return -1;
    }

    if ( 0 < videoFd ) {
        close(videoFd);
    }

    return 0;
}

int openCamera() {
	String16 name("cameratest");
    printf("openCamera(camera_index=%d)\n", camera_index);
    camera = Camera::connect(camera_index,name,Camera::USE_CALLING_UID);

    if ( NULL == camera.get() ) {
        printf("Unable to connect to CameraService\n");
        printf("Retrying... \n");
        sleep(1);
        camera = Camera::connect(camera_index,name,Camera::USE_CALLING_UID);

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
    if (reSizePreview) {

        if(recordingMode)
        {
            previewWidth = GET_SUPPORTED(VcaptureSize)[VcaptureSizeIDX].width;
            previewHeight = GET_SUPPORTED(VcaptureSize)[VcaptureSizeIDX].height;
        }else
        {
            previewWidth = GET_SUPPORTED(previewSize)[previewSizeIDX].width;
            previewHeight = GET_SUPPORTED(previewSize)[previewSizeIDX].height;
        }

        if ( createPreviewSurface(previewWidth,
                                  previewHeight,
                                  GET_SUPPORTED(pixelformat)[previewFormat].pixelFormatDesc) < 0 ) {
            printf("Error while creating preview surface\n");
            return -1;
        }

        if ( !hardwareActive ) {
            openCamera();
        }

        params.setPreviewSize(previewWidth, previewHeight);
        params.setPictureSize(GET_SUPPORTED(captureSize)[captureSizeIDX].width, GET_SUPPORTED(captureSize)[captureSizeIDX].height);

        camera->setParameters(params.flatten());
        //camera->setPreviewDisplay(previewSurface);

        if(!hardwareActive) prevcnt = 0;

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

void initDefaults() {

    initModelDefaults(camera_index);

    params.setPreviewSize(GET_SUPPORTED(previewSize)[previewSizeIDX].width, GET_SUPPORTED(previewSize)[previewSizeIDX].height);
    params.setPictureSize(GET_SUPPORTED(captureSize)[captureSizeIDX].width, GET_SUPPORTED(captureSize)[captureSizeIDX].height);
    params.set(CameraParameters::KEY_ROTATION, rotation);
    params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, (int) (compensation));
    params.set(params.KEY_WHITE_BALANCE, GET_SUPPORTED(strawb_mode)[awb_mode]);
    params.set(params.KEY_SCENE_MODE, GET_SUPPORTED(scene)[scene_mode]);
    if(iso_mode < (int)GET_SUPPORTED(length_iso))
    {
        params.set(KEY_ISO, GET_SUPPORTED(iso)[iso_mode]);
    }
    params.set(KEY_SHARPNESS, sharpness);
    params.set(KEY_CONTRAST, contrast);
    params.set(CameraParameters::KEY_ZOOM, GET_SUPPORTED(zoom)[zoomIDX].idx);
    params.set(KEY_BRIGHTNESS, brightness);
    params.set(KEY_SATURATION, saturation);
    params.set(params.KEY_EFFECT, GET_SUPPORTED(effects)[effects_mode]);
    params.setPreviewFrameRate(GET_SUPPORTED(fpsConstRanges)[frameRateIDX].constFramerate);
    params.set(params.KEY_ANTIBANDING, GET_SUPPORTED(antibanding)[antibanding_mode]);
    params.set(params.KEY_FOCUS_MODE, GET_SUPPORTED(focus)[focus_mode]);
    params.set(CameraParameters::KEY_JPEG_QUALITY, jpegQuality);
    params.setPreviewFormat(GET_SUPPORTED(pixelformat)[previewFormat].pixformat);
    params.setPictureFormat(GET_SUPPORTED(codingformat)[pictureFormat]);
    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, GET_SUPPORTED(thumbSize)[thumbSizeIDX].width);
    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, GET_SUPPORTED(thumbSize)[thumbSizeIDX].height);
    params.set(KEY_EXIF_MODEL, MODEL);
    params.set(KEY_EXIF_MAKE, MAKE);
}

int menu_gps() {
    char ch;
    char coord_str[100];

    if (print_menu) {
        printf("\n\n== GPS MENU ============================\n\n");
        printf("   e. Latitude:       %.7lf\n", latitude);
        printf("   d. Longitude:      %.7lf\n", longitude);
        printf("   c. Altitude:       %.7lf\n", altitude);
        printf("\n");
        printf("   q. Return to main menu\n");
        printf("\n");
        printf("   Choice: ");
    }

    ch = getchar();
    printf("%c", ch);

    print_menu = 1;

    switch (ch) {

        case 'e':
            latitude += degree_by_step;

            if (latitude > 90.0) {
                latitude -= 180.0;
            }

            snprintf(coord_str, 7, "%.7lf", latitude);
            params.set(params.KEY_GPS_LATITUDE, coord_str);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'd':
            longitude += degree_by_step;

            if (longitude > 180.0) {
                longitude -= 360.0;
            }

            snprintf(coord_str, 7, "%.7lf", longitude);
            params.set(params.KEY_GPS_LONGITUDE, coord_str);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'c':
            altitude += 12345.67890123456789;

            if (altitude > 100000.0) {
                altitude -= 200000.0;
            }

            snprintf(coord_str, 100, "%.20lf", altitude);
            params.set(params.KEY_GPS_ALTITUDE, coord_str);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'Q':
        case 'q':
            return -1;

        default:
            print_menu = 0;
            break;
    }

    return 0;
}

int functional_menu() {
    char ch;

    if (print_menu) {

        printf("\n\n=========== FUNCTIONAL TEST MENU ===================\n\n");

        printf(" \n\nSTART / STOP / GENERAL SERVICES \n");
        printf(" -----------------------------\n");
        printf("   A  Select Camera %s\n", cameras[camera_index]);
        printf("   [. Resume Preview after capture\n");
        printf("   0. Reset to defaults\n");
        printf("   q. Quit\n");
        printf("   @. Disconnect and Reconnect to CameraService \n");
        printf("   /. Enable/Disable showfps: %s\n", ((showfps)? "Enabled":"Disabled"));
        printf("   a. GEO tagging settings menu\n");
        printf("   E.  Camera Capability Dump");


        printf(" \n\n PREVIEW SUB MENU \n");
        printf(" -----------------------------\n");
        printf("   1. Start Preview\n");
        printf("   2. Stop Preview\n");
        printf("   ~. Preview format %s\n",GET_SUPPORTED(pixelformat)[previewFormat].pixformat);
        printf("   4. Preview size:   %4d x %4d - %s\n",GET_SUPPORTED(previewSize)[previewSizeIDX].width,  GET_SUPPORTED(previewSize)[previewSizeIDX].height, GET_SUPPORTED(previewSize)[previewSizeIDX].desc);
        printf("   &. Dump a preview frame\n");

        printf(" \n\n IMAGE CAPTURE SUB MENU \n");
        printf(" -----------------------------\n");
        printf("   p. Take picture/Full Press\n");
        printf("   $. Picture Format: %s\n", GET_SUPPORTED(codingformat)[pictureFormat]);
        printf("   3. Picture Rotation:       %3d degree\n", rotation );
        printf("   5. Picture size:   %4d x %4d - %s\n",GET_SUPPORTED(captureSize)[captureSizeIDX].width, GET_SUPPORTED(captureSize)[captureSizeIDX].height,              GET_SUPPORTED(captureSize)[captureSizeIDX].name);
        printf("   i. ISO mode:       %s\n", GET_SUPPORTED(iso)[iso_mode]);
        printf("   o. Jpeg Quality:   %d\n", jpegQuality);
        printf("   :. Thumbnail Size:  %4d x %4d - %s\n",GET_SUPPORTED(previewSize)[thumbSizeIDX].width, GET_SUPPORTED(previewSize)[thumbSizeIDX].height, GET_SUPPORTED(previewSize)[thumbSizeIDX].desc);
        printf("   ': Thumbnail Quality %d\n", thumbQuality);

        printf(" \n\n VIDEO CAPTURE SUB MENU \n");
        printf(" -----------------------------\n");

        printf("   6. Start Video Recording\n");
        printf("   2. Stop Recording\n");
        printf("   l. Video Capture resolution:   %4d x %4d - %s\n",GET_SUPPORTED(VcaptureSize)[VcaptureSizeIDX].width,GET_SUPPORTED(VcaptureSize)[VcaptureSizeIDX].height, GET_SUPPORTED(VcaptureSize)[VcaptureSizeIDX].desc);
        printf("   ]. Video Bit rate :  %s\n", VbitRate[VbitRateIDX].desc);
        printf("   9. Video Codec:    %s\n", videoCodecs[videoCodecIDX].desc);
        printf("   D. Audio Codec:    %s\n", audioCodecs[audioCodecIDX].desc);
        printf("   v. Output Format:  %s\n", outputFormat[outputFormatIDX].desc);

        printf("   r. Framerate:     %d\n", GET_SUPPORTED(fpsConstRanges)[frameRateIDX].constFramerate);

        printf(" \n\n 3A SETTING SUB MENU \n");
        printf(" -----------------------------\n");

        printf("   F. Start face detection \n");
        printf("   T. Stop face detection \n");
        printf("   G. Touch/Focus area AF\n");
        printf("   f. Auto Focus/Half Press\n");
        printf("   J.Flash:              %s\n", GET_SUPPORTED(flashModes)[flashIdx]);
        printf("   7. EV offset:      %d\n", compensation);
        printf("   8. AWB mode:       %s\n", GET_SUPPORTED(strawb_mode)[awb_mode]);
        printf("   z. Zoom            %s\n", GET_SUPPORTED(zoom)[zoomIDX].zoom_description);
        printf("   e. Effect:         %s\n", GET_SUPPORTED(effects)[effects_mode]);
        printf("   w. Scene:          %s\n", GET_SUPPORTED(scene)[scene_mode]);
        printf("   s. Saturation:     %d\n", saturation);
        printf("   c. Contrast:       %d\n", contrast);
        printf("   h. Sharpness:      %d\n", sharpness);
        printf("   b. Brightness:     %d\n", brightness);
        printf("   x. Antibanding:    %s\n", GET_SUPPORTED(antibanding)[antibanding_mode]);
        printf("   g. Focus mode:     %s\n", GET_SUPPORTED(focus)[focus_mode]);
        printf("   <. Exposure Lock:     %s\n", GET_SUPPORTED(lock)[elockidx]);
        printf("   >. WhiteBalance Lock:  %s\n",GET_SUPPORTED(lock)[wblockidx]);
        printf("\n");
        printf("   Choice: ");
    }

    ch = getchar();
    printf("%c", ch);

    print_menu = 1;

    switch (ch) {
    case 'A':
        camera_index++;
        camera_index %= length_cameras;
        closeCamera();

        openCamera();

        params.setPreviewFrameRate(GET_SUPPORTED(fpsConstRanges)[frameRateIDX].constFramerate);

        break;
    case '[':
        if ( hardwareActive ) {
            camera->setParameters(params.flatten());
            camera->startPreview();
        }
        break;

    case '0':
        initDefaults();
        break;

    case '1':

        if ( startPreview() < 0 ) {
            printf("Error while starting preview\n");

            return -1;
        }

        break;

    case '2':
#if 0
        stopPreview();

        if ( recordingMode ) {
            camera->disconnect();
            camera.clear();
            stopRecording();
            closeRecorder();

            camera = Camera::connect(camera_index);
              if ( NULL == camera.get() ) {
                  sleep(1);
                  camera = Camera::connect(camera_index);
                  if ( NULL == camera.get() ) {
                      return -1;
                  }
              }
              camera->setListener(new CameraHandler());
              camera->setParameters(params.flatten());
              recordingMode = false;
        }
#else
        if ( recordingMode ) {
            printf("stop recorder");
            stopRecording();
            closeRecorder();
            recordingMode = false;
        }
        else {
            printf("stop preview");
            stopPreview();
        }
#endif

        break;

    case '3':
        rotation += 90;
        rotation %= 360;
        params.set(CameraParameters::KEY_ROTATION, rotation);
        if ( hardwareActive )
            camera->setParameters(params.flatten());

        break;

    case '4':
        previewSizeIDX += 1;
        previewSizeIDX %=GET_SUPPORTED(length_previewSize);
        
        reSizePreview = true;

        if ( hardwareActive && previewRunning ) {
            camera->stopPreview();
            camera->setParameters(params.flatten());
            camera->startPreview();
        } else if ( hardwareActive ) {
            camera->setParameters(params.flatten());
        }

        break;

    case '5':
        captureSizeIDX += 1;
        captureSizeIDX %= GET_SUPPORTED(length_captureSize);
        params.setPictureSize(GET_SUPPORTED(captureSize)[captureSizeIDX].width, GET_SUPPORTED(captureSize)[captureSizeIDX].height);

        if ( hardwareActive )
            camera->setParameters(params.flatten());
        break;

    case 'l':
    case 'L':
        VcaptureSizeIDX++;
        VcaptureSizeIDX %= GET_SUPPORTED(length_VcaptureSize);
        break;

    case ']':
        VbitRateIDX++;
        VbitRateIDX %= length_VbitRate;
        break;


    case '6':

        if ( !recordingMode ) {

            recordingMode = true;

            if ( startPreview() < 0 ) {
                printf("Error while starting preview\n");

                return -1;
            }

            if ( openRecorder() < 0 ) {
                printf("Error while openning video recorder\n");

                return -1;
            }

            if ( configureRecorder() < 0 ) {
                printf("Error while configuring video recorder\n");

                return -1;
            }

            if ( startRecording() < 0 ) {
                printf("Error while starting video recording\n");

                return -1;
            }
        }

        break;

    case '7':

        if ( compensation > GET_SUPPORTED(compensationMax)) {
            compensation = GET_SUPPORTED(compensationMin);
        } else {
            compensation += 1;
        }

        params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, (int) (compensation));

        if ( hardwareActive )
            camera->setParameters(params.flatten());

        break;

    case '8':
        awb_mode++;
        awb_mode %= GET_SUPPORTED(length_strawb_mode);
        params.set(params.KEY_WHITE_BALANCE, GET_SUPPORTED(strawb_mode)[awb_mode]);

        if ( hardwareActive )
            camera->setParameters(params.flatten());

        break;

    case '9':
        videoCodecIDX++;
        videoCodecIDX %= length_videoCodecs;
        break;
    case '~':
        previewFormat += 1;
        previewFormat %= GET_SUPPORTED(length_pixelformat) ;
        params.setPreviewFormat(GET_SUPPORTED(pixelformat)[previewFormat].pixformat);

        if ( hardwareActive )
            camera->setParameters(params.flatten());

        break;
    case '$':
        pictureFormat += 1;
        pictureFormat %= GET_SUPPORTED(length_codingformat);
        params.setPictureFormat(GET_SUPPORTED(codingformat)[pictureFormat]);
        if ( hardwareActive )
            camera->setParameters(params.flatten());

        break;

    case ':':
        thumbSizeIDX += 1;
        thumbSizeIDX %= GET_SUPPORTED(length_thumbSize);
        params.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, GET_SUPPORTED(thumbSize)[thumbSizeIDX].width);
        params.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, GET_SUPPORTED(thumbSize)[thumbSizeIDX].height);

        if ( hardwareActive )
            camera->setParameters(params.flatten());

        break;

    case '\'':
        if ( thumbQuality >= 100) {
            thumbQuality = 0;
        } else {
            thumbQuality += 5;
        }

        params.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, thumbQuality);
        if ( hardwareActive )
            camera->setParameters(params.flatten());
        break;

    case 'E':
        if(hardwareActive)
            params.unflatten(camera->getParameters());
        printSupportedParams();
        break;

    case '*':
        if ( hardwareActive )
            camera->startRecording();
        break;

    case 'o':
        if ( jpegQuality >= 100) {
            jpegQuality = 0;
        } else {
            jpegQuality += 5;
        }

        params.set(CameraParameters::KEY_JPEG_QUALITY, jpegQuality);
        if ( hardwareActive )
            camera->setParameters(params.flatten());
        break;

   
    case 'F':
        if ( hardwareActive )
            camera->sendCommand(CAMERA_CMD_START_FACE_DETECTION, 0, 0);

        break;

    case 'T':

        if ( hardwareActive )
            camera->sendCommand(CAMERA_CMD_STOP_FACE_DETECTION, 0, 0);

        break;

    case '@':
        if ( hardwareActive ) {

            closeCamera();

            if ( 0 >= openCamera() ) {
                printf( "Reconnected to CameraService \n");
            }
        }

        break;

    case 'J':
        flashIdx++;
        flashIdx %= GET_SUPPORTED(length_flashModes);
        params.set(CameraParameters::KEY_FLASH_MODE, (GET_SUPPORTED(flashModes)[flashIdx]));

        if ( hardwareActive )
            camera->setParameters(params.flatten());

        break;


    case 'w':
        scene_mode++;
        scene_mode %= GET_SUPPORTED(length_scene);
        params.set(params.KEY_SCENE_MODE, GET_SUPPORTED(scene)[scene_mode]);

        if ( hardwareActive )
            camera->setParameters(params.flatten());

        break;

    case 'i':
        iso_mode++;
        iso_mode %= GET_SUPPORTED(length_iso);
        params.set(KEY_ISO, GET_SUPPORTED(iso)[iso_mode]);

        if ( hardwareActive )
            camera->setParameters(params.flatten());
        break;

    case 'h':
        if ( sharpness > GET_SUPPORTED(sharpnessMax)) {
            sharpness = GET_SUPPORTED(sharpnessMin);
        } else {
            sharpness += 1;
        }
        params.set(KEY_SHARPNESS, sharpness);
        if ( hardwareActive )
            camera->setParameters(params.flatten());
        break;

    case 'D':
    {
        audioCodecIDX++;
        audioCodecIDX %= length_audioCodecs;
        break;
    }

    case 'v':
    {
        outputFormatIDX++;
        outputFormatIDX %= length_outputFormat;
        break;
    }

    case 'z':
        zoomIDX++;
        zoomIDX %= GET_SUPPORTED(length_zoom);
        params.set(CameraParameters::KEY_ZOOM, GET_SUPPORTED(zoom)[zoomIDX].idx);

        if ( hardwareActive )
            camera->setParameters(params.flatten());

        break;


    case 'c':
        if( contrast > GET_SUPPORTED(contrastMax)){
            contrast = GET_SUPPORTED(contrastMin);
        } else {
            contrast += 1;
        }
        params.set(KEY_CONTRAST, contrast);
        if ( hardwareActive )
            camera->setParameters(params.flatten());
        break;
    case 'b':
        if( brightness > GET_SUPPORTED(brightnessMax)){
            brightness = GET_SUPPORTED(brightnessMin);
        } else {
            brightness += 1;
        }

        params.set(KEY_BRIGHTNESS, brightness);

        if ( hardwareActive )
            camera->setParameters(params.flatten());

        break;

    case 's':
    case 'S':
        if( saturation > GET_SUPPORTED(saturationMax)){
            saturation = GET_SUPPORTED(saturationMin);
        } else {
            saturation += 1;
        }

        params.set(KEY_SATURATION, saturation);

        if ( hardwareActive )
            camera->setParameters(params.flatten());

        break;

    case 'e':
        effects_mode++;
        effects_mode %= GET_SUPPORTED(length_effects);
        params.set(params.KEY_EFFECT, GET_SUPPORTED(effects)[effects_mode]);

        if ( hardwareActive )
            camera->setParameters(params.flatten());

        break;

    case 'r':


        frameRateIDX += 1;
        frameRateIDX %= GET_SUPPORTED(length_fpsConstRanges);
        params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, GET_SUPPORTED(fpsConstRanges)[frameRateIDX].range);

        if ( hardwareActive ) {
            camera->setParameters(params.flatten());
        }

    break;

 

    case 'x':
        antibanding_mode++;
        antibanding_mode %= GET_SUPPORTED(length_antibanding);
        params.set(params.KEY_ANTIBANDING, GET_SUPPORTED(antibanding)[antibanding_mode]);

        if ( hardwareActive )
            camera->setParameters(params.flatten());

        break;

    case 'g':
        focus_mode++;
        focus_mode %= GET_SUPPORTED(length_focus);
        params.set(params.KEY_FOCUS_MODE, GET_SUPPORTED(focus)[focus_mode]);

        if ( hardwareActive )
            camera->setParameters(params.flatten());

        break;

    case 'G':

        params.set(CameraParameters::KEY_FOCUS_AREAS, TEST_FOCUS_AREA);

        if ( hardwareActive )
            camera->setParameters(params.flatten());

        params.remove(CameraParameters::KEY_FOCUS_AREAS);

    case 'f':

        gettimeofday(&autofocus_start, 0);

        if ( hardwareActive )
            camera->autoFocus();

        break;

    case 'p':

        gettimeofday(&picture_start, 0);

        if ( hardwareActive )
            camera->takePicture(CAMERA_MSG_COMPRESSED_IMAGE|CAMERA_MSG_RAW_IMAGE);

        break;

    case '&':
        printf("Enabling Preview Callback");
        dump_preview = 1;
        if ( hardwareActive )
        camera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_ENABLE_MASK);
        break;

   
    case 'a':

        while (1) {
            if ( menu_gps() < 0)
                break;
        };

        break;

    case 'q':

        stopPreview();

        return -1;

    case '/':
    {
        if (showfps)
        {
            property_set("debug.image.showfps", "0");
            showfps = false;
        }
        else
        {
            property_set("debug.image.showfps", "1");
            showfps = true;
        }
        break;
    }

    case '<':
      elockidx += 1;
      elockidx %=GET_SUPPORTED(length_lock);
      params.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, GET_SUPPORTED(lock)[elockidx]);
      if ( hardwareActive )
        camera->setParameters(params.flatten());
      break;

    case '>':
      wblockidx += 1;
      wblockidx %= GET_SUPPORTED(length_lock);
      params.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, GET_SUPPORTED(lock)[wblockidx]);
      if ( hardwareActive )
        camera->setParameters(params.flatten());
      break;

default:
        print_menu = 0;

        break;
    }

    return 0;
}

void print_usage() {
    printf(" USAGE: camera_test  <param>  <script>\n");
    printf(" <param>\n-----------\n\n");
    printf(" F or f -> Functional tests \n");
    printf(" A or a -> API tests \n");
    printf(" E or e -> Error scenario tests \n");
    printf(" S or s -> Stress tests; with syslink trace \n");
    printf(" SN or sn -> Stress tests; No syslink trace \n\n");
    printf(" <script>\n----------\n");
    printf("Script name (Only for stress tests)\n\n");
    return;
}

int error_scenario() {
    char ch;
    status_t stat = NO_ERROR;

    if (print_menu) {
        printf("   0. Buffer need\n");
        printf("   1. Not enough memory\n");
        printf("   2. Media server crash\n");
        printf("   3. Overlay object request\n");
        printf("   4. Pass unsupported preview&picture format\n");
        printf("   5. Pass unsupported preview&picture resolution\n");
        printf("   6. Pass unsupported preview framerate\n");

        printf("   q. Quit\n");
        printf("   Choice: ");
    }

    print_menu = 1;
    ch = getchar();
    printf("%c\n", ch);

    switch (ch) {
        case '0': {
            printf("Case0:Buffer need\n");
            bufferStarvationTest = 1;
            params.set(KEY_BUFF_STARV, bufferStarvationTest); //enable buffer starvation

            if ( !recordingMode ) {
                recordingMode = true;
                if ( startPreview() < 0 ) {
                    printf("Error while starting preview\n");

                    return -1;
                }

                if ( openRecorder() < 0 ) {
                    printf("Error while openning video recorder\n");

                    return -1;
                }

                if ( configureRecorder() < 0 ) {
                    printf("Error while configuring video recorder\n");

                    return -1;
                }

                if ( startRecording() < 0 ) {
                    printf("Error while starting video recording\n");

                    return -1;
                }

            }

            usleep(1000000);//1s

            stopPreview();

            if ( recordingMode ) {
                stopRecording();
                closeRecorder();

                recordingMode = false;
            }

            break;
        }

        case '1': {
            printf("Case1:Not enough memory\n");
            int* tMemoryEater = new int[999999999];

            if (!tMemoryEater) {
                printf("Not enough memory\n");
                return -1;
            } else {
                delete tMemoryEater;
            }

            break;
        }

        case '2': {
            printf("Case2:Media server crash\n");
            //camera = Camera::connect();

            if ( NULL == camera.get() ) {
                printf("Unable to connect to CameraService\n");
                return -1;
            }

            break;
        }

        case '3': {
            printf("Case3:Overlay object request\n");
            int err = 0;

            err = open("/dev/video0", O_RDWR);

            if (err < 0) {
                printf("Could not open the camera device0: %d\n",  err );
                return err;
            }

            if ( startPreview() < 0 ) {
                printf("Error while starting preview\n");
                return -1;
            }

            usleep(1000000);//1s

            stopPreview();

            close(err);
            break;
        }

        case '4': {

            if ( hardwareActive ) {

                params.setPictureFormat("invalid-format");
                params.setPreviewFormat("invalid-format");

                stat = camera->setParameters(params.flatten());

                if ( NO_ERROR != stat ) {
                    printf("Test passed!\n");
                } else {
                    printf("Test failed!\n");
                }

                initDefaults();
            }

            break;
        }

        case '5': {

            if ( hardwareActive ) {

                params.setPictureSize(-1, -1);
                params.setPreviewSize(-1, -1);

                stat = camera->setParameters(params.flatten());

                if ( NO_ERROR != stat ) {
                    printf("Test passed!\n");
                } else {
                    printf("Test failed!\n");
                }

                initDefaults();
            }

            break;
        }

        case '6': {

            if ( hardwareActive ) {

                params.setPreviewFrameRate(-1);

                stat = camera->setParameters(params.flatten());

                if ( NO_ERROR != stat ) {
                    printf("Test passed!\n");
                } else {
                    printf("Test failed!\n");
                }

                initDefaults();
            }


            break;
        }

        case 'q': {
            return -1;
        }

        default: {
            print_menu = 0;
            break;
        }
    }

    return 0;
}

int restartCamera() {

  const char dir_path_name[80] = SDCARD_PATH;

  printf("+++Restarting Camera After Error+++\n");
  stopPreview();

  if (recordingMode) {
    stopRecording();
    closeRecorder();

    recordingMode = false;
  }

  sleep(3); //Wait a bit before restarting

  restartCount++;

  if (strcpy(dir_path, dir_path_name) == NULL)
  {
    printf("Error reseting dir name");
    return -1;
  }

  if ( openCamera() < 0 )
  {
    printf("+++Camera Restarted Failed+++\n");
    system("echo camerahal_test > /sys/power/wake_unlock");
    return -1;
  }

  initDefaults();

  stopScript = false;

  printf("+++Camera Restarted Successfully+++\n");
  return 0;
}

int main(int argc, char *argv[]) {
    char *cmd;
    int pid;
    sp<ProcessState> proc(ProcessState::self());

    unsigned long long st, end, delay;
    timeval current_time;

    init_supported();

    gettimeofday(&current_time, 0);

    st = current_time.tv_sec * 1000000 + current_time.tv_usec;

    cmd = NULL;

    if ( argc < 2 ) {
        printf(" Please enter atleast 1 argument\n");
        print_usage();

        return 0;
    }
    system("echo camerahal_test > /sys/power/wake_lock");
    if ( argc < 3 ) {
        switch (*argv[1]) {
            case 'S':
            case 's':
                printf("This is stress / regression tests \n");
                printf("Provide script file as 2nd argument\n");

                break;

            case 'F':
            case 'f':
                ProcessState::self()->startThreadPool();

                if ( openCamera() < 0 ) {
                    printf("Camera initialization failed\n");
                    system("echo camerahal_test > /sys/power/wake_unlock");
                    return -1;
                }

                initDefaults();
                print_menu = 1;

                while ( 1 ) {
                    if ( functional_menu() < 0 )
                        break;
                };

                break;

            case 'A':
            case 'a':
                printf("API level test cases coming soon ... \n");

                break;

            case 'E':
            case 'e': {
                ProcessState::self()->startThreadPool();

                if ( openCamera() < 0 ) {
                    printf("Camera initialization failed\n");
                    system("echo camerahal_test > /sys/power/wake_unlock");
                    return -1;
                }

                initDefaults();
                print_menu = 1;

                while (1) {
                    if (error_scenario() < 0) {
                        break;
                    }
                }

                break;
            }

            default:
                printf("INVALID OPTION USED\n");
                print_usage();

                break;
        }
    } else if ( ( argc == 3) && ( ( *argv[1] == 'S' ) || ( *argv[1] == 's') ) ) {

        if((argv[1][1] == 'N') || (argv[1][1] == 'n')) {
            bLogSysLinkTrace = false;
        }

        ProcessState::self()->startThreadPool();

        if ( openCamera() < 0 ) {
            printf("Camera initialization failed\n");
            system("echo camerahal_test > /sys/power/wake_unlock");
            return -1;
        }

        initDefaults();

        cmd = load_script(argv[2]);

        if ( cmd != NULL) {
            start_logging(argv[2], pid);
            stressTest = true;

            while (1)
              {
                if ( execute_functional_script(cmd) == 0 )
                  {
                    break;
                  }
                else
                  {
                    printf("CameraTest Restarting Camera...\n");

                    free(cmd);
                    cmd = NULL;

                    if ( (restartCamera() != 0)  || ((cmd = load_script(argv[2])) == NULL) )
                      {
                        printf("ERROR::CameraTest Restarting Camera...\n");
                        break;
                      }
                  }
              }
            free(cmd);
            stop_logging(pid);
        }
    } else if ( ( argc == 3) && ( ( *argv[1] == 'E' ) || ( *argv[1] == 'e') ) ) {

        ProcessState::self()->startThreadPool();

        if ( openCamera() < 0 ) {
            printf("Camera initialization failed\n");
            system("echo camerahal_test > /sys/power/wake_unlock");
            return -1;
        }

        initDefaults();

        cmd = load_script(argv[2]);

        if ( cmd != NULL) {
            start_logging(argv[2], pid);
            execute_error_script(cmd);
            free(cmd);
            stop_logging(pid);
        }

    } else {
        printf("INVALID OPTION USED\n");
        print_usage();
    }

    gettimeofday(&current_time, 0);
    end = current_time.tv_sec * 1000000 + current_time.tv_usec;
    delay = end - st;
    printf("Application clossed after: %llu ms\n", delay);
    system("echo camerahal_test > /sys/power/wake_unlock");
    return 0;
}
