/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_HDMI_INTERFACE_H
#define ANDROID_HDMI_INTERFACE_H

#include <hardware/hardware.h>

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>


#define HDMI_HARDWARE_DEVICE "/dev/hdmi"

/**
 * The id of this module
 */
#define HDMI_HARDWARE_MODULE_ID "hdmi"
#define HDMI_HARDWARE_HDMI0		"hdmi0"

#if 0
//typedef struct Audio_Settings hdmi_aud_t;
struct hdmi_aud_t{
    unsigned int audio_enable;
    unsigned int  sample_rate;
    unsigned int  sample_precision;
    unsigned int channel_num;
};



//typedef struct Video_Settings hdmi_vid_t;
struct hdmi_vid_t{
    int color_space;
    int video_id_code;
};


//typedef struct sink_capabilities sink_caps_t;
struct sink_caps_t{
    int max_channel_num;
    int sample_rate;
    int speaker_info;
    int color_space;
    int video_id_code[2];
}
#endif

 struct sink_capabilities_t{
	unsigned int hdmi_mode;
	/*
	 * audio capabilites
	 * for now(20090817), only Linear PCM(IEC60958) considered
	 */

	/*
	 * maximum audio channel number
	 * it should be (<=8)
	 */
	unsigned int max_channel_cap;

	/*
	 * audio sampling rate
	 *
	 * for each bit, if the value is 1 one sampling rate is supported. if 0, not supported.
	 * bit0: when the value is 1, 32kHz    is supported. when 0, 32kHz    is not supported.
	 * bit1: when the value is 1, 44.1kHz  is supported. when 0, 44.1kHz  is not supported.
	 * bit2: when the value is 1, 48kHz    is supported. when 0, 48kHz    is not supported.
	 * bit3: when the value is 1, 88.2kHz  is supported. when 0, 88.2kHz  is not supported.
	 * bit4: when the value is 1, 96kHz    is supported. when 0, 96kHz    is not supported.
	 * bit5: when the value is 1, 176.4kHz is supported. when 0, 176.4kHz is not supported.
	 * bit6: when the value is 1, 192kHz   is supported. when 0, 192kHz   is not supported.
	 * bit7~31: reserved
	 */
	unsigned int sampling_rate_cap;

	/*
	 * audio sample size
	 *
	 * for each bit, if the value is 1 one sampling size is supported. if 0, not supported.
	 * bit0: when the value is 1, 16-bit is supported. when 0, 16-bit is not supported.
	 * bit1: when the value is 1, 20-bit is supported. when 0, 20-bit is not supported.
	 * bit2: when the value is 1, 24-bit is supported. when 0, 24-bit is not supported.
	 * bit3~31: reserved
	 */
	unsigned int sampling_size_cap;

	/*
	 * speaker allocation information
	 *
	 * bit0: when the value is 1, FL/FR   is supported. when 0, FL/FR   is not supported.
	 * bit1: when the value is 1, LFE     is supported. when 0, LFE     is not supported.
	 * bit2: when the value is 1, FC      is supported. when 0, FC      is not supported.
	 * bit3: when the value is 1, RL/RR   is supported. when 0, RL/RR   is not supported.
	 * bit4: when the value is 1, RC      is supported. when 0, RC      is not supported.
	 * bit5: when the value is 1, FLC/FRC is supported. when 0, FLC/FRC is not supported.
	 * bit6: when the value is 1, RLC/RRC is supported. when 0, RLC/RRC is not supported.
	 * bit7~31: reserved
	 *
	 * NOTICE:
	 *      FL/FR, RL/RR, FLC/FRC, RLC/RRC should be presented in pairs.
	 */
	unsigned int speader_allo_info;

	/*
	 * video capabilites
	 */

	/*
	 * pixel encoding (byte3(starting from 0) of CEA Extension Version3)
	 *	  Only pixel encodings of RGB, YCBCR4:2:2, and YCBCR4:4:4 may be used on HDMI.
	 *	  All HDMI Sources and Sinks shall be capable of supporting RGB pixel encoding.
	 *	  If an HDMI Sink supports either YCBCR4:2:2 or YCBCR4:4:4 then both shall be supported.
	 *	  An HDMI Source may determine the pixel-encodings that are supported by the Sink through
	 *		  the use of the E-EDID. If the Sink indicates that it supports YCBCR-formatted video
	 *		  data and if the Source can deliver YCBCR data, then it can enable the transfer of
	 *		  this data across the link.
	 *
	 * bit0: when the value is 1, RGB   is supported. when 0, RGB   is not supported.
	 * bit1: when the value is 1, YCBCR4:4:4 is supported. when 0, YCBCR4:4:4 is not supported.
	 * bit2: when the value is 1, YCBCR4:2:2 is supported. when 0, YCBCR4:2:2 is not supported.
	 * bit3~31: reserved
	 */
	unsigned int pixel_encoding;

	/*
	 * video formats
	 *
	 *  the value of each bit indicates whether one video format is supported by sink device.
	 *  video format ID can be found in enum VIDEO_ID_TABLE.
	 *  the bit postion corresponds to the video format ID in enum VIDEO_ID_TABLE. For example,
	 *  bit  0 of video_formats[0] corresponds to VID640x480P_60_4VS3
	 *  bit 31 of video_formats[0] corresponds to VID1920x1080P_24_16VS9
	 *  bit  0 of video_formats[1] corresponds to VID1920x1080P_25_16VS9
	 *  bit 21 of video_formats[2] corresponds to VID1920x1080P_59P94_16VS9
	 *  when it is 1, the video format is supported, when 0, the video format is not supported.
	 *
	 */
	unsigned int video_formats[4];
};

typedef struct {
    unsigned int vid;
    unsigned int width;      /* width for each video format */
    unsigned int height;     /* height for each video format */
    unsigned int Is_progressive;
} video_setting_t;




/**
 * Every hardware module must have a data structure named HAL_MODULE_INFO_SYM
 * and the fields of this data structure must begin with hw_module_t
 * followed by module specific information.
 */
struct hdmi_module_t {
    struct hw_module_t common;
};

/**
 * Every device data structure must begin with hw_device_t
 * followed by module specific public methods and attributes.
 */
struct hdmi_device_t {
    struct hw_device_t common;
    struct sink_capabilities_t sink_cap;

    int (*enable)(struct hdmi_device_t *dev);
    int (*disable)(struct hdmi_device_t *dev);
    int (*get_capability)(struct hdmi_device_t *dev, char *sink_cap);
    int (*resolve_config)(struct hdmi_device_t *dev, 				/* write back to Hdmi */ 
    			video_setting_t	*vid_setting    /* for DE output */    
    			);

    //int (*set_video_config)(struct hdmi_device_t *dev, hdmi_vid_t *vid_in);
    //int (*set_audio)(struct hdmi_device_t *dev, hdmi_aud_t *aud_in, int v);
    int (*is_connected)(struct hdmi_device_t *dev);
    int (*set_mode)(struct hdmi_device_t *dev, int width, int height, float hz,int isprogressive, int aspect_ratio);
    int (*set_vid)(struct hdmi_device_t *dev, int vid);
};


/** convenience API for opening and closing a device */

static inline int hdmi_dev_open(const struct hw_module_t* module, 
        struct hdmi_device_t** device) {
    return module->methods->open(module, 
            HDMI_HARDWARE_HDMI0, (struct hw_device_t**)device);
}

static inline int hdmi_dev_close(struct hdmi_device_t* device) {
    return device->common.close(&device->common);
}


//__END_DECLS

#endif  // ANDROID_HDMI_INTERFACE_H

