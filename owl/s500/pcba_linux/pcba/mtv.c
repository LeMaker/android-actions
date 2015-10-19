#include "case.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define RAONTV_DEV_NAME		"mtv350"
#define TRUE			1
#define FALSE			0
#define RAONTV_IOC_MAGIC	'R'

typedef enum
{
	RTV_COUNTRY_BAND_JAPAN = 0,
	RTV_COUNTRY_BAND_KOREA,		
	RTV_COUNTRY_BAND_BRAZIL,
	RTV_COUNTRY_BAND_ARGENTINA 
} E_RTV_COUNTRY_BAND_TYPE;


/*==============================================================================
 * ISDB-T IO control commands(10~29)
 *============================================================================*/
typedef struct
{
	unsigned int 	lock_mask;
	unsigned int	ant_level;
	unsigned int	ber; // output
	unsigned int	cnr;  // output
	unsigned int	per;  // output
	int 		rssi;  // output
} IOCTL_ISDBT_SIGNAL_INFO;



#define IOCTL_ISDBT_POWER_ON		_IOW(RAONTV_IOC_MAGIC,10, E_RTV_COUNTRY_BAND_TYPE)
#define IOCTL_ISDBT_POWER_OFF		_IO(RAONTV_IOC_MAGIC, 11)
#define IOCTL_ISDBT_SCAN_FREQ		_IOW(RAONTV_IOC_MAGIC,12, unsigned int)
#define IOCTL_ISDBT_SET_FREQ		_IOW(RAONTV_IOC_MAGIC,13, unsigned int)
#define IOCTL_ISDBT_GET_LOCK_STATUS    _IOR(RAONTV_IOC_MAGIC,14, unsigned int)
#define IOCTL_ISDBT_GET_TMCC		_IOR(RAONTV_IOC_MAGIC,15, RTV_ISDBT_TMCC_INFO)
#define IOCTL_ISDBT_GET_SIGNAL_INFO	_IOR(RAONTV_IOC_MAGIC,16, IOCTL_ISDBT_SIGNAL_INFO)
#define IOCTL_ISDBT_START_TS		_IO(RAONTV_IOC_MAGIC, 17)
#define IOCTL_ISDBT_STOP_TS		_IO(RAONTV_IOC_MAGIC, 18)
#define IOCTL_ISDBT_GET_PLATFORM_ID		_IOR(RAONTV_IOC_MAGIC, 19,IOCTL_ISDBT_PLATFORM_ID)



#define RTV_ISDBT_OFDM_LOCK_MASK	0x1
#define RTV_ISDBT_TMCC_LOCK_MASK	0x2
#define RTV_ISDBT_CHANNEL_LOCK_OK	(RTV_ISDBT_OFDM_LOCK_MASK|RTV_ISDBT_TMCC_LOCK_MASK)


#define RTV_ISDBT_BER_DIVIDER		100000
#define RTV_ISDBT_CNR_DIVIDER		10000
#define RTV_ISDBT_RSSI_DIVIDER		10


unsigned int isdbt_area_idx = 0;
unsigned int isdbt_is_power_on = 0;
unsigned int mtv_prev_channel = 0;

static int isdbt_power_up(int fd_dmb_dev)
{
	int ret;
	E_RTV_COUNTRY_BAND_TYPE country_band_type; 

	if(isdbt_is_power_on == TRUE)
		return 0;

	if(isdbt_area_idx == 0)
		country_band_type = RTV_COUNTRY_BAND_JAPAN;
	else
		country_band_type = RTV_COUNTRY_BAND_BRAZIL;

	if((ret = ioctl(fd_dmb_dev, IOCTL_ISDBT_POWER_ON, &country_band_type)) < 0)
	{
		printf("[ISDBT] IOCTL_ISDBT_POWER_ON failed: %d\n", ret);
		return ret;		
	}

	isdbt_is_power_on = TRUE;

//	mtv_prev_channel = 0;

	return 0;
}

static int isdbt_power_down(unsigned int fd_dmb_dev)
{
	if(isdbt_is_power_on == FALSE)
		return 0;

	if(ioctl(fd_dmb_dev, IOCTL_ISDBT_POWER_OFF) < 0)
	{
		printf("[ISDBT] IOCTL_ISDBT_POWER_OFF failed\n");
	}

	isdbt_is_power_on = FALSE;
	return 0;
}

/*********************************************************************************
*/
static int isdbt_set_channel(int fd_dmb_dev, unsigned int ch_num)
{
	if(ch_num == mtv_prev_channel)
	{
		printf("[ISDBT] Already opened channed ID(%d)\n", ch_num);
		return 0;
	}

//	isdbt_disable_ts();
		
	if(ioctl(fd_dmb_dev, IOCTL_ISDBT_SET_FREQ, &ch_num) < 0)
	{
		printf("[ISDBT] IOCTL_ISDBT_SET_FREQ failed\n");
		return -2;
	}

//	mtv_prev_channel = ch_num;

	return 0;
}

/*****************************************************
   ****************************************************/
static int isdbt_check_lock_status(int fd_dmb_dev, int *lst)
{
	unsigned int lock_mask;
	
	if(ioctl(fd_dmb_dev, IOCTL_ISDBT_GET_LOCK_STATUS, &lock_mask) == 0)
		printf("lock_mask = %d\n", lock_mask);			
	else{
		printf("[ISDBT] IOCTL_ISDBT_GET_LOCK_STATUS failed\n");
		return -1;
	}
	
	*lst = lock_mask;
	printf("\n");

	return 0;
}

static int isdbt_check_signal_info(int fd_dmb_dev, IOCTL_ISDBT_SIGNAL_INFO *sig_info)
{
	int ret;
	unsigned int lock;
	
	ret = ioctl(fd_dmb_dev, IOCTL_ISDBT_GET_SIGNAL_INFO, sig_info);
	if(ret < 0)
	{
		printf("[ISDBT] IOCTL_ISDBT_GET_SIGNAL_INFO failed\n");
		return ret;
	}

	lock = (sig_info->lock_mask == RTV_ISDBT_CHANNEL_LOCK_OK) ? 1 : 0;

	// printf("\t########## [ISDBTSignal Inforamtions] ##############\n");
	// printf("\t# LOCK: %u (1:LOCK, 0: UNLOCK)\n", lock);
	// printf("\t# Antenna Level: %u\n", sig_info.ant_level);
	// printf("\t# ber: %f\n", (float)sig_info.ber/RTV_ISDBT_BER_DIVIDER);
	// printf("\t# cnr: %f\n", (float)sig_info.cnr/RTV_ISDBT_CNR_DIVIDER);
	// printf("\t# rssi: %f\n", (float)sig_info.rssi/RTV_ISDBT_RSSI_DIVIDER);
	// printf("\t# per: %u\n", sig_info.per);
	// printf("\t###################################################\n");
				
	return lock;
}

bool test_mtv(case_t *mtv_case)
{
	int fd_dmb_dev; 			/* MTV device file descriptor. */
	char name[128] = { '0'};
	unsigned int channel[4] = {23, 33, 43, 13};
	int ret = -1, i = 0;
	int lst = 0;
	bool result = false;
	IOCTL_ISDBT_SIGNAL_INFO sig_info;

	sprintf(name,"/dev/%s", RAONTV_DEV_NAME);
	fd_dmb_dev = open(name, O_RDWR);
	if(fd_dmb_dev < 0)
	{
		perror("open failed:");
		return false;
	}

	printf("open success! \n");

	//open the power of mtv 
	ret = isdbt_power_up(fd_dmb_dev);
	if(ret < 0)
	{
		printf("power up failed! \n");
		goto exit;
	}
	printf("power up success! \n");

	for(i = 0; i < 4; i++)
	{
		printf("channel: %d \n", channel[i]);
		ret = isdbt_set_channel(fd_dmb_dev, channel[i]);
		if(ret < 0)
		{
			perror("isdbt set channel failed:");
			goto exit;
		}
	
		sleep(3);

		ret = isdbt_check_lock_status(fd_dmb_dev, &lst);
		if(ret < 0)
		{
			perror("isdbt set channel failed:");
			goto exit;
		}
		else
		{
			printf("lock status: %d \n", lst);
			if(lst){
				for(;;)
				{
					ret = isdbt_check_signal_info(fd_dmb_dev, &sig_info);
					if(ret < 0)
					{
						perror("isdbt set channel failed:");
						goto exit;
					}
					sprintf(mtv_case->pass_string, "%d, %.2f, %.2f",ret, (float)sig_info.rssi / RTV_ISDBT_RSSI_DIVIDER, (float)sig_info.cnr / RTV_ISDBT_CNR_DIVIDER);
					result = true;
					draw_result(mtv_case, true);
					direct_thread_sleep(100000);
				}
				break;
			}else
				continue;
		}
	}

	isdbt_power_down(fd_dmb_dev);
exit:
	close(fd_dmb_dev);
	
	return result;
}

