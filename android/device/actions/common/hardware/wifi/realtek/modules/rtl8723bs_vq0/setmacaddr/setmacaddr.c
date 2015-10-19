#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <utils/Log.h>

#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stddef.h>
#include <errno.h>
#include <sys/utsname.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/system_properties.h>

#define MAX_WIFIMAC_SIZE 				32
#define MISC_INFO_TYPE_EXT_WIFIMAC      6

#define MISC_INFO_READ					0
#define MISC_INFO_WRITE 				1
//ioctl cmd
#define ACCESS_MISC_INFO 				0
#define ACCESS_MISC_INFO_EXT			1


#define READ_BUF_SIZE 80
#define MAC_END 17
#define MAC_HALF_END 8
#define HALF_MAC_LEN 8
#define HALF_ETH_ALEN 3
//misc info for accessing
typedef struct
{
	int dir;		// 0: read; 1: write
	int type;		// information type
	int size;		// size of the info(key)
    void *buf;		// buffer for holding the key
}MiscInfo_t;

typedef struct {
	int valid;
	unsigned int u_mac_vendor;
	unsigned int u_addr_start;
	unsigned int u_addr_end;
}MacLimit_t;
MacLimit_t maclimit = {0,0,0,0};

typedef struct {
        const char *mac_vendor;
        const char *addr_start;
        const char *addr_end;
}Wifimac_info_t;

char read_buf[80];
	
int miscinfo_read_xyz(unsigned int fd,char* buf,unsigned int cmd,
													unsigned int type,unsigned int max_size)
{
	int ret;
	MiscInfo_t misc;
	
	memset(read_buf,0,max_size);

	misc.type 	= type;
	misc.buf 	= read_buf;
	misc.size 	= max_size;
	misc.dir  	= MISC_INFO_READ;
	ret = ioctl(fd, cmd, &misc);
	if(ret <= 0){
		printf("not burned.\n");
	}
	else{
		//printf("burned.\n");
		memcpy(buf,read_buf,ret);
	}
	return ret;
}

int miscinfo_write_xyz(unsigned int fd,char* wBuf,unsigned int cmd,
													unsigned int type,unsigned int wSize)
{
	int ret;
	MiscInfo_t misc;

	misc.type 	= type;
	misc.buf 	= wBuf;
	misc.size 	= wSize;
	misc.dir  	= MISC_INFO_WRITE;
	ret = ioctl(fd, cmd, &misc);
	if(ret <= 0){
		printf("burn failed.\n");
	}
	else{
		//printf("burn success.\n");
	}

	return ret;
}

//WIFIMAC
int miscinfo_read_wifimac(unsigned int fd,char* buf)
{
	int ret = 0;
	ret = miscinfo_read_xyz(fd,buf,ACCESS_MISC_INFO_EXT,MISC_INFO_TYPE_EXT_WIFIMAC,MAX_WIFIMAC_SIZE);
	
	return ret;
}

int miscinfo_write_wifimac(unsigned int fd,char* wBuf,unsigned int wSize)
{
	return miscinfo_write_xyz(fd,wBuf,ACCESS_MISC_INFO_EXT,MISC_INFO_TYPE_EXT_WIFIMAC,wSize);
}

/*把半mac地址转换成数值*/
unsigned int halfmac_strtoul(char * str)
{
	unsigned int ret = 0;
	unsigned char *head, *end;
	unsigned char source_addr[HALF_MAC_LEN + 1];
	unsigned char destination_addr[HALF_ETH_ALEN];
	int i;

	memset(source_addr, 0, HALF_MAC_LEN + 1);	
	memset(destination_addr, 0, HALF_ETH_ALEN);
	
	strncpy(source_addr, str, HALF_MAC_LEN);
	source_addr[HALF_MAC_LEN] = ':';
	head = end = source_addr;
	
	for (i = 0; i < HALF_ETH_ALEN; i++) {
		while (end && (*end != ':') )//每次结束循环时,end 为 :
			end++;

		if (end && (*end == ':') )//把source_addr分割成3个char
			*end = '\0';

		destination_addr[i] = strtoul(head, NULL, 16);
		//printf("halfmac_strtoul ,destination_addr[%d]:%x\n", i, destination_addr[i]);
		if (end) {
			end++;
			head = end;
		}
	}
	
	ret = destination_addr[0];
	ret = (ret << 8) | destination_addr[1];
	ret = (ret << 8) | destination_addr[2];
	//ret = (destination_addr[0] << 16) | (destination_addr[1] << 8) | (destination_addr[2]);

	printf("halfmac_strtoul :%s, to :%x\n", str, ret);
	return ret;
}


int check_half_mac(char * buf)
{
	int i, ret = 1;
	for (i = 0; i < MAC_HALF_END; i++){
		if (0 == ((i + 1)%3)){
			if (':' != buf[i]){
				printf("check_half_mac buf[%d]:%c\n", i, buf[i]);
				ret = 0;
				break;//return ret;
			}
		}else{
			if ((('0' <= buf[i]) && (buf[i] <= '9'))
				||(('a' <= buf[i]) && (buf[i] <= 'f'))
				||(('A' <= buf[i]) && (buf[i] <= 'F'))){
				;
			}else{
				printf("check_half_mac buf[%d]:%c\n", i, buf[i]);
				ret = 0;
				break;//return ret;
			}
		}
	}
	//printf("check_half_mac buf:%s, ret is:%d\n", buf, ret);
	return ret;
}

/*
return :
	0:check faild
	1:check ok, whole mac address is range
*/
int check_mac_is_range(char *buf)
{
	int ret = 0;
	char buf_mac_vendor[HALF_MAC_LEN + 1];
	char buf_mac_addr[HALF_MAC_LEN + 1];
	
	printf("E check_mac_is_range\n");
	
	unsigned int u_mac_vendor = 0;
	unsigned int u_mac_addr = 0;

	memset(buf_mac_vendor, 0, HALF_MAC_LEN + 1);	
	memset(buf_mac_addr, 0, HALF_MAC_LEN + 1);	

	strncpy(buf_mac_vendor, buf, HALF_MAC_LEN);
	strncpy(buf_mac_addr, (buf + HALF_MAC_LEN + 1), HALF_MAC_LEN);

	printf("check_mac_is_range ->  buf_mac_vendor:%s\n", buf_mac_vendor);
	printf("check_mac_is_range ->  buf_mac_addr:%s\n", buf_mac_addr);
	
	u_mac_vendor = halfmac_strtoul(buf_mac_vendor);
	u_mac_addr = halfmac_strtoul(buf_mac_addr);

	if(maclimit.valid){//属性中获取的半地址是有效的，需要把buf中保存的地址拿来对比了
		//把buf中的地址分割成两部分
		//判断厂商是否一致
		if (u_mac_vendor == maclimit.u_mac_vendor){
			//如果一致，则再看范围
			if((maclimit.u_addr_start <= u_mac_addr) && (u_mac_addr <= maclimit.u_addr_end)){
				printf("   check_mac_is_range, is range\n");
				ret = 1;//都一致，则ok,否者faild
			}else{
				printf("   check_mac_is_range, vendor is range, but addr is not\n");
				ret = 0;
			}
		}else{
			printf("   check_mac_is_range, vendor is not range\n");
			ret = 0;
		}
	}
	else {
		printf("   check_mac_is_range, haven't range\n");
		ret = 1;//没有限制或者无效的限制，则一定是满足需求的
	}

	printf("X check_mac_is_range, ret:%d\n", ret);
	return ret;
}

/*check比较严格了，如果有范围限制，则要在里面校验正常的MAC地址是否在范围内。*/
/*
	return :
		0:check faild
		1:check ok
*/
int check_mac(char * buf)
{
	int i, ret = 1;
	for (i = 0; i < MAC_END; i++){
		if (0 == ((i + 1)%3)){
			if (':' != buf[i]){
				printf("check_mac buf[%d]:%c\n", i, buf[i]);
				ret = 0;
				break;//return ret;
			}
		}else{
			if ((('0' <= buf[i]) && (buf[i] <= '9'))
				||(('a' <= buf[i]) && (buf[i] <= 'f'))
				||(('A' <= buf[i]) && (buf[i] <= 'F'))){
				;
			}else{
				printf("check_mac buf[%d]:%c\n", i, buf[i]);
				ret = 0;
				break;//return ret;
			}
		}
	}
	//printf("check_mac buf:%s, ret is:%d\n", buf, ret);
	if(ret){//当数据有效的时候才继续校验，无效的数据不用再继续校验
		ret = check_mac_is_range(buf);
	}
	return ret;
}


int get_mac_prop(MacLimit_t * p_maclimit)
{
	int ret = 0, i;
	char s_mac_vendor[HALF_MAC_LEN + 1];
	char s_mac_addr_start[HALF_MAC_LEN + 1];
	char s_mac_addr_end[HALF_MAC_LEN + 1];
	unsigned int tmp = 0;
	
	printf("E get_mac_prop\n");

	Wifimac_info_t wifimac_info[] = {
        { "ro.wifimac.vendor", "ro.wifimac.address.start", "ro.wifimac.address.end"},
    };
	
    //for (i = 0; i < ARRAY_SIZE(wifimac_info); i++) {
	for (i = 0; i < 1; i++) {
        property_get(wifimac_info[i].mac_vendor, s_mac_vendor, "");
		s_mac_vendor[HALF_MAC_LEN] = '\0';
		property_get(wifimac_info[i].addr_start, s_mac_addr_start, "");
		s_mac_addr_start[HALF_MAC_LEN] = '\0';
		property_get(wifimac_info[i].addr_end, s_mac_addr_end, "");
		s_mac_addr_end[HALF_MAC_LEN] = '\0';
    }
	
	printf("mac_vendor:%s\n", s_mac_vendor);
	printf("mac_addr_start:%s\n",s_mac_addr_start);
	printf("mac_addr_end:%s\n", s_mac_addr_end);
	
	ret = check_half_mac(s_mac_vendor) && check_half_mac(s_mac_addr_start) && check_half_mac(s_mac_addr_end);
	
	if(ret)
		printf("get_mac_prop is ok\n");
	else
		printf("get_mac_prop is err\n");
	
	p_maclimit->valid = ret;/*表示属性中设置了有效的限制地址范围*/
	if (p_maclimit->valid) {
		printf("   mac_vendor --- p_maclimit->valid\n");
		p_maclimit->u_mac_vendor = halfmac_strtoul(s_mac_vendor);
		p_maclimit->u_addr_start = halfmac_strtoul(s_mac_addr_start);
		p_maclimit->u_addr_end = halfmac_strtoul(s_mac_addr_end);
		/*内部校验一下，防止start和end写反*/
		if (p_maclimit->u_addr_end < p_maclimit->u_addr_start)
		{
			tmp = p_maclimit->u_addr_start;
			p_maclimit->u_addr_start = p_maclimit->u_addr_end;
			p_maclimit->u_addr_end = tmp;
			
		}
	}
	printf("X get_mac_prop, ret:%d\n",ret);
	return ret;
}


static void write_randmac_file(char *macaddr,char filepath[],int randvar1,int randvar2)
{
	int fd;
	char buf[80];
	int modify_randvar1;
	unsigned int dis, pos;
	printf("E write_randmac_file\n");
	
	//sprintf(macaddr, "00:e0:4c:%02x:%02x:%02x",
	//changed by tingle for 6 bytes random
	//Byte1,bit0,bit1 can't be 1.
	if(maclimit.valid){
		dis = maclimit.u_addr_end - maclimit.u_addr_start + 1;
		pos = maclimit.u_addr_start;
		modify_randvar1 = randvar1 % dis + pos;//[a,b] (rand()%(b-a+1))+a

		printf("   maclimit.valid maclimit.u_mac_vendor:%x\n", maclimit.u_mac_vendor);
		
		sprintf(macaddr, "%02x:%02x:%02x:%02x:%02x:%02x", \
		        (unsigned char)((maclimit.u_mac_vendor>>16)&0xFC), \
		        (unsigned char)((maclimit.u_mac_vendor>>8)&0xFF), \
		        (unsigned char)((maclimit.u_mac_vendor)&0xFF), \
				(unsigned char)((modify_randvar1>>16)&0xFF), \
		        (unsigned char)((modify_randvar1>>8)&0xFF), \
		        (unsigned char)((modify_randvar1)&0xFF));
		
		printf("   maclimit.valid macaddr:%s\n", macaddr);
		
	}else{
		sprintf(macaddr, "%02x:%02x:%02x:%02x:%02x:%02x", \
		        (unsigned char)((randvar2)&0xFC), \
		        (unsigned char)((randvar2>>8)&0xFF), \
		        (unsigned char)((randvar2>>16)&0xFF), \
				(unsigned char)((randvar1)&0xFF), \
		        (unsigned char)((randvar1>>8)&0xFF), \
		        (unsigned char)((randvar1>>16)&0xFF));
		
		printf("   macaddr:%s\n", macaddr);
	}
        umask(0);
		
	fd = open(filepath, O_CREAT|O_TRUNC|O_RDWR, 0644);
	if( fd >= 0)
	{
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%s",macaddr);
		write(fd, buf, sizeof(buf));
		close(fd);
	}
	//ALOGD("%s: %s fd=%d, data=%s",__FUNCTION__, filepath, fd,buf);
	printf("X write_randmac_file :%s fd=%d, data=%s\n", filepath, fd, buf);
}

unsigned int gen_randseed()
{
	int fd;
	int rc;
	unsigned int randseed;
	size_t len;
	struct timeval tval;

	len =  sizeof(randseed);
	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
	{
		//ALOGD("%s: Open /dev/urandom fail\n", __FUNCTION__);
		printf("gen_randseed  Open /dev/urandom fail\n");
		return -1;
	}
	rc = read(fd, &randseed, len);
	close(fd);
	if(rc <0)
	{
		if (gettimeofday(&tval, (struct timezone *)0) > 0)
			randseed = (unsigned int) tval.tv_usec;
		else
			randseed = (unsigned int) time(NULL);

		//ALOGD("open /dev/urandom fail, using system time for randseed\n");
		printf("gen_randseed open /dev/urandom fail, using system time for randseed\n");

	}
	return randseed;
}


void generate_mac(char *macaddr,char filepath[])
{
	unsigned int randseed;
	int rand_var1,rand_var2;
	char txt_buf[READ_BUF_SIZE];
	int txt_fd;
	int ret;
        int mac_fd;
	//ALOGD("Enter %s %s\n",__FUNCTION__,filepath);
	printf("Enter generate_mac %s\n",filepath);
	
#if 1
        mac_fd = open("/dev/miscinfo", O_RDWR);
        if(mac_fd < 0){/*miscinfo field error*/
                printf("1 no /dev/miscinfo node.\n");
                return;
        }

        ret = miscinfo_read_wifimac(mac_fd, read_buf);
        if(ret < 0) {
                printf("read wifiMac failed.\n\n");
                ret = close(mac_fd);
                if(ret != 0){
                        printf("1 mac_fd cannot closed.\n");
                }
        }
        read_buf[MAC_END] = '\0';
        printf("miscinfo wifiMac:%s\n", read_buf);

        ret = close(mac_fd);
        if(ret != 0){
                printf("2 mac_fd cannot closed.\n");
        //      return;
        }
 
        if (check_mac(read_buf)){
                printf("wifimac is avalid ! \n");
		if(access(filepath, F_OK) == 0){
		    printf("/data/wifimac.txt is exists\n");
		    return;
		}else {
		    printf("/data/wifimac.txt isn't exists\n");
                }
        }
#else
	/*Check MAC_ADDR_FILE exist */

	if(access(filepath, F_OK) == 0)
	{
		//ALOGD("%s: %s exists",__FUNCTION__, filepath);
		printf("X generate_mac: %s exists\n", filepath);
		
		/*************************************************/
		/*check wifimac.txt*/
		/*open wifimac.txt and read*/
		txt_fd = open(filepath, O_RDWR, 0644);
		if( txt_fd < 0)
		{
			printf("generate_mac txt_fd cannot open.\n");
		}else {
			memset(txt_buf, 0, READ_BUF_SIZE);
			ret = read(txt_fd, txt_buf, MAC_END);
			if( txt_fd < 0)
			{
				printf("read wifimac.txt failed.\n\n");
				ret = close(txt_fd);
				if(ret != 0){
					printf("1 generate_mac txt_fd cannot closed.\n\n");
				}
			}else{
				txt_buf[MAC_END] = '\0';
				printf("txt wifiMac:%s, length is:%d\n\n", txt_buf, ret);

				ret = close(txt_fd);
				if(ret != 0){
					printf("2 generate_mac txt_fd cannot closed.\n");
				}else {
				/*end wifimac.txt and read*/				
					if (check_mac(txt_buf)){/*data is valid*/
						return;
					}
				}
			}
		}
		/*************************************************/
	}
#endif
	randseed = gen_randseed();
	if(randseed == -1)
		return ;
	srand(randseed);

	rand_var1 = rand();
	rand_var2 = rand();
	//ALOGD("%s:	rand_var1 =0x%x, rand_var2=0x%x",__FUNCTION__, rand_var1,rand_var2);
	printf("generate_mac:	rand_var1 =0x%x, rand_var2=0x%x\n", rand_var1,rand_var2);
	write_randmac_file(macaddr,filepath,rand_var1,rand_var2);
	
	printf("X generate_mac\n");
}


void check_miscinfo_wifitxt(char filepath[])
{
	unsigned int addr;
	unsigned int len;
	int ret = 0;
	int type;
	int mac_fd, txt_fd;
	int i,j;
	const char *param_flag;
	char *value;
	char *s="\n";
	char txt_buf[READ_BUF_SIZE];
	
	printf("E check_miscinfo_wifitxt\n");

	/*open miscinfo wifiMac and read*/
	/*1. read miscinfo WIFIMAC field*/
	mac_fd = open("/dev/miscinfo", O_RDWR);
	if(mac_fd < 0){/*miscinfo field error*/
		printf("1 no /dev/miscinfo node.\n");
		return;
	}

	//memset(read_buf, 0, READ_BUF_SIZE);
	/*miscinfo exist; read_buf ok*/
	ret = miscinfo_read_wifimac(mac_fd, read_buf);
	if(ret < 0) {
		printf("read wifiMac failed.\n\n");
		ret = close(mac_fd);
		if(ret != 0){
			printf("1 mac_fd cannot closed.\n");
		}
	//	return;
	}
	read_buf[MAC_END] = '\0';
	printf("miscinfo wifiMac:%s\n", read_buf);
	
	ret = close(mac_fd);
	if(ret != 0){
		printf("2 mac_fd cannot closed.\n");
	//	return;
	}
	/*end open miscinfo wifiMac and read*/
	
	/***********************************/
	/*open wifimac.txt and read*/
	txt_fd = open(filepath, O_RDWR, 0644);
	if( txt_fd < 0)
	{
		printf("1 txt_fd cannot open.\n");
	//	return;
	}

	memset(txt_buf, 0, READ_BUF_SIZE);
	ret = read(txt_fd, txt_buf, MAC_END);
	if( ret < 0)
	{
		printf("read wifimac.txt failed.\n\n");
		ret = close(txt_fd);
		if(ret != 0){
			printf("1 txt_fd cannot closed.\n\n");
		}
	//	return;
	}

	txt_buf[MAC_END] = '\0';
	printf("txt wifiMac:%s, length is:%d\n\n", txt_buf, ret);

	ret = close(txt_fd);
	if(ret != 0){
		printf("2 txt_fd cannot closed.\n");
	//	return;
	}
	/*end open wifimac.txt and read*/
	
	/***********************************/
	if (check_mac(read_buf)){/*miscinfo wifiMac avalid: "%02x:%02x:%02x:%02x:%02x:%02x" */
		printf("prepare write miscinfo  to  wifiMac.txt data:%s\n", read_buf);
		//if ((NULL == strstr(read_buf, txt_buf)) || (check_mac(txt_buf) == 0)){
		if ((NULL != strncmp(read_buf, txt_buf, MAC_END)) || (check_mac(txt_buf) == 0)){
			printf("Data isn't the same.\n");
                        umask(0);
			txt_fd = open(filepath, O_CREAT|O_TRUNC|O_RDWR, 0644);
			if( txt_fd < 0)
			{
				printf("2 txt_fd cannot open.\n");
				return;
			}
			ret = write(txt_fd, read_buf, sizeof(read_buf));
			ret = close(txt_fd);
			if(ret != 0){
				printf("3 txt_fd cannot closed.\n");
			}
		}else{
			printf("Data is the same.\n");
			printf("miscinfo wifiMac:%s\n", read_buf);
			printf("wifimac.txt:%s\n", txt_buf);		
		}
	}else{
		printf("prepare write  wifiMac.txt to miscinfo data:%s\n", txt_buf);
		mac_fd = open("/dev/miscinfo", O_RDWR);
		if(mac_fd < 0){/*miscinfo field error*/
			printf("2 no /dev/miscinfo node.\n");
			return;
		}
		ret = miscinfo_write_wifimac(mac_fd, txt_buf, strlen(txt_buf));
		if(ret < 0) printf("wifiMac write miscinfo failed.\n\n");
		else printf("wifiMac write miscinfo ok.\n\n");		
		ret = close(mac_fd);
		if(ret != 0){
			printf("6 mac_fd cannot closed.\n");
		}
	}

	/***********************************/
	printf("X check_miscinfo_wifitxt.\n");
}


int main(int argc, char ** argv)
{
	char macaddr[32],filepath[80];
	memset(macaddr, 0, sizeof(macaddr));
	memset(filepath, 0, sizeof(filepath));
	if(argc != 2)
	{
			//ALOGD("Usage:setmacaddr <mac_addr_path>\n");
			printf("main Usage:setmacaddr <mac_addr_path>\n");
			return 0;
	}
	strncpy(filepath, argv[1], strlen(argv[1]));
	get_mac_prop(&maclimit);//可以不要参数，改为全局变量了
	generate_mac(macaddr,filepath);
	check_miscinfo_wifitxt(filepath);
	

	
	return 0;
}
