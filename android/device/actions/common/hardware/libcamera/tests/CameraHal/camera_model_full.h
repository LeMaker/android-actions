
#ifndef __CAMERA_MODEL_FULL_H__
#define __CAMERA_MODEL_FULL_H__

namespace androidfull
{
    const char *faceDetection[] = {"disable", "enable"};
    size_t length_fd =  ARRAY_SIZE(faceDetection);

    const char *lock[] = {"false", "true"};
    size_t length_lock =  ARRAY_SIZE(lock);

    const char *iso [] = { "auto", "100", "200", "400", "800", "1200", "1600"};
    size_t length_iso =  ARRAY_SIZE(faceDetection);

    const char *effects [] = {
        "none",
        "mono",
        "negative",
        "solarize",
        "sepia",
        "posterize",
        "whiteboard",
        "blackboard",
        "aqua",
    };
    size_t length_effects =  ARRAY_SIZE(effects);

    const char *flashModes[] = {
        "off",
        "auto",
        "on",
        "red-eye",
        "torch",
    };
    size_t length_flashModes =  ARRAY_SIZE(flashModes);


    const char *scene [] = {
        "auto",
        "ation",
        "portrait",
        "landscape",
        "night",
        "night-portrait",
        "theatre",
        "beach",
        "snow",
        "sunset",
        "steadyphoto",
        "fireworks",
        "sports",
        "party",
        "candlelight",
        "barcode",
    };
    size_t length_scene =  ARRAY_SIZE(scene);


    const char *strawb_mode[] = {
        "auto",
        "incandescent",
        "fluorescent",
        "daylight",
        "horizon",
        "shadow",
        "tungsten",
        "shade",
        "twilight",
        "warm-fluorescent",
        "facepriority",
        "sunset"
    };
    size_t length_strawb_mode=  ARRAY_SIZE(strawb_mode);



    thumb_size thumbSize [] = {
        { 128, 96, "SQCIF" },
    };

    size_t length_thumbSize =  ARRAY_SIZE(thumbSize);

    preview_size previewSize [] = {
        { 128, 96, "SQCIF" },
        { 176, 144, "QCIF" },
        { 320, 240, "QVGA" },
        { 352, 288, "CIF" },
        { 640, 480, "VGA" },
        { 720, 480, "NTSC" },
        { 720, 576, "PAL" },
        { 800, 480, "WVGA" },
        { 800, 600, "SVGA" },
        { 1280, 720, "HD" },
        { 1600, 1200, "UXGA" },
        { 1920, 1080, "FULLHD"},
    };

    size_t length_previewSize =  ARRAY_SIZE(previewSize);

    Vcapture_size VcaptureSize [] = {
        { 128, 96, "SQCIF" },
        { 176, 144, "QCIF" },
        { 352, 288, "CIF" },
        { 320, 240, "QVGA" },
        { 640, 480, "VGA" },
        { 704, 480, "TVNTSC" },
        { 704, 576, "TVPAL" },
        { 720, 480, "D1NTSC" },
        { 720, 576, "D1PAL" },
        { 800, 480, "WVGA" },
        { 800, 600, "SVGA" },
        { 1280, 720, "HD" },
        { 1600, 1200,  "UXGA" },
        { 1920, 1080, "FULLHD"},
    };

    size_t length_VcaptureSize = ARRAY_SIZE(VcaptureSize);

    capture_Size captureSize[] = {
        {  320, 240,  "QVGA" },
        {  640, 480,  "VGA" },
        {  800, 600,  "SVGA" },
        { 1152, 864,  "1MP" },
        { 1280, 1024, "1.3MP" },
        { 1600, 1200,  "2MP" },
        { 2048, 1536,  "3MP" },
        { 2592, 1944,  "5MP" },
        { 2608, 1960,  "5MP" },
        { 3264, 2448,  "8MP" },
        { 3648, 2736, "10MP"},
        { 4032, 3024, "12MP"},
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
        { "5000,5000", "[5:5]", 5 },
        { "10000,10000", "[10:10]", 10 },
        { "15000,15000", "[15:15]", 15 },
        { "20000,20000", "[20:20]", 20 },
        { "25000,25000", "[25:25]", 25 },
        { "30000,30000", "[30:30]", 30 },
    };

    size_t length_fpsConstRanges = ARRAY_SIZE(fpsConstRanges);

    const char *antibanding[] = {
        "off",
        "auto",
        "50hz",
        "60hz",
    };

    size_t length_antibanding = ARRAY_SIZE(antibanding);

    const char *focus[] = {
        "auto",
        "infinity",
        "macro",
        "fixed",
        "edof",
        "continuous-video",
        "continuous-picture",
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
camera_supported_t camera_supported_androidfull = 
{
    (const char **)&androidfull::faceDetection,
    androidfull::length_fd,

    (const char **)&androidfull::lock,
    androidfull::length_lock,

    (const char **)&androidfull::iso,
    androidfull::length_iso,

    (const char **)&androidfull::effects,
    androidfull::length_effects,

    (const char **)&androidfull::flashModes,
    androidfull::length_flashModes,

    (const char **)&androidfull::scene,
    androidfull::length_scene,

    (const char **)&androidfull::strawb_mode,
    androidfull::length_strawb_mode,

    (thumb_size*)&androidfull::thumbSize,
    androidfull::length_thumbSize,

    (preview_size *)&androidfull::previewSize,
    androidfull::length_previewSize,

    (Vcapture_size*)&androidfull::VcaptureSize,
    androidfull::length_VcaptureSize,

    (capture_Size *)&androidfull::captureSize,
    androidfull::length_captureSize,

    (Zoom *)&androidfull::zoom,
    androidfull::length_zoom,

    (fpsConst_Ranges *)&androidfull::fpsConstRanges,
    androidfull::length_fpsConstRanges,

    (const char **)&androidfull::antibanding,
    androidfull::length_antibanding,

    (const char **)&androidfull::focus,
    androidfull::length_focus,

    (pixel_format*)&androidfull::pixelformat,
    androidfull::length_pixelformat,

    (const char **)&androidfull::codingformat,
    androidfull::length_codingformat,

    androidfull::compensationMin,
    androidfull::compensationMax,
    androidfull::compensationStep,

    androidfull::sharpnessMin,
    androidfull::sharpnessMax,  
    androidfull::sharpnessStep,  

    androidfull::contrastMin,
    androidfull::contrastMax,  
    androidfull::contrastStep,  

    androidfull::brightnessMin,
    androidfull::brightnessMax,  
    androidfull::brightnessStep,  

    androidfull::saturationMin,
    androidfull::saturationMax,
    androidfull::saturationStep,

};

void initModelDefaults_androidfull(){

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

int getModelSupported_androidfull(camera_supported_t *camera_supported)
{
    if(camera_supported != NULL)
    {
        memcpy(camera_supported, &camera_supported_androidfull, sizeof(camera_supported_t));
        return 0;
    }
    return -1;
}



#endif

