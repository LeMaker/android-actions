#ifndef TSD_BLOCK_H
#define TSD_BLOCK_H

#define  PLATFORMINC  ../../../../owl/platform/include
#define  BOOTINC 	  ../../../../owl/platform/boot/include

#include "../../../../owl/platform/include/asoc_ioctl.h"
#include "../../../../owl/platform/include/mbr_info.h"     //for CapInfo_t

#ifndef TRUE
#define TRUE   0
#endif

#ifndef FALSE
#define FALSE   1
#endif

#ifndef NULL
#define NULL (void *)0
#endif



struct MiscInfoType_t
{
    unsigned short Magic;        
    unsigned short InfoType;     //sounds not necessary
    unsigned short Size;         //size of one misc info
    unsigned short Offset;       //offset from the beginning
    unsigned short Chksum;
	unsigned int Burn;
    unsigned char  Name[64];
};

// MISC INFORMATION BLOCK
struct MiscInfoBlk_t
{
	unsigned short die;
	unsigned short sblk;
	unsigned short sblkBak;      //for backup
	unsigned short TotalSize;
	unsigned int Burn;
	struct MiscInfoType_t Drm;
	struct MiscInfoType_t Hdcp;
	struct MiscInfoType_t Sn;
	struct MiscInfoType_t DevNum;

	struct MiscInfoType_t ExtSpace;
	struct MiscInfoType_t Reserved[3];
};

#define BOOT_PHY_SIZE			(1024*1024)
#define MBRC_NUM				4
#define BURN_FLAG				0xabcddcba
#define SECTOR_SIZE             512

#define FORMATBYTE   			4096
#define PSECTBYTE	 			512


#define DISABLE_WRITE           _IO('V',0) 
#define ENABLE_WRITE            _IO('V',1) 
#define DISABLE_READ            _IO('V',2)
#define ENABLE_READ             _IO('V',3)

#define NAND_IS_SMALL_BLOCK	0
#define DATA_BLK_NUM_PER_ZONE 0
#define FLASH_TYPE_CARD (0x03)

#define TSD_BLOCKSZ 512
#define TSD_WRITE 1
#define TSD_READ   0

//BOOT INFO:define in tSD card
#define SEC_SIZE 512 //in bytes
#define PAGE_PER_BLK 256 
#define SEC_PER_PAGE 8      // 4k
#define SEC_PER_BOOT_PAGE 8
#define SEC_PER_BLOCK  (PAGE_PER_BLK * SEC_PER_PAGE) // 1M
#define BLOCK_IN_PHY 4 
#define SEC_PHY_BLOCK  (SEC_PER_BLOCK * BLOCK_IN_PHY) // 4M
#define LOGIC_SEC_START SEC_PHY_BLOCK

#define REMAIN_SPACE	0
#define PART_FREE	0x55
#define PART_DUMMY	0xff
#define PART_READONLY	0x85
#define PART_WRITEONLY	0x86
#define PART_NO_ACCESS	0x87

#define NAND_PART_OP_RO		(1 << 0)	// read only.
#define NAND_PART_OP_WO		(1 << 1)	// write only.
#define NAND_PART_OP_NA		(1 << 2)	// not accessable.

//define for compile
typedef unsigned char  UINT8;
//#define TRUE 1
#define MALLOC(size) kmalloc(size, GFP_KERNEL)
#define FREE(d)  kfree(d)
#define MBR_SIZE        1024
#define MEMCPY(to,from,size)   memcpy(to,from,size)
#define MEMSET(s,c,size)   memset(s,c,size)


struct tSD_partinfo {
	unsigned long partsize;					
	unsigned long off_size;
	unsigned char type;
};


typedef struct __tSD_partition_t
{
	int num;				//current partion number
	unsigned long size;		// space size.
	unsigned long offset;	//offset in the who nand device.
	unsigned int attr;		//attribute.
	struct gendisk * disk;	//gendisk to register to system.
//	struct mmc_blk_data *md;
}tSD_partition_t;

typedef struct __boot_medium_info_t
{
    unsigned int medium_type;       //0x1 large block nand; 0x0 small block nand.
    unsigned short pageAddrPerBlock;
    unsigned short pagePerBlock;
    unsigned int sec_per_page;  
    unsigned int sec_per_boot_page;
    unsigned int ecc_bits;          //boot ecc config-bchX.
    unsigned int ud_bytes;          //userdata bytes count per ecc unit.
    unsigned char slc_mode;            //share page slc mode?
    unsigned char tlc_enhance_cmd;
    unsigned char reserve[2];
    unsigned int data_blk_per_zone; //
    unsigned char chipid[64];       //chipid item without Mark.
    unsigned char lsb_tbl[128];     //useable when nand need use share page slc mode.
    unsigned char badblk_tbl[80];   //bad block table.
} boot_medium_info_t;

typedef struct _boot_op_t
{
	unsigned int blk;
	unsigned int page;
	unsigned char * buffer;
}boot_op_t;


/*
 * define for tsd card.
 */
struct mmc_blk_data {
	char *name;						//predefined device name.
	int major;						//predefined major device number.
	int minorbits;
	int users;	
	const char *subname;
	
	struct semaphore mutex;
	struct semaphore	thread_sem;
	spinlock_t *lock;
	spinlock_t	slock;
	spinlock_t	dlock;
	spinlock_t	ulock;
	struct gendisk	*disk;
	tSD_partition_t	*partitions;
	//tSD_partition_t	partitions[MAX_PARTITION];
#define QUEUE_NUM   3
	int req_state;
	struct mmc_queue *queue;
	struct mmc_queue squeue;
	struct mmc_queue dqueue;
	struct mmc_queue uqueue;
	struct list_head part;
	struct task_struct	*thread;

	unsigned int	flags;
#define MMC_BLK_CMD23	(1 << 0)	/* Can do SET_BLOCK_COUNT for multiblock */
#define MMC_BLK_REL_WR	(1 << 1)	/* MMC Reliable write support */
#define MMC_BLK_PACKED_CMD	(1 << 2)	/* MMC packed command support */


	unsigned int	usage;
	unsigned int	read_only;
	unsigned int	part_type;
	unsigned int	name_idx;
	unsigned int	reset_done;
#define MMC_BLK_READ		BIT(0)
#define MMC_BLK_WRITE		BIT(1)
#define MMC_BLK_DISCARD		BIT(2)
#define MMC_BLK_SECDISCARD	BIT(3)

	/*
	 * Only set in main mmc_blk_data associated
	 * with mmc_card with mmc_set_drvdata, and keeps
	 * track of the current selected device partition.
	 */
	unsigned int	part_curr;
	struct device_attribute force_ro;
	struct device_attribute power_ro_lock;
	int	area_type;
};

#endif
