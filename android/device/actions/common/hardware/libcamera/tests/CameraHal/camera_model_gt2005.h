
#ifndef __CAMERA_MODEL_GT2005_H__
#define __CAMERA_MODEL_GT2005_H__

namespace gt2005
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
        { 800, 600, "SVGA" },
        { 1280, 720, "HD" },
    };

    size_t length_previewSize =  ARRAY_SIZE(previewSize);

    Vcapture_size VcaptureSize [] = {
        { 800, 600, "SVGA" },
        { 1280, 720, "HD" },
    };

    size_t length_VcaptureSize = ARRAY_SIZE(VcaptureSize);

    capture_Size captureSize[] = {
        { 800, 600, "SVGA" },
        { 1600, 1200, "2MP" },
        { 2048, 1536, "3MP" },
        { 2560, 1920, "5MP" },
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
camera_supported_t camera_supported_gt2005 = 
{
    (const char **)&gt2005::faceDetection,
    gt2005::length_fd,

    (const char **)&gt2005::lock,
    gt2005::length_lock,

    (const char **)&gt2005::iso,
    gt2005::length_iso,

    (const char **)&gt2005::effects,
    gt2005::length_effects,

    (const char **)&gt2005::flashModes,
    gt2005::length_flashModes,

    (const char **)&gt2005::scene,
    gt2005::length_scene,

    (const char **)&gt2005::strawb_mode,
    gt2005::length_strawb_mode,

    (thumb_size*)&gt2005::thumbSize,
    gt2005::length_thumbSize,

    (preview_size *)&gt2005::previewSize,
    gt2005::length_previewSize,

    (Vcapture_size*)&gt2005::VcaptureSize,
    gt2005::length_VcaptureSize,

    (capture_Size *)&gt2005::captureSize,
    gt2005::length_captureSize,

    (Zoom *)&gt2005::zoom,
    gt2005::length_zoom,

    (fpsConst_Ranges *)&gt2005::fpsConstRanges,
    gt2005::length_fpsConstRanges,

    (const char **)&gt2005::antibanding,
    gt2005::length_antibanding,

    (const char **)&gt2005::focus,
    gt2005::length_focus,

    (pixel_format*)&gt2005::pixelformat,
    gt2005::length_pixelformat,

    (const char **)&gt2005::codingformat,
    gt2005::length_codingformat,

    gt2005::compensationMin,
    gt2005::compensationMax,
    gt2005::compensationStep,

    gt2005::sharpnessMin,
    gt2005::sharpnessMax,  
    gt2005::sharpnessStep,  

    gt2005::contrastMin,
    gt2005::contrastMax,  
    gt2005::contrastStep,  

    gt2005::brightnessMin,
    gt2005::brightnessMax,  
    gt2005::brightnessStep,  

    gt2005::saturationMin,
    gt2005::saturationMax,
    gt2005::saturationStep,

};

void initModelDefaults_gt2005(){

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

int getModelSupported_gt2005(camera_supported_t *camera_supported)
{
    if(camera_supported != NULL)
    {
        memcpy(camera_supported, &camera_supported_gt2005, sizeof(camera_supported_t));
        return 0;
    }
    return -1;
}



#endif

