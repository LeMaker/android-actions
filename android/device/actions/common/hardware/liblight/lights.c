/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "lights"

#include <cutils/log.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <sys/types.h>

#include <hardware/lights.h>

#define MAPPING_POINT_NUM 3
static double android_backlight_percentage_array[MAPPING_POINT_NUM] = {0, 0.5f, 1};
static double actual_backlight_percentage_array[MAPPING_POINT_NUM] = {0, 0.3f, 1};

struct line_coefficient_t {
	double a;
	double b;
};
/******************************************************************************/
static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static int max_brightness = -1;
static int dim_brightness = 0;

/* brightness & max_brightness file name */
static char f_backlight_dir[100] = "/sys/class/backlight/owl_backlight";
static char f_brightness[100];
static char f_max_brightness[100];

const char *SETTING_BRIGHTNESS_FILE = "/data/setting/setting_backlight_value";
const char *SETTING_AUTO_ADJUST_FILE = "/data/setting/setting_backlight_auto_enable";
//char const *const f_brightness_MIN = "/sys/class/backlight/backlight.2/min_brightness";
/**
 * device methods
 */

void init_globals(void)
{
	int i;
	
	// init the mutex
	ALOGD("init_globals");
	pthread_mutex_init(&g_lock, NULL);
	
	if ((access(f_backlight_dir, F_OK)) < 0) {
		/* search for backlight's directory */
		f_backlight_dir[0] = 0;
		for (i = 0; i < 20; i++) {
			sprintf(f_backlight_dir,
				"/sys/class/backlight/backlight.%d", i);
			if ((access(f_backlight_dir, F_OK)) < 0) {
				f_backlight_dir[0] = 0;
				continue;
			} else {
				break;
			}
		}
	}
	
	if (f_backlight_dir[0] == 0) {
		ALOGD("can not get backlight directory");
		return;
	}
	sprintf(f_brightness, "%s/brightness", f_backlight_dir);
	sprintf(f_max_brightness, "%s/max_brightness", f_backlight_dir);
	
	ALOGD("%s: %s", __func__, f_backlight_dir);
	ALOGD("%s: %s", __func__, f_brightness);
	ALOGD("%s: %s", __func__, f_max_brightness);
}

static int write_int(char const *path, int value)
{
	int fd;
	
	fd = open(path, O_RDWR, 0);
	
	//ALOGI("write_int: path %s, value %d", path, value);
	//ALOGI("max_brightness = %d, dim_brightness = %d", max_brightness, dim_brightness);

	if (fd >= 0) {
		char buffer[20];
		int bytes = sprintf(buffer, "%d\n", value);
		int amt = write(fd, buffer, bytes);
		close(fd);
		return amt == -1 ? -errno : 0;
	} else {
	  ALOGE("write_int failed to open %s\n", path);
		ALOGE("fd=%d errno=%d (%s)", fd, errno, strerror(errno));
		return -errno;
	}
}

static int readFromFile(const char* path, char* buf, size_t size)
{
	if (!path)
		return -1;
	int fd = open(path, O_RDONLY, 0);
	if (fd == -1) {
		ALOGE("Could not open '%s'", path);
		ALOGE("fd=%d errno=%d (%s)", fd, errno, strerror(errno));
		return -1;
	}
	
	size_t count = read(fd, buf, size);
	if (count > 0) {
		count = (count < size) ? count : size - 1;
		while (count > 0 && buf[count-1] == '\n') count--;
		buf[count] = '\0';
	} else {
		buf[0] = '\0';
	} 
	
	close(fd);
	return count;
}

static int rgb_to_brightness(struct light_state_t const *state)
{
	int color = state->color & 0x00ffffff;

	return ((77*((color>>16) & 0x00ff))
		+ (150*((color>>8) & 0x00ff)) + (29*(color & 0x00ff))) >> 8;
}

static struct line_coefficient_t get_line_coefficient(double x0, double y0, double x1, double y1) {
	struct line_coefficient_t coeff;

	if(x0 != x1) {
		coeff.a = (y1 - y0) / (x1 - x0);
		coeff.b = (y0*x1 - y1*x0) / (x1 - x0);
	}
	return coeff;
}

static int getBrightnessByLineMapping(int brightness) {
	static int coeff_inited = 0;
	static struct line_coefficient_t coeffs[MAPPING_POINT_NUM-1];
	
	if(!coeff_inited) {
		int i = 0;
		double m[MAPPING_POINT_NUM];
		double n[MAPPING_POINT_NUM];

		for(i = 0; i < MAPPING_POINT_NUM; i++) {
			m[i] = android_backlight_percentage_array[i]
				* BRIGHTNESS_ON;
			n[i] = actual_backlight_percentage_array[i]
				* (max_brightness -  dim_brightness)
				+ dim_brightness;
			if(i > 0) {
				coeffs[i-1] = get_line_coefficient(m[i-1], n[i-1], m[i], n[i]);
			}
		}
		coeff_inited = 1;
	}

	if(brightness) {
		int i =1;
		for(; i < MAPPING_POINT_NUM; i++) {
			if(brightness <= android_backlight_percentage_array[i] * BRIGHTNESS_ON) {
				break;
			}
		}
		
		brightness = coeffs[i-1].a*brightness + coeffs[i-1].b;
	}
	
	return brightness;
}

static int set_light_backlight(struct light_device_t *dev,
			struct light_state_t const *state)
{
	int err = 0;	
	int brightness = rgb_to_brightness(state);
	char buf[512];
	int atuo_adjust = 0;
	brightness = getBrightnessByLineMapping(brightness);

	pthread_mutex_lock(&g_lock);
	
	if(readFromFile(SETTING_AUTO_ADJUST_FILE, buf, 512) == -1){
		buf[0] = '0';
		ALOGE("readFromFile %s failed ", SETTING_AUTO_ADJUST_FILE);

		atuo_adjust = 0;
	} else {
		atuo_adjust = atoi(buf);
	}

	//ALOGE("set_light_backlight atuo_adjust %d ", atuo_adjust);
	if(atuo_adjust){
		write_int(SETTING_BRIGHTNESS_FILE, brightness);		
	}else{
		err = write_int(f_brightness, brightness);
	}
	pthread_mutex_unlock(&g_lock);
	
	return err;
}

/** Close the lights device */
static int close_lights(struct light_device_t *dev) 
{
	if (dev) {
		free(dev);
	}
	return 0;
}
/******************************************************************************/

/**
 * module methods
 */

/** Open a new instance of a lights device using name */
static int open_lights(const struct hw_module_t* module, char const* name,
			struct hw_device_t** device)
{
	static bool first_open = true;
	char buf[10];
	int (*set_light)(struct light_device_t* dev,
	struct light_state_t const* state);
	
	if (0 == strcmp(LIGHT_ID_BACKLIGHT, name)) {
		set_light = set_light_backlight;
		ALOGD("#######open_lights  (%s)######\n", name); 
	} else {
		return -EINVAL;
	}
	
	pthread_once(&g_init, init_globals);
	
	struct light_device_t *dev = malloc(sizeof(struct light_device_t));
	memset(dev, 0, sizeof(*dev));
	
	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = (struct hw_module_t*)module;
	dev->common.close = (int (*)(struct hw_device_t*))close_lights;
	dev->set_light = set_light;
	
	*device = (struct hw_device_t*)dev;
	
	pthread_mutex_lock(&g_lock);
	if(first_open) {
		if(readFromFile(f_max_brightness, buf, 10) == -1){
			pthread_mutex_unlock(&g_lock);
			return -1;
		}
			
		max_brightness = atoi(buf);
			
		/*dim_brightness is a constant value 0 --removed by zzzhou*/
		#if 0
		if(readFromFile(f_brightness_MIN, buf, 10) == -1){
			pthread_mutex_unlock(&g_lock);
			return -1;
		}
		dim_brightness = atoi(buf);
		#endif
					
  		first_open = false;
	}
	pthread_mutex_unlock(&g_lock);
	ALOGD("#######open_lights  (%s) OK######\n", name); 
	return 0;
}


static struct hw_module_methods_t lights_module_methods = {
	.open =  open_lights,
};

/*
 * The lights Module
 */
struct hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = LIGHTS_HARDWARE_MODULE_ID,
	.name = "lights Module",
	.author = "actions",
	.methods = &lights_module_methods,
};
