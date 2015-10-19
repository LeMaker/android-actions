
#ifndef __CAMERA_MODEL_GC0308_H__
#define __CAMERA_MODEL_GC0308_H__

namespace gc0308
{
    const char *faceDetection[] = {"disable"};
    size_t length_fd =  ARRAY_SIZE(faceDetection);

    const char *lock[] = {"false"};
    size_t length_lock =  ARRAY_SIZE(lock);

    const char *iso [] = { 
        "auto",
    };
    size_t length_iso =  ARRAY_SIZE(iso);

    const char *effects [] = {
        "none",
        "mono",
        "negative",
        "solarize",
        "sepia",

    };
    size_t length_effects =  ARRAY_SIZE(effects);

    const char *flashModes[] = {
        "off",
    };

    size_t length_flashModes =  ARRAY_SIZE(flashModes);


    const char *scene [] = {
        "auto",
        "landscape",
        "night",

    };
    size_t length_scene =  ARRAY_SIZE(scene);


    const char *strawb_mode[] = {
        "auto",
        "incandescent",
        "fluorescent",
        "daylight",
        "cloudy-daylight",

    };
    size_t length_strawb_mode=  ARRAY_SIZE(strawb_mode);



    thumb_size thumbSize [] = {
        { 128, 96, "SQCIF" },
    };

    size_t length_thumbSize =  ARRAY_SIZE(thumbSize);

    preview_size previewSize [] = {
        { 320, 240, "QVGA" },
        { 640, 480, "VGA" },
    };

    size_t length_previewSize =  ARRAY_SIZE(previewSize);

    Vcapture_size VcaptureSize [] = {
        { 320, 240, "QVGA" },
        { 640, 480, "VGA" },
    };

    size_t length_VcaptureSize = ARRAY_SIZE(VcaptureSize);

    capture_Size captureSize[] = {
        { 320, 240, "QVGA" },
        { 640, 480, "VGA" },
    };

    size_t length_captureSize = ARRAY_SIZE(captureSize);


    Zoom zoom[] = {
        { 0,  "1x"  },
        { 12, "1.5x"},
        { 20, "2x"  },
        { 27, "2.5x"},
        { 32, "3x"  },
        { 36, "3.5x"},
        { 40, "4x"  },
    };

    size_t length_zoom = ARRAY_SIZE(zoom);


    fpsConst_Ranges fpsConstRanges[] = {
        /*
           { "5000,5000", "[5:5]", 5 },
           { "10000,10000", "[10:10]", 10 },
           { "15000,15000", "[15:15]", 15 },
           { "20000,20000", "[20:20]", 20 },
           { "25000,25000", "[25:25]", 25 },
           */
        { "5000,30000", "[5:30]", 30 },
    };

    size_t length_fpsConstRanges = ARRAY_SIZE(fpsConstRanges);

    const char *antibanding[] = {
        "off",
    };

    size_t length_antibanding = ARRAY_SIZE(antibanding);

    const char *focus[] = {
        "auto",
        "infinity",
    };
    size_t length_focus = ARRAY_SIZE(focus);

    pixel_format pixelformat[] = {
        { HAL_PIXEL_FORMAT_YCrCb_420_SP, CameraParameters::PIXEL_FORMAT_YUV420SP },
    };
    size_t length_pixelformat = ARRAY_SIZE(pixelformat);

    const char *codingformat[] = {"yuv420sp", "jpeg"};
    size_t length_codingformat = ARRAY_SIZE(codingformat);


    int compensationMin = -6;
    int compensationMax = 6;
    int compensationStep = 1;


    int sharpnessMin = -6;
    int sharpnessMax = 6;  
    int sharpnessStep = 1;  

    int contrastMin = -6;
    int contrastMax = 6;  
    int contrastStep = 1;  

    int brightnessMin = -6;
    int brightnessMax = 6;  
    int brightnessStep = 1;  

    int saturationMin = -6;
    int saturationMax = 6;
    int saturationStep = 1;

};
camera_supported_t camera_supported_gc0308 =
{
    (const char **)&gc0308::faceDetection,
    gc0308::length_fd,

    (const char **)&gc0308::lock,
    gc0308::length_lock,

    (const char **)&gc0308::iso,
    gc0308::length_iso,

    (const char **)&gc0308::effects,
    gc0308::length_effects,

    (const char **)&gc0308::flashModes,
    gc0308::length_flashModes,

    (const char **)&gc0308::scene,
    gc0308::length_scene,

    (const char **)&gc0308::strawb_mode,
    gc0308::length_strawb_mode,

    (thumb_size*)&gc0308::thumbSize,
    gc0308::length_thumbSize,

    (preview_size *)&gc0308::previewSize,
    gc0308::length_previewSize,

    (Vcapture_size*)&gc0308::VcaptureSize,
    gc0308::length_VcaptureSize,

    (capture_Size *)&gc0308::captureSize,
    gc0308::length_captureSize,

    (Zoom *)&gc0308::zoom,
    gc0308::length_zoom,

    (fpsConst_Ranges *)&gc0308::fpsConstRanges,
    gc0308::length_fpsConstRanges,

    (const char **)&gc0308::antibanding,
    gc0308::length_antibanding,

    (const char **)&gc0308::focus,
    gc0308::length_focus,

    (pixel_format*)&gc0308::pixelformat,
    gc0308::length_pixelformat,

    (const char **)&gc0308::codingformat,
    gc0308::length_codingformat,

    gc0308::compensationMin,
    gc0308::compensationMax,
    gc0308::compensationStep,

    gc0308::sharpnessMin,
    gc0308::sharpnessMax,  
    gc0308::sharpnessStep,  

    gc0308::contrastMin,
    gc0308::contrastMax,  
    gc0308::contrastStep,  

    gc0308::brightnessMin,
    gc0308::brightnessMax,  
    gc0308::brightnessStep,  

    gc0308::saturationMin,
    gc0308::saturationMax,
    gc0308::saturationStep,

};

void initModelDefaults_gc0308(){

    camera_index = 0;
    antibanding_mode = 0;
    focus_mode = 0;
    previewSizeIDX = 0;  
    captureSizeIDX = 0; 
    frameRateIDX = 0;     
    VcaptureSizeIDX = 0;
    VbitRateIDX = 0;        
    thumbSizeIDX = 0;
    compensation = 0;
    awb_mode = 0;
    effects_mode = 0;
    scene_mode = 0;
    flashIdx = 0;
    rotation = 0;
    zoomIDX = 0;
    videoCodecIDX = 0;

    contrast = 0;
    brightness = 0;
    sharpness = 0;
    saturation = 0;

    iso_mode = 0;
    jpegQuality = 85;
    bufferStarvationTest = 0;
    previewFormat = 0;
    pictureFormat = 1;
}

int getModelSupported_gc0308(camera_supported_t *camera_supported)
{
    if(camera_supported != NULL)
    {
        memcpy(camera_supported, &camera_supported_gc0308, sizeof(camera_supported_t));
        return 0;
    }
    return -1;
}



#endif

