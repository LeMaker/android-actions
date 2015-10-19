/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009 Actions Semi Inc.
*/
/******************************************************************************/

/******************************************************************************/
#ifndef __MBR_INFO_H__
#define __MBR_INFO_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PARTITION   	12
#define HDCP_KEY_SIZE		308 	//bytes
#define SERIAL_NO_SIZE		16   	//bytes
#define PARTITION_TBL_SIZE	(MAX_PARTITION * sizeof(partition_info_t))


#define RECOVERY_ACCESS      0
#define MISC_ACCESS       1
#define ROOTFS_ACCESS       2
#define ANDROID_DATA_ACCESS 3
#define ANDROID_CACHE_ACCESS 4

#define SNAPSHOT_ACCESS		5
#define WHD_ACCESS			6

#define CONFIG_RW_ACCESS 	7


#define UDISK_ACCESS        10

typedef struct
{
    unsigned char   flash_ptn;                  //flash partition number
    unsigned char   partition_num;              //每个分区对应其所在的flash partition的编号
    unsigned short  reserved;                   //reserved：将拓展成该分区的属性
    unsigned int    partition_cap;              //对应分区的大小
}__attribute__ ((packed)) partition_info_t;


typedef struct
{
    unsigned char   flash_ptn;                  //flash partition number
    unsigned char   partition_num;              //每个分区对应其所在的flash partition的编号
    unsigned short  phy_info;                   //reserved：将拓展成该分区的属性
    unsigned int    partition_cap;              //对应分区的大小
}__attribute__ ((packed)) CapInfo_t;


/*
 * don't re-order
 */
typedef struct
{
    partition_info_t partition_info[MAX_PARTITION];            //分区信息表
    unsigned char HdcpKey[HDCP_KEY_SIZE];
    unsigned char SerialNo[SERIAL_NO_SIZE];
    unsigned char reserved[0x400 - PARTITION_TBL_SIZE - HDCP_KEY_SIZE - SERIAL_NO_SIZE];     //mbr_info_t，大小为1k，为以后扩展
}__attribute__ ((packed)) mbr_info_t;



/********************************************************************
分区方式，原则上分区按顺序排列：
            flash_ptn       partition_num       partition_cap的单位
mbrc:           0                   0               block
vmlinux         0                   1               M
rootfs          1                   0               M
configfs        1                   1               M
others          2                   0~n             M

以partition_num为0xff表明最后一个分区
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif
