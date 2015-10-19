/*
 * flash handle, write boot, partition. 
 * card burn or update boot
 * add by lty
 */

#ifndef _BOOTW_H_
#define _BOOTW_H_

/*burn err*/
#define BURN_ERR_UNZIP_BOOT			80
#define BURN_ERR_DECRYP_BOOT		81
#define BURN_ERR_CHANGE_BOOTDEV		82
#define BURN_ERR_INSTALL_NAND		83
#define BURN_ERR_INSTALL_CRAD		84
#define BURN_ERR_WRITE_DATABAK		85
#define BURN_ERR_WRITE_MEDIA		86
#define BURN_ERR_WRITE_VENDOR		87
#define BURN_ERR_WRITE_UDISK		88
#define BURN_ERR_WRITE_DATA			89
#define BURN_ERR_WRITE_SYSTEM		90
#define BURN_ERR_WRITE_MISC			91
#define BURN_ERR_WRITE_REC			92
#define BURN_ERR_WRITE_BOOT			93

/*update err*/
#define UPDATE_ERR_BYCFG			64
#define UPDATE_ERR_WRITE_DATABAK	65
#define UPDATE_ERR_WRITE_MEDIA		66
#define UPDATE_ERR_WRITE_VENDOR		67
#define UPDATE_ERR_WRITE_UDISK		68
#define UPDATE_ERR_WRITE_DATA		69
#define UPDATE_ERR_WRITE_SYSTEM		70
#define UPDATE_ERR_WRITE_MISC		71
#define UPDATE_ERR_WRITE_REC		72
#define UPDATE_ERR_WRITE_BOOT		73

int check_burn( FILE* cmd_fp); /*return -1 normal update, else return > 0  burn fail, 0 burn ok*/
int check_update(ZipArchive *za, const char *pack_zip, char *script_buf, int size, FILE* cmd_fp);
int boot_update_other(ZipArchive *za, FILE* cmd_fp);
int update_del_data(ZipArchive *za);
/*return < 0 fail, 0 update continue, > 0 update stop*/
int parse_update_cfg(FILE* cmd_fp);

#endif