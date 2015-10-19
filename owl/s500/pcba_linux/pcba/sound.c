#include <directfb.h>
#include <linux/videodev2.h>
#include <linux/soundcard.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "encoderpp.h"
#include "case.h"
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#ifndef __GS702C__
#include "asoundlib.h"
#endif

bool record_flg = false;
bool play_avoid_flg = false;
bool hdmi_flg = false;
bool headphone_flg = false;

#ifndef __GS702C__
struct pcm *pcm = NULL;
struct pcm *pcm_in = NULL;
struct pcm *pcm_hdmi = NULL; 
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

static int instance_ref = 0;

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
	if (!mixer) {
		fprintf(stderr, "Failed to open mixer\n");
		return -1;
	}
	
	ctl = mixer_get_ctl(mixer, id);
	if (!ctl) {
		fprintf(stderr, " mixer_get_ctl:Invalid mixer control\n");
			mixer_close(mixer);
		return -1;
	}
	num_ctl_values = mixer_ctl_get_num_values(ctl);
		/* Set all values the same */
	for (i = 0; i < num_ctl_values; i++) {
		if (mixer_ctl_set_value(ctl, i, value)) {
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
	if (!mixer) {
		fprintf(stderr, "Failed to open mixer\n");
		return -1;
	}

	ctl = mixer_get_ctl(mixer, id);
	if (!ctl) {
		fprintf(stderr, " mixer_get_ctl:Invalid mixer control\n");
			mixer_close(mixer);
		return -1;
	}
	
	if (mixer_ctl_set_enum_by_string(ctl, string)){
		fprintf(stderr, "mixer_ctl_set_enum_by_string error: invalid enum value\n");
		mixer_close(mixer);
		return -1;
	}
	mixer_close(mixer);
	return 0;
}

static int init_mixer_to_i2s(int card)
{ 
	set_mixer_int_value(card, ADC0_GAIN, 9); 
	set_mixer_int_value(card, AMP1_GAIN_BOOST_RANGE, 3);
	set_mixer_int_value(card, ADC0_DIGIT_GAIN_CTR, 0);
	set_mixer_enum_value(card, MIC0_MODE_MUX, "Differential");
 //	set_mixer_int_value(card, DUMMY_MIC_MODE, 1);
	set_mixer_enum_value(card, PA_OUTPUT_SWING_MUX, "Vpp1.6");
	set_mixer_int_value(card, DAC_PA_VOL, 40);
	set_mixer_int_value(card,  DAC_FL_FR_PLAYBACK, 1);
	set_mixer_int_value(card, DAC_FL_GAIN, 170); 
	set_mixer_int_value(card,  DAC_FR_GAIN, 170);
	set_mixer_int_value(card,  DAC_PA_SWITCH, 1);
	set_mixer_int_value(card,  DAC_PA_OUTPUT_STAGE, 1);
	set_mixer_int_value(card,  DAC_DIGIT_FL_FR, 1);
	set_mixer_int_value(card,  INTER_MIC_POWER, 0);
	set_mixer_int_value(card, EXTER_MIC_POWER_VOLT, 1); 
	set_mixer_int_value(card, ADC0_DIGIT_GAIN, 0);
	set_mixer_int_value(card, AOUT_MIXER_FL_FR_SWITCH, 1);
	set_mixer_int_value(card, AOUT_MIXER_MIC_SWITCH, 0);
	set_mixer_int_value(card, AOUT_MIXER_FM_SWITCH, 0);
	set_mixer_enum_value(card, AUDIO_OUTPUT_MODE, "i2s"); 
	
	if (headphone_flg){
		set_mixer_int_value(card, SPEAKER_SWITCH, 0);
	}
	else{
		set_mixer_int_value(card, SPEAKER_SWITCH, 1);
	}
	
#ifdef __GS900A__
	set_mixer_int_value(card,   EXTER_MIC_POWER, 0);
	set_mixer_enum_value(card,  ADC0_MUX,"FM");
#else
	set_mixer_int_value(card,   EXTER_MIC_POWER, 1);
	set_mixer_enum_value(card,	ADC0_MUX,"None");
#endif	
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

/*
* configure mixer to capture voice
*/
static int init_mixer_to_cap(int card)
{ 
	set_mixer_int_value(card, ADC0_GAIN, 7); //10
	set_mixer_int_value(card, AMP1_GAIN_BOOST_RANGE, 7); 
	set_mixer_int_value(card, ADC0_DIGIT_GAIN_CTR, 3);  // 8 
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
	set_mixer_int_value(card,  INTER_MIC_POWER, 1);
	set_mixer_int_value(card,   EXTER_MIC_POWER, 1);
	set_mixer_int_value(card, EXTER_MIC_POWER_VOLT, 1); 
	set_mixer_int_value(card, ADC0_DIGIT_GAIN, 3); // 8
	set_mixer_enum_value(card, AUDIO_OUTPUT_MODE, "i2s");	 
	set_mixer_int_value(card, AOUT_MIXER_FL_FR_SWITCH, 0);
	set_mixer_int_value(card, AOUT_MIXER_MIC_SWITCH, 0);
	set_mixer_int_value(card, AOUT_MIXER_FM_SWITCH, 0);
	set_mixer_enum_value(card,  ADC0_MUX,"MIC0");
	set_mixer_int_value(card, SPEAKER_SWITCH, 0); 
	
	return 0;   
}

static struct pcm *create_pcm_in(unsigned int card, unsigned int device,
                            unsigned int channels, unsigned int rate, int bits)
{
	struct pcm_config config;
	struct pcm *pcm;

	config.channels = channels;
	config.rate = rate;
	config.period_size = 1024;
	config.period_count = 4; 
	config.start_threshold = 0;
	config.stop_threshold = 0;
	config.silence_threshold = 0;

	if (bits == 32)
		config.format = PCM_FORMAT_S32_LE;
	else if (bits == 16)
		config.format = PCM_FORMAT_S16_LE;

	pcm = pcm_open(card, device, PCM_IN, &config);
	if (!pcm || !pcm_is_ready(pcm)) {
		fprintf(stderr, "Unable to open PCM_IN device[C%dD%d] (%s)\n",card, device,
			pcm_get_error(pcm));
		pcm_close(pcm);
		return NULL;
	} 
	return pcm;
} 

static struct pcm *create_pcm_out(unsigned int card, unsigned int device,
	unsigned int channels, unsigned int rate, int bits)
{ 
	struct pcm_config config;  
	struct pcm *pcm;
	config.channels = channels;
	config.rate = rate;
	config.period_size = 1024;
	config.period_count = 4;   
								  
	if (bits == 32)
		config.format = PCM_FORMAT_S32_LE;
	else if (bits == 16)
		config.format = PCM_FORMAT_S16_LE;
	config.start_threshold = 0;
	config.stop_threshold = 0;
	config.silence_threshold = 0;
	pcm = pcm_open(card, device, PCM_OUT, &config);
	if (!pcm || !pcm_is_ready(pcm)){
		fprintf(stderr, "Unable to open PCM_OUT device[C%dD%d] (%s)\n",card, device,
		    pcm_get_error(pcm));
		pcm_close(pcm);
		return NULL;
	}  
	return pcm;
}

void pcm_cleanup()
{
	printf("pcm_cleanup\n");
	if (pcm){
		printf("close pcm\n");
		pcm_close(pcm);
		pcm = NULL;
	}

	if (pcm_in){
		printf("close pcm_in\n");
		pcm_close(pcm_in);
		pcm_in = NULL;
	
	}
	if (pcm_hdmi){
		printf("close pcm_hdmi\n");
		pcm_close(pcm_hdmi);
		pcm_hdmi = NULL;
	}
}



void check_output_mode(void)
{	
	static bool local_hdmi = false;

	if(local_hdmi != hdmi_flg)
	{
		local_hdmi = hdmi_flg;
		if (local_hdmi)
		{
			printf("switch output mode to hdmi\n");
			init_mixer_to_hdmi(0);
			if (!pcm_hdmi)
			{
				pcm_hdmi = create_pcm_out(0, 1, 2, 44100, 16);
			}
			
		}
		else
		{
			printf("switch output mode to i2s\n");
			if (pcm_hdmi)
			{
				pcm_close(pcm_hdmi);
				pcm_hdmi = NULL;
			}
			init_mixer_to_i2s(0);
		}
	}
}

void check_headphone(void)
{	
	int card = 0;
	
	static bool local_headphone = false;
	
	if(local_headphone != headphone_flg)
	{
		local_headphone = headphone_flg;
		if(local_headphone)
		{
			//speaker off
			set_mixer_int_value(card, SPEAKER_SWITCH, 0);
		}
		else
		{
			set_mixer_int_value(card, SPEAKER_SWITCH, 1);
		}
	}
}

void refresh_mixer_to_play(void)
{	
	check_output_mode();
	check_headphone();
}

static int music_play()
{
	FILE *file = NULL;
	char *filename = "/misc/dk44k_2ch_30s.wav";

	unsigned char *buffer; 
	unsigned short *buffer_s;
	unsigned int *buffer_i;
	int num_read,size,i;
 
	
	file = fopen(filename, "rb");
	if (!file) 
	{		
		printf("Unable to open file '%s' Music thread exit!\n", filename);
		return -1;    
	}	

	if (fseek(file, 0, SEEK_SET) < 0)  //put in the last to check if fread wav file error
	{
		printf("fread .wav file in music thread error:%s\n",strerror(errno));
		return -1;
	}
	
	/*set i2s as default*/
	if (!pcm)
	{
		init_mixer_to_i2s(0);
		pcm = create_pcm_out(0, 0, 2, 44100, 16);
	}

	
	/*	frames = period_size*period_count,
	   buffer_bytes = frames * (bits>>3) * channel	*/
	
	size = 4096;
	printf("########PCM_OUT size = %d\n",size);
	buffer = (unsigned char *)malloc(size);
	if (!buffer) 
	{
		fprintf(stderr, "Unable to allocate %d bytes\n", size); 
		return -1;
	} 
	buffer_s = (unsigned short *)buffer;  
	buffer_i =(unsigned int *)malloc(2*size);
	if (!buffer_i) 
	{
		fprintf(stderr, "Unable to allocate %d bytes\n", 2*size); 
		free(buffer);
		return -1;
	}

	while((num_read = fread(buffer, 1, size, file)))
	{
		if (record_flg)
		{
			break;
		}
		/*to change data type from 16bits to 32 bits*/ 
		for(i=0; i<num_read/2; i++) 
		{
			buffer_i[i] = ((unsigned int)buffer_s[i]) << 16;
		} 
		
		refresh_mixer_to_play();
		if (hdmi_flg && pcm_hdmi)
		{	
			if (pcm_write(pcm_hdmi, buffer_i, num_read*2))	
			{			
				printf("Error pcm hdmi write:%s\n",strerror(errno));			
			}
		}			
		if (!hdmi_flg && pcm)	
		{		
			if (pcm_write(pcm, buffer_i, num_read*2)) 
			{						
				printf("Error pcm write:%s\n",strerror(errno));				
			}	
		}
	}
	fclose(file);
	free(buffer_i);
	free(buffer);
	return 0;
}

/*
 * This function is create to play music sample file
*/
static int sample_play(char *filename)
{ 
    unsigned char *buffer; 
    int num_read,size;
	

	printf("start to play sample\n");
	
	FILE *file = fopen(filename, "rb");
	if (!file) 
	{
		fprintf(stderr, "Unable to open file temp.wav\n");
		return -1;
	}
	if (fseek(file, 0, SEEK_SET) < 0) 
	{
		printf("fread .wav file in mic cap thread error:%s\n",strerror(errno));
		return -1;
	} 
	
	if (hdmi_flg)		
	{

		printf("switch from capture to hdmi\n");
		init_mixer_to_hdmi(0);
		if (!pcm_hdmi)
		{
			pcm_hdmi = create_pcm_out(0, 1, 2, 44100, 16);
		}

	}
	else
	{
		printf("switch from capture to i2s\n");
		init_mixer_to_i2s(0);
	}
	
	/*  frames = period_size*period_count,
	buffer_bytes = frames * (bits>>3) * channel  */
	size = 4096; 

	buffer = (unsigned char *)malloc(size);
	if (!buffer) 
	{
	fprintf(stderr, "Unable to allocate %d bytes\n", size); 
		return -1;
	} 					//send buffer pointer

	while((num_read = fread(buffer, 1, size, file)))
	{  
		refresh_mixer_to_play();
		
		if (hdmi_flg && pcm_hdmi)		
		{			
			
			if (pcm_write(pcm_hdmi, buffer, num_read))
			{
				printf("Error pcm hdmi write:%s\n",strerror(errno));
			}

		}
		if (!hdmi_flg && pcm)
		{
			if (pcm_write(pcm, buffer, num_read))
			{
				printf("Error pcm write:%s\n",strerror(errno));
			}
		}

	}
	printf("play sample end\n");
	fclose(file);
	free(buffer);	
	return 0;
}




static int sample_capture(char *filename, void *arg)
{  
	FILE *file = NULL;
	unsigned int card = 0;
	unsigned int device = 0;
	unsigned int channels = 2;
	unsigned int rate = 44100;     
	int bits = 16;
	
	IDirectFBSurface *surface = arg;
	int width = 0, height = 0, mic_height;
	int energy_ret, energy_out;
	int bytes, size, i;
	int read_count = 0;
	unsigned char *buffer;
	unsigned int *i_buffer;
	unsigned short *s_buffer;
	
	printf("start to capture sample\n");
	bytes = 15 * sizeof(short) * 2 * 44100 ; //15s
	   /*thread main loop*/

	/*run into below when record_flg is true*/
	printf("start record\n");
	DFBCHECK(surface->GetSize(surface, &width, &height));

	file = fopen(filename, "wb");
	if (!file) 
	{
		fprintf(stderr, "Unable to create file\n");
		return -1;
	} 
	
#ifdef __GS900A__
	if (pcm_hdmi)
	{
		pcm_close(pcm_hdmi);
		pcm_hdmi = NULL;
	}
#endif
	
	init_mixer_to_cap(card);
	if (!pcm_in)
	{
		pcm_in = create_pcm_in(card, device, channels, rate, bits);
	}
	
	size = pcm_frames_to_bytes(pcm_in, pcm_get_buffer_size(pcm_in));
	size /= 4;
	printf("########PCM_IN size = %d\n",size);

	buffer = malloc(size);
	if (!buffer) 
	{
	    fprintf(stderr, "Unable to allocate %d bytes\n", size);
	    return -1;
	}
	
	read_count = bytes/size;	

	while(record_flg && read_count--)
	{
		if (pcm_read(pcm_in, buffer, size))
		{
			printf("pcm_read() error![%s]\n", strerror(errno)); 
			fclose(file);
			free(buffer);
			return -1;
		}
		fwrite(buffer, 1, size, file);
		fflush(file); 

	 	s_buffer = (unsigned short *)&buffer[0];
		i_buffer = (unsigned int *)&buffer[0];
		int len = size/4;
		for (i = 0; i < len; i++)
		{
			s_buffer[i] = (i_buffer[i] & 0xffff0000) >> 16;
		}  
		
		energy_ret = cal_energy((short *)(buffer), 9, 1);
		energy_out = set_energy_out(energy_ret);

		mic_height = height / 32 * energy_out;
		if(mic_height == 0)
			mic_height = 1;

		DFBCHECK(surface->SetColor(surface, 0, 0, 0, 0xff));
		DFBCHECK(surface->FillRectangle(surface, 0, 0, width, height));
		DFBCHECK(surface->SetColor(surface, 0xF8, 0xB6, 0x2C, 0xff));
		DFBCHECK(surface->FillRectangle(surface, 0, height - mic_height, width, mic_height));
		DFBCHECK(surface->Flip(surface, NULL, DSFLIP_NONE));
	}
	printf("record over\n");
	fclose(file);
	free(buffer);
	return 0;
}

void *tinyalsa_mic_thread(DirectThread *thread, void *arg)
{
	  /*Main loop*/ 
	instance_ref++; 
	for(;;)
	{
		pthread_mutex_lock(&mutex);
		while(record_flg == false)
			pthread_cond_wait(&condition, &mutex);
		play_avoid_flg = true;
		pthread_mutex_unlock(&mutex);
		
		pthread_mutex_lock(&lock);
		if (sample_capture("temp.wav", arg) < 0)
		{
			pthread_mutex_unlock(&lock);
			break;
		}
		if (sample_play("temp.wav") < 0)
		{
			pthread_mutex_unlock(&lock);
			break;
		}
		pthread_mutex_unlock(&lock);
		
		pthread_mutex_lock(&mutex);
		record_flg = false;
		play_avoid_flg = false;
		pthread_mutex_unlock(&mutex);
		pthread_cond_broadcast(&condition);
	}
	instance_ref--; 
	if (instance_ref == 0)
	{
		 pcm_cleanup();
	}
	printf("mic thread :: error and exit"); 
}

void *tinyalsa_music_thread(DirectThread *thread, void *arg)
{
	  /*Main loop*/ 
	instance_ref++; 
	for(;;)
	{
		pthread_mutex_lock(&mutex); 
		while(record_flg == true)
			pthread_cond_wait(&condition, &mutex);
		pthread_mutex_unlock(&mutex);

		pthread_mutex_lock(&lock);
		if (music_play() < 0)
		{
			pthread_mutex_unlock(&lock);
			break;
		}
		pthread_mutex_unlock(&lock);
		direct_thread_sleep(1000000);
	}
	instance_ref--;
	if (instance_ref == 0)
	{
		 pcm_cleanup();
	}
	printf("music thread :: error and exit"); 
}

#endif

#ifdef __GS702C__
/*
 * music play thread for gs702c
*/
void * music_thread(DirectThread *thread, void *arg)
{
 	int buf_size = 2048;
	int sound_fd = 0;
	int volume = 40;
	int mode = 5;
	FILE *fp = NULL;
	char *buf = NULL;
	int play_stop = 0;
	int play_count = 0;
	play_stop = 5 * sizeof(short) * 2 * 44100;
	bool wait_record = true;
	bool local_hdmi = false;
	bool local_headphone = false;
	
	sound_fd = open("/dev/actsnd", O_WRONLY);
	if (-1 == sound_fd)
	{
		printf("open sound device error\n");
		return;
	}
	
	fp = fopen("/misc/dk44k_2ch_30s.wav", "rb");
	if(NULL == fp)
	{
		printf("open sound file error\n");
		close(sound_fd);
		return;
	}

	ioctl(sound_fd, SOUND_MIXER_WRITE_VOLUME, &volume);	
	
	buf = (char *)malloc(buf_size);
	while(1)
	{
		fseek(fp, 0, SEEK_SET);
		while(fread(buf, buf_size, 1, fp))
		{
			write(sound_fd, buf, buf_size);
			play_count += buf_size;

			while(play_avoid_flg)
			{
				// printf("avoid record play\n");
				direct_thread_sleep(100000);
			}
			if(local_hdmi != hdmi_flg)
			{
				local_hdmi = hdmi_flg;
				if(local_hdmi)
				{
					mode = 5;
				}
				else
				{
					mode = 0;
				}
				ioctl(sound_fd, SNDRV_SOUTMODE, &mode);	
			}
			if(local_headphone != headphone_flg)
			{
				local_headphone = headphone_flg;
				if(local_headphone)
				{
					mode = 0;
				}
				else
				{
					mode = 1;
				}
				ioctl(sound_fd, SNDRV_SSPEAKER, &mode);
				printf("now speak is %d\n", mode);
			}
		}
		printf("play music end\n");
		direct_thread_sleep(1000000);
	}
	free(buf);
	close(sound_fd);
	fclose(fp); 
}

/*
 * mic capture thread for gs702c
*/

void * mic_thread(DirectThread *thread, void *arg)
{
	char *record_buf = NULL;
	int record_fd = 0;
	int play_fd = 0;
	int buf_size = 2048;
	char buf[buf_size];
	int width = 0, height = 0, mic_height;
	int energy_ret, energy_out;
	IDirectFBSurface *surface = arg;
	int record_size = 0;
	int record_time = 15;
	int record_count = 0;
	int play_count = 0;
	int read_size = 0;
	struct stat st;
	
	while (-1 == stat("/dev/actsnd", &st)) //获取文件信息
	{
		printf("sound driver not found\n");
		direct_thread_sleep(1000000);
	}
	
	play_fd = open("/dev/actsnd", O_WRONLY);
	if (-1 == play_fd)
	{
		printf("open sound device error\n");
		return;
	}
	
	record_fd = open("/dev/actsnd", O_RDONLY);
	if (-1 == record_fd)
	{
		printf("open sound device error\n");
		return;
	}
	
	record_size = (record_time + 1) * sizeof(short) * 2 * 44100;

	record_buf = (char *)malloc(record_size);
	if(NULL == record_buf)
	{
		printf("mic thread OOM\n");
		return;
	}
	record_size = record_time * sizeof(short) * 2 * 44100;

	while(1)
	{
		record_count = play_count = 0;
		while(!record_flg)
		{
			direct_thread_sleep(100000);
		}
		
		printf("start record\n");
	
		DFBCHECK(surface->GetSize(surface, &width, &height));
	
		while(record_flg)
		{
			play_avoid_flg = true;
			read_size = read(record_fd, buf, buf_size);
			if(read_size != buf_size)
			{
				printf("read_size = %d\n", read_size);
				break;
			}
			
			if(record_count < record_size)
			{
				memcpy(record_buf + record_count, buf, buf_size);
				record_count += read_size;
			}
			else
			{
				printf("record enough, auto stop\n");
				record_flg = false;
				break;
			}
			
			energy_ret = cal_energy((short *)(buf), 9, 1);
			energy_out = set_energy_out(energy_ret);

			mic_height = height / 32 * energy_out;
			if(mic_height == 0)
				mic_height = 1;

			DFBCHECK(surface->SetColor(surface, 0, 0, 0, 0xff));
			DFBCHECK(surface->FillRectangle(surface, 0, 0, width, height));
			DFBCHECK(surface->SetColor(surface, 0xF8, 0xB6, 0x2C, 0xff));
			DFBCHECK(surface->FillRectangle(surface, 0, height - mic_height, width, mic_height));
			DFBCHECK(surface->Flip(surface, NULL, DSFLIP_NONE));
		}
		// play_avoid_flg = false;
		printf("record over\n");
		
		if(!record_flg)
		{
			while(record_count > 0)
			{
				play_avoid_flg = true;
				memcpy(buf, record_buf + play_count, buf_size);
				play_count += buf_size;
				write(play_fd, buf, buf_size);
				record_count -= buf_size;
			}
			play_avoid_flg = false;
		}
	}
	close(record_fd);
	close(play_fd);
	free(record_buf);
}
#endif