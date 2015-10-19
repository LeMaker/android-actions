#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <errno.h>
#include <stdarg.h>
#include <mtd/mtd-user.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>   
#include <sys/select.h>   
#include <errno.h>   
#include <sys/inotify.h>  
#include <pthread.h>
#include <sys/sysinfo.h>
#include "property_service.h"
#include "log.h"

static int work_done=0;
static void do_some_hack(void)
{
	
	int ret;
	int fd;
	char *prefs="/data/data/com.google.android.inputmethod.pinyin/shared_prefs/com.google.android.inputmethod.pinyin_preferences.xml";
	char *prefs_dir1="/data/data/com.google.android.inputmethod.pinyin";
	char *prefs_dir2="/data/data/com.google.android.inputmethod.pinyin/shared_prefs";
	char *data= "<?xml version='1.0' encoding='utf-8' standalone='yes' ?> \n\
	<map> \n\
	<boolean name=\"enable_sound_on_keypress\" value=\"true\" /> \n\
    <int name=\"HAD_FIRST_RUN\" value=\"1\" />     \n\
    </map> \n";
	
	ret=mkdir(prefs_dir1, S_IRWXU|S_IRGRP|S_IWGRP|S_IRWXO);
	if(ret!=0){
		ERROR("failed to create %s \n", prefs_dir1);
	}
	mkdir(prefs_dir2, S_IRWXU|S_IRGRP|S_IWGRP|S_IRWXO);
	if(ret!=0){
		ERROR("failed to create %s \n", prefs_dir2);
	}
	fd=open(prefs,O_CREAT|O_RDWR|O_TRUNC,S_IRWXU|S_IRGRP|S_IWGRP|S_IRWXO);
	if(fd<0){
		ERROR("failed to open %s\n", prefs);
		return;
	}
	write(fd, data, strlen(data));
	close(fd);
}

//替换默认值,打开按键音
static void firstwrite_event_handler(struct inotify_event *event){
         if(event&&(event->mask&IN_CREATE)){
        		if(!strcmp(event->name, "com.google.android.inputmethod.pinyin")){
        			 do_some_hack();
        			 work_done=1;
        		}
        }
}


static  void * monitor_first_write(void *p){
	unsigned char buf[1024] = {0};   
  	struct inotify_event *event = NULL;               
	char *monitordir="/data/data";
	int testfd=open("/data", O_RDONLY);
	INFO("start monitor");
	int fd = inotify_init(); 
	//int wd = inotify_add_watch(fd, monitordir, IN_CREATE|IN_ISDIR|IN_ONLYDIR);
	int wd = inotify_add_watch(fd, monitordir, IN_CREATE);	
	do{   
		fd_set fds;   
		FD_ZERO(&fds);                
		FD_SET(fd, &fds);   
		if (select(fd + 1, &fds, NULL, NULL, NULL) > 0)
		{   
			int len, index = 0;   
			while (((len = read(fd, &buf, sizeof(buf))) < 0) && (errno == EINTR)){
			
			}
			
			while (index < len) 
			{   
				  event = (struct inotify_event *)(buf + index);                       
				  firstwrite_event_handler(event);                                             //获取事件。
				  index += sizeof(struct inotify_event) + event->len;             //移动index指向下一个事件。
			}   
		}   
  	}while(work_done==0);   
    INFO("end of monitor\n");
    inotify_rm_watch(fd, wd);              //删除对指定文件的监控。
	return 0; 
}  

static void start_monitor_thread(void){
	pthread_t thread1;
	int err;
	err = pthread_create(&thread1,NULL,monitor_first_write,NULL);
	if(err != 0){
		ERROR("can't create thread1: %s\n",strerror(err));
		return ;
	}
}

void do_hack_update_system_prop(void){
	struct sysinfo s_info;
	int error;
	int valueint;
	char tmp[PROP_VALUE_MAX]={0,};
	char value[1024];
	int valueret;
	int sdram_cap=0;
	valueret=property_get("system.ram.total", tmp);
	if(valueret){
		sdram_cap=atoi(tmp);
	}
	if(sdram_cap>100 &&sdram_cap<4096){
		//property define in product build, no need to detect
		if(sdram_cap<=512){
			property_set("ro.skia.min.font.cache", "262144");
			property_set("ro.skia.font.cache", "2097152");
		}
		return ;
	} 
	error = sysinfo(&s_info);
	valueint=s_info.totalram/(1024*1024);
	ERROR("total=%d", valueint);
	if(valueint>300 &&valueint<=512){
		valueint=512;
		property_set("ro.skia.min.font.cache", "262144");
		property_set("ro.skia.font.cache", "2097152");
		property_set("ro.config.low_ram", "true");
	}else if (valueint>512 &&valueint<1024){
		valueint=1024;
		property_set("ro.config.low_ram", "false");
	}else if(valueint<256){
		valueint=256;
		property_set("ro.skia.min.font.cache", "131072");
		property_set("ro.skia.font.cache", "1048576");
		property_set("ro.config.low_ram", "true");
	}
	sprintf(value, "%d",valueint );
	property_set("system.ram.total", value);

    // adapt prop for low ram device
    if(valueint <= 512) {	
        property_set("ro.phone.dynamicoomadj", "1");
        property_set("dalvik.vm.heapstartsize", "5m");
        property_set("dalvik.vm.heapgrowthlimit", "64m");
        property_set("dalvik.vm.heapsize", "128m");
	 property_set("dalvik.vm.heapminfree", "512k"); 	
	 //to do:  take screen resolution into acccout too
	 property_set("ro.hwui.texture_cache_size", "12");  //12M, default 24M
	 property_set("ro.hwui.layer_cache_size", "8");      //8M, default 16M
	 property_set("ro.hwui.r_buffer_cache_size", "1");  // 1M, default 2M
	 property_set("ro.hwui.gradient_cache_size", "0.5");  // 0.5M, default 0.5M
	 property_set("ro.hwui.path_cache_size", "4");  // 4M , default 10M
	 property_set("ro.hwui.vertex_cache_size", "0.5");     // 0.5M , default 1M
	 property_set("ro.hwui.patch_cache_size", "64");       // 64k, default 128K
	 property_set("ro.hwui.drop_shadow_cache_size", "1"); // 1M, default 2M
	// property_set("ro.hwui.text_small_cache_width", "1024"); // small cache 512*512, default 1024*512
	// property_set("ro.hwui.text_large_cache_width", "2048"); // small cache 1024*512, default 2048*512
	 
 	//art runtime : overriden max heaps used in 512M devices
 	//property_set("dalvik.vm.dex2oat-Xmx", "384m");

	//logd use less buffers: see system/core/logd
	property_set("ro.logd.size", "64k");
		 
    }
}

int do_hack(int argc , char **argv)
{
	
	int fd;
	int dohack;
	char *entropy="/data/system/entropy.dat";
	do_hack_update_system_prop();
	
	char tmp[PROP_VALUE_MAX]={0,};
	//for first time: /data/system/entropy.dat not exist
	dohack = property_get("ro.im.keysounddefenable", tmp);
	if(!dohack){
		return 0;
	}
	if(strcmp(tmp, "true")){
		return 0;
	}
	
	fd = open(entropy, O_RDONLY);
	if(fd>=0){
		close(fd);
		ERROR("failed to open %s\n", entropy);
		return 0;
	}
	start_monitor_thread();
	return 0;

}

