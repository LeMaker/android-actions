/* tinyplay.c
**
** Copyright 2011, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of The Android Open Source Project nor the names of
**       its contributors may be used to endorse or promote products derived
**       from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY The Android Open Source Project ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL The Android Open Source Project BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
*/

#include <tinyalsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include<errno.h>


#define __GS900A__

void play_sample(FILE *file, unsigned int card, unsigned int device, unsigned int channels,
                 unsigned int rate, unsigned int bits, unsigned int period_size,
                 unsigned int period_count);

					
enum mixer_config{
	DUMMY_MIC_GAIN,   //0
	DUMMY_EARPHONE_GAIN, //1
	DUMMY_SPEAKER_GAIN,  //2
	DUMMY_SPEAKER_VOL,   //3
	DUMMY_EARPHONE_VOL,  //4
	DUMMY_MIC_NUM,        //5
	ADC0_GAIN,            //6
	AMP1_GAIN_BOOST_RANGE,//7
	ADC0_DIGIT_GAIN_CTR,  //8
	MIC0_MODE_MUX,         //9
#ifdef __GS705A__
	DUMMY_MIC_MODE,        //10
	DUMMY_EARPHONE_DETECT, //11
#endif
	PA_OUTPUT_SWING_MUX,   //12
	DAC_PA_VOL,            //13 
	DAC_FL_FR_PLAYBACK,    //14
	DAC_FL_GAIN,           //15 
	DAC_FR_GAIN,           //16 
	DAC_PA_SWITCH,         //17 
	DAC_PA_OUTPUT_STAGE,   //18
	DAC_DIGIT_FL_FR,       //19 
	INTER_MIC_POWER,       //20 
	EXTER_MIC_POWER,       //21
	EXTER_MIC_POWER_VOLT,  //22
	ADC0_DIGIT_GAIN,       //23 
	AUDIO_OUTPUT_MODE,     //24 
	AOUT_MIXER_FL_FR_SWITCH,  //25
	AOUT_MIXER_MIC_SWITCH,    //26
	AOUT_MIXER_FM_SWITCH,     //27
	ADC0_MUX,                 //28 
	SPEAKER_SWITCH,           //29
	};
/*
* configure mixer int type attr
*/ 
static int set_mixer_int_value(int card, int id, int value)
{
	struct mixer *mixer;
    struct mixer_ctl *ctl;
    unsigned int num_ctl_values;
    unsigned int i;
	
	mixer = mixer_open(card);
	if (!mixer) 
	{
		fprintf(stderr, "Failed to open mixer\n");
		return -1;
	}
	
    ctl = mixer_get_ctl(mixer, id);
    if (!ctl) 
	{
        fprintf(stderr, " mixer_get_ctl:Invalid mixer control\n");
		mixer_close(mixer);
        return -1;
    }

    num_ctl_values = mixer_ctl_get_num_values(ctl);
		/* Set all values the same */
	for (i = 0; i < num_ctl_values; i++) 
	{
		if (mixer_ctl_set_value(ctl, i, value)) 
		{
			fprintf(stderr, "id = %d,mixer_ctl_set_value error: Invalid value\n",id);
			mixer_close(mixer);
			return -1;
		}
	}
	mixer_close(mixer);
	return 0;
}


static int set_mixer_enum_value(int card, int id, char *string)
{
	struct mixer *mixer;
	struct mixer_ctl *ctl;

	mixer = mixer_open(card);
	if (!mixer) 
	{
		fprintf(stderr, "Failed to open mixer\n");
		return -1;
	}

	ctl = mixer_get_ctl(mixer, id);
    if (!ctl) 
	{
        fprintf(stderr, " mixer_get_ctl:Invalid mixer control\n");
		mixer_close(mixer);
        return -1;
    }
	
	if (mixer_ctl_set_enum_by_string(ctl, string))
	{
		fprintf(stderr, "mixer_ctl_set_enum_by_string error: invalid enum value\n");
		mixer_close(mixer);
        return -1;
	}
	mixer_close(mixer);
	return 0;
}

static int init_mixer_to_hdmi(int card)
{ 
	set_mixer_int_value(card, ADC0_GAIN, 9); 
	set_mixer_int_value(card, AMP1_GAIN_BOOST_RANGE, 3);
	set_mixer_int_value(card, ADC0_DIGIT_GAIN_CTR, 0);
	set_mixer_enum_value(card, MIC0_MODE_MUX, "Differential");
 //	set_mixer_int_value(card, DUMMY_MIC_MODE, 1);
	set_mixer_enum_value(card, PA_OUTPUT_SWING_MUX, "Vpp1.6");
	set_mixer_int_value(card, DAC_PA_VOL, 0);
	set_mixer_int_value(card,  DAC_FL_FR_PLAYBACK, 0);
	set_mixer_int_value(card, DAC_FL_GAIN, 0); 
	set_mixer_int_value(card,  DAC_FR_GAIN, 0);
	set_mixer_int_value(card,  DAC_PA_SWITCH, 1);
	set_mixer_int_value(card,  DAC_PA_OUTPUT_STAGE, 1);
	set_mixer_int_value(card,  DAC_DIGIT_FL_FR, 1);
	set_mixer_int_value(card,  INTER_MIC_POWER, 0);
	set_mixer_int_value(card, EXTER_MIC_POWER_VOLT, 1); 
	set_mixer_int_value(card, ADC0_DIGIT_GAIN, 0);
	set_mixer_int_value(card, AOUT_MIXER_FL_FR_SWITCH, 0);
	set_mixer_int_value(card, AOUT_MIXER_MIC_SWITCH, 0);
	set_mixer_int_value(card, AOUT_MIXER_FM_SWITCH, 0);
	set_mixer_enum_value(card, AUDIO_OUTPUT_MODE, "hdmi");
	set_mixer_int_value(card, SPEAKER_SWITCH, 1);
	
#ifdef __GS900A__
	set_mixer_int_value(card,   EXTER_MIC_POWER, 0);
	set_mixer_enum_value(card,  ADC0_MUX,"FM");
#else
	set_mixer_int_value(card,   EXTER_MIC_POWER, 1);
	set_mixer_enum_value(card,	ADC0_MUX,"None");
#endif	
	return 0; 
}
						
int main(int argc, char **argv)
{
    FILE *file;
    unsigned int device = 1;
    unsigned int card = 0;
    unsigned int channels = 2;
    unsigned int sample_rate = 44100;
    unsigned int bits_per_sample = 16;
    unsigned int period_size = 1024;
    unsigned int period_count = 4;

    if (argc < 2) {
	    fprintf(stderr, " arguments too few\n", argv[0]);
	    return 1;
    }

    file = fopen(argv[1], "rb");
    if (!file) {
	    fprintf(stderr, "Unable to open file '%s'\n", argv[1]);
	    return 1;
    }
	init_mixer_to_hdmi(card);
	printf("init mixer to hdmi successfully \n");
    /* parse command line arguments */
    play_sample(file, card, device, channels, sample_rate,
		    bits_per_sample, period_size, period_count);

    fclose(file);

    return 0;

}
/*
int sample_is_playable(unsigned int card, unsigned int device, unsigned int channels,
                        unsigned int rate, unsigned int bits, unsigned int period_size,
                        unsigned int period_count)
{
    struct pcm_params *params;
    int can_play;

    params = pcm_params_get(card, device, PCM_OUT);
    if (params == NULL) {
        fprintf(stderr, "Unable to open PCM device %u.\n", device);
        return 0;
    }

    can_play = check_param(params, PCM_PARAM_RATE, rate, "Sample rate", "Hz");
    can_play &= check_param(params, PCM_PARAM_CHANNELS, channels, "Sample", " channels");
    can_play &= check_param(params, PCM_PARAM_SAMPLE_BITS, bits, "Bitrate", " bits");
    can_play &= check_param(params, PCM_PARAM_PERIOD_SIZE, period_size, "Period size", "Hz");
    can_play &= check_param(params, PCM_PARAM_PERIODS, period_count, "Period count", "Hz");

    pcm_params_free(params);

    return can_play;
}
*/
void play_sample(FILE *file, unsigned int card, unsigned int device, unsigned int channels,
                 unsigned int rate, unsigned int bits, unsigned int period_size,
                 unsigned int period_count)
{
    struct pcm_config config;
    struct pcm *pcm;
    int size;
    unsigned char *buffer;
    unsigned short *buffer_s;
    unsigned int *buffer_i;
    int num_read;
    int i;

    config.channels = channels;
    config.rate = rate;
    config.period_size = period_size;
    config.period_count = period_count;
    if (bits == 32)
        config.format = PCM_FORMAT_S32_LE;
    else if (bits == 16)
        config.format = PCM_FORMAT_S16_LE;
    config.start_threshold = period_size;
    config.stop_threshold = period_size*period_count;
    config.silence_threshold = 0;

	printf("start play sample!\n");
    
	pcm = pcm_open(card, device, PCM_OUT, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        fprintf(stderr, "Unable to open PCM device %u (%s)\n",
                device, pcm_get_error(pcm));
        return;
    }
	
	printf("pcm_open success!\n");
	
    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
	printf("pcm_frames_to_bytes=%d\n",size);
    size /= 4;
    buffer = malloc(size);
    if (!buffer) {
        fprintf(stderr, "Unable to allocate %d bytes\n", size);
        free(buffer);
        pcm_close(pcm);
        return;
    }
	
    buffer_s = (unsigned short *)buffer;
    buffer_i =(unsigned int *)malloc(2*size);
	if (!buffer_i) 
	{
		fprintf(stderr, "Unable to allocate %d bytes\n", 2*size); 
		free(buffer);
		pcm_close(pcm);
		return;
	}
    printf("Playing sample: %u ch, %u hz, %u bit\n", channels, rate, bits);

    do {
        num_read = fread(buffer, 1, size, file);
        if (num_read > 0) 
		{
			for(i=0; i<num_read/2; i++) 
			{
				buffer_i[i] = (unsigned int)(buffer_s[i] << 16);
			}
			if (pcm_write(pcm, buffer_i, num_read*2))  //write wav file data to device to play music
			{
                fprintf(stderr, "Error playing sample:%s\n",strerror(errno));
                break;
			}
        }
		else
		{
			printf("num_read = %d\n", num_read);
		}
    } while (num_read > 0);

    free(buffer);
    free(buffer_i);
    pcm_close(pcm);
}

