/*
	actions firmware infomation ( para)

*/

#include <common.h>
#include <asm/arch/owl_afi.h>
#include <mmc.h>

#define UBOOT_PARA_ADDR 0xB4060300
static int fs_flag = 0;

int owl_get_boot_mode(void)
{
	afinfo_t * pafi;
	pafi = (afinfo_t *)UBOOT_PARA_ADDR;
	if (memcmp(pafi->magic, "AFI", 3) == 0 )
		 return pafi->boot_mode;

	return BOOT_MODE_PRODUCE;
}

int owl_get_boot_dev(void)
{
	int value = -1;
	afinfo_t * pafi;
	pafi = (afinfo_t *)UBOOT_PARA_ADDR;
	block_dev_desc_t *mmc_dev;
	if(!fs_flag){
	mmc_dev = mmc_get_dev(0);
	if(mmc_dev != NULL && (mmc_dev->type != DEV_TYPE_UNKNOWN && mmc_dev->part_type == PART_TYPE_EFI)){
		//print_part(mmc_dev);
		fs_flag = 1;
		return OWL_BOOTDEV_SD0;
	}	
	mmc_dev = mmc_get_dev(1);
	if(mmc_dev != NULL && mmc_dev->type != DEV_TYPE_UNKNOWN){
                //print_part(mmc_dev);
		fs_flag = 2;
                return OWL_BOOTDEV_SD2;
        }
	} else {
		if(fs_flag == 1){
			return OWL_BOOTDEV_SD0;
		}
		if(fs_flag == 2){
			return OWL_BOOTDEV_SD2;
		}
	}
	
	if (memcmp(pafi->magic, "AFI", 3) == 0 )
		 return pafi->boot_dev;

	return OWL_BOOTDEV_SD0;
}

/*serial*/
int owl_afi_get_serial_number(void)
{
	afinfo_t * pafi;
	pafi = (afinfo_t *)UBOOT_PARA_ADDR;
	if (memcmp(pafi->magic, "AFI", 3) == 0 )
		 return pafi->uart_index;
	return 5;
}

int owl_afi_get_serial_pad(void)
{
	afinfo_t * pafi;
	
	pafi = (afinfo_t *)UBOOT_PARA_ADDR;
	if (memcmp(pafi->magic, "AFI", 3) == 0 )
		 return pafi->uart_pad;
	return 0;
}

int owl_afi_get_serial_baudrate(void)
{
	afinfo_t * pafi;
	pafi = (afinfo_t *)UBOOT_PARA_ADDR;
	if (memcmp(pafi->magic, "AFI", 3) == 0 )
		 return pafi->uart_baudrate;

	return 115200;
}

unsigned long owl_get_ddr_size(void)
{
	afinfo_t * pafi;
	pafi = (afinfo_t *)UBOOT_PARA_ADDR;
	if (memcmp(pafi->magic, "AFI", 3) == 0 )
		 return pafi->ddr_size;

	return 0;
}


/*cant use printf, the sequence is so inportant*/
int owl_afi_init(void)
{
#if 0
	afinfo_t * pafi;
	pafi = (afinfo_t *)UBOOT_PARA_ADDR;
	if (memcmp(pafi->magic, "AFI", 3) == 0 ){
		memcpy(&afinfo, pafi, sizeof(afinfo_t));
		uindex = pafi->uart_index;
	} else {
		afinfo.boot_mode = BOOT_MODE_NORMAL;
		afinfo.boot_dev = OWL_BOOTDEV_SD0;
		afinfo.uart_index = 3;
	}
#endif
	return 0;
}




