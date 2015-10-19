/*
 * Copyright (C) 2011 The Android Open Source Project
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

#define LOG_TAG "audio_hw_default"
//#define LOG_NDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <stdlib.h>

#include <tinyalsa/asoundlib.h>
#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>

#include <actionsconfig.h>

#include <cutils/properties.h>


//#define  DUMP_READIN_PCM	
//#define  DUMP_OUTWRITE_PCM	

#define ON	1
#define OFF	0
#define MAX_VALUE_NUM 2

/* 1024 frames per normal period */
#define NORMAL_PERIOD_SIZE	1024
#define NORMAL_PERIOD_COUNT	9
#define LONG_PERIOD_SIZE	2048
#define LONG_PERIOD_COUNT	5

#define DEFAULT_CARD	0
#define DEFAULT_PORT	0
#define HDMI_PORT	1

#define IN_MODE_SINGLE_END	"Single ended"
#define IN_MODE_DIFFERENTIAL	"Differential"

struct actions_stream_out;
struct actions_stream_in;

struct pcm_config pcm_config_normal_playback = {
	.channels = 2,
	.rate = 44100,
	.period_size = NORMAL_PERIOD_SIZE,
	.period_count = NORMAL_PERIOD_COUNT,
	.start_threshold = NORMAL_PERIOD_SIZE,
	.stop_threshold = NORMAL_PERIOD_SIZE * NORMAL_PERIOD_COUNT,
	.format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_long_playback = {
	.channels = 2,
	.rate = 44100,
	.period_size = LONG_PERIOD_SIZE,
	.period_count = LONG_PERIOD_COUNT,
	.start_threshold = LONG_PERIOD_SIZE,
	.stop_threshold = LONG_PERIOD_SIZE * LONG_PERIOD_COUNT,
	.format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_normal_capture = {
	.channels = 2,
	.rate = 44100,
	.period_size = NORMAL_PERIOD_SIZE,
	.period_count = NORMAL_PERIOD_COUNT,
	.format = PCM_FORMAT_S16_LE,
};

struct actions_audio_device {
	struct audio_hw_device device;

	pthread_mutex_t lock;
	struct mixer *mixer;
	struct actions_stream_out *active_output;
	struct actions_stream_in *active_input;
	int mode;
	int devices;
	int headphone_on;
	int speaker_on;
    	bool mic_mute;
    	int is_mono_output;
    	int is_reverse_earphone_channels;
};

struct actions_stream_out {
	struct audio_stream_out stream;

	pthread_mutex_t lock; 
	struct pcm_config config;
	unsigned int channel_mask;
	struct pcm *pcm;
	struct actions_audio_device *dev;
	int *buffer;
	int standby;
	//ActionsCode(author:yuchen, new_code): this timestamp to record written frames 	
	int64_t timestamp;
};

struct actions_stream_in {
	struct audio_stream_in stream;

	pthread_mutex_t lock; 
	struct pcm_config config;
	unsigned int channel_mask;
	struct pcm *pcm;
	struct actions_audio_device *dev;
	char *buffer;
	int standby;
    int drop_frames;
    //int frames_droped;
    int drop_count;
};

enum {
	BST_DB_0,
	BST_DB_20,
	BST_DB_24,
	BST_DB_30,
	BST_DB_35,
	BST_DB_40,
	BST_DB_44,
	BST_DB_50,
	BST_DB_52,
};

/* define the volume range of headphone and speaker */
enum {
	HPSP_VOLUME_DB_N_46P5,
	HPSP_VOLUME_DB_N_34P5 = 8,
	HPSP_VOLUME_DB_N_22P5 = 16,
	HPSP_VOLUME_DB_N_10P5 = 24,
	HPSP_VOLUME_DB_1P5 = 32,
	HPSP_VOLUME_DB_4P5 = 34,
	HPSP_VOLUME_DB_7P5 = 36,
	HPSP_VOLUME_DB_10P5 = 38,
	HPSP_VOLUME_DB_12 = 39,
};

struct ele_ctl {
	char *name;
	char *str_val;
	int val;
};

struct ele_ctl atv5302_speaker_playback_arr[] = {
    {"AOUT FL FR Mixer FL FR Switch",NULL,ON},
    {"DAC PA Volume",NULL,0x28},
    {"DAC FL Gain",NULL,0xaa},
    {"DAC FR Gain",NULL,0xaa},
    {"DAC PA Switch",NULL,ON},
    {"DAC PA OUTPUT Stage Switch",NULL,ON},
    {"DAC Digital FL FR Switch",NULL,ON},
    //{"PA Output Swing Mux","Vpp2.4",0},
    {"PA Output Swing Mux","Vpp1.6",1},
    {"speaker on off switch",NULL,ON},
};

struct ele_ctl atv5302_headphone_playback_arr[] = {
    {"speaker on off switch",NULL,OFF},
    {"AOUT FL FR Mixer FL FR Switch",NULL,ON},
    {"DAC PA Volume",NULL,0x28},
    {"DAC FL Gain",NULL,0xb5},
    {"DAC FR Gain",NULL,0xb5},
    {"DAC PA Switch",NULL,ON},
    {"DAC PA OUTPUT Stage Switch",NULL,ON},
    {"DAC Digital FL FR Switch",NULL,ON},
    {"PA Output Swing Mux","Vpp1.6",1},
};

struct ele_ctl atv5302_playback_off_arr[] = {
    {"speaker on off switch",NULL,OFF},
    {"DAC FL FR PLAYBACK Switch",NULL,OFF},
    {"DAC PA Volume",NULL,0x00},
    {"DAC FL Gain",NULL,0x00},
    {"DAC FR Gain",NULL,0x00},
};

struct ele_ctl atv5302_hdmi_playback_arr[] = {
    {"audio output mode switch","hdmi",0},
};

struct ele_ctl atv5302_i2s_playback_arr[] = {
    {"audio output mode switch","i2s",0},
};

struct ele_ctl atv5302_capture_arr[] = {
	{"AOUT FL FR Mixer MIC Switch",NULL,OFF},
	{"AOUT FL FR Mixer FM Switch",NULL,OFF},
	{"ADC0 Mux", "MIC0", 0},
	{"Adc0 Gain", NULL, 0x7},
	{"ADC0 Digital Gain control", NULL, 3},
	{"AMP1 Gain boost Range select", NULL,7},
	{"Mic0 Mode Mux", "Differential", 0},//diff//single mode
	{"Internal Mic Power Switch",NULL,ON},
	{"External Mic Power Switch", NULL, ON},
	{"External MIC Power Voltage", NULL, 1},
};

static int capture_can_operate_vmic = 0;
struct ele_ctl atv5302_capture_off_arr_without_vmic[] = { 
	{"AOUT FL FR Mixer MIC Switch",NULL,OFF},
	{"AOUT FL FR Mixer FM Switch",NULL,OFF},
	{"Internal Mic Power Switch",NULL,OFF},
//	{"External Mic Power Switch", NULL, OFF},
};

struct ele_ctl atv5302_capture_off_arr_with_vmic[] = { 
	{"AOUT FL FR Mixer MIC Switch",NULL,OFF},
	{"AOUT FL FR Mixer FM Switch",NULL,OFF},
	{"Internal Mic Power Switch",NULL,OFF},
	{"External Mic Power Switch", NULL, OFF},
};


struct ele_ctl atv5302_volume_muted_arr[] = {
    {"speaker on off switch",NULL,OFF},
};

struct ele_ctl atv5302_volume_no_mute_arr[] = {
    {"speaker on off switch",NULL,ON},
};

static void tinymix_set_value_by_name(struct mixer *mixer, const char * name, int val,
                              const char *str_val)
{
    struct mixer_ctl *ctl;
    enum mixer_ctl_type type;
    unsigned int num_values;
    unsigned int i;

    ctl = mixer_get_ctl_by_name(mixer, name);
    type = mixer_ctl_get_type(ctl);
    num_values = mixer_ctl_get_num_values(ctl);

    if (str_val == NULL) {
        for (i = 0; i < num_values; i++) {
            if (mixer_ctl_set_value(ctl, i, val)) {
                ALOGE("invalid value\n");
                return;
            }
        }
    } else {
        if (type == MIXER_CTL_TYPE_ENUM) {
            //20141202 change_code by yuchen: mic mode read from dts config instead of fixed in code.
            if (strcmp(name, "Dummy mic mode")==0)
            {
            	if(val == 1)
            	{
            	    if(mixer_ctl_set_enum_by_string(ctl, "Differential"))
            	    {
            	        ALOGE("invalid enum value Differential\n");
            	    }
            	    return;
            	}
            	else if(val == 2)
            	{
            	    if(mixer_ctl_set_enum_by_string(ctl, "Single ended"))
            	    {
            	    	ALOGE("invalid enum value Single ended\n");
            	    }
            	    return;
            	}
            }	
        	
            if (mixer_ctl_set_enum_by_string(ctl, str_val))
                ALOGE("invalid enum value str_val=%s\n",str_val);
        } else {
            ALOGE("only enum types can be set with strings str_val=%s\n",str_val);
        }
    }
}

static int tinymix_ctrl_set(struct mixer *mixer, struct ele_ctl * ctls, int count)
{
	int i;
	
	for (i=0; i<count; i++) {
		//ALOGW("ctls[i] %s, %d", ctls[i].name, ctls[i].val);
		tinymix_set_value_by_name(mixer, ctls[i].name
				, ctls[i].val
				, ctls[i].str_val);
    }

	return 0;
}


static int tinymix_ctrl_get(struct mixer *mixer, char *name, int *value)
{
    struct mixer_ctl *ctl;
    int num_values;
    int i;

    ctl = mixer_get_ctl_by_name(mixer, name);
    num_values = mixer_ctl_get_num_values(ctl);
    if (num_values <= MAX_VALUE_NUM) {
        for (i = 0; i < num_values; i++) {
            value[i] = mixer_ctl_get_value(ctl, i);
        }
    }
    else {
        ALOGE("%s too much values，num_values=%d", __FUNCTION__, num_values);
    }
    return num_values;

}

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
	return 44100;
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
	return 0; 
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
	struct actions_stream_out *out = (struct actions_stream_out *)stream;	
	return out->config.period_size * audio_stream_frame_size((struct audio_stream *)stream);
}

static uint32_t out_get_channels(const struct audio_stream *stream)
{
	return AUDIO_CHANNEL_OUT_STEREO;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
	return AUDIO_FORMAT_PCM_16_BIT;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
	return 0;
}

static int out_standby(struct audio_stream *stream)
{
	struct actions_stream_out *out = (struct actions_stream_out *)stream;
	struct actions_audio_device *adev = out->dev;

	ALOGD("out_standby!!");
	adev->active_output = out;
	
	//if (out->standby == 0) {
        tinymix_ctrl_set(adev->mixer, atv5302_playback_off_arr, 
                sizeof(atv5302_playback_off_arr)/sizeof(atv5302_playback_off_arr[0]));

        adev->speaker_on = 0;
        adev->headphone_on = 0;

        if (out->pcm){
            pcm_close(out->pcm);
            out->pcm = NULL;
        }
		out->standby = 1;
	//}
	return 0;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
	return 0;
}

static void select_output_device(struct actions_audio_device *adev)
{
    int headphone_on = 0;
    int speaker_on = 0;
    struct actions_stream_out* stream = adev->active_output;

    headphone_on = adev->devices & AUDIO_DEVICE_OUT_WIRED_HEADSET;
    headphone_on = headphone_on | (adev->devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE);
    speaker_on = adev->devices & AUDIO_DEVICE_OUT_SPEAKER;

    // Switch output devices when sound is playing
    // standy first. to fix hdmi bug


    if ((adev->devices & AUDIO_DEVICE_OUT_AUX_DIGITAL) == AUDIO_DEVICE_OUT_AUX_DIGITAL) { // if hdmi device

            if (stream) {
                if (stream->standby == 0) {
                    out_standby(&(stream->stream.common));
                }
            }
    
            tinymix_ctrl_set(adev->mixer, atv5302_playback_off_arr, 
                    sizeof(atv5302_playback_off_arr)/sizeof(atv5302_playback_off_arr[0])); // turn off speaker and headphone
            
            tinymix_ctrl_set(adev->mixer, atv5302_hdmi_playback_arr, 
                    sizeof(atv5302_hdmi_playback_arr)/sizeof(atv5302_hdmi_playback_arr[0]));
            ALOGD("%s: hdmi on",__FUNCTION__);

            adev->speaker_on = 0;
            adev->headphone_on = 0;

    }
    else {
    
        //ALOGE("%s: i2s on",__FUNCTION__);
        ALOGE("headphone_on is %d", headphone_on);
        ALOGE("speaker_on is %d", speaker_on);

        if ((adev->speaker_on != speaker_on)&&((adev->headphone_on != headphone_on)||(headphone_on == 0))) {
            if (stream) {
                if (stream->standby == 0) {
                	//ALOGE("%s %d standby", __FUNCTION__, __LINE__);
                    out_standby(&(stream->stream.common));
                }
            }

            tinymix_ctrl_set(adev->mixer, atv5302_i2s_playback_arr, 
                    sizeof(atv5302_i2s_playback_arr)/sizeof(atv5302_i2s_playback_arr[0]));

        	
            if (speaker_on && !headphone_on) {
                ALOGE("%s: playback route is on speaker", __FUNCTION__);

                tinymix_ctrl_set(adev->mixer, atv5302_speaker_playback_arr, 
                        sizeof(atv5302_speaker_playback_arr)/sizeof(atv5302_speaker_playback_arr[0]));
                usleep(150000); // sleep for the PA turning on stablely,to avoid pop nosie
            } else {
                tinymix_ctrl_set(adev->mixer, atv5302_playback_off_arr, 
                        sizeof(atv5302_playback_off_arr)/sizeof(atv5302_playback_off_arr[0]));
            }
            adev->speaker_on = speaker_on;
            ALOGW("%s %d", __FUNCTION__, __LINE__);
        }

        if (adev->headphone_on != headphone_on) {
            if (stream) {
                if (stream->standby == 0) {
                	//ALOGE("%s %d standby", __FUNCTION__, __LINE__);
                    out_standby(&(stream->stream.common));
                }
            }

            tinymix_ctrl_set(adev->mixer, atv5302_i2s_playback_arr, 
                    sizeof(atv5302_i2s_playback_arr)/sizeof(atv5302_i2s_playback_arr[0]));

        	
            if (headphone_on) {
                ALOGE("%s: playback route is on earphone", __FUNCTION__);

                tinymix_ctrl_set(adev->mixer, atv5302_headphone_playback_arr, 
                        sizeof(atv5302_headphone_playback_arr)/sizeof(atv5302_headphone_playback_arr[0]));
                usleep(150000); // sleep for the PA turning off stablely, to avoid pop noise
            } else {
                tinymix_ctrl_set(adev->mixer, atv5302_playback_off_arr, 
                        sizeof(atv5302_playback_off_arr)/sizeof(atv5302_playback_off_arr[0]));
            }
            adev->headphone_on = headphone_on;
        }
   
    }
}

/**
 *NEW_FEATURE: shutdown speaker when volume tuned to zero.
 ************************************
 *      
 *ActionsCode(author:yuchen, change_code)
 */                     
static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct actions_stream_out *out = (struct actions_stream_out *)stream;
    struct actions_audio_device *adev = out->dev;
    struct str_parms *parms;
    char *str;
    char value[32];
    int ret, val = 0;
    int get_values[MAX_VALUE_NUM], get_value_num;
    int pa_onoff = 0;
    int headphone_on = 0;
    //ALOGW("*********%s, %s",__FUNCTION__, kvpairs);
    headphone_on = adev->devices & AUDIO_DEVICE_OUT_WIRED_HEADSET;
    headphone_on = headphone_on | (adev->devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE);
    

    parms = (struct str_parms*)str_parms_create_str(kvpairs);
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    //ALOGD("%s : value:%s",__FUNCTION__, value);
    if (ret >= 0) {
    	val = atoi(value);
    	
           if ((val == AUDIO_DEVICE_OUT_AUX_DIGITAL)||(((adev->devices & AUDIO_DEVICE_OUT_ALL) != val) && (val != 0))) {
               ALOGE("%s %d", __FUNCTION__, __LINE__);
               adev->devices &= ~AUDIO_DEVICE_OUT_ALL;
               adev->devices |= val;
               select_output_device(adev);
    	}
    }
    
#if ACTIONS_FEATURE_SHUTDOWN_PA_WHEN_MUTED_ON==1     
    //ActionsCode(author:yuchen, change_code): check volume to decide whether to shutdown speaker
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_VOLUME_INDEX, value, sizeof(value));
    if (ret >= 0) {
    	val = atoi(value);
    	
       	get_value_num = tinymix_ctrl_get(adev->mixer, "speaker on off switch", get_values);
       	if (get_value_num > 0) {
            pa_onoff = get_values[0];
       	}
    	
    	if (val == 0)
    	{
	    //volume index decrease to 0, and no headphone, turn off pa
	    if((headphone_on == 0)&&(pa_onoff != 0))
	    {
                tinymix_ctrl_set(adev->mixer, atv5302_volume_muted_arr, 
                   	sizeof(atv5302_volume_muted_arr)/sizeof(atv5302_volume_muted_arr[0])); // turn off speaker and headphone
			
	    }
    	}
    	else
    	{
    	    //volume index bigger than 0, and no headphone plugged, turn on pa
    	    if((out->standby == 0)&&(headphone_on == 0)&&(pa_onoff == 0))
    	    {
    	    	ALOGE("%s %d\n", __FUNCTION__, __LINE__);
                tinymix_ctrl_set(adev->mixer, atv5302_volume_no_mute_arr, 
                       	sizeof(atv5302_volume_no_mute_arr)/sizeof(atv5302_volume_no_mute_arr[0])); // turn off speaker and headphone
    			
    	    }
    	}
    }
#endif    
    
    return 0;
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
	return strdup("");
}

/**
 *NEW_FEATURE: limit latency to 1 period, because dma will start after 1 period data is filled.
 ************************************
 *      
 *ActionsCode(author:yuchen, change_code)
 */                     
static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
	//ActionsCode(author:yuchen, change_code): 1 period data is about 23ms, round to 30
	//return (NORMAL_PERIOD_COUNT * NORMAL_PERIOD_SIZE * 1000) / 44100;
	return 30;
}

static int out_set_volume(struct audio_stream_out *stream, float left,
		float right)
{
	ALOGW("*********%s, %f, %f",__FUNCTION__, left, right);
	return 0;
}

static int start_output_stream(struct actions_stream_out *out)
{
	struct actions_audio_device *adev = out->dev;
	unsigned int card = DEFAULT_CARD;
	unsigned int port = DEFAULT_PORT;
	

	//unsigned int port = 2;
	int buffer_size;

	if ((adev->devices & AUDIO_DEVICE_OUT_AUX_DIGITAL) == AUDIO_DEVICE_OUT_AUX_DIGITAL) {
		port = HDMI_PORT;
	}

    ALOGD("%s: devices= %d, port= %d", __FUNCTION__, adev->devices, port);
	
    adev->active_output = out;

	out->pcm = pcm_open(card, port, PCM_OUT, &out->config);

    select_output_device(adev);

	if (!out->pcm || !pcm_is_ready(out->pcm)) {
		ALOGE("cannot open pcm_out driver: %s", pcm_get_error(out->pcm));
		pcm_close(out->pcm);
        out->pcm = NULL;
		adev->active_output = NULL;
		return -ENOMEM;
	}

	return 0;
}

/**
 *NEW_FEATURE: open data dump code. it wont work until a outdbg.pcm is created in device.
 *		it's debugwise, for device no sound bug.
 ************************************
 *      
 *ActionsCode(author:yuchen, change_code)
 */                     
static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
		size_t bytes)
{
	int ret;
    unsigned int i;
	struct actions_stream_out *out = (struct actions_stream_out *)stream;
	short *buffer_s = (short *)buffer;
	struct actions_audio_device *adev = out->dev;

	if (out->standby) {
		ret = start_output_stream(out);
		if (ret != 0) {
			goto exit;
		}
		out->standby = 0;
	}
//#ifdef DUMP_OUTWRITE_PCM	
/*
	if (strncmp(system("cat /sys/class/switch/h2w/dbgflag"), "0", 1)==0) {
		
	}
	else
*/
	if(1)
	{
		//dump frames
		FILE *fp = NULL;
		int dbgfd;
		char test = -1;
		
		if((fp = fopen("/sys/class/switch/h2w/dbgflag","rb")) != NULL){
			fread(&test, 1, 1, fp);	
			fclose(fp);
		}
		
				
		if((test != '3')||(fp = fopen("/data/outdbg.pcm","ab")) == NULL){
			//ALOGE("nononono");
		}
		else {	
			//ALOGE("Wawawawa");		
			fwrite(buffer_s,1,bytes,fp);			
			fclose(fp);
			fp = NULL;
		}	
	}
//#endif
	/* convert the 16-bit sample to 32-bit sample, because our hardware fifo is 32-bit width*/
	//if(adev->speaker_on && (adev->headphone_on == 0))
	if((adev->is_mono_output == 1)&&(adev->speaker_on && (adev->headphone_on == 0)))
	{	
        	for(i=0; i<bytes/2; i+=2) {
        		int temp;
        		int mix_pcm = 0;
        		//temp = buffer_s[i] + buffer_s[i+1];
        		mix_pcm = (buffer_s[i] + buffer_s[i+1])>>1;
        		out->buffer[i] = (mix_pcm) << 16;
        		out->buffer[i+1] = (mix_pcm) << 16;
            	}
        }
        else if((adev->is_reverse_earphone_channels == 1)&& adev->headphone_on)
        {
        	//ActionsCode(author:yuchen, change_code):  special method for 
        	//customers who accidentally reverse their earphone channels on board.
        	for(i=0; i<bytes/2; i+=2) {
        		out->buffer[i] = ((int)buffer_s[i+1]) << 16;
        		out->buffer[i+1] = ((int)buffer_s[i]) << 16;
            	}
        	
        }
        else
        {
               for(i=0; i<bytes/2; i++) {
                       out->buffer[i] = ((int)buffer_s[i]) << 16;
               }
        	
        }

#ifdef DUMP_OUTWRITE_PCM	
	if (1) {
		//dump frames
		FILE *fp = NULL;		
		if((fp = fopen("/data/outint.pcm","ab")) == NULL){
			//ALOGE("open out pcm file failed");
		}
		else {			
			fwrite(out->buffer,1,2*bytes,fp);			
			fclose(fp);
			fp = NULL;
		}	
	}
#endif
	ret = pcm_write(out->pcm, out->buffer, 2*bytes);
exit:
	if (ret != 0) {
        ALOGE("*******%s: pcm write error error no = %d!!!!!",__FUNCTION__,ret);
		usleep(bytes * 1000000 / audio_stream_frame_size(&stream->common) /
				out_get_sample_rate(&stream->common));
	}
	
	//ActionsCode(author:yuchen, new_code): this timestamp to record written frames 		
	out->timestamp = out->timestamp + (int64_t)(bytes/4);
	return bytes;
}

static int out_get_render_position(const struct audio_stream_out *stream,
		uint32_t *dsp_frames)
{
	return -EINVAL;
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
	return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
	return 0;
}

static int out_get_next_write_timestamp(const struct audio_stream_out *stream,
		int64_t *timestamp)
{
	return -EINVAL;
}

/**
 *BUGFIX: imeplement this interface for cts AudioTrack getTimeStamp test.
 ************************************
 *      
 *ActionsCode(author:yuchen, change_code)
 */                     
static int out_get_presentation_position(const struct audio_stream_out *stream,
		uint64_t *frames, struct timespec *timestamp)
{
	//ActionsCode(author:yuchen, new_code): return the frames presented and a timespec
	// 				as interface header instructed. 
	//FIXME: the frames and timestamp returned is not exact, just for cts now, not sure
	//       if these code will affect ordinary video/audio playback;
	struct actions_stream_out *out = (struct actions_stream_out *)stream;
	int ret = -1;
	pthread_mutex_lock(&out->lock);

	size_t avail;
	/*
	if (pcm_get_htimestamp(out->pcm, &avail, timestamp) == 0) {
		size_t kernel_buffer_size = out->config.period_size * out->config.period_count;
		// FIXME This calculation is incorrect if there is buffering after app processor
		int64_t signed_frames = out->timestamp - kernel_buffer_size + avail;
		ALOGE("%s %d kernel_buffer_size %d, avail %d", __FUNCTION__, __LINE__, kernel_buffer_size, avail);
		// It would be unusual for this value to be negative, but check just in case ...
		if (signed_frames >= 0) {
			//*frames = signed_frames;
			ret = 0;
		}
	}
	*/

	*frames = out->timestamp;
	clock_gettime(CLOCK_MONOTONIC, timestamp);
	pthread_mutex_unlock(&out->lock);

	return 0;
}

/** audio_stream_in implementation **/
static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
	/* Why we set the capture rate 44100? The i2s rx&tx share the 
	 * same MCLK & LRCLK & BCLK, so the sample rate of capture and 
	 * playback should be same if we want to support playback and
	 * capture run at the same time.
	 * */
	return 44100;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
	return 0;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
	struct actions_stream_in *in = (struct actions_stream_in *)stream;	
	return in->config.period_size * 
		audio_stream_frame_size((struct audio_stream *)stream);
}

static uint32_t in_get_channels(const struct audio_stream *stream)
{
	struct actions_stream_in *in = (struct actions_stream_in *)stream;
	return in->channel_mask;
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
	return AUDIO_FORMAT_PCM_16_BIT;
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
	return 0;
}

static int in_standby(struct audio_stream *stream)
{
	struct actions_stream_in *in = (struct actions_stream_in *)stream;
	ALOGD("in_standby!!");

	struct actions_audio_device *adev = in->dev;
	adev->active_input = in;
	if(capture_can_operate_vmic == 0)
	{
		tinymix_ctrl_set(adev->mixer, atv5302_capture_off_arr_without_vmic, 
			sizeof(atv5302_capture_off_arr_without_vmic)/sizeof(atv5302_capture_off_arr_without_vmic[0]));
	}
	else
	{
		tinymix_ctrl_set(adev->mixer, atv5302_capture_off_arr_with_vmic, 
			sizeof(atv5302_capture_off_arr_with_vmic)/sizeof(atv5302_capture_off_arr_with_vmic[0]));		
	}

	if (in->standby == 0) {
		pcm_close(in->pcm);
        in->pcm = NULL;
		in->standby = 1;
	}

	return 0;
}

static int in_dump(const struct audio_stream *stream, int fd)
{
	return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
	return 0;
}

static char * in_get_parameters(const struct audio_stream *stream,
		const char *keys)
{
	return strdup("");
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
	return 0;
}

static int start_input_stream(struct actions_stream_in *in)
{
	struct actions_audio_device *adev = in->dev;
	unsigned int card = DEFAULT_CARD;
	unsigned int port = DEFAULT_PORT;

	adev->active_input = in;

	in->pcm = pcm_open(card, port, PCM_IN, &in->config);

	tinymix_ctrl_set(adev->mixer, atv5302_capture_arr, 
			sizeof(atv5302_capture_arr)/sizeof(atv5302_capture_arr[0]));
	if (!in->pcm || !pcm_is_ready(in->pcm)) {
		ALOGE("cannot open pcm_in driver: %s", pcm_get_error(in->pcm));
		pcm_close(in->pcm);
        in->pcm = NULL;
		adev->active_input = NULL;
		return -ENOMEM;
	}

	return 0;
}

static ssize_t in_read(struct audio_stream_in *stream, void* buffer,
		size_t bytes)
{
	int ret, i,k;
	struct actions_stream_in *in = (struct actions_stream_in *)stream;
    struct actions_audio_device* adev = in->dev;
	int *buffer_i = (int *)in->buffer;
	short *buffer_s = (short *)buffer;
	int channels = popcount(in->channel_mask);
	int frames_to_read = bytes/audio_stream_frame_size(&stream->common);

	if (in->standby) {
		ret = start_input_stream(in);
		if (ret != 0) {
			goto exit;
		}
		in->standby = 0;
	}
	
	//why read in bytes = frames_to_read * 8?
	//when capture mode is enabled, the input pcm from singal source is send to both left & right channel
	//and the i2s is 32bit depth, thus, the captured framesize = 4*2 = 8 and readin bytes = frames_to_read*8
	ret = pcm_read(in->pcm, in->buffer, frames_to_read*8);
	if (ret != 0)
		goto exit;		

	for(i=0,k=0; i<frames_to_read; i++,k+=channels) {
		buffer_s[k] = (buffer_i[i*2] >> 16); //samuel for test, enlarge the ample of pcm.
		if (channels == 2) {
			buffer_s[k + 1] = (buffer_i[i*2+1] >> 16);
		}
	}

    // bellow are added for CTS test.
    // if mic_mute is true, mic should be mute,buffer date are all 0s, samuel.
    if (adev->mic_mute) {
        memset(buffer, 0, bytes);
    }
	
    // drop the first 300ms frames to avoid pop noise. 
    if (in->drop_frames) {
        for(k=0; k<frames_to_read*channels; k++) {
            buffer_s[k] = buffer_s[k] >> (16-in->drop_count); 
        }

        in->drop_count++;
        if (in->drop_count >=16) { 
            in->drop_frames = 0;
        }
    }
#ifdef DUMP_READIN_PCM	
	if (1) {
		//dump frames
		FILE *fp = NULL;		
		if((fp = fopen("/data/in.pcm","ab")) == NULL){
			ALOGE("open captured pcm file failed");
		}
		else {			
			fwrite(buffer_s,1,bytes,fp);			
			fclose(fp);
			fp = NULL;
		}	
	}
#endif

exit:
	if (ret != 0)
		usleep(bytes * 1000000 / audio_stream_frame_size(&stream->common) /
				in_get_sample_rate(&stream->common));
	return bytes;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
	return 0;
}

static int in_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
	return 0;
}

static int in_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
	return 0;
}

/**
 *BUGFIX: a newly implemented interface for cts AudioTrack getTimeStamp test is added.
 ************************************
 *      
 *ActionsCode(author:yuchen, change_code)
 */                     
static int adev_open_output_stream(struct audio_hw_device *dev,
		audio_io_handle_t handle,
		audio_devices_t devices,
		audio_output_flags_t flags,
		struct audio_config *config,
		struct audio_stream_out **stream_out)
{
	struct actions_audio_device *ladev = (struct actions_audio_device *)dev;
	struct actions_stream_out *out;
	int ret, buffer_size;
    //ALOGV("*********%s",__FUNCTION__);
	out = (struct actions_stream_out *)calloc(1, sizeof(struct actions_stream_out));
	if (!out)
		return -ENOMEM;

    memset((void*)out, 0, sizeof(struct actions_stream_out));
	out->stream.common.get_sample_rate = out_get_sample_rate;
	out->stream.common.set_sample_rate = out_set_sample_rate;
	out->stream.common.get_buffer_size = out_get_buffer_size;
	out->stream.common.get_channels = out_get_channels;
	out->stream.common.get_format = out_get_format;
	out->stream.common.set_format = out_set_format;
	out->stream.common.standby = out_standby;
	out->stream.common.dump = out_dump;
	out->stream.common.set_parameters = out_set_parameters;
	out->stream.common.get_parameters = out_get_parameters;
	out->stream.common.add_audio_effect = out_add_audio_effect;
	out->stream.common.remove_audio_effect = out_remove_audio_effect;
	out->stream.get_latency = out_get_latency;
	out->stream.set_volume = out_set_volume;
	out->stream.write = out_write;
	out->stream.get_render_position = out_get_render_position;
	out->stream.get_next_write_timestamp = out_get_next_write_timestamp;
	
	//ActionsCode(author:yuchen, new_code): newly implemented interface for 5.0 cts.
	//				timestamp to record written frames 		
	out->stream.get_presentation_position = out_get_presentation_position;	
	out->timestamp = 0;

	out->config = pcm_config_normal_playback;
	out->dev = ladev;
    
    out_standby(&out->stream.common);
    out->standby = 1;

	config->format = out_get_format(&out->stream.common);
	config->channel_mask = out_get_channels(&out->stream.common);
	config->sample_rate = out_get_sample_rate(&out->stream.common);

	/*
	 * Why we need to alloc this buffer? The I2S RX&TX and HDMI and SPDIF FIFO
	 * of the MAIN IC are all 32-bit long per channel, but the actual sample length
	 * we use is 16 bits, so we need a buffer to convert the audio data between 
	 * 16-bit samples and 32-bit samples;
	 * */
	buffer_size = out_get_buffer_size((struct audio_stream*)&out->stream) * 2;
	out->buffer = (int *)malloc(buffer_size);
	if (out->buffer == NULL) {
		ALOGE("cannot alloc memory for output stream!\n");
		ret = -ENOMEM;
		goto err_open;
	}

	*stream_out = &out->stream;
	return 0;

err_open:
	free(out);
	*stream_out = NULL;
	return ret;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
		struct audio_stream_out *stream)
{
	struct actions_stream_out *out = (struct actions_stream_out *)stream;

	if (!out->standby) {
		out_standby(&stream->common);
    }

	if (out->buffer) {
		free(out->buffer);
    }

	free(stream);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
	return 0;
}

static char * adev_get_parameters(const struct audio_hw_device *dev,
		const char *keys)
{
	return NULL;
}

static int adev_init_check(const struct audio_hw_device *dev)
{
	return 0;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
	return 0;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    return -ENOSYS;
}

static int adev_get_master_volume(struct audio_hw_device *dev, float *volume)
{
    return -ENOSYS;
}

static int adev_set_master_mute(struct audio_hw_device *dev, bool muted)
{
    return -ENOSYS;
}

static int adev_get_master_mute(struct audio_hw_device *dev, bool *muted)
{
    return -ENOSYS;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
	return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    struct actions_audio_device* adev = (struct actions_audio_device*)dev;

    adev->mic_mute = state;

	return 0;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    struct actions_audio_device* adev = (struct actions_audio_device*)dev;
    
    *state = adev->mic_mute;

	return 0;
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
		const struct audio_config *config)
{
	int period_size = 0,frame_size = 0;
	
	period_size = NORMAL_PERIOD_SIZE;
	frame_size = audio_bytes_per_sample(config->format)*popcount(config->channel_mask);
	return period_size* frame_size;
}

static int adev_open_input_stream(struct audio_hw_device *dev,
		audio_io_handle_t handle,
		audio_devices_t devices,
		struct audio_config *config,
		struct audio_stream_in **stream_in)
{
	struct actions_audio_device *ladev = (struct actions_audio_device *)dev;
	struct actions_stream_in *in;
	int ret;

	in = (struct actions_stream_in *)calloc(1, sizeof(struct actions_stream_in));
	if (!in)
		return -ENOMEM;

	in->stream.common.get_sample_rate = in_get_sample_rate;
	in->stream.common.set_sample_rate = in_set_sample_rate;
	in->stream.common.get_buffer_size = in_get_buffer_size;
	in->stream.common.get_channels = in_get_channels;
	in->stream.common.get_format = in_get_format;
	in->stream.common.set_format = in_set_format;
	in->stream.common.standby = in_standby;
	in->stream.common.dump = in_dump;
	in->stream.common.set_parameters = in_set_parameters;
	in->stream.common.get_parameters = in_get_parameters;
	in->stream.common.add_audio_effect = in_add_audio_effect;
	in->stream.common.remove_audio_effect = in_remove_audio_effect;
	in->stream.set_gain = in_set_gain;
	in->stream.read = in_read;
	in->stream.get_input_frames_lost = in_get_input_frames_lost;

	in->config = pcm_config_normal_capture;
	in->dev = ladev;
	in->standby = 1;
    in->drop_frames = 1;
    in->drop_count = 0;


	//in->config.channels = popcount(config->channel_mask);
	in->channel_mask = config->channel_mask;
	config->sample_rate = in_get_sample_rate(&in->stream.common);

	in->buffer = malloc(in->config.period_size*8);
	ALOGD("channels:%d rate:%d format:%#x\n",
			in->config.channels,
			in->config.rate,
			in->config.format);
	if (!in->buffer) {
		ALOGE("alloc mem for instream buffer failed!!\n");
		ret = -ENOMEM;
		goto err_open;
	}

	*stream_in = &in->stream;
	
	return 0;

err_open:
	free(in);
	*stream_in = NULL;
	return ret;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
		struct audio_stream_in *stream)
{
	struct actions_stream_in * in = (struct actions_stream_in *)stream;
	if (in->standby)
		in_standby(&stream->common);

	if (in->buffer)
		free(in->buffer);

    free(stream);
	return;
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
	return 0;
}

static int adev_close(hw_device_t *device)
{
	struct actions_audio_device *adev = (struct actions_audio_device *)device;
	mixer_close(adev->mixer);
	free(device);
	return 0;
}

static uint32_t adev_get_supported_devices(const struct audio_hw_device *dev)
{
	return (/* OUT */
			AUDIO_DEVICE_OUT_EARPIECE |
			AUDIO_DEVICE_OUT_SPEAKER |
			AUDIO_DEVICE_OUT_WIRED_HEADSET |
			AUDIO_DEVICE_OUT_WIRED_HEADPHONE |
			AUDIO_DEVICE_OUT_AUX_DIGITAL |
			AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET |
			AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET |
			AUDIO_DEVICE_OUT_ALL_SCO |
			AUDIO_DEVICE_OUT_DEFAULT |
			/* IN */
			AUDIO_DEVICE_IN_COMMUNICATION |
			AUDIO_DEVICE_IN_AMBIENT |
			AUDIO_DEVICE_IN_BUILTIN_MIC |
			AUDIO_DEVICE_IN_WIRED_HEADSET |
			AUDIO_DEVICE_IN_AUX_DIGITAL |
			AUDIO_DEVICE_IN_BACK_MIC |
			AUDIO_DEVICE_IN_ALL_SCO |
			AUDIO_DEVICE_IN_DEFAULT);
}

static int adev_open(const hw_module_t* module, const char* name,
		hw_device_t** device)
{
	struct actions_audio_device *adev;
	int ret;
    	int values[MAX_VALUE_NUM], value_num, i;
    	char prop_value[PROPERTY_VALUE_MAX];
    	ALOGE("adev_open");

	if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
		return -EINVAL;

	adev = calloc(1, sizeof(struct actions_audio_device));
	if (!adev)
		return -ENOMEM;

	adev->device.common.tag = HARDWARE_DEVICE_TAG;
	adev->device.common.version = AUDIO_DEVICE_API_VERSION_2_0;
	adev->device.common.module = (struct hw_module_t *) module;
	adev->device.common.close = adev_close;

	adev->device.get_supported_devices = adev_get_supported_devices;
	adev->device.init_check = adev_init_check;
	adev->device.set_voice_volume = adev_set_voice_volume;
	adev->device.set_master_volume = adev_set_master_volume;
    adev->device.get_master_volume = adev_get_master_volume;
    adev->device.set_master_mute = adev_set_master_mute;
    adev->device.get_master_mute = adev_get_master_mute;
	adev->device.set_mode = adev_set_mode;
	adev->device.set_mic_mute = adev_set_mic_mute;
	adev->device.get_mic_mute = adev_get_mic_mute;
	adev->device.set_parameters = adev_set_parameters;
	adev->device.get_parameters = adev_get_parameters;
	adev->device.get_input_buffer_size = adev_get_input_buffer_size;
	adev->device.open_output_stream = adev_open_output_stream;
	adev->device.close_output_stream = adev_close_output_stream;
	adev->device.open_input_stream = adev_open_input_stream;
	adev->device.close_input_stream = adev_close_input_stream;
	adev->device.dump = adev_dump;

	adev->mixer = mixer_open(DEFAULT_CARD);

	if (!adev->mixer) {
		ALOGE("Failed to open mixer\n");
		return -EAGAIN;
	}

    tinymix_ctrl_set(adev->mixer, atv5302_playback_off_arr, 
        sizeof(atv5302_playback_off_arr)/sizeof(atv5302_playback_off_arr[0]));


    // get volume and gain values from DTS, store them into the global variables.
    // speaker
    value_num = tinymix_ctrl_get(adev->mixer, "Dummy speaker volume", values);
    if (value_num > 0) {
        atv5302_speaker_playback_arr[1].val = values[0];
    }
    value_num = tinymix_ctrl_get(adev->mixer, "Dummy speaker gain", values);
    if (value_num > 0) {
        atv5302_speaker_playback_arr[2].val = values[0];
        atv5302_speaker_playback_arr[3].val = values[1];
    }

    // headphone
    value_num = tinymix_ctrl_get(adev->mixer, "Dummy earphone volume", values);
    if (value_num > 0) {
        atv5302_headphone_playback_arr[2].val = values[0];
    }
    value_num = tinymix_ctrl_get(adev->mixer, "Dummy earphone gain", values);
    if (value_num > 0) {
        atv5302_headphone_playback_arr[3].val = values[0];
        atv5302_headphone_playback_arr[4].val = values[1];
    }

    // MIC
    value_num = tinymix_ctrl_get(adev->mixer, "Dummy mic Gain", values);
    if (value_num > 0) {
        atv5302_capture_arr[3].val = values[0];
    }
    
    value_num = tinymix_ctrl_get(adev->mixer, "Dummy mic mode", values);
    if (value_num > 0) {
    	ALOGE("%s %d", __FILE__, __LINE__);
        atv5302_capture_arr[6].val = values[0];
    }
    
    value_num = tinymix_ctrl_get(adev->mixer, "Dummy earphone detect method", values);
    if (value_num > 0) {
    	ALOGE("%s %d method %d", __FILE__, __LINE__, values[0]);
        capture_can_operate_vmic = values[0];
    }    

    property_get("ro.audio.output.force_mono", prop_value, "no");
    if(strncmp(prop_value, "yes", 3) == 0) {
        // stereo need to mix into mono
        adev->is_mono_output = 1;
    } else {
        adev->is_mono_output = 0;
    }
    
    
    property_get("ro.audio.output.rev_earphone", prop_value, "no");
    if(strncmp(prop_value, "yes", 3) == 0) {
        // stereo need to mix into mono
        adev->is_reverse_earphone_channels = 1;
    } else {
        adev->is_reverse_earphone_channels = 0;
    }
    

	*device = &adev->device.common;

	return 0;
}

static struct hw_module_methods_t hal_module_methods = {
	.open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
	.common = {
		.tag = HARDWARE_MODULE_TAG,
		.module_api_version = AUDIO_MODULE_API_VERSION_0_1,
		.hal_api_version = HARDWARE_HAL_API_VERSION,
		.id = AUDIO_HARDWARE_MODULE_ID,
		.name = "Default audio HW HAL",
		.author = "The Android Open Source Project",
		.methods = &hal_module_methods,
	},
};
