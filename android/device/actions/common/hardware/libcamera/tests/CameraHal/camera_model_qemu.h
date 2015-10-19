
#ifndef __CAMERA_MODEL_OV2643_H__
#define __CAMERA_MODEL_OV2643_H__


const char *cameras[] = {"Primary Camera"};
size_t length_cameras =  ARRAY_SIZE(cameras);

const char *faceDetection[] = {"disable"};
size_t length_fd =  ARRAY_SIZE(faceDetection);

const char *lock[] = {"false"};
size_t length_lock =  ARRAY_SIZE(lock);

const char *iso [] = { "auto"};
size_t length_iso =  ARRAY_SIZE(iso);

const char *effects [] = {
    "none",
  
};
size_t length_effects =  ARRAY_SIZE(effects);

const char *flashModes[] = {
    "off",
   
};
size_t length_flashModes =  ARRAY_SIZE(flashModes);


const char *scene [] = {
    "auto",
  
};
size_t length_scene =  ARRAY_SIZE(scene);


const char *strawb_mode[] = {
    "auto",
   
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
  { 1280, 720, "1MP" },
};

size_t length_captureSize = ARRAY_SIZE(captureSize);


outformat outputFormat[] = {
        { OUTPUT_FORMAT_THREE_GPP, "3gp" },
        { OUTPUT_FORMAT_MPEG_4, "mp4" },
    };

size_t length_outputFormat = ARRAY_SIZE(outputFormat);

video_Codecs videoCodecs[] = {
  { VIDEO_ENCODER_H263, "H263" },
  { VIDEO_ENCODER_H264, "H264" },
  { VIDEO_ENCODER_MPEG_4_SP, "MPEG4"}
};

size_t length_videoCodecs = ARRAY_SIZE(videoCodecs);

audio_Codecs audioCodecs[] = {
  { AUDIO_ENCODER_AMR_NB, "AMR_NB" },
  { AUDIO_ENCODER_AMR_WB, "AMR_WB" },
  { AUDIO_ENCODER_AAC, "AAC" },
  { AUDIO_ENCODER_HE_AAC, "AAC+" },
  { AUDIO_ENCODER_LIST_END, "disabled"},
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

Zoom zoom[] = {
  { 0,  "1x"  },
  { 12, "1.5x"},
  { 20, "2x"  },
  { 27, "2.5x"},
  { 32, "3x"  },
  { 36, "3.5x"},
  { 40, "4x"  },
  { 60, "8x"  },
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

fpsConst_RangesSec fpsConstRangesSec[] = {
  { "5000,5000", "[5:5]", 5 },
  { "10000,10000", "[10:10]", 10 },
  { "15000,15000", "[15:15]", 15 },
  { "20000,20000", "[20:20]", 20 },
  { "25000,25000", "[25:25]", 25 },
  { "30000,30000", "[30:30]", 30 },
};

size_t length_fpsConstRangesSec = ARRAY_SIZE(fpsConstRangesSec);

const char *antibanding[] = {
    "off",
    "auto",
    "50hz",
    "60hz",
};

size_t length_antibanding = ARRAY_SIZE(antibanding);

const char *focus[] = {
    "infinity",
};
size_t length_focus = ARRAY_SIZE(focus);

pixel_format pixelformat[] = {
  { HAL_PIXEL_FORMAT_RGB_565, CameraParameters::PIXEL_FORMAT_RGB565 },
  };
size_t length_pixelformat = ARRAY_SIZE(pixelformat);

const char *codingformat[] = {"rgb565", "jpeg"};
size_t length_codingformat = ARRAY_SIZE(codingformat);


int compensationMin = -6;
int compensationMax = 6;


int sharpnessMin = -6;
int sharpnessMax = 6;  

int contrastMin = -6;
int contrastMax = 6;  

int brightnessMin = -6;
int brightnessMax = 6;  

int saturationMin = -6;
int saturationMax = 6;  

void initModelDefaults(){

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

#endif

