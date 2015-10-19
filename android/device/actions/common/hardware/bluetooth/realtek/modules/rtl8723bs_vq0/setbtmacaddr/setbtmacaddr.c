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

#define MAX_BTMAC_SIZE 				    32
#define MISC_INFO_TYPE_EXT_BTMAC        7

#define MISC_INFO_READ					0
#define MISC_INFO_WRITE 				1
//ioctl cmd
#define ACCESS_MISC_INFO 				0
#define ACCESS_MISC_INFO_EXT			1


#define READ_BUF_SIZE 80
#define MAC_LEN 17
#define MAC_END 17
#define ETH_ALEN 6


//misc info for accessing
typedef struct
{
	int dir;		// 0: read; 1: write
	int type;		// information type
	int size;		// size of the info(key)
    void *buf;		// buffer for holding the key
}MiscInfo_t;

typedef struct {
        const char *addr_start;
        const char *addr_end;
}BTmac_info_t;

typedef struct {
	int valid;
	unsigned long long u_addr_start;
	unsigned long long u_addr_end;
}MacRange_t;
MacRange_t macrange = {0,0,0};


/*************************************************/
/*Base BTMAC operator:read & write in miscinfo field*/

char read_misc_buf[READ_BUF_SIZE];
char read_file_buf[READ_BUF_SIZE];

int miscinfo_read_xyz(unsigned int fd,char* buf,unsigned int cmd,
													unsigned int type,unsigned int max_size)
{
	int ret;
	MiscInfo_t misc;
	
	memset(read_misc_buf,0,max_size);

	misc.type 	= type;
	misc.buf 	= read_misc_buf;
	misc.size 	= max_size;
	misc.dir  	= MISC_INFO_READ;
	ret = ioctl(fd, cmd, &misc);
	if(ret <= 0){
		printf("not burned.\n");
	}
	else{
		//printf("burned.\n");
		memcpy(buf,read_misc_buf,ret);
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

//BTMAC
int miscinfo_read_btmac(unsigned int fd,char* buf)
{
	int ret = 0;
	ret = miscinfo_read_xyz(fd,buf,ACCESS_MISC_INFO_EXT,MISC_INFO_TYPE_EXT_BTMAC,MAX_BTMAC_SIZE);
	
	return ret;
}

int miscinfo_write_btmac(unsigned int fd,char* wBuf,unsigned int wSize)
{
	return miscinfo_write_xyz(fd,wBuf,ACCESS_MISC_INFO_EXT,MISC_INFO_TYPE_EXT_BTMAC,wSize);
}

/*************************************************/
/*把bt mac地址转换成数值:%02x:%02x:%02x:%02x:%02x:%02x*/
/* in: 
 * 00:1D:10:A4:5B:DB*/
unsigned long long mac_strtoul(char * str)
{
	unsigned long long ret = 0;
	unsigned int h, l;
	unsigned char *head, *end;
	unsigned char source_addr[MAC_LEN + 1];
	unsigned char destination_addr[ETH_ALEN];
	int i;

	memset(source_addr, 0, MAC_LEN + 1);	
	memset(destination_addr, 0, ETH_ALEN);
	
	strncpy(source_addr, str, MAC_LEN);
	source_addr[MAC_LEN] = ':';
	head = end = source_addr;
	
	for (i = 0; i < ETH_ALEN; i++) {
		while (end && (*end != ':') )//每次结束循环时,end 为 :
			end++;

		if (end && (*end == ':') )//把source_addr分割成3个char
			*end = '\0';

		destination_addr[i] = strtoul(head, NULL, 16);
		//printf("mac_strtoul ,destination_addr[%d]:%x\n", i, destination_addr[i]);
		if (end) {
			end++;
			head = end;
		}
	}
	
	ret = destination_addr[0];
	ret = (ret << 8) | destination_addr[1];
	ret = (ret << 8) | destination_addr[2];
	ret = (ret << 8) | destination_addr[3];
	ret = (ret << 8) | destination_addr[4];
	ret = (ret << 8) | destination_addr[5];
	//ret = (destination_addr[0] << 16) | (destination_addr[1] << 8) | (destination_addr[2]);

	h = (destination_addr[0] << 16) | (destination_addr[1] << 8) | (destination_addr[2]);
	l = (destination_addr[3] << 16) | (destination_addr[4] << 8) | (destination_addr[5]);
	printf("IN mac_strtoul :%s, to :%x,%x\n", str, h, l);
	return ret;
}

/*************************************************/
/*Check whether the BTmac is valid
 *return:
 * 0:invalid
 * 1:Ok,valid
 *in: 
 * 00:1D:10:A4:5B:DB
 */
int check_mac_is_valid(char * addr)
{
	int i, ret = 1;
	for (i = 0; i < MAC_END; i++){
		if (0 == ((i + 1)%3)){
			if (':' != addr[i]){
				printf("check_mac_is_valid error addr[%d]:%c\n", i, addr[i]);
				ret = 0;
				break;
			}
		}else{
			if ((('0' <= addr[i]) && (addr[i] <= '9'))
				||(('a' <= addr[i]) && (addr[i] <= 'f'))
				||(('A' <= addr[i]) && (addr[i] <= 'F'))){
				;
			}else{
				printf("check_mac_is_valid error addr[%d]:%c\n", i, addr[i]);
				ret = 0;
				break;
			}
		}
	}
	
	if(1 == ret)
		printf("IN check_mac_is_valid OK\n");
	else
		printf("IN check_mac_is_valid Faild\n");
	return ret;
}


/*************************************************/
/*Check whether the BTmac is in range
 *return:
 * 0:out of range
 * 1:Ok,in range
 * in: 
 * 00:1D:10:A4:5B:DB
 */
int check_mac_in_range(char * addr)
{	
	int ret = 0;
	char str_mac_addr[MAC_LEN + 1];
	unsigned long long u_mac_addr = 0;

	printf("E check_mac_in_range\n");

	memset(str_mac_addr, 0, MAC_LEN + 1);	
	strncpy(str_mac_addr, addr, MAC_LEN);
	printf("   check_mac_is_range ->  str_mac_addr:%s\n", str_mac_addr);
	
	u_mac_addr = mac_strtoul(str_mac_addr);

	if(macrange.valid){
		if((macrange.u_addr_start <= u_mac_addr) && (u_mac_addr <= macrange.u_addr_end)){
			printf("   check_mac_is_range, is range\n");
			ret = 1;//都一致，则ok,否者faild
		}else{
			printf("   check_mac_is_range, is not range\n");
			ret = 0;
		}
	}
	else {
		printf("   check_mac_is_range, haven't range\n");
		ret = 1;//没有限制或者无效的限制，则一定是满足需求的
	}

	if(1 == ret)
		printf("X check_mac_in_range OK\n");
	else
		printf("X check_mac_in_range Faild\n");

	return ret;
}

/*************************************************/
/*Check whether the BTmac is special property
 *BT Mac address 第一個 byte 必須是 00: , 後面三個 byte不可為 0x9e8b00 ~ 0x9e8b3f,
 *return:
 * 0:out of range
 * 1:Ok,in range
 * in: 
 * 00:1D:10:A4:5B:DB
 */
int check_mac_is_specialprop(char * addr)
{
	int i, ret = 0;
	char str_mac_addr[MAC_LEN + 1];
	unsigned long long u_mac_addr = 0;
	uint8_t bt_addr[6];

	printf("E check_mac_is_specialprop\n");

	memset(str_mac_addr, 0, MAC_LEN + 1);	
	strncpy(str_mac_addr, addr, MAC_LEN);
	
	u_mac_addr = mac_strtoul(str_mac_addr);

	bt_addr[5] = (uint8_t)(u_mac_addr & 0xff);
	bt_addr[4] = (uint8_t)((u_mac_addr >> 8) & 0xff);
	bt_addr[3] = (uint8_t)((u_mac_addr >> 16) & 0xff);
	bt_addr[2] = (uint8_t)((u_mac_addr >> 24) & 0xff);
	bt_addr[1] = (uint8_t)((u_mac_addr >> 32) & 0xff);
	bt_addr[0] = (uint8_t)((u_mac_addr >> 40) & 0xff);


	if (0x0  == bt_addr[0]){//Ok, continue  check
		if(0x9e == bt_addr[3] && 0x8b == bt_addr[4] && (bt_addr[5] <= 0x3f)){
			printf("   check_mac_is_specialprop NO\n");
			ret = 0;
		}
		else{//other ok
			printf("   check_mac_is_specialprop OK\n");
			ret = 1;
		}
	}else{
		printf("   check_mac_is_specialprop no 0x00\n");
		ret = 0;
	}

	if(1 == ret)
		printf("X check_mac_is_specialprop OK\n");
	else
		printf("X check_mac_is_specialprop Faild\n");
	return ret;
}

/*************************************************/
/*Check the BTmac
 *return:
 * 0:faild
 * 1:Ok
 *in: 
 * 00:1D:10:A4:5B:DB
 */
int check_btmac(char * addr)
{
	int ret = 0;
	printf("E check_btmac\n");
	ret = check_mac_is_valid(addr);
	if (1 == ret){
		ret = check_mac_in_range(addr);
		if (1 == ret)
			ret = check_mac_is_specialprop(addr);
	}
	if(1 == ret)
		printf("   check_btmac OK\n");
	else
		printf("   check_btmac Faild\n");

	printf("X check_btmac\n");
	return ret;
}


/*************************************************/
/*read android prop, get btmac address range
 *return:
 * 0:faild
 * 1:Ok
 */
int get_btmac_proprange(void)
{
	int ret = 0, i, ret1 = 0, ret2 = 0;
	char s_mac_addr_start[MAC_LEN + 1]= {0, };
	char s_mac_addr_end[MAC_LEN + 1]= {0, };
	unsigned long long tmp = 0;
	unsigned int h, l;
	BTmac_info_t btmac_info[] = {
		{"ro.btmac.address.start", "ro.btmac.address.end"},
	};

	printf("E get_btmac_proprange\n");

	for (i = 0; i < 1; i++) {
		property_get(btmac_info[i].addr_start, s_mac_addr_start, "");
		s_mac_addr_start[MAC_LEN] = '\0';

		property_get(btmac_info[i].addr_end, s_mac_addr_end, "");
		s_mac_addr_end[MAC_LEN] = '\0';
	}
	printf("   mac_addr_start:%s\n",s_mac_addr_start);
	printf("   mac_addr_end:%s\n", s_mac_addr_end);

	ret = check_mac_is_valid(s_mac_addr_start) && check_mac_is_valid(s_mac_addr_end);

	/*即便格式有效，但不一定符合bt地址的规则，如果不符合，也为无效*/
	if(1 == ret){
		printf("   get_btmac_proprange is valid\n");
		
		macrange.u_addr_start = mac_strtoul(s_mac_addr_start);
		macrange.u_addr_end = mac_strtoul(s_mac_addr_end);
		if (macrange.u_addr_end < macrange.u_addr_start)
		{
			tmp = macrange.u_addr_start;
			macrange.u_addr_start = macrange.u_addr_end;
			macrange.u_addr_end = tmp;
			printf("   swap mac_addr_start:%s\n",s_mac_addr_end);
			printf("   swap mac_addr_end:%s\n", s_mac_addr_start);
			
			ret1 = check_mac_is_specialprop(s_mac_addr_end); //start
			ret2 = check_mac_is_specialprop(s_mac_addr_start);//end
		}else{
			ret1 = check_mac_is_specialprop(s_mac_addr_start); 
			ret2 = check_mac_is_specialprop(s_mac_addr_end);
		}
		macrange.valid = 1;
		if((1 == ret1) && (1 == ret2)){
			printf("   get_btmac_proprange ret1:1; ret2:1\n");
			;//macrange.valid = 1;
		}else if((1 == ret1) && (0 == ret2)){
			h = (macrange.u_addr_end >> 24) & 0x00ffffff;
			l = 0x9e8aff;
			macrange.u_addr_end = h;
			macrange.u_addr_end = (macrange.u_addr_end << 24) | l;
			printf("   get_btmac_proprange ret1:1; ret2:0\n");
			//macrange.valid = 1;
		}else if ((0 == ret1) && (1 == ret2)){
			h = (macrange.u_addr_start >> 24) & 0x00ffffff;
			l = 0x9e8b40;
			macrange.u_addr_start = h;
			macrange.u_addr_start = (macrange.u_addr_start << 24) | l;
			printf("   get_btmac_proprange ret1:0; ret2:1\n");
			//macrange.valid = 1;
		}else if (0 == ret1 && 0 == ret2){
			tmp = macrange.u_addr_end - macrange.u_addr_start;
			if (tmp < 0x40){
				macrange.valid = 0;//must
				printf("   get_btmac_proprange isn't specialprop\n");
			}else{
				h = (macrange.u_addr_start >> 24) & 0x00ffffff;
				l = 0x9e8b40;
				macrange.u_addr_start = h;
				macrange.u_addr_start = (macrange.u_addr_start << 24) | l;

				h = (macrange.u_addr_end >> 24) & 0x00ffffff;
				l = 0x9e8aff;
				macrange.u_addr_end = h;
				macrange.u_addr_end = (macrange.u_addr_end << 24) | l;
				printf("   get_btmac_proprange ret1:0; ret2:0, but ok\n");
				//macrange.valid = 1;
			}
		}

		if (1 == macrange.valid){
			h = (macrange.u_addr_start >> 24) & 0x00ffffff;
			l = (macrange.u_addr_start) & 0x00ffffff;;
			printf("   new mac_addr_start:%x,%x\n",h, l);
			
			h = (macrange.u_addr_end >> 24) & 0x00ffffff;
			l = (macrange.u_addr_end) & 0x00ffffff;;
			printf("   new mac_addr_end:%x,%x\n",h, l);
		}
		/*
		ret = check_mac_is_specialprop(s_mac_addr_start) && check_mac_is_specialprop(s_mac_addr_end);
		if (1 == ret){	
			macrange.valid = ret;//表示属性中设置了有效的限制地址范围
			if (macrange.valid) {
				printf("   get_btmac_proprange is specialprop\n");
				macrange.u_addr_start = mac_strtoul(s_mac_addr_start);
				macrange.u_addr_end = mac_strtoul(s_mac_addr_end);
				//内部校验一下，防止start和end写反
				if (macrange.u_addr_end < macrange.u_addr_start)
				{
					tmp = macrange.u_addr_start;
					macrange.u_addr_start = macrange.u_addr_end;
					macrange.u_addr_end = tmp;
					printf("   new mac_addr_start:%s\n",s_mac_addr_start);
					printf("   new mac_addr_end:%s\n", s_mac_addr_end);
				}
			}
		}else		
			printf("   get_btmac_proprange isn't  specialprop\n");
		*/
	}else{
		macrange.valid = 0;
		printf("   get_btmac_proprange is invalid\n");
	}
	printf("X get_btmac_proprange, ret:%d\n",ret);
	return ret;
}


/*************************************************/
static void generate_btmac(char filepath[])
{
	int fd, i;
	char buf[30];
	uint8_t bt_addr[6];
	int data_fd, ret, data_len;
	char data_buf[READ_BUF_SIZE];
	char str_randvar[MAC_LEN + 1];
	unsigned long long randvar, modify_randvar;
	unsigned long long dis, pos;
	unsigned int h, l;
	//printf("Enter %s %s\n",__FUNCTION__,filepath);
	printf("E generate_btmac %s\n", filepath);

	memset(data_buf, 0, READ_BUF_SIZE);
	memset(read_file_buf, 0, READ_BUF_SIZE);

	int mac_fd;
	//ALOGD("Enter %s %s\n",__FUNCTION__,filepath);
	printf("Enter generate_mac %s\n",filepath);
	
#if 1
        mac_fd = open("/dev/miscinfo", O_RDWR);
        if(mac_fd < 0){/*miscinfo field error*/
                printf("1 no /dev/miscinfo node.\n");
                return;
        }

        ret = miscinfo_read_btmac(mac_fd, data_buf);
        if(ret < 0) {
                printf("read BT Mac failed.\n\n");
                ret = close(mac_fd);
                if(ret != 0){
                        printf("1 mac_fd cannot closed.\n");
                }
        }
        data_buf[MAC_END] = '\0';
        printf("miscinfo BT Mac:%s\n", data_buf);

        ret = close(mac_fd);
        if(ret != 0){
                printf("2 mac_fd cannot closed.\n");
        //      return;
        }
 
        if (check_btmac(data_buf)){
            printf("BT mac is avalid ! \n");
			if(access(filepath, F_OK) == 0){
				printf("/data/misc/bluedroid/bdaddr is exists\n");
				return;
			}else {
				printf("/data/misc/bluedroid/bdaddr isn't exists\n");
			}
        }
#else

	/*Check MAC_ADDR_FILE exist */
	/*即便打开是正常的也不能保证数据是有效的*/
	if(access(filepath, F_OK) == 0)
	{
		printf("   generate_btmac: %s exists\n", filepath);
		
		data_fd = open(filepath, O_RDWR, 0644);
		if(data_fd < 0)
		{
			printf("   generate_mac %s cannot open.\n", filepath);
		}else {
			memset(data_buf, 0, READ_BUF_SIZE);
			data_len = read(data_fd, data_buf, MAC_END);
			
			ret = close(data_fd);
			if(ret != 0){
				printf("   generate_mac %s can't close.\n", filepath);
			}
				
			if(data_len < 0)
			{
				printf("   read %s failed.\n", filepath);

			}else{
				data_buf[MAC_END] = '\0';
				printf("%s:%s, length is:%d\n", filepath, data_buf, data_len);
				
				if (check_btmac(data_buf)){/*data is valid*/
					printf("X generate_btmac: %s , it's BT Mac is OK\n", filepath);
					strncpy(read_file_buf, data_buf, MAC_END + 1);
					return;
				}else
					printf("X generate_btmac: %s , it's BT Mac is invalid\n", filepath);
			}
		}
	}

#endif

	/*如果指定了有效范围，则必须在这里限制*/
	if(macrange.valid){
		printf("   generate_btmac, macrange is valid.\n");
		dis = macrange.u_addr_end - macrange.u_addr_start + 1;
		pos = macrange.u_addr_start;
		while(1){
			printf("   generate_btmac, while creat btmac.\n");
			srand(time(NULL) + getpid());
			memset(bt_addr, 0, sizeof(bt_addr));
			bt_addr[0] = 0x0;
			for(i=1; i<6; i++) {
				bt_addr[i] = (uint8_t) (rand() >> 8) & 0xFF;
			}

			/*
			memset(str_randvar, 0, MAC_LEN + 1);
			
			sprintf(str_randvar, "%02x:%02x:%02x:%02x:%02x:%02x\0", \
			        (unsigned char)(bt_addr[0]&0x00), \
			        (unsigned char)(bt_addr[1]&0xFF), \
			        (unsigned char)(bt_addr[2]&0xFF), \
					(unsigned char)(bt_addr[3]&0xFF), \
			        (unsigned char)(bt_addr[4]&0xFF), \
			        (unsigned char)(bt_addr[5]&0xFF));

			randvar = mac_strtoul(str_randvar);*/
			randvar = bt_addr[0];
			randvar = (randvar << 8) | bt_addr[1];
			randvar = (randvar << 8) | bt_addr[2];
			randvar = (randvar << 8) | bt_addr[3];
			randvar = (randvar << 8) | bt_addr[4];
			randvar = (randvar << 8) | bt_addr[5];
			modify_randvar = randvar % dis + pos;//[a,b] (rand()%(b-a+1))+a

			/*再把这个数值反回到bt_addr中*/
			bt_addr[5] = (uint8_t)(modify_randvar & 0xff);
			bt_addr[4] = (uint8_t)((modify_randvar >> 8) & 0xff);
			bt_addr[3] = (uint8_t)((modify_randvar >> 16) & 0xff);
			bt_addr[2] = (uint8_t)((modify_randvar >> 24) & 0xff);
			bt_addr[1] = (uint8_t)((modify_randvar >> 32) & 0xff);
			bt_addr[0] = (uint8_t)((modify_randvar >> 40) & 0xff);
			
			h = (bt_addr[0] << 16) | (bt_addr[1] << 8) | (bt_addr[2]);
			l = (bt_addr[3] << 16) | (bt_addr[4] << 8) | (bt_addr[5]);
			printf("   generate_btmac, creat bt mac is range:%x,%x\n", h,l);
					
			if (0x0  == bt_addr[0]){
				if(0x9e == bt_addr[3] && 0x8b == bt_addr[4] && (bt_addr[5] <= 0x3f)){
					printf("   generate_btmac, in 0x9e8b00~0x9e8b3f\n");
					//continue;
				}
				else{//other ok
					printf("   generate_btmac create OK\n");
					break;
				}
			}else{
				printf("   generate_btmac, bt_addr[0] isn't 0x00\n");
				//continue;
			}
		}
	}else{
		printf("   generate_btmac, macrange isn't range.\n");
		srand(time(NULL) + getpid());
		memset(bt_addr, 0, sizeof(bt_addr));
		bt_addr[0] = 0x0;
		for(i=1; i<6; i++) {
			bt_addr[i] = (uint8_t) (rand() >> 8) & 0xFF;
		}
		if(0x9e == bt_addr[3] && 0x8b == bt_addr[4] && (bt_addr[5] <= 0x3f)){
			//get random value
			bt_addr[3] = 0x00;
		}
	}

	umask(0);
	fd = open(filepath, O_CREAT|O_TRUNC|O_RDWR, 0644);
	if( fd >= 0)
	{
		memset(buf, 0, sizeof(buf));
		sprintf(buf,"%02X:%02X:%02X:%02X:%02X:%02X\0", bt_addr[0], bt_addr[1], bt_addr[2], bt_addr[3], bt_addr[4], bt_addr[5]);
		write(fd, buf, sizeof(buf));
		close(fd);
	}

	strncpy(read_file_buf, buf, MAC_END + 1);
	printf("X generate_btmac: %s fd=%d, creat BT mac addr=%s\n", filepath, fd, buf);
	return;
}

/*************************************************/
/*get miscinfo btmac
 *return:
 * 0:faild, need write right BTmac to miscinfo
 * 1:Ok, need read, and write BTmac filepath
 */
int get_btmac_miscinfo(void)
{
	int mac_fd, ret = 0;
	int data_len;

	printf("E get_btmac_miscinfo\n");
	
	/*open miscinfo btMac and read*/
	/*1. read miscinfo WIFIMAC field*/
	mac_fd = open("/dev/miscinfo", O_RDWR);
	if(mac_fd < 0){/*miscinfo field error*/
		printf(" no /dev/miscinfo node.\n");
		ret = 0;
	}else{
		/*miscinfo exist; read_misc_buf ok*/
		data_len = miscinfo_read_btmac(mac_fd, read_misc_buf);

		ret = close(mac_fd);
		if(ret != 0){
			printf("   get_btmac_miscinfo bt mac_fd can't close.\n");
		}
		
		if(data_len < 0) {
			printf("   get_btmac_miscinfo read btMac failed.\n");
			ret = 0;
		}else{
			read_misc_buf[MAC_END] = '\0';
			printf("   get_btmac_miscinfo OK,miscinfo btMac:%s\n", read_misc_buf);
			ret = 1;
		}
	}

	if (1 == ret)
		ret = check_btmac(read_misc_buf);
		
	printf("X get_btmac_miscinfo\n");
	return ret;
}

/*************************************************/

void save_btmac(char filepath[])
{
	int ret = 0;
	int mac_fd, file_fd;
	
	printf("E save_btmac\n");

	ret = get_btmac_miscinfo();

	if (1 == ret){/*miscinfo bt Mac valid: "%02x:%02x:%02x:%02x:%02x:%02x" */
		printf("   save_btmac write bt Mac,from miscinfo to file:%s, read_file_buf:%s\n", read_misc_buf, read_file_buf);
		//if (NULL == strstr(read_misc_buf, read_file_buf)){
		if (NULL != strncmp(read_misc_buf, read_file_buf, MAC_LEN)){
			printf("   save_btmac Data isn't the same.\n");
			umask(0);
			file_fd = open(filepath, O_CREAT|O_TRUNC|O_RDWR, 0644);
			if(file_fd < 0)
			{
				printf("   save_btmac file_fd can't open.\n");
				return;
			}
			ret = write(file_fd, read_misc_buf, MAC_LEN + 1);
			if(ret < 0) printf("   save_btmac write btmac to file_fd Failed.\n\n");
			else printf("   save_btmac write btmac to file_fd OK.\n\n");
		
			ret = close(file_fd);
			if(ret != 0){
				printf("   save_btmac can't close file_fd . \n");
			}
		}else{
			printf("  save_btmac bt Mac is the same:%s, read_file_buf:%s\n", read_misc_buf, read_file_buf);
		}
	}else{/*BT mac from file to miscinfo*/
		printf("   save_btmac write bt Mac, from file to miscinfo:%s\n", read_file_buf);
		mac_fd = open("/dev/miscinfo", O_RDWR);
		if(mac_fd < 0){/*miscinfo field error*/
			printf("   save_btmac no no no /dev/miscinfo node.\n");
			return;
		}
		ret = miscinfo_write_btmac(mac_fd, read_file_buf, MAC_LEN + 1);
		if(ret < 0) printf("   save_btmac write btmac to miscinfo Failed.\n");
		else printf("   save_btmac write btmac to miscinfo OK.\n");
		
		ret = close(mac_fd);
		if(ret != 0){
			printf("   save_btmac mac_fd can't close.\n");
		}
	}

	/***********************************/
	printf("X save_btmac.\n");
	return;
}

/*
sizeof(unsigned long long):8
sizeof(unsigned long ):4
*/
int main(int argc, char ** argv)
{
	char filepath[80];
	
	usleep(2000000);//2s
	
	memset(filepath, 0, sizeof(filepath));	
	if(argc != 2)
	{
			printf("Usage:setbtmacaddr <mac_addr_path>\n");
			return 0;
	}
	strncpy(filepath, argv[1], strlen(argv[1]));
	get_btmac_proprange();
	generate_btmac(filepath);
	save_btmac(filepath);

	
	printf("main chmod\n");
	chmod(filepath, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	
	return 0;
}
