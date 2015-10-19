#ifndef __CASE_H___
#define __CASE_H___
#include <directfb.h>
#include<linux/input.h>
#include <poll.h>
// #define _5202C_
// #define _GS702C_

/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x...) \
     {                                                               \
           int err = x;                                                  \
           if (err != DFB_OK) {                                      \
              fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
              DirectFBErrorFatal( #x, err );                         \
           }                                                         \
     }

#define SNDRV_SOUTMODE                 0xffff0000
#define SNDRV_SSPEAKER                 0xffff0001
	 
typedef struct {
    unsigned int type;			//自动or手动
    unsigned int index;       	//号码
	unsigned int position;		//位置
	int enable;					//是否可用
    char name[20];         		//测试名字
	char dev_name[100];			//驱动名字			
	char nod_path[60];			//参数
	char init_string[50];       
	char pass_string[100];
	char fail_string[50];
	int  doubleline;
	IDirectFBSurface *surface;
}case_t;

typedef struct {
    bool enable;
    bool detected;
	unsigned int position;		//位置
	unsigned int count;			//计数
    char name[20];         		//按键名字
	char *para;					//其他参数
}key_para;

typedef struct {
    int lock;
	short x;
	short y;
	short z;
}iio_info;
enum case_name {
	mem, 		//0
	ddrsize, 
	flash, 
	wifi, 
	bt, 
	gsensor, 	//5
	gyro, 
	comp, 
	lightsensor, 
	rtc, 
	uart,       //10
	ethernet, 
	gps,		
	sdcard, 
	usb, 
	usbpc,     //15
	hdmi, 
	headphone, 	
	key, 
	onoff, 
	charge,   //20
	mtv, 
	tp, 		
	flashlight,
	global
};

int parser_config_file(char *, case_t *);

bool wait_dev_plugin(char *);
bool is_nod_exists(char *);
int cat_file(char *);
int get_input_event_name(char *, char *);
void draw_result(case_t *, bool);
void wait_nod_state(case_t *, bool *);
bool pcba_system(char *);

bool test_wifi(case_t *);
bool test_bt(case_t *);
bool test_gsensor(case_t *);
bool test_iio(case_t *, iio_info *);
bool test_key(case_t *);
bool test_key_onff(case_t *);
bool test_mmc(case_t *);
bool test_rtc(case_t *);
bool test_gps(case_t *);
bool test_usb(case_t *);
bool test_mtv(case_t *);
bool test_charge(case_t *);
bool test_lightsensor(case_t *);
void test_vibrate(int);
void test_flashlight(case_t *);
bool test_uart(case_t *);
bool test_ethernet(case_t *);

int wait_tp(char *);
int track_tp(IDirectFBSurface *, char *, int);
#endif