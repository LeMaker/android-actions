/*
 * test_pcba.c
 *
 *  Created on: 2013-7-1
 *      Author: xuyingfei
 */
#include <directfb.h>
#include <linux/videodev2.h>
#include <linux/soundcard.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "vad_detect.h"
#include "encoderpp.h"
#include "camera.h"
#include "case.h"

extern bool hdmi_flg;
extern bool headphone_flg;
extern bool record_flg;


//702c or tinyalsa
#ifdef __GS702C__
extern void * music_thread(DirectThread *thread, void *arg);
extern void * mic_thread(DirectThread *thread, void *arg);
#else
extern void * tinyalsa_mic_thread(DirectThread *thread, void *arg);
extern void * tinyalsa_music_thread(DirectThread *thread, void *arg);
#endif


const int case_num = global - 2;	
static iio_info info[3];

/*************************
****global configs********
*************************/
int camera_pixel_width = 640;
int camera_pixel_height = 480;
int camera_frame = 0;
int font_size = 24;
char *ictype = NULL;



IDirectFBSurface *windowsurface;
IDirectFBSurface *camera_source;
IDirectFB  *dfb;



void deal_with_ic_type(case_t *case_x)
{
	ictype = (case_x + global)->dev_name;
	printf("ic type is atm%s\n", ictype);
	//default ic is atm7029b
	if(strlen(ictype) == 0)
	{
		strcpy(ictype, "7029b");
	}
	if(!strcmp(ictype, "7021"))
	{
		strcpy(case_x[usb].dev_name, "7021");
		strcpy(case_x[usbpc].dev_name, "7021");
	}
	else if(!strcmp(ictype, "7029a"))
	{
		strcpy(case_x[sdcard].dev_name, "7029a");
	}
	else if(!strcmp(ictype, "7059"))
	{
		strcpy(case_x[usb].dev_name, "7059");
		strcpy(case_x[usbpc].dev_name, "7059");
		strcpy(case_x[sdcard].dev_name, "7059");
	}
	else if(!strcmp(ictype, "9009"))
	{
		strcpy(case_x[usb].dev_name, "9009");
		strcpy(case_x[usbpc].dev_name, "9009");
		strcpy(case_x[sdcard].dev_name, "9009");
	}
}


void deal_with_common_sets(char *buf)
{
	char *s = buf;
	while(NULL != s && '\0' != s)
	{
		if('c' == s[0])
		{
			camera_frame = atoi(s + 1);
		}
		if('f' == s[0])
		{
			font_size = atoi(s + 1);
		}
		if ('p' == s[0])
		{
			if (strchr(s + 1, '*') != NULL)
			{
				camera_pixel_width = atoi(s + 1);
				s = strchr(s + 1 , '*');
				camera_pixel_height = atoi(s + 1);
			}
		}
		s = strchr(s + 1, ':');
		if(s != NULL)
			s++;
	}	
}

void parse_global_config(case_t *case_x)
{
	deal_with_ic_type(case_x);
	deal_with_common_sets(case_x[global].nod_path);
}



void drawRGB(IDirectFBSurface *surface, int x, int y, int width, int height)
{
	int width_r = width / 5;
	DFBCHECK(surface->SetColor(surface, 0xff, 0, 0, 0xff));
	DFBCHECK(surface->FillRectangle(surface, x, y, width_r , height / 2));
	DFBCHECK(surface->SetColor(surface, 0, 0xff, 0, 0xff));
	DFBCHECK(surface->FillRectangle(surface, x + width_r, y, width_r , height / 2));
	DFBCHECK(surface->SetColor(surface, 0, 0, 0xff, 0xff));
	DFBCHECK(surface->FillRectangle(surface, x + 2 * width_r, y, width_r , height / 2));
	DFBCHECK(surface->SetColor(surface, 0, 0, 0, 0xff));
	DFBCHECK(surface->FillRectangle(surface, x + 3 * width_r, y, width_r , height / 2));
	DFBCHECK(surface->SetColor(surface, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(surface->FillRectangle(surface, x + 4 * width_r, y, width - 4 * width_r , height / 2));
	DFBCHECK(surface->FillRectangle(surface, x , y + height / 2, width, height / 2));
	DFBCHECK(surface->Flip(surface, NULL, 0));	
}

static void * tp_thread(DirectThread *thread, void *arg)
{	
	int rotate = 0;
	case_t *case_tp = arg;
	// printf("tp dev name %s\n", case_tp->dev_name);
	if(case_tp->nod_path[5] == '1')
	{
		printf("got a interrupt tp\n");
		wait_tp(case_tp->dev_name);
	}
	else
		printf("got a polling tp\n");
	
	rotate = atoi(case_tp->nod_path);
	
	if(rotate > 0)
	{
		rotate--;
	}
	else
	{
		rotate += 5;
	}
	if(rotate < 0 || rotate > 7)
	{
		rotate = 0;
	}
	
	track_tp(case_tp->surface, case_tp->dev_name, rotate);
}

static void * camera_thread(DirectThread *thread, void *arg)
{
	int camera_dev_num = 0;
	int second_dev_num = -1;
	int fid;
	struct stat st;
	bool same_sensor_flg = false;
	char *back = "b";
	char *front = "f";
	int cur_stat = 0;
	int *num = arg;
    int width = camera_pixel_width;
	int height = camera_pixel_height;
    
	printf("camera num = %d\n", *num);
	
	if (0 == stat("/dev/video3", &st))
	{
		printf("got two sensor at video0 video3\n");
		second_dev_num = 3;
	}
#ifndef __GS702C__
	else if (0 == stat("/dev/video1", &st))
	{

		printf("got two sensor at video0 video1\n");
		second_dev_num = 1;
	}
#endif
	if (second_dev_num < 0)
	{
		same_sensor_flg = true;
		printf("got two same sensor\n");
	}

	while(true)
	{
		do
		{
			if(open_camera_device(camera_dev_num, *num) == -1)
			{
				printf("open camera %d error\n", camera_dev_num);
				direct_thread_sleep( 1000000 );
				break;
			}
			if(init_camera_device(&width, &height) == -1)
			{
				printf("init camera %d error\n", camera_dev_num);
				close_camera_device();
				break;
			}
			camera_start_capturing();
			camera_mainloop();
			camera_stop_capturing();
			uninit_camera_device();
			close_camera_device();
		}while(0);
		
		if(same_sensor_flg)
		{
			fid = open("/sys/bus/platform/devices/camera_flag_device.0/front_back", O_WRONLY);
			if(0 == cur_stat)
			{
				write(fid,back,1);
				close(fid);
				cur_stat = 1;
			}
			else
			{
				write(fid,front,1);
				close(fid);
				cur_stat = 0;
			}
		}
		else
		{
			if(camera_dev_num)
			{
				camera_dev_num = 0;
				//printf("switch to video0, the rear camera \n");
			}
			else
			{
				camera_dev_num = second_dev_num;
				printf("switch to video%d, front-facing cameras \n", second_dev_num);
			}
		}
	}
} 

static void * case_thread(DirectThread *thread, void *arg)
{
	int ret;
	case_t *caseP = arg;
	bool passflg = false;
	FILE *fp = NULL;
	char command[100];
	struct stat st;
	bool iio_enable;
	
	iio_enable = is_iio_enable();
	printf("iio_enable = %d\n", iio_enable);

	if(caseP->type == 0)
	{
		switch(caseP->index)
		{
			//mem
			case mem:
				sprintf(command, "memtester %s 1 2 > /memerr.log 1> /dev/null", caseP->nod_path);
				// printf("command = %s\n", command);
				ret = system(command);
				// printf("mem ret = %d\n", ret);
				if(!ret && stat("/memerr.log", &st) >= 0) //获取文件信息
				{
					if(st.st_size == 0)
					{
						passflg = true;
					}
				}
				break;
			case ddrsize:
				passflg = test_ddrsize(caseP);
				break;	
			//flash	
			case flash:;
				passflg = test_flash(caseP);
				break;
			//wifi
			case wifi:
				passflg = test_wifi(caseP);
				break;
			case bt:
				passflg = test_bt(caseP);
				break;
			//gsensor	
			case gsensor:
				if(!iio_enable)
					passflg = test_gsensor(caseP);
				else
					passflg = test_iio(caseP, info);
				break;
			case gyro:
				if(!iio_enable)
					passflg = false;
				else
					passflg = test_iio(caseP, info + 1);
				break;
			case comp:
				if(!iio_enable)
					passflg = false;
				else
					passflg = test_iio(caseP, info + 2);
				break;
			case lightsensor:
				passflg = test_lightsensor(caseP);
				break;
			//camera	
			// case camera:
				// passflg = wait_dev_plugin(caseP->nod_path);
				// break;
			//rtc
			case rtc:
				passflg = test_rtc(caseP);
				break;
			case gps:
				passflg = test_gps(caseP);
				break;
			case uart:
				passflg = test_uart(caseP);
				break;
			case ethernet:
				passflg = test_ethernet(caseP);
				break;
		}
	}
	else if(caseP->type == 1)
	{
		switch(caseP->index)
		{
			//sdcard
			case 0:
				passflg = test_mmc(caseP);
				break;
			//usb
			case 1:
				passflg = test_usb(caseP);
				break;
			case 2:
				passflg = test_pc(caseP);
				break;
			//hdmi
			case 3:
				wait_nod_state(caseP, &hdmi_flg);
				//test_hdmi(caseP, &hdmi_flg);
				break;
			//headphone
			case 4:
				wait_nod_state(caseP, &headphone_flg);
				break;
			//key
			case 5:
				passflg = test_key(caseP);
				break;
			//onoff
			case 6:
				passflg = test_key_onff(caseP);
				break;
			//charege
			case 7:
				passflg = test_charge(caseP);
				break;
			//mtv
			case 8:
				passflg = test_mtv(caseP);
				break;
		}
	}
	
	draw_result(caseP, passflg);
}

static void * flashlight_thread(DirectThread *thread, void *arg)
{
	case_t *case_flashlight = arg;
	
	test_flashlight(case_flashlight);
}


static void *handle_record_button(DirectThread *thread, void *arg)
{
	IDirectFBSurface *surface = arg;
	IDirectFBFont 		*font, *font_s;
	DFBFontDescription 	font_desc;
	bool local_flg = false;
	int width, height;
	int font_big_height, font_little_height;
	
	char font_file[50] = "/misc/font/wqy-zenhei.ttc";
	
	DFBCHECK(surface->GetSize(surface, &width, &height));
	
	font_big_height = 48;
	while(font_big_height > (height / 2))
	{
		font_big_height -= 4;
	}
	font_desc.flags = DFDESC_HEIGHT;
	font_desc.height = font_big_height;
	DFBCHECK(dfb->CreateFont( dfb, font_file,  &font_desc, &font));
	
	font_little_height = 32;
	while(font_little_height > (height / 4))
	{
		font_little_height -= 4;
	}
	font_desc.height = font_little_height;
	DFBCHECK(dfb->CreateFont( dfb, font_file,  &font_desc, &font_s));
	
	printf("font size is %d %d\n", font_big_height, font_little_height);
	
	DFBCHECK(surface->SetFont(surface, font_s));
	
	DFBCHECK(surface->SetColor(surface, 0x8C, 0x8C, 0x8C, 0xff));
	DFBCHECK(surface->DrawString(surface, "点击录音", -1, width / 2, 0, DSTF_TOPCENTER));
	
	DFBCHECK(surface->SetColor(surface, 0x41, 0x41, 0x41, 0xff));
	DFBCHECK(surface->DrawString(surface, "测试TP请避开此区域", -1, width / 2, height / 4, DSTF_TOPCENTER));
	
	DFBCHECK(surface->SetFont(surface, font));
	DFBCHECK(surface->SetColor(surface, 0xE3, 0x6C, 0x4C, 0xff));
	DFBCHECK(surface->DrawString(surface, "状态：停止录音", -1, width / 2, height / 2, DSTF_TOPCENTER));
	DFBCHECK(surface->Flip(surface, NULL, 0));
	while(true)
	{
		if(record_flg != local_flg)
		{
			local_flg = record_flg;
			DFBCHECK(surface->SetColor(surface, 0xff, 0xff, 0xff, 0xff));
			DFBCHECK(surface->FillRectangle(surface, 0 , height / 2, width, font_big_height + 4));  //need fix
			DFBCHECK(surface->SetColor(surface, 0xE3, 0x6C, 0x4C, 0xff));
			if(local_flg)
			{
				DFBCHECK(surface->DrawString(surface, "状态：正在录音", -1, width / 2, height / 2, DSTF_TOPCENTER));
			}
			else
			{
				DFBCHECK(surface->DrawString(surface, "状态：停止录音", -1, width / 2, height / 2, DSTF_TOPCENTER));
			}
			DFBCHECK(surface->Flip(surface, NULL, 0));
		}
		direct_thread_sleep(100000);
	}
	
	font->Release(font);
	font_s->Release(font_s);
}

int main(int argc, char *argv[])
{
	IDirectFBDisplayLayer 	*layer;
	DFBWindowDescription	dwsc;
	DFBSurfaceDescription 	dsc;
	DFBDisplayLayerConfig	config;
	IDirectFBWindow 	*window_mic, *window_camera, *window_case, *window_tp;
	IDirectFBSurface 	*surface_case, *surface_tp, *surface_camera;
	IDirectFBSurface 	*surface_mic, *primary, *surface_button;
	IDirectFBFont 		*font;
	DFBRectangle		rect;
	DFBFontDescription 	font_desc;
	DirectThread     *thread_camera, *thread_music, *thread_mic, *thread_tp, *thread_flashlight, *thread_case[case_num];
	DirectThread 	*thread_button;
	 

//	int font_size = 24;
//	int camera_frame = 0;

	int i;
	char font_file[50] = "/misc/font/wqy-zenhei.ttc";
	char conf_file[50] = "/misc/pcba.conf";

	
	bool overflow = false;
	
	case_t *case_x;
	case_x = malloc((case_num + 3) * sizeof(case_t));
	
	//20 case,11 auto case,gps is the last autocase
	init_case(case_x, case_num + 3, gps + 1);
	
	if(parser_config_file(conf_file, case_x))
	{
		printf("config file open error, use default\n");
	}
	
	parse_global_config(case_x);

	printf("camera_frame = %d\n", camera_frame);
	printf("font_size = %d\n", font_size);
	printf("camera pixel [%d*%d]\n", camera_pixel_width, camera_pixel_height);

#ifndef __GS900A__
	ion_open_alc();
#endif
	//define window size
	int screen_width, screen_height;
	int mic_width, camera_window_width, camera_window_height;
	int case_width, case_name_align, case_result_align;
	int font_height;
	
	//change env
	char event_path[100];
	char env_gsensor[256];
	
	//enable gsensor
	i = get_input_event_name(case_x[gsensor].dev_name, event_path);
	if(i != -1)
	{
		sprintf(env_gsensor, "echo 1 > /sys/class/input/input%d/enable", i);
		system(env_gsensor);
	}
	else
	{
		printf("----error! gsensor not found\n");
	}
	
	//vibrate test for 3 second
	test_vibrate(3);
	
	//dfb init
	DFBCHECK(DirectFBInit(&argc, &argv));
	DFBCHECK(DirectFBCreate(&dfb));
	
	font_desc.flags = DFDESC_HEIGHT;
	font_desc.height = font_size;
	DFBCHECK(dfb->CreateFont( dfb, font_file,  &font_desc, &font));
	DFBCHECK(font->GetHeight(font, &font_height));
	DFBCHECK(dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &layer));
	DFBCHECK(layer->GetConfiguration( layer, &config ));
	// DFBCHECK(layer->SetRotation( layer, 180 ));
	//set window size
	screen_width = config.width;
	screen_height = config.height;
	mic_width = screen_width / 20;
	camera_window_width = (screen_width - mic_width) / 2;
	camera_window_height = screen_height / 2;
	case_width = screen_width - mic_width - camera_window_width;
	case_name_align = font_size;
	case_result_align = 7 * font_size;
	
	//create tp window
	dwsc.flags = DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS;
	dwsc.posx = 0;
	dwsc.posy = 0;
	dwsc.width = screen_width;
	dwsc.height = screen_height;
	dwsc.caps = DWCAPS_ALPHACHANNEL;
	
	DFBCHECK(layer->CreateWindow(layer, &dwsc, &window_tp));
	DFBCHECK(window_tp->GetSurface(window_tp, &surface_tp));
	DFBCHECK(window_tp->SetOptions(window_tp, DWOP_ALPHACHANNEL));
	DFBCHECK(window_tp->SetStackingClass(window_tp, DWSC_UPPER));
	DFBCHECK(window_tp->RaiseToTop(window_tp));
	DFBCHECK(surface_tp->Clear(surface_tp, 0, 0, 0, 0));
	DFBCHECK(surface_tp->SetColor(surface_tp, 0, 0xff, 0, 0xff));
	DFBCHECK(window_tp->SetOpacity(window_tp, 0xff));
	
	//config camera window
	dwsc.flags = DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT;
	dwsc.posx = case_width + mic_width;
	dwsc.posy = 0;
	dwsc.width = camera_window_width;
	dwsc.height = screen_height;
	
	DFBCHECK(layer->CreateWindow( layer, &dwsc, &window_camera ));
	DFBCHECK(window_camera->GetSurface(window_camera, &windowsurface));
	DFBCHECK(windowsurface->SetColor(windowsurface, 0, 0, 0, 0xff));
	DFBCHECK(windowsurface->FillRectangle(windowsurface, 0, 0, camera_window_width , screen_height));
	DFBCHECK(windowsurface->Flip(windowsurface, NULL, 0));
	DFBCHECK(window_camera->SetOpacity( window_camera, 0xff ));
	
	drawRGB(windowsurface, 0, screen_height / 2, camera_window_width, screen_height /2);
	
	rect.x = 0;
	rect.y = 0;
	rect.w = camera_window_width;
	rect.h = camera_window_height;
	DFBCHECK(windowsurface->GetSubSurface(windowsurface, &rect, &surface_camera));
	
	rect.x = 0;
	rect.y = screen_height / 4 * 3;
	rect.w = camera_window_width;
	rect.h = screen_height - rect.y;
	DFBCHECK(windowsurface->GetSubSurface(windowsurface, &rect, &surface_button));
	
	//create camera surface
	dsc.flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
    dsc.pixelformat = DSPF_NV12;
	dsc.width = camera_pixel_width;
	dsc.height = camera_pixel_height;
	DFBCHECK(dfb->CreateSurface(dfb, &dsc, &camera_source));
	
	//config mic window
	dwsc.flags = DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT;
	dwsc.posx = case_width;
	dwsc.posy = 0;
	dwsc.width = mic_width;
	dwsc.height = screen_height;
	
	DFBCHECK(layer->CreateWindow(layer, &dwsc, &window_mic));
	DFBCHECK(window_mic->GetSurface(window_mic, &surface_mic));
	DFBCHECK(surface_mic->SetColor(surface_mic, 0, 0, 0, 0xff));
	DFBCHECK(surface_mic->FillRectangle(surface_mic, 0, 0, mic_width , screen_height));
	DFBCHECK(surface_mic->Flip(surface_mic, NULL, 0));
	DFBCHECK(window_mic->SetOpacity( window_mic, 0xff));
	
	//config case window
	dwsc.posx = 0;
	dwsc.posy = 0;
	dwsc.width = case_width;
	dwsc.height = screen_height;
	
	DFBCHECK(layer->CreateWindow(layer, &dwsc, &window_case));
	DFBCHECK(window_case->GetSurface(window_case, &surface_case));
	DFBCHECK(surface_case->SetColor(surface_case, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(surface_case->SetFont(surface_case, font));
	DFBCHECK(surface_case->FillRectangle(surface_case, 0, 0, case_width, screen_height));
	
	//draw headline
	DFBCHECK(surface_case->SetColor(surface_case, 0xff, 0xff, 0, 0xff));
	DFBCHECK(surface_case->FillRectangle(surface_case, 0, 0, case_width, font_size + 2));
	DFBCHECK(surface_case->FillRectangle(surface_case, 0, screen_height / 2, case_width, font_size + 2));
	DFBCHECK(surface_case->SetColor(surface_case, 0, 0, 0, 0xff));
	DFBCHECK(surface_case->DrawString(surface_case, "自动测试选项", -1, case_width / 2, 0, DSTF_TOPCENTER));
	DFBCHECK(surface_case->DrawString(surface_case, "手动测试选项", -1, case_width / 2, screen_height / 2, DSTF_TOPCENTER));
	
	int auto_skip = 0;
	int manu_skip = 0;
	int skip_height = 0;
	bool double_line = false;
	//draw string
	for(i = 0; i < case_num; i++)
	{
		double_line = false;
		if(overflow)
		{
			case_x[i].enable = 0;
			continue;
		}
		if(!case_x[i].enable)
		{
			if(case_x[i].type)
				manu_skip++;
			else
				auto_skip++;
				
			//case_x[i].position = -1;
			continue;
		}
		if(case_x[i].doubleline == 1)
		{
			double_line = true;
		}
		if(case_x[i].type)
			skip_height = manu_skip * (font_height);
		else
			skip_height = auto_skip * (font_height);
		
		case_x[i].position = case_x[i].type * screen_height / 2 + (case_x[i].index + 1) * (font_height);
		case_x[i].position -= skip_height;
		
		// printf("name_h = %d\n", case_x[i].position);
		DFBCHECK(surface_case->SetColor(surface_case, 0, 0, 0, 0xff));
		DFBCHECK(surface_case->DrawString(surface_case, case_x[i].name, -1, case_name_align, case_x[i].position, DSTF_TOPLEFT));
		DFBCHECK(surface_case->SetColor(surface_case, 0xc0, 0xc0, 0xc0, 0xff));
		DFBCHECK(surface_case->DrawString(surface_case, case_x[i].init_string, -1, case_result_align, case_x[i].position, DSTF_TOPLEFT));
		
		rect.x = case_result_align;
		rect.y = case_x[i].position;
		rect.w = case_width - case_result_align;
		rect.h = font_height;
		if((rect.y + rect.h) > screen_height)
		{
			overflow = true;
			case_x[i].enable = 0;
			printf("case %d overflow, disable all case after it\n", i);
			continue;
		}
		if(double_line)
		{
			// case_x[i].position = -2;
			rect.h *= 2;
			if(case_x[i].type)
				manu_skip--;
			else
				auto_skip--;
		}
		DFBCHECK(surface_case->GetSubSurface(surface_case, &rect, &case_x[i].surface));
		DFBCHECK(case_x[i].surface->SetFont(case_x[i].surface, font));
	}
	
	DFBCHECK(window_case->SetOpacity(window_case, 0xff ));

#ifdef __GS900A__
	system("echo 1 > /sys/devices/e0250000.hdmi/enable");
#endif
	//start case thread
	for(i = 0; i < case_num; i++)
	{
		char thread_name[10];
		sprintf(thread_name,"thread_%d", i);
		if(case_x[i].enable)
		{
			thread_case[i] = direct_thread_create(DTT_DEFAULT, case_thread, &case_x[i], thread_name);
		}
	}
	
	case_x[tp].surface = surface_tp;
	
	//start thread
#ifdef __GS702C__
	thread_music = direct_thread_create(DTT_DEFAULT, music_thread, NULL, "music");
	thread_mic = direct_thread_create(DTT_DEFAULT, mic_thread, surface_mic, "mic");
#else
	thread_music  = direct_thread_create(DTT_DEFAULT, tinyalsa_music_thread, NULL, "tinyalsa_music");
	thread_mic = direct_thread_create(DTT_DEFAULT, tinyalsa_mic_thread, surface_mic, "tinyalsa_mic");
#endif

	thread_tp = direct_thread_create(DTT_DEFAULT, tp_thread, &case_x[tp], "tp");
	thread_flashlight = direct_thread_create(DTT_DEFAULT, flashlight_thread, &case_x[flashlight], "flashlight");
	thread_camera = direct_thread_create(DTT_DEFAULT, camera_thread, &camera_frame, "camera" );
	//thread_mic = direct_thread_create(DTT_DEFAULT, mic_thread, surface_mic, "mic");
	thread_button = direct_thread_create(DTT_DEFAULT, handle_record_button, surface_button, "button");
	sleep(1);
	
	DFBCHECK(surface_case->Flip(surface_case, NULL, 0));
	
	if(iio_read(info, case_x[gsensor].enable, case_x[gyro].enable, case_x[comp].enable) == -1)
		printf("get iio device error\n");
		
	//block in iio_read
	//useless statement
	for(i = 0; i < case_num; i++)
	{
		if(case_x[i].enable)
		{
			direct_thread_join(thread_case[i]);
			case_x[i].surface->Release(case_x[i].surface);
			direct_thread_destroy(thread_case[i]);
			printf("thread %d destroyed\n", i);
		}
	}

	direct_thread_join(thread_camera);
	direct_thread_destroy(thread_camera);
	direct_thread_join(thread_music);
	direct_thread_destroy(thread_music);
	direct_thread_join(thread_mic);
	direct_thread_destroy(thread_mic);

	surface_button->Release(surface_button);
	surface_camera->Release(surface_camera);
	camera_source->Release(camera_source);
	windowsurface->Release(windowsurface);
	window_camera->Release(window_camera);
	
	surface_case->Release(surface_case);
	window_case->Release(window_case);
	layer->Release(layer);
	font->Release(font);
	dfb->Release(dfb);
	
#ifndef __GS900A__
	ion_close_alc();
#endif
}
