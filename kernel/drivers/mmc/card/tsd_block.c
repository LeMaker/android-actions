/*
 * Block driver for media (i.e., flash cards)
 *
 * Copyright 2002 Hewlett-Packard Company
 * Copyright 2005-2008 Pierre Ossman
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * HEWLETT-PACKARD COMPANY MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 * Many thanks to Alessandro Rubini and Jonathan Corbet!
 *
 * Author:  Andrew Christian
 *          28 May 2002
 */
 
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>
#include <linux/mutex.h>
#include <linux/scatterlist.h>
#include <linux/string_helpers.h>
#include <linux/delay.h>
#include <linux/capability.h>
#include <linux/compat.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/kthread.h>

#define CREATE_TRACE_POINTS
#include <trace/events/mmc.h>

#include <linux/mmc/ioctl.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>

#include <asm/uaccess.h>
#include <linux/decompress/mm.h>
#include <mach/secure_storage.h>
#include <mach/bootdev.h>
#include "tsd_queue.h"
#include "tsd_block.h"

#include "../../../../owl/platform/boot/include/address.h"
#include "../../../../owl/platform/boot/include/afinfo.h"


/*
MODULE_ALIAS("mmc:block");
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX "mmcblk."
*/

#define INAND_CMD38_ARG_EXT_CSD  113
#define INAND_CMD38_ARG_ERASE    0x00
#define INAND_CMD38_ARG_TRIM     0x01
#define INAND_CMD38_ARG_SECERASE 0x80
#define INAND_CMD38_ARG_SECTRIM1 0x81
#define INAND_CMD38_ARG_SECTRIM2 0x88
#define MMC_BLK_TIMEOUT_MS  (10 * 60 * 1000)        /* 10 minute timeout */

#define mmc_req_rel_wr(req)	(((req->cmd_flags & REQ_FUA) || \
				  (req->cmd_flags & REQ_META)) && \
				  (rq_data_dir(req) == WRITE))
#define PACKED_CMD_VER	0x01
#define PACKED_CMD_WR	0x02

static DEFINE_MUTEX(block_mutex);

/*
 * The defaults come from config options but can be overriden by module
 * or bootarg options.
 */

static int perdev_minors = CONFIG_MMC_BLOCK_MINORS;

/*
 * We've only got one major, so number of mmcblk devices is
 * limited to 256 / number of minors per device.
 */
static int max_devices;

/* 256 minors, so at most 256 separate devices */
static DECLARE_BITMAP(dev_use, 256);
static DECLARE_BITMAP(name_use, 256);

static int is_for_upgrade = 0; //fot test
module_param(is_for_upgrade, int, S_IRUGO | S_IWUSR);
static int is_force_format = 0;
module_param(is_force_format, int, S_IRUGO | S_IWUSR);
static int card_to_card = 0;
module_param(card_to_card, int, S_IRUGO | S_IWUSR);

static int tsd_major = 93;  ////fot test
static char *blkdev_name = "flash"; 
	
//#define UPGRADE_DBG
#ifdef UPGRADE_DBG
#define UPGRADE_DBG_INF(fmt,args...)  printk("%s,%d,"fmt,__FUNCTION__,__LINE__,##args);
#else
#define UPGRADE_DBG_INF(fmt,args...) do {} while(0)
#endif

int unassign_partnum;
int new_partition_table = 0;
int partition_inmbr = 0;
int partition__logic_incard = 0;

struct mmc_card *tSD_card;
struct tSD_partinfo *tSD_part = NULL;
partition_info_t *capinfo =NULL;

char *mbrc;
afinfo_t *p_afinfo = NULL;
unsigned int card_total_size = 0;
//unsigned char g_pcba_test_flag = 0;      //for pcba test, disable by default.
	
//int sd2_pcba_test = 1;
//EXPORT_SYMBOL(sd2_pcba_test);	
	
struct mmc_blk_data tSD_device_md = 
{
	.name = "tsd_card",
	.major = 93,	
	.minorbits = 3,
	.usage = 1,
	.partitions = NULL,
};

unsigned int miscinfo_start_addr;	//in secters
	
#define MISC_INFO_WR		0
#define MISC_INFO_RD		1
#define MISC_INFO_SECTERS	2048 // 1M byte

#define DRM_KEY_SIZE		64
#define HDCP_KEY_SIZE		308		//must aligned to 2bytes
#define SN_SIZE				32
#define DEVNUM_SIZE			32
#define EXTSPACE_SIZE			4096   //4  
struct MiscInfoBlk_t 	MiscInfo = {
		.die  = 0xffff,
		.sblk = 0xffff,
		.sblkBak = 0xffff,
		.TotalSize = sizeof(struct MiscInfoBlk_t) + SN_SIZE + DRM_KEY_SIZE + HDCP_KEY_SIZE \
					+ DEVNUM_SIZE + EXTSPACE_SIZE,
		.Burn = 0 ,

		.Drm = {
				.Magic    = 0x55,
				.InfoType = 1,
				.Size     = DRM_KEY_SIZE,
				.Name     = "KEY DRM\0",
				.Burn = 0 ,
		},
		.Hdcp = {
				.Magic    = 0x55,
				.InfoType = 2,
				.Size     = HDCP_KEY_SIZE,
				.Name     = "KEY HDCP\0",
				.Burn 	  = 0 ,				
		},
		.Sn = {
				.Magic    = 0x55,
				.InfoType = 0,
				.Size     = SN_SIZE,
				.Name     = "SN\0",
				.Burn 	  = 0 ,				
		},
		.DevNum = {
				.Magic    = 0x55,
				.InfoType = 0,
				.Size     = DEVNUM_SIZE,
				.Name     = "DEV NUM\0",
				.Burn 	  = 0 ,				
		},
		.ExtSpace = {
				.Magic	  = 0x55,
				.InfoType = 0, 
				.Size 	  = EXTSPACE_SIZE,
				.Name     = "EXTSPACE\0",
				.Burn     = 0 ,				
		},

};



static struct semaphore miscMutex;
static void _miscMetuxInit(void)
{
    sema_init(&miscMutex, 1);
}

static void _miscMetuxLock(void)
{
    down(&miscMutex);
}

static void _miscMetuxUnlock(void)
{
    up(&miscMutex);
}

static int do_rw_miscinfo (unsigned int offset, char *buf, int size, int wr_flag);
static int owl_miscinfo_is_burn(void);
unsigned int tSD_op_read(unsigned long start, unsigned long nsector, 
						void *buf, struct inode * i);
unsigned int tSD_op_write(unsigned long start, unsigned long nsector,
						void *buf, struct inode * i);
int tSD_pre_data_transfer(unsigned start, unsigned nsector, 
							void *buf, unsigned blksz, int write);
int tSD_adfu_read(unsigned long start, unsigned long nsector, void *buf, 
					struct uparam * i);
int tSD_adfu_write(unsigned long start, unsigned long nsector, void *buf, 
					struct uparam * i);
int __do_adfu_read(unsigned start, unsigned nsector, void *buf);
int __do_adfu_write(unsigned start, unsigned nsector, void *buf);
int tSD_data_transfer(struct mmc_card *card, unsigned char *buf,
	unsigned  start, unsigned blocks, unsigned blksz, int write);
static void tSD_prepare_mrq(struct mmc_card *card, unsigned char *buf,
	struct mmc_request *mrq, unsigned start, unsigned blocks, unsigned blksz, int write);
static int tSD_wait_busy(struct mmc_card *card);
static int tSD_test_busy(struct mmc_command *cmd);
static int tSD_test_check_result(struct mmc_card *card, struct mmc_request *mrq);

int tSD_single_blk_test(unsigned start, unsigned blocks);

static int tSD_queue_init(struct mmc_blk_data *tSD_device);
static int tSD_partition_init(struct mmc_blk_data *tSD_device, int part_num);
static int owl_hdcp_is_burn(void);
static int tSD_blk_ioctl(struct block_device * bdev, fmode_t mode,
						unsigned int cmd, unsigned long arg);
int get_boot_media_info(unsigned int arg);
int boot_operation(unsigned int arg, unsigned int cmd);
int boot_phy_op_entry(boot_op_t * op, unsigned int cmd);
unsigned  convert_to_sector(unsigned blk, unsigned page);
static unsigned int tSD_prep_sector(struct request *req);

int init_board_cfg(void);
int get_afi_configuration(void);
partition_info_t *GetMbrFromUser(void);
unsigned int GetAfiFromUser(afinfo_t * afinfo);
unsigned int tSD_get_logic_cat(void);
int init_tSD_part_myself(void);
static unsigned int get_cap_offset(int part);
unsigned int UpdateMbrToUsr(partition_info_t * partition_info_tbl, afinfo_t *p_afi);
unsigned int UpdateMbrFromPhyToUsr(unsigned int *p_nand_part, mbr_info_t *p_mbr_info);
unsigned int ReadAfinfo(void);
unsigned int calCRC(unsigned char *buf, unsigned int length, unsigned char nBytes);
int NAND_GetMiscInfo(int type, char *buf, int size);
int NAND_WriteMiscInfo(int type, char *buf, int size);
void NAND_InitMiscInfo(void);
int handle_misc_info(unsigned int arg);
int calculate_part_num(void);
int owl_set_carddev_match_name(void);




 // external functions defined in adfus.ko
typedef unsigned int (*func_t)(unsigned int *p_nand_part, mbr_info_t *p_mbr_info);
typedef void (*func_t1)(void);
typedef int (*func_t4)(unsigned long, unsigned long , void *, struct uparam *);
extern func_t AdfuUpdateMbrFromPhyToUsr;
extern func_t1 adfu_flush_nand_cache;
extern func_t4 adfus_nand_read;
extern func_t4 adfus_nand_write;


extern void dump_mem(void *startaddr, unsigned int size,
						unsigned int showaddr, unsigned int show_bytes);


static DEFINE_MUTEX(open_lock);

enum {
	MMC_PACKED_NR_IDX = -1,
	MMC_PACKED_NR_ZERO,
	MMC_PACKED_NR_SINGLE,
};

module_param(perdev_minors, int, 0444);
MODULE_PARM_DESC(perdev_minors, "Minors numbers to allocate per device");

struct secure_storage card_secure =
{
    .name = "card",
    .read_data = NAND_GetMiscInfo,
    .write_data = NAND_WriteMiscInfo,
};

static inline int mmc_blk_part_switch(struct mmc_card *card,
				      struct mmc_blk_data *md);
static int get_card_status(struct mmc_card *card, u32 *status, int retries);

static inline void mmc_blk_clear_packed(struct mmc_queue_req *mqrq)
{
	struct mmc_packed *packed = mqrq->packed;

	BUG_ON(!packed);

	mqrq->cmd_type = MMC_PACKED_NONE;
	packed->nr_entries = MMC_PACKED_NR_ZERO;
	packed->idx_failure = MMC_PACKED_NR_IDX;
	packed->retries = 0;
	packed->blocks = 0;
}

#if 0
static struct mmc_blk_data *mmc_blk_get(struct gendisk *disk)
{
	struct mmc_blk_data *md;

	mutex_lock(&open_lock);
	md = disk->private_data;
	if (md && md->usage == 0)
		md = NULL;
	if (md)
		md->usage++;
	mutex_unlock(&open_lock);

	return md;
}

#endif
static inline int mmc_get_devidx(struct gendisk *disk)
{
	int devidx = disk->first_minor / perdev_minors;
	return devidx;
}

#if 0
static void mmc_blk_put(struct mmc_blk_data *md)
{
	mutex_lock(&open_lock);
	md->usage--;
	if (md->usage == 0) {
		int devidx = mmc_get_devidx(md->disk);
		blk_cleanup_queue(md->queue.queue);

		__clear_bit(devidx, dev_use);

		put_disk(md->disk);
		kfree(md);
	}
	mutex_unlock(&open_lock);
}
#endif
#if 0
static void tSD_blk_put(struct mmc_blk_data *tSD_device)
{
	int i;
	mutex_lock(&open_lock);
	tSD_device->usage--;
	if(tSD_device->usage == 0){
		blk_cleanup_queue(tSD_device->squeue.queue);
		blk_cleanup_queue(tSD_device->dqueue.queue);
		blk_cleanup_queue(tSD_device->uqueue.queue);

		for(i = 0; i < partition__logic_incard; i++)
		{
			put_disk(tSD_device->partitions[i].disk);
		}
	}
	mutex_unlock(&open_lock);
}
#endif
#if 0
static ssize_t power_ro_lock_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	struct mmc_card *card = md->queue.card;
	int locked = 0;

	if (card->ext_csd.boot_ro_lock & EXT_CSD_BOOT_WP_B_PERM_WP_EN)
		locked = 2;
	else if (card->ext_csd.boot_ro_lock & EXT_CSD_BOOT_WP_B_PWR_WP_EN)
		locked = 1;

	ret = snprintf(buf, PAGE_SIZE, "%d\n", locked);

	return ret;
}

static ssize_t power_ro_lock_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	struct mmc_blk_data *md, *part_md;
	struct mmc_card *card;
	unsigned long set;

	if (kstrtoul(buf, 0, &set))
		return -EINVAL;

	if (set != 1)
		return count;

	md = mmc_blk_get(dev_to_disk(dev));
	card = md->queue.card;

	mmc_claim_host(card->host);

	ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BOOT_WP,
				card->ext_csd.boot_ro_lock |
				EXT_CSD_BOOT_WP_B_PWR_WP_EN,
				card->ext_csd.part_time);
	if (ret)
		pr_err("%s: Locking boot partition ro until next power on failed: %d\n", md->disk->disk_name, ret);
	else
		card->ext_csd.boot_ro_lock |= EXT_CSD_BOOT_WP_B_PWR_WP_EN;

	mmc_release_host(card->host);

	if (!ret) {
		pr_info("%s: Locking boot partition ro until next power on\n",
			md->disk->disk_name);
		set_disk_ro(md->disk, 1);

		list_for_each_entry(part_md, &md->part, part)
			if (part_md->area_type == MMC_BLK_DATA_AREA_BOOT) {
				pr_info("%s: Locking boot partition ro until next power on\n", part_md->disk->disk_name);
				set_disk_ro(part_md->disk, 1);
			}
	}

	mmc_blk_put(md);
	return count;
}

static ssize_t force_ro_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	int ret;
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));

	ret = snprintf(buf, PAGE_SIZE, "%d",
		       get_disk_ro(dev_to_disk(dev)) ^
		       md->read_only);
	mmc_blk_put(md);
	return ret;
}

static ssize_t force_ro_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int ret;
	char *end;
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	unsigned long set = simple_strtoul(buf, &end, 0);
	if (end == buf) {
		ret = -EINVAL;
		goto out;
	}

	set_disk_ro(dev_to_disk(dev), set || md->read_only);
	ret = count;
out:
	mmc_blk_put(md);
	return ret;
}

static int mmc_blk_open(struct block_device *bdev, fmode_t mode)
{
	struct mmc_blk_data *md = mmc_blk_get(bdev->bd_disk);
	int ret = -ENXIO;

	mutex_lock(&block_mutex);
	if (md) {
		if (md->usage == 2)
			check_disk_change(bdev);
		ret = 0;

		if ((mode & FMODE_WRITE) && md->read_only) {
			mmc_blk_put(md);
			ret = -EROFS;
		}
	}
	mutex_unlock(&block_mutex);

	return ret;
}
#endif
static void mmc_blk_release(struct gendisk *disk, fmode_t mode)
{
	return ;
#if 0	
	struct mmc_blk_data *md = disk->private_data;

	mutex_lock(&block_mutex);
	mmc_blk_put(md);
	mutex_unlock(&block_mutex);
#endif	
}


static int
mmc_blk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	//geo->cylinders = get_capacity(bdev->bd_disk) / (4 * 16);
	//geo->heads = 4;
	//geo->sectors = 16;
	return 0;
}

struct mmc_blk_ioc_data {
	struct mmc_ioc_cmd ic;
	unsigned char *buf;
	u64 buf_bytes;
};

#if 0
static struct mmc_blk_ioc_data *mmc_blk_ioctl_copy_from_user(
	struct mmc_ioc_cmd __user *user)
{
	struct mmc_blk_ioc_data *idata;
	int err;

	idata = kzalloc(sizeof(*idata), GFP_KERNEL);
	if (!idata) {
		err = -ENOMEM;
		goto out;
	}

	if (copy_from_user(&idata->ic, user, sizeof(idata->ic))) {
		err = -EFAULT;
		goto idata_err;
	}

	idata->buf_bytes = (u64) idata->ic.blksz * idata->ic.blocks;
	if (idata->buf_bytes > MMC_IOC_MAX_BYTES) {
		err = -EOVERFLOW;
		goto idata_err;
	}

	if (!idata->buf_bytes)
		return idata;

	idata->buf = kzalloc(idata->buf_bytes, GFP_KERNEL);
	if (!idata->buf) {
		err = -ENOMEM;
		goto idata_err;
	}

	if (copy_from_user(idata->buf, (void __user *)(unsigned long)
					idata->ic.data_ptr, idata->buf_bytes)) {
		err = -EFAULT;
		goto copy_err;
	}

	return idata;

copy_err:
	kfree(idata->buf);
idata_err:
	kfree(idata);
out:
	return ERR_PTR(err);
}

static int ioctl_rpmb_card_status_poll(struct mmc_card *card, u32 *status,
				       u32 retries_max)
{
	int err;
	u32 retry_count = 0;

	if (!status || !retries_max)
		return -EINVAL;

	do {
		err = get_card_status(card, status, 5);
		if (err)
			break;

		if (!R1_STATUS(*status) &&
				(R1_CURRENT_STATE(*status) != R1_STATE_PRG))
			break; /* RPMB programming operation complete */

		/*
		 * Rechedule to give the MMC device a chance to continue
		 * processing the previous command without being polled too
		 * frequently.
		 */
		usleep_range(1000, 5000);
	} while (++retry_count < retries_max);

	if (retry_count == retries_max)
		err = -EPERM;

	return err;
}

static int mmc_blk_ioctl_cmd(struct block_device *bdev,
	struct mmc_ioc_cmd __user *ic_ptr)
{
	struct mmc_blk_ioc_data *idata;
	struct mmc_blk_data *md;
	struct mmc_card *card;
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct mmc_request mrq = {NULL};
	struct scatterlist sg;
	int err;
	int is_rpmb = false;
	u32 status = 0;

	/*
	 * The caller must have CAP_SYS_RAWIO, and must be calling this on the
	 * whole block device, not on a partition.  This prevents overspray
	 * between sibling partitions.
	 */
	if ((!capable(CAP_SYS_RAWIO)) || (bdev != bdev->bd_contains))
		return -EPERM;

	idata = mmc_blk_ioctl_copy_from_user(ic_ptr);
	if (IS_ERR(idata))
		return PTR_ERR(idata);

	md = mmc_blk_get(bdev->bd_disk);
	if (!md) {
		err = -EINVAL;
		goto cmd_err;
	}

	if (md->area_type & MMC_BLK_DATA_AREA_RPMB)
		is_rpmb = true;

	card = md->queue.card;
	if (IS_ERR(card)) {
		err = PTR_ERR(card);
		goto cmd_done;
	}

	cmd.opcode = idata->ic.opcode;
	cmd.arg = idata->ic.arg;
	cmd.flags = idata->ic.flags;

	if (idata->buf_bytes) {
		data.sg = &sg;
		data.sg_len = 1;
		data.blksz = idata->ic.blksz;
		data.blocks = idata->ic.blocks;

		sg_init_one(data.sg, idata->buf, idata->buf_bytes);

		if (idata->ic.write_flag)
			data.flags = MMC_DATA_WRITE;
		else
			data.flags = MMC_DATA_READ;

		/* data.flags must already be set before doing this. */
		mmc_set_data_timeout(&data, card);

		/* Allow overriding the timeout_ns for empirical tuning. */
		if (idata->ic.data_timeout_ns)
			data.timeout_ns = idata->ic.data_timeout_ns;

		if ((cmd.flags & MMC_RSP_R1B) == MMC_RSP_R1B) {
			/*
			 * Pretend this is a data transfer and rely on the
			 * host driver to compute timeout.  When all host
			 * drivers support cmd.cmd_timeout for R1B, this
			 * can be changed to:
			 *
			 *     mrq.data = NULL;
			 *     cmd.cmd_timeout = idata->ic.cmd_timeout_ms;
			 */
			data.timeout_ns = idata->ic.cmd_timeout_ms * 1000000;
		}

		mrq.data = &data;
	}

	mrq.cmd = &cmd;

	mmc_claim_host(card->host);

	err = mmc_blk_part_switch(card, md);
	if (err)
		goto cmd_rel_host;

	if (idata->ic.is_acmd) {
		err = mmc_app_cmd(card->host, card);
		if (err)
			goto cmd_rel_host;
	}

	if (is_rpmb) {
		err = mmc_set_blockcount(card, data.blocks,
			idata->ic.write_flag & (1 << 31));
		if (err)
			goto cmd_rel_host;
	}

	mmc_wait_for_req(card->host, &mrq);

	if (cmd.error) {
		dev_err(mmc_dev(card->host), "%s: cmd error %d\n",
						__func__, cmd.error);
		err = cmd.error;
		goto cmd_rel_host;
	}
	if (data.error) {
		dev_err(mmc_dev(card->host), "%s: data error %d\n",
						__func__, data.error);
		err = data.error;
		goto cmd_rel_host;
	}

	/*
	 * According to the SD specs, some commands require a delay after
	 * issuing the command.
	 */
	if (idata->ic.postsleep_min_us)
		usleep_range(idata->ic.postsleep_min_us, idata->ic.postsleep_max_us);

	if (copy_to_user(&(ic_ptr->response), cmd.resp, sizeof(cmd.resp))) {
		err = -EFAULT;
		goto cmd_rel_host;
	}

	if (!idata->ic.write_flag) {
		if (copy_to_user((void __user *)(unsigned long) idata->ic.data_ptr,
						idata->buf, idata->buf_bytes)) {
			err = -EFAULT;
			goto cmd_rel_host;
		}
	}

	if (is_rpmb) {
		/*
		 * Ensure RPMB command has completed by polling CMD13
		 * "Send Status".
		 */
		err = ioctl_rpmb_card_status_poll(card, &status, 5);
		if (err)
			dev_err(mmc_dev(card->host),
					"%s: Card Status=0x%08X, error %d\n",
					__func__, status, err);
	}

cmd_rel_host:
	mmc_release_host(card->host);

cmd_done:
	mmc_blk_put(md);
cmd_err:
	kfree(idata->buf);
	kfree(idata);
	return err;
}

static int mmc_blk_ioctl(struct block_device *bdev, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;
	if (cmd == MMC_IOC_CMD)
		ret = mmc_blk_ioctl_cmd(bdev, (struct mmc_ioc_cmd __user *)arg);
	return ret;
}



#ifdef CONFIG_COMPAT
static int mmc_blk_compat_ioctl(struct block_device *bdev, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
	return mmc_blk_ioctl(bdev, mode, cmd, (unsigned long) compat_ptr(arg));
}
#endif

#endif
static void tSD_op_flush(void)
{
    
}

static const struct block_device_operations mmc_bdops = {
//	.open			= mmc_blk_open,
	.release		= mmc_blk_release,
	.getgeo			= mmc_blk_getgeo,
	.owner			= THIS_MODULE,
//	.ioctl			= mmc_blk_ioctl,
	.ioctl			= tSD_blk_ioctl,
    	.blk_read       = tSD_op_read,
    	.blk_write      = tSD_op_write,
    	.flush_disk_cache = tSD_op_flush, 
#ifdef CONFIG_COMPAT
//	.compat_ioctl		= mmc_blk_compat_ioctl,
#endif
};

static inline int mmc_blk_part_switch(struct mmc_card *card,
				      struct mmc_blk_data *md)
{
	int ret;
	struct mmc_blk_data *main_md = mmc_get_drvdata(card);

	if (main_md->part_curr == md->part_type)
		return 0;

	if (mmc_card_mmc(card)) {
		u8 part_config = card->ext_csd.part_config;

		part_config &= ~EXT_CSD_PART_CONFIG_ACC_MASK;
		part_config |= md->part_type;

		ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_PART_CONFIG, part_config,
				 card->ext_csd.part_time);
		if (ret)
			return ret;

		card->ext_csd.part_config = part_config;
	}

	main_md->part_curr = md->part_type;
	return 0;
}

static u32 mmc_sd_num_wr_blocks(struct mmc_card *card)
{
	int err;
	u32 result;
	__be32 *blocks;

	struct mmc_request mrq = {NULL};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};

	struct scatterlist sg;

	cmd.opcode = MMC_APP_CMD;
	cmd.arg = card->rca << 16;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;

	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err)
		return (u32)-1;
	if (!mmc_host_is_spi(card->host) && !(cmd.resp[0] & R1_APP_CMD))
		return (u32)-1;

	memset(&cmd, 0, sizeof(struct mmc_command));

	cmd.opcode = SD_APP_SEND_NUM_WR_BLKS;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	data.blksz = 4;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.sg = &sg;
	data.sg_len = 1;
	mmc_set_data_timeout(&data, card);

	mrq.cmd = &cmd;
	mrq.data = &data;

	blocks = kmalloc(4, GFP_KERNEL);
	if (!blocks)
		return (u32)-1;

	sg_init_one(&sg, blocks, 4);

	mmc_wait_for_req(card->host, &mrq);

	result = ntohl(*blocks);
	kfree(blocks);

	if (cmd.error || data.error)
		result = (u32)-1;

	return result;
}

static int send_stop(struct mmc_card *card, u32 *status)
{
	struct mmc_command cmd = {0};
	int err;

	cmd.opcode = MMC_STOP_TRANSMISSION;
	cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	err = mmc_wait_for_cmd(card->host, &cmd, 5);
	if (err == 0)
		*status = cmd.resp[0];
	return err;
}

static int get_card_status(struct mmc_card *card, u32 *status, int retries)
{
	struct mmc_command cmd = {0};
	int err;

	cmd.opcode = MMC_SEND_STATUS;
	if (!mmc_host_is_spi(card->host))
		cmd.arg = card->rca << 16;
	cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;
	err = mmc_wait_for_cmd(card->host, &cmd, retries);
	if (err == 0)
		*status = cmd.resp[0];
	return err;
}

#define ERR_NOMEDIUM	3
#define ERR_RETRY	2
#define ERR_ABORT	1
#define ERR_CONTINUE	0

static int mmc_blk_cmd_error(struct request *req, const char *name, int error,
	bool status_valid, u32 status)
{
	switch (error) {
	case -EILSEQ:
		/* response crc error, retry the r/w cmd */
		pr_err("%s: %s sending %s command, card status %#x\n",
			req->rq_disk->disk_name, "response CRC error",
			name, status);
		return ERR_RETRY;

	case -ETIMEDOUT:
		pr_err("%s: %s sending %s command, card status %#x\n",
			req->rq_disk->disk_name, "timed out", name, status);

		/* If the status cmd initially failed, retry the r/w cmd */
		if (!status_valid) {
			pr_err("%s: status not valid, retrying timeout\n", req->rq_disk->disk_name);
			return ERR_RETRY;
		}
		/*
		 * If it was a r/w cmd crc error, or illegal command
		 * (eg, issued in wrong state) then retry - we should
		 * have corrected the state problem above.
		 */
		if (status & (R1_COM_CRC_ERROR | R1_ILLEGAL_COMMAND)) {
			pr_err("%s: command error, retrying timeout\n", req->rq_disk->disk_name);
			return ERR_RETRY;
		}

		/* Otherwise abort the command */
		pr_err("%s: not retrying timeout\n", req->rq_disk->disk_name);
		return ERR_ABORT;

	default:
		/* We don't understand the error code the driver gave us */
		pr_err("%s: unknown error %d sending read/write command, card status %#x\n",
		       req->rq_disk->disk_name, error, status);
		return ERR_ABORT;
	}
}

/*
 * Initial r/w and stop cmd error recovery.
 * We don't know whether the card received the r/w cmd or not, so try to
 * restore things back to a sane state.  Essentially, we do this as follows:
 * - Obtain card status.  If the first attempt to obtain card status fails,
 *   the status word will reflect the failed status cmd, not the failed
 *   r/w cmd.  If we fail to obtain card status, it suggests we can no
 *   longer communicate with the card.
 * - Check the card state.  If the card received the cmd but there was a
 *   transient problem with the response, it might still be in a data transfer
 *   mode.  Try to send it a stop command.  If this fails, we can't recover.
 * - If the r/w cmd failed due to a response CRC error, it was probably
 *   transient, so retry the cmd.
 * - If the r/w cmd timed out, but we didn't get the r/w cmd status, retry.
 * - If the r/w cmd timed out, and the r/w cmd failed due to CRC error or
 *   illegal cmd, retry.
 * Otherwise we don't understand what happened, so abort.
 */
static int mmc_blk_cmd_recovery(struct mmc_card *card, struct request *req,
	struct mmc_blk_request *brq, int *ecc_err, int *gen_err)
{
	bool prev_cmd_status_valid = true;
	u32 status, stop_status = 0;
	int err, retry;

	if (mmc_card_removed(card))
		return ERR_NOMEDIUM;

	/*
	 * Try to get card status which indicates both the card state
	 * and why there was no response.  If the first attempt fails,
	 * we can't be sure the returned status is for the r/w command.
	 */
	for (retry = 2; retry >= 0; retry--) {
		err = get_card_status(card, &status, 0);
		if (!err)
			break;

		prev_cmd_status_valid = false;
		pr_err("%s: error %d sending status command, %sing\n",
		       req->rq_disk->disk_name, err, retry ? "retry" : "abort");
	}

	/* We couldn't get a response from the card.  Give up. */
	if (err) {
		/* Check if the card is removed */
		if (mmc_detect_card_removed(card->host))
			return ERR_NOMEDIUM;
		return ERR_ABORT;
	}

	/* Flag ECC errors */
	if ((status & R1_CARD_ECC_FAILED) ||
	    (brq->stop.resp[0] & R1_CARD_ECC_FAILED) ||
	    (brq->cmd.resp[0] & R1_CARD_ECC_FAILED))
		*ecc_err = 1;

	/* Flag General errors */
	if (!mmc_host_is_spi(card->host) && rq_data_dir(req) != READ)
		if ((status & R1_ERROR) ||
			(brq->stop.resp[0] & R1_ERROR)) {
			pr_err("%s: %s: general error sending stop or status command, stop cmd response %#x, card status %#x\n",
			       req->rq_disk->disk_name, __func__,
			       brq->stop.resp[0], status);
			*gen_err = 1;
		}

	/*
	 * Check the current card state.  If it is in some data transfer
	 * mode, tell it to stop (and hopefully transition back to TRAN.)
	 */
	if (R1_CURRENT_STATE(status) == R1_STATE_DATA ||
	    R1_CURRENT_STATE(status) == R1_STATE_RCV) {
		err = send_stop(card, &stop_status);
		if (err)
			pr_err("%s: error %d sending stop command\n",
			       req->rq_disk->disk_name, err);

		/*
		 * If the stop cmd also timed out, the card is probably
		 * not present, so abort.  Other errors are bad news too.
		 */
		if (err)
			return ERR_ABORT;
		if (stop_status & R1_CARD_ECC_FAILED)
			*ecc_err = 1;
		if (!mmc_host_is_spi(card->host) && rq_data_dir(req) != READ)
			if (stop_status & R1_ERROR) {
				pr_err("%s: %s: general error sending stop command, stop cmd response %#x\n",
				       req->rq_disk->disk_name, __func__,
				       stop_status);
				*gen_err = 1;
			}
	}

	/* Check for set block count errors */
	if (brq->sbc.error)
		return mmc_blk_cmd_error(req, "SET_BLOCK_COUNT", brq->sbc.error,
				prev_cmd_status_valid, status);

	/* Check for r/w command errors */
	if (brq->cmd.error)
		return mmc_blk_cmd_error(req, "r/w cmd", brq->cmd.error,
				prev_cmd_status_valid, status);

	/* Data errors */
	if (!brq->stop.error)
		return ERR_CONTINUE;

	/* Now for stop errors.  These aren't fatal to the transfer. */
	pr_err("%s: error %d sending stop command, original cmd response %#x, card status %#x\n",
	       req->rq_disk->disk_name, brq->stop.error,
	       brq->cmd.resp[0], status);

	/*
	 * Subsitute in our own stop status as this will give the error
	 * state which happened during the execution of the r/w command.
	 */
	if (stop_status) {
		brq->stop.resp[0] = stop_status;
		brq->stop.error = 0;
	}
	return ERR_CONTINUE;
}

static int mmc_blk_reset(struct mmc_blk_data *md, struct mmc_host *host,
			 int type)
{
	int err;

	if (md->reset_done & type)
		return -EEXIST;

	md->reset_done |= type;
	err = mmc_hw_reset(host);
	/* Ensure we switch back to the correct partition */
	if (err != -EOPNOTSUPP) {
		struct mmc_blk_data *main_md = mmc_get_drvdata(host->card);
		int part_err;

		main_md->part_curr = main_md->part_type;
		part_err = mmc_blk_part_switch(host->card, md);
		if (part_err) {
			/*
			 * We have failed to get back into the correct
			 * partition, so we need to abort the whole request.
			 */
			return -ENODEV;
		}
	}
	return err;
}

static inline void mmc_blk_reset_success(struct mmc_blk_data *md, int type)
{
	md->reset_done &= ~type;
}

static int mmc_blk_issue_discard_rq(struct mmc_queue *mq, struct request *req)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue->card;

	unsigned int from, nr, arg;
	int err = 0, type = MMC_BLK_DISCARD;


	if (!mmc_can_erase(card)) {
		err = -EOPNOTSUPP;
		goto out;
	}

	from = tSD_prep_sector(req);
	nr 	 = blk_rq_sectors(req);

	if (mmc_can_discard(card)){
		arg = MMC_DISCARD_ARG;
	}

	else if (mmc_can_trim(card)){
		arg = MMC_TRIM_ARG;
	}

	else{
		arg = MMC_ERASE_ARG;
	}

retry:
	if (card->quirks & MMC_QUIRK_INAND_CMD38) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 INAND_CMD38_ARG_EXT_CSD,
				 arg == MMC_TRIM_ARG ?
				 INAND_CMD38_ARG_TRIM :
				 INAND_CMD38_ARG_ERASE,
				 0);
		if (err)
			goto out;
	}
	err = mmc_erase(card, from, nr, arg);
out:
	if (err == -EIO && !mmc_blk_reset(md, card->host, type))
		goto retry;
	if (!err)
		mmc_blk_reset_success(md, type);
	blk_end_request(req, err, blk_rq_bytes(req));

	return err ? 0 : 1;
}

static int mmc_blk_issue_secdiscard_rq(struct mmc_queue *mq,
				       struct request *req)

{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue->card;
	
	unsigned int from, nr, arg, trim_arg, erase_arg;
	int err = 0, type = MMC_BLK_SECDISCARD;

	if (!(mmc_can_secure_erase_trim(card) || mmc_can_sanitize(card))) {
		err = -EOPNOTSUPP;
		goto out;
	}

	from = tSD_prep_sector(req);
	nr 	 = blk_rq_sectors(req);

	/* The sanitize operation is supported at v4.5 only */
	if (mmc_can_sanitize(card)) {
		erase_arg = MMC_ERASE_ARG;
		trim_arg = MMC_TRIM_ARG;
	} else {
		erase_arg = MMC_SECURE_ERASE_ARG;
		trim_arg = MMC_SECURE_TRIM1_ARG;
	}

	if (mmc_erase_group_aligned(card, from, nr))
		arg = erase_arg;
	else if (mmc_can_trim(card))
		arg = trim_arg;
	else {
		err = -EINVAL;
		goto out;
	}
retry:
	if (card->quirks & MMC_QUIRK_INAND_CMD38) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 INAND_CMD38_ARG_EXT_CSD,
				 arg == MMC_SECURE_TRIM1_ARG ?
				 INAND_CMD38_ARG_SECTRIM1 :
				 INAND_CMD38_ARG_SECERASE,
				 0);
		if (err)
			goto out_retry;
	}

	err = mmc_erase(card, from, nr, arg);
	if (err == -EIO)
		goto out_retry;
	if (err)
		goto out;

	if (arg == MMC_SECURE_TRIM1_ARG) {
		if (card->quirks & MMC_QUIRK_INAND_CMD38) {
			err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					 INAND_CMD38_ARG_EXT_CSD,
					 INAND_CMD38_ARG_SECTRIM2,
					 0);
			if (err)
				goto out_retry;
		}

		err = mmc_erase(card, from, nr, MMC_SECURE_TRIM2_ARG);
		if (err == -EIO)
			goto out_retry;
		if (err)
			goto out;
	}

	if (mmc_can_sanitize(card)) {
//		trace_mmc_blk_erase_start(EXT_CSD_SANITIZE_START, 0, 0);
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_SANITIZE_START, 1, 0);
//		trace_mmc_blk_erase_end(EXT_CSD_SANITIZE_START, 0, 0);
	}
out_retry:
	if (err && !mmc_blk_reset(md, card->host, type))
		goto retry;
	if (!err)
		mmc_blk_reset_success(md, type);
out:
	blk_end_request(req, err, blk_rq_bytes(req));

	return err ? 0 : 1;
}

static int mmc_blk_issue_flush(struct mmc_queue *mq, struct request *req)
{
	printk("%s,%d\n",__FUNCTION__,__LINE__);
	return 0;
#if 0	
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	int ret = 0;

	ret = mmc_flush_cache(card);
	if (ret)
		ret = -EIO;

	blk_end_request_all(req, ret);

	return ret ? 0 : 1;
#endif	
}

/*
 * Reformat current write as a reliable write, supporting
 * both legacy and the enhanced reliable write MMC cards.
 * In each transfer we'll handle only as much as a single
 * reliable write can handle, thus finish the request in
 * partial completions.
 */
static inline void mmc_apply_rel_rw(struct mmc_blk_request *brq,
				    struct mmc_card *card,
				    struct request *req)
{
	printk("%s,%d\n",__FUNCTION__,__LINE__);
#if 0	
	if (!(card->ext_csd.rel_param & EXT_CSD_WR_REL_PARAM_EN)) {
		/* Legacy mode imposes restrictions on transfers. */
		if (!IS_ALIGNED(brq->cmd.arg, card->ext_csd.rel_sectors))
			brq->data.blocks = 1;

		if (brq->data.blocks > card->ext_csd.rel_sectors)
			brq->data.blocks = card->ext_csd.rel_sectors;
		else if (brq->data.blocks < card->ext_csd.rel_sectors)
			brq->data.blocks = 1;
	}
#endif	
}

#define CMD_ERRORS							\
	(R1_OUT_OF_RANGE |	/* Command argument out of range */	\
	 R1_ADDRESS_ERROR |	/* Misaligned address */		\
	 R1_BLOCK_LEN_ERROR |	/* Transferred block length incorrect */\
	 R1_WP_VIOLATION |	/* Tried to write to protected block */	\
	 R1_CC_ERROR |		/* Card controller error */		\
	 R1_ERROR)		/* General/unknown error */

static int mmc_blk_err_check(struct mmc_card *card,
			     struct mmc_async_req *areq)
{
	struct mmc_queue_req *mq_mrq = container_of(areq, struct mmc_queue_req,
						    mmc_active);
	struct mmc_blk_request *brq = &mq_mrq->brq;
	struct request *req = mq_mrq->req;
	int ecc_err = 0, gen_err = 0;

	/*
	 * sbc.error indicates a problem with the set block count
	 * command.  No data will have been transferred.
	 *
	 * cmd.error indicates a problem with the r/w command.  No
	 * data will have been transferred.
	 *
	 * stop.error indicates a problem with the stop command.  Data
	 * may have been transferred, or may still be transferring.
	 */
	if (brq->sbc.error || brq->cmd.error || brq->stop.error ||
	    brq->data.error) {
		switch (mmc_blk_cmd_recovery(card, req, brq, &ecc_err, &gen_err)) {
		case ERR_RETRY:
			return MMC_BLK_RETRY;
		case ERR_ABORT:
			return MMC_BLK_ABORT;
		case ERR_NOMEDIUM:
			return MMC_BLK_NOMEDIUM;
		case ERR_CONTINUE:
			break;
		}
	}

	/*
	 * Check for errors relating to the execution of the
	 * initial command - such as address errors.  No data
	 * has been transferred.
	 */
	if (brq->cmd.resp[0] & CMD_ERRORS) {
		pr_err("%s: r/w command failed, status = %#x\n",
		       req->rq_disk->disk_name, brq->cmd.resp[0]);
		return MMC_BLK_ABORT;
	}

	/*
	 * Everything else is either success, or a data error of some
	 * kind.  If it was a write, we may have transitioned to
	 * program mode, which we have to wait for it to complete.
	 */
	if (!mmc_host_is_spi(card->host) && rq_data_dir(req) != READ) {
		u32 status;
		unsigned long timeout;

		/* Check stop command response */
		if (brq->stop.resp[0] & R1_ERROR) {
			pr_err("%s: %s: general error sending stop command, stop cmd response %#x\n",
			       req->rq_disk->disk_name, __func__,
			       brq->stop.resp[0]);
			gen_err = 1;
		}

		timeout = jiffies + msecs_to_jiffies(MMC_BLK_TIMEOUT_MS);
		do {
			int err = get_card_status(card, &status, 5);
			if (err) {
				pr_err("%s: error %d requesting status\n",
				       req->rq_disk->disk_name, err);
				return MMC_BLK_CMD_ERR;
			}

			if (status & R1_ERROR) {
				pr_err("%s: %s: general error sending status command, card status %#x\n",
				       req->rq_disk->disk_name, __func__,
				       status);
				gen_err = 1;
			}

			/* Timeout if the device never becomes ready for data
			 * and never leaves the program state.
			 */
			if (time_after(jiffies, timeout)) {
				pr_err("%s: Card stuck in programming state!"\
					" %s %s\n", mmc_hostname(card->host),
					req->rq_disk->disk_name, __func__);

				return MMC_BLK_CMD_ERR;
			}
			/*
			 * Some cards mishandle the status bits,
			 * so make sure to check both the busy
			 * indication and the card state.
			 */
		} while (!(status & R1_READY_FOR_DATA) ||
			 (R1_CURRENT_STATE(status) == R1_STATE_PRG));
	}

	/* if general error occurs, retry the write operation. */
	if (gen_err) {
		pr_warn("%s: retrying write for general error\n",
				req->rq_disk->disk_name);
		return MMC_BLK_RETRY;
	}

	if (brq->data.error) {
		pr_err("%s: error %d transferring data, sector %u, nr %u, cmd response %#x, card status %#x\n",
		       req->rq_disk->disk_name, brq->data.error,
		       (unsigned)blk_rq_pos(req),
		       (unsigned)blk_rq_sectors(req),
		       brq->cmd.resp[0], brq->stop.resp[0]);

		if (rq_data_dir(req) == READ) {
			if (ecc_err)
				return MMC_BLK_ECC_ERR;
			return MMC_BLK_DATA_ERR;
		} else {
			return MMC_BLK_CMD_ERR;
		}
	}

	if (!brq->data.bytes_xfered)
		return MMC_BLK_RETRY;

	if (mmc_packed_cmd(mq_mrq->cmd_type)) {
		if (unlikely(brq->data.blocks << 9 != brq->data.bytes_xfered))
			return MMC_BLK_PARTIAL;
		else
			return MMC_BLK_SUCCESS;
	}

	if (blk_rq_bytes(req) != brq->data.bytes_xfered)
		return MMC_BLK_PARTIAL;

	return MMC_BLK_SUCCESS;
}

static int mmc_blk_packed_err_check(struct mmc_card *card,
				    struct mmc_async_req *areq)
{
	struct mmc_queue_req *mq_rq = container_of(areq, struct mmc_queue_req,
			mmc_active);
	struct request *req = mq_rq->req;
	struct mmc_packed *packed = mq_rq->packed;
	int err, check, status;
	u8 *ext_csd;

	BUG_ON(!packed);

	packed->retries--;
	check = mmc_blk_err_check(card, areq);
	err = get_card_status(card, &status, 0);
	if (err) {
		pr_err("%s: error %d sending status command\n",
		       req->rq_disk->disk_name, err);
		return MMC_BLK_ABORT;
	}

	if (status & R1_EXCEPTION_EVENT) {
		ext_csd = kzalloc(512, GFP_KERNEL);
		if (!ext_csd) {
			pr_err("%s: unable to allocate buffer for ext_csd\n",
			       req->rq_disk->disk_name);
			return -ENOMEM;
		}

		err = mmc_send_ext_csd(card, ext_csd);
		if (err) {
			pr_err("%s: error %d sending ext_csd\n",
			       req->rq_disk->disk_name, err);
			check = MMC_BLK_ABORT;
			goto free;
		}

		if ((ext_csd[EXT_CSD_EXP_EVENTS_STATUS] &
		     EXT_CSD_PACKED_FAILURE) &&
		    (ext_csd[EXT_CSD_PACKED_CMD_STATUS] &
		     EXT_CSD_PACKED_GENERIC_ERROR)) {
			if (ext_csd[EXT_CSD_PACKED_CMD_STATUS] &
			    EXT_CSD_PACKED_INDEXED_ERROR) {
				packed->idx_failure =
				  ext_csd[EXT_CSD_PACKED_FAILURE_INDEX] - 1;
				check = MMC_BLK_PARTIAL;
			}
			pr_err("%s: packed cmd failed, nr %u, sectors %u, "
			       "failure index: %d\n",
			       req->rq_disk->disk_name, packed->nr_entries,
			       packed->blocks, packed->idx_failure);
		}
free:
		kfree(ext_csd);
	}

	return check;
}

static unsigned int tSD_prep_sector(struct request *req)
{
         struct gendisk * gd;
         unsigned int part_no;
         unsigned int start;
         unsigned int nsector;

         gd = req->rq_disk;
         part_no = gd->first_minor >> 3;
         start = blk_rq_pos(req);
         nsector = blk_rq_sectors(req);

		 //printk("%s gendisk[%d]  start:0x%x, nsector:0x%x ,get_capacity(gd) high:0x%x get_capacity(gd) low:0x%x!\n",__FUNCTION__,part_no, start, nsector, (unsigned int)(get_capacity(gd)>>32), (unsigned int)get_capacity(gd));
		 
         if(nsector <= (get_capacity(gd) - start))
         {
                start += tSD_device_md.partitions[part_no].offset; 
	       		start += SEC_PHY_BLOCK;
//	       UPGRADE_DBG_INF("start = %u, part_no = %d \n", start, part_no);
         }
         else
         {
                printk("%s err!gendisk[%d] over limit start:0x%x, nsector:0x%x , get_capacity(gd) high:0x%x get_capacity(gd) low:0x%x!\n",__FUNCTION__,part_no, start, nsector, (unsigned int)(get_capacity(gd)>>32), (unsigned int)get_capacity(gd));
				//return -1;
                //todo
         }       
         
         return start;
}

static void mmc_blk_rw_rq_prep(struct mmc_queue_req *mqrq,
			       struct mmc_card *card,
			       int disable_multi,
			       struct mmc_queue *mq)
{
	u32 readcmd, writecmd;
	struct mmc_blk_request *brq = &mqrq->brq;
	struct request *req = mqrq->req;
	struct mmc_blk_data *md = mq->data;
	bool do_data_tag;
	unsigned int start;

	/*
	 * Reliable writes are used to implement Forced Unit Access and
	 * REQ_META accesses, and are supported only on MMCs.
	 *
	 * XXX: this really needs a good explanation of why REQ_META
	 * is treated special.
	 */
	bool do_rel_wr = ((req->cmd_flags & REQ_FUA) ||
			  (req->cmd_flags & REQ_META)) &&
		(rq_data_dir(req) == WRITE) &&
		(md->flags & MMC_BLK_REL_WR);

	memset(brq, 0, sizeof(struct mmc_blk_request));
	brq->mrq.cmd = &brq->cmd;
	brq->mrq.data = &brq->data;

	//brq->cmd.arg = blk_rq_pos(req);
	/* add for mmc partion*/
	start = tSD_prep_sector(req);
        brq->cmd.arg = start;

	if (!mmc_card_blockaddr(card))
		brq->cmd.arg <<= 9;
	brq->cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	brq->data.blksz = 512;
	brq->stop.opcode = MMC_STOP_TRANSMISSION;
	brq->stop.arg = 0;
	brq->stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	brq->data.blocks = blk_rq_sectors(req);

	/*
	 * The block layer doesn't support all sector count
	 * restrictions, so we need to be prepared for too big
	 * requests.
	 */
	if (brq->data.blocks > card->host->max_blk_count)
		brq->data.blocks = card->host->max_blk_count;

	if (brq->data.blocks > 1) {
		/*
		 * After a read error, we redo the request one sector
		 * at a time in order to accurately determine which
		 * sectors can be read successfully.
		 */
		if (disable_multi)
			brq->data.blocks = 1;

		/* Some controllers can't do multiblock reads due to hw bugs */
		if (card->host->caps2 & MMC_CAP2_NO_MULTI_READ &&
		    rq_data_dir(req) == READ)
			brq->data.blocks = 1;
	}

	if (brq->data.blocks > 1 || do_rel_wr) {
		/* SPI multiblock writes terminate using a special
		 * token, not a STOP_TRANSMISSION request.
		 */
		if (!mmc_host_is_spi(card->host) ||
		    rq_data_dir(req) == READ)
			brq->mrq.stop = &brq->stop;
		readcmd = MMC_READ_MULTIPLE_BLOCK;
		writecmd = MMC_WRITE_MULTIPLE_BLOCK;
	} else {
		brq->mrq.stop = NULL;
		readcmd = MMC_READ_SINGLE_BLOCK;
		writecmd = MMC_WRITE_BLOCK;
	}
	if (rq_data_dir(req) == READ) {
		brq->cmd.opcode = readcmd;
		brq->data.flags |= MMC_DATA_READ;
	} else {
		brq->cmd.opcode = writecmd;
		brq->data.flags |= MMC_DATA_WRITE;
	}

	if (do_rel_wr)
		mmc_apply_rel_rw(brq, card, req);

	/*
	 * Data tag is used only during writing meta data to speed
	 * up write and any subsequent read of this meta data
	 */
	do_data_tag = (card->ext_csd.data_tag_unit_size) &&
		(req->cmd_flags & REQ_META) &&
		(rq_data_dir(req) == WRITE) &&
		((brq->data.blocks * brq->data.blksz) >=
		 card->ext_csd.data_tag_unit_size);

	/*
	 * Pre-defined multi-block transfers are preferable to
	 * open ended-ones (and necessary for reliable writes).
	 * However, it is not sufficient to just send CMD23,
	 * and avoid the final CMD12, as on an error condition
	 * CMD12 (stop) needs to be sent anyway. This, coupled
	 * with Auto-CMD23 enhancements provided by some
	 * hosts, means that the complexity of dealing
	 * with this is best left to the host. If CMD23 is
	 * supported by card and host, we'll fill sbc in and let
	 * the host deal with handling it correctly. This means
	 * that for hosts that don't expose MMC_CAP_CMD23, no
	 * change of behavior will be observed.
	 *
	 * N.B: Some MMC cards experience perf degradation.
	 * We'll avoid using CMD23-bounded multiblock writes for
	 * these, while retaining features like reliable writes.
	 */
	if ((md->flags & MMC_BLK_CMD23) && mmc_op_multi(brq->cmd.opcode) &&
	    (do_rel_wr || !(card->quirks & MMC_QUIRK_BLK_NO_CMD23) ||
	     do_data_tag)) {
		brq->sbc.opcode = MMC_SET_BLOCK_COUNT;
		brq->sbc.arg = brq->data.blocks |
			(do_rel_wr ? (1 << 31) : 0) |
			(do_data_tag ? (1 << 29) : 0);
		brq->sbc.flags = MMC_RSP_R1 | MMC_CMD_AC;
		brq->mrq.sbc = &brq->sbc;
	}

	mmc_set_data_timeout(&brq->data, card);

	brq->data.sg = mqrq->sg;
	brq->data.sg_len = mmc_queue_map_sg(mq, mqrq);

	/*
	 * Adjust the sg list so it is the same size as the
	 * request.
	 */
	if (brq->data.blocks != blk_rq_sectors(req)) {
		int i, data_size = brq->data.blocks << 9;
		struct scatterlist *sg;

		for_each_sg(brq->data.sg, sg, brq->data.sg_len, i) {
			data_size -= sg->length;
			if (data_size <= 0) {
				sg->length += data_size;
				i++;
				break;
			}
		}
		brq->data.sg_len = i;
	}

	mqrq->mmc_active.mrq = &brq->mrq;
	mqrq->mmc_active.err_check = mmc_blk_err_check;

	mmc_queue_bounce_pre(mqrq);
}

static inline u8 mmc_calc_packed_hdr_segs(struct request_queue *q,
					  struct mmc_card *card)
{
	unsigned int hdr_sz = mmc_large_sector(card) ? 4096 : 512;
	unsigned int max_seg_sz = queue_max_segment_size(q);
	unsigned int len, nr_segs = 0;

	do {
		len = min(hdr_sz, max_seg_sz);
		hdr_sz -= len;
		nr_segs++;
	} while (hdr_sz);

	return nr_segs;
}

static u8 mmc_blk_prep_packed_list(struct mmc_queue *mq, struct request *req)
{
	struct request_queue *q = mq->queue;
	struct mmc_card *card = mq->card;
	struct request *cur = req, *next = NULL;
	struct mmc_blk_data *md = mq->data;
	struct mmc_queue_req *mqrq = mq->mqrq_cur;
	bool en_rel_wr = card->ext_csd.rel_param & EXT_CSD_WR_REL_PARAM_EN;
	unsigned int req_sectors = 0, phys_segments = 0;
	unsigned int max_blk_count, max_phys_segs;
	bool put_back = true;
	u8 max_packed_rw = 0;
	u8 reqs = 0;

	if (!(md->flags & MMC_BLK_PACKED_CMD))
		goto no_packed;

	if ((rq_data_dir(cur) == WRITE) &&
	    mmc_host_packed_wr(card->host))
		max_packed_rw = card->ext_csd.max_packed_writes;

	if (max_packed_rw == 0)
		goto no_packed;

	if (mmc_req_rel_wr(cur) &&
	    (md->flags & MMC_BLK_REL_WR) && !en_rel_wr)
		goto no_packed;

	if (mmc_large_sector(card) &&
	    !IS_ALIGNED(blk_rq_sectors(cur), 8))
		goto no_packed;

	mmc_blk_clear_packed(mqrq);

	max_blk_count = min(card->host->max_blk_count,
			    card->host->max_req_size >> 9);
	if (unlikely(max_blk_count > 0xffff))
		max_blk_count = 0xffff;

	max_phys_segs = queue_max_segments(q);
	req_sectors += blk_rq_sectors(cur);
	phys_segments += cur->nr_phys_segments;

	if (rq_data_dir(cur) == WRITE) {
		req_sectors += mmc_large_sector(card) ? 8 : 1;
		phys_segments += mmc_calc_packed_hdr_segs(q, card);
	}

	do {
		if (reqs >= max_packed_rw - 1) {
			put_back = false;
			break;
		}

		spin_lock_irq(q->queue_lock);
		next = blk_fetch_request(q);
		spin_unlock_irq(q->queue_lock);
		if (!next) {
			put_back = false;
			break;
		}

		if (mmc_large_sector(card) &&
		    !IS_ALIGNED(blk_rq_sectors(next), 8))
			break;

		if (next->cmd_flags & REQ_DISCARD ||
		    next->cmd_flags & REQ_FLUSH)
			break;

		if (rq_data_dir(cur) != rq_data_dir(next))
			break;

		if (mmc_req_rel_wr(next) &&
		    (md->flags & MMC_BLK_REL_WR) && !en_rel_wr)
			break;

		req_sectors += blk_rq_sectors(next);
		if (req_sectors > max_blk_count)
			break;

		phys_segments +=  next->nr_phys_segments;
		if (phys_segments > max_phys_segs)
			break;

		list_add_tail(&next->queuelist, &mqrq->packed->list);
		cur = next;
		reqs++;
	} while (1);

	if (put_back) {
		spin_lock_irq(q->queue_lock);
		blk_requeue_request(q, next);
		spin_unlock_irq(q->queue_lock);
	}

	if (reqs > 0) {
		list_add(&req->queuelist, &mqrq->packed->list);
		mqrq->packed->nr_entries = ++reqs;
		mqrq->packed->retries = reqs;
		return reqs;
	}

no_packed:
	mqrq->cmd_type = MMC_PACKED_NONE;
	return 0;
}

static void mmc_blk_packed_hdr_wrq_prep(struct mmc_queue_req *mqrq,
					struct mmc_card *card,
					struct mmc_queue *mq)
{
	struct mmc_blk_request *brq = &mqrq->brq;
	struct request *req = mqrq->req;
	struct request *prq;
	struct mmc_blk_data *md = mq->data;
	struct mmc_packed *packed = mqrq->packed;
	bool do_rel_wr, do_data_tag;
	u32 *packed_cmd_hdr;
	u8 hdr_blocks;
	u8 i = 1;

	BUG_ON(!packed);

	mqrq->cmd_type = MMC_PACKED_WRITE;
	packed->blocks = 0;
	packed->idx_failure = MMC_PACKED_NR_IDX;

	packed_cmd_hdr = packed->cmd_hdr;
	memset(packed_cmd_hdr, 0, sizeof(packed->cmd_hdr));
	packed_cmd_hdr[0] = (packed->nr_entries << 16) |
		(PACKED_CMD_WR << 8) | PACKED_CMD_VER;
	hdr_blocks = mmc_large_sector(card) ? 8 : 1;

	/*
	 * Argument for each entry of packed group
	 */
	list_for_each_entry(prq, &packed->list, queuelist) {
		do_rel_wr = mmc_req_rel_wr(prq) && (md->flags & MMC_BLK_REL_WR);
		do_data_tag = (card->ext_csd.data_tag_unit_size) &&
			(prq->cmd_flags & REQ_META) &&
			(rq_data_dir(prq) == WRITE) &&
			((brq->data.blocks * brq->data.blksz) >=
			 card->ext_csd.data_tag_unit_size);
		/* Argument of CMD23 */
		packed_cmd_hdr[(i * 2)] =
			(do_rel_wr ? MMC_CMD23_ARG_REL_WR : 0) |
			(do_data_tag ? MMC_CMD23_ARG_TAG_REQ : 0) |
			blk_rq_sectors(prq);
		/* Argument of CMD18 or CMD25 */
		packed_cmd_hdr[((i * 2)) + 1] =
			mmc_card_blockaddr(card) ?
			blk_rq_pos(prq) : blk_rq_pos(prq) << 9;
		packed->blocks += blk_rq_sectors(prq);
		i++;
	}

	memset(brq, 0, sizeof(struct mmc_blk_request));
	brq->mrq.cmd = &brq->cmd;
	brq->mrq.data = &brq->data;
	brq->mrq.sbc = &brq->sbc;
	brq->mrq.stop = &brq->stop;

	brq->sbc.opcode = MMC_SET_BLOCK_COUNT;
	brq->sbc.arg = MMC_CMD23_ARG_PACKED | (packed->blocks + hdr_blocks);
	brq->sbc.flags = MMC_RSP_R1 | MMC_CMD_AC;

	brq->cmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
	brq->cmd.arg = blk_rq_pos(req);
	if (!mmc_card_blockaddr(card))
		brq->cmd.arg <<= 9;
	brq->cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	brq->data.blksz = 512;
	brq->data.blocks = packed->blocks + hdr_blocks;
	brq->data.flags |= MMC_DATA_WRITE;

	brq->stop.opcode = MMC_STOP_TRANSMISSION;
	brq->stop.arg = 0;
	brq->stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;

	mmc_set_data_timeout(&brq->data, card);

	brq->data.sg = mqrq->sg;
	brq->data.sg_len = mmc_queue_map_sg(mq, mqrq);

	mqrq->mmc_active.mrq = &brq->mrq;
	mqrq->mmc_active.err_check = mmc_blk_packed_err_check;

	mmc_queue_bounce_pre(mqrq);
}

static int mmc_blk_cmd_err(struct mmc_blk_data *md, struct mmc_card *card,
			   struct mmc_blk_request *brq, struct request *req,
			   int ret)
{
	struct mmc_queue_req *mq_rq;
	mq_rq = container_of(brq, struct mmc_queue_req, brq);

	/*
	 * If this is an SD card and we're writing, we can first
	 * mark the known good sectors as ok.
	 *
	 * If the card is not SD, we can still ok written sectors
	 * as reported by the controller (which might be less than
	 * the real number of written sectors, but never more).
	 */
	if (mmc_card_sd(card)) {
		u32 blocks;

		blocks = mmc_sd_num_wr_blocks(card);
		if (blocks != (u32)-1) {
			ret = blk_end_request(req, 0, blocks << 9);
		}
	} else {
		if (!mmc_packed_cmd(mq_rq->cmd_type))
			ret = blk_end_request(req, 0, brq->data.bytes_xfered);
	}
	return ret;
}

static int mmc_blk_end_packed_req(struct mmc_queue_req *mq_rq)
{
	struct request *prq;
	struct mmc_packed *packed = mq_rq->packed;
	int idx = packed->idx_failure, i = 0;
	int ret = 0;

	BUG_ON(!packed);

	while (!list_empty(&packed->list)) {
		prq = list_entry_rq(packed->list.next);
		if (idx == i) {
			/* retry from error index */
			packed->nr_entries -= idx;
			mq_rq->req = prq;
			ret = 1;

			if (packed->nr_entries == MMC_PACKED_NR_SINGLE) {
				list_del_init(&prq->queuelist);
				mmc_blk_clear_packed(mq_rq);
			}
			return ret;
		}
		list_del_init(&prq->queuelist);
		blk_end_request(prq, 0, blk_rq_bytes(prq));
		i++;
	}

	mmc_blk_clear_packed(mq_rq);
	return ret;
}

static void mmc_blk_abort_packed_req(struct mmc_queue_req *mq_rq)
{
	struct request *prq;
	struct mmc_packed *packed = mq_rq->packed;

	BUG_ON(!packed);

	while (!list_empty(&packed->list)) {
		prq = list_entry_rq(packed->list.next);
		list_del_init(&prq->queuelist);
		blk_end_request(prq, -EIO, blk_rq_bytes(prq));
	}

	mmc_blk_clear_packed(mq_rq);
}

static void mmc_blk_revert_packed_req(struct mmc_queue *mq,
				      struct mmc_queue_req *mq_rq)
{
	struct request *prq;
	struct request_queue *q = mq->queue;
	struct mmc_packed *packed = mq_rq->packed;

	BUG_ON(!packed);

	while (!list_empty(&packed->list)) {
		prq = list_entry_rq(packed->list.prev);
		if (prq->queuelist.prev != &packed->list) {
			list_del_init(&prq->queuelist);
			spin_lock_irq(q->queue_lock);
			blk_requeue_request(mq->queue, prq);
			spin_unlock_irq(q->queue_lock);
		} else {
			list_del_init(&prq->queuelist);
		}
	}

	mmc_blk_clear_packed(mq_rq);
}

static int mmc_blk_issue_rw_rq(struct mmc_queue *mq, struct request *rqc)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue->card;
	struct mmc_blk_request *brq = &mq->mqrq_cur->brq;
	int ret = 1, disable_multi = 0, retry = 0, type;
	enum mmc_blk_status status;
	struct mmc_queue_req *mq_rq;
	struct request *req = rqc;
	struct mmc_async_req *areq;
	const u8 packed_nr = 2;
	u8 reqs = 0;

	if (!rqc && !mq->mqrq_prev->req)
		return 0;

	if (rqc)
		reqs = mmc_blk_prep_packed_list(mq, rqc);

	do {
		if (rqc) {
			/*
			 * When 4KB native sector is enabled, only 8 blocks
			 * multiple read or write is allowed
			 */
			if ((brq->data.blocks & 0x07) &&
			    (card->ext_csd.data_sector_size == 4096)) {
				pr_err("%s: Transfer size is not 4KB sector size aligned\n",
					req->rq_disk->disk_name);
				mq_rq = mq->mqrq_cur;
				goto cmd_abort;
			}

			if (reqs >= packed_nr)
				mmc_blk_packed_hdr_wrq_prep(mq->mqrq_cur,
							    card, mq);
			else
				mmc_blk_rw_rq_prep(mq->mqrq_cur, card, 0, mq);
			areq = &mq->mqrq_cur->mmc_active;
		} else
			areq = NULL;
		areq = mmc_start_req(card->host, areq, (int *) &status);
		if (!areq) {
			if (status == MMC_BLK_NEW_REQUEST)
				mq->flags |= MMC_QUEUE_NEW_REQUEST;
			return 0;
		}

		mq_rq = container_of(areq, struct mmc_queue_req, mmc_active);
		brq = &mq_rq->brq;
		req = mq_rq->req;
		type = rq_data_dir(req) == READ ? MMC_BLK_READ : MMC_BLK_WRITE;
		mmc_queue_bounce_post(mq_rq);

		switch (status) {
		case MMC_BLK_SUCCESS:
		case MMC_BLK_PARTIAL:
			/*
			 * A block was successfully transferred.
			 */
			mmc_blk_reset_success(md, type);

			if (mmc_packed_cmd(mq_rq->cmd_type)) {
				ret = mmc_blk_end_packed_req(mq_rq);
				break;
			} else {
				ret = blk_end_request(req, 0,
						brq->data.bytes_xfered);
			}

			/*
			 * If the blk_end_request function returns non-zero even
			 * though all data has been transferred and no errors
			 * were returned by the host controller, it's a bug.
			 */
			if (status == MMC_BLK_SUCCESS && ret) {
				pr_err("%s BUG rq_tot %d d_xfer %d\n",
				       __func__, blk_rq_bytes(req),
				       brq->data.bytes_xfered);
				rqc = NULL;
				goto cmd_abort;
			}
			break;
		case MMC_BLK_CMD_ERR:
			ret = mmc_blk_cmd_err(md, card, brq, req, ret);
			if (!mmc_blk_reset(md, card->host, type))
				break;
			goto cmd_abort;
		case MMC_BLK_RETRY:
			if (retry++ < 5)
				break;
			/* Fall through */
		case MMC_BLK_ABORT:
			if (!mmc_blk_reset(md, card->host, type))
				break;
			goto cmd_abort;
		case MMC_BLK_DATA_ERR: {
			int err;

			err = mmc_blk_reset(md, card->host, type);
			if (!err)
				break;
			if (err == -ENODEV ||
				mmc_packed_cmd(mq_rq->cmd_type))
				goto cmd_abort;
			/* Fall through */
		}
		case MMC_BLK_ECC_ERR:
			if (brq->data.blocks > 1) {
				/* Redo read one sector at a time */
				pr_warning("%s: retrying using single block read\n",
					   req->rq_disk->disk_name);
				disable_multi = 1;
				break;
			}
			/*
			 * After an error, we redo I/O one sector at a
			 * time, so we only reach here after trying to
			 * read a single sector.
			 */
			ret = blk_end_request(req, -EIO,
						brq->data.blksz);
			if (!ret)
				goto start_new_req;
			break;
		case MMC_BLK_NOMEDIUM:
			goto cmd_abort;
		default:
			pr_err("%s: Unhandled return value (%d)",
					req->rq_disk->disk_name, status);
			goto cmd_abort;
		}

		if (ret) {
			if (mmc_packed_cmd(mq_rq->cmd_type)) {
				if (!mq_rq->packed->retries)
					goto cmd_abort;
				mmc_blk_packed_hdr_wrq_prep(mq_rq, card, mq);
				mmc_start_req(card->host,
					      &mq_rq->mmc_active, NULL);
			} else {

				/*
				 * In case of a incomplete request
				 * prepare it again and resend.
				 */
				mmc_blk_rw_rq_prep(mq_rq, card,
						disable_multi, mq);
				mmc_start_req(card->host,
						&mq_rq->mmc_active, NULL);
			}
		}
	} while (ret);

	return 1;

 cmd_abort:
	if (mmc_packed_cmd(mq_rq->cmd_type)) {
		mmc_blk_abort_packed_req(mq_rq);
	} else {
		if (mmc_card_removed(card))
			req->cmd_flags |= REQ_QUIET;
		while (ret)
			ret = blk_end_request(req, -EIO,
					blk_rq_cur_bytes(req));
	}

 start_new_req:
	if (rqc) {
		if (mmc_card_removed(card)) {
			rqc->cmd_flags |= REQ_QUIET;
			blk_end_request_all(rqc, -EIO);
		} else {
			/*
			 * If current request is packed, it needs to put back.
			 */
			if (mmc_packed_cmd(mq->mqrq_cur->cmd_type))
				mmc_blk_revert_packed_req(mq, mq->mqrq_cur);

			mmc_blk_rw_rq_prep(mq->mqrq_cur, card, 0, mq);
			mmc_start_req(card->host,
				      &mq->mqrq_cur->mmc_active, NULL);
		}
	}

	return 0;
}

static int mmc_blk_issue_rq(struct mmc_queue *mq, struct request *req)
{
	int ret;
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue->card;
	struct mmc_host *host = card->host;
	unsigned long flags;
	unsigned int cmd_flags = req ? req->cmd_flags : 0;

#if 0
#ifdef CONFIG_MMC_BLOCK_DEFERRED_RESUME
	if (mmc_bus_needs_resume(card->host))
		mmc_resume_bus(card->host);
#endif

	if (req && !mq->mqrq_prev->req)
		/* claim host only for the first request */
		mmc_claim_host(card->host);

	ret = mmc_blk_part_switch(card, md);
	if (ret) {
		if (req) {
			blk_end_request_all(req, -EIO);
		}
		ret = 0;
		goto out;
	}

#endif
	mmc_claim_host(card->host);
	mq->flags &= ~MMC_QUEUE_NEW_REQUEST;
	if (cmd_flags & REQ_DISCARD) {
		/* complete ongoing async transfer before issuing discard */
		if (card->host->areq)
			mmc_blk_issue_rw_rq(mq, NULL);
		if (req->cmd_flags & REQ_SECURE &&
			!(card->quirks & MMC_QUIRK_SEC_ERASE_TRIM_BROKEN))
			ret = mmc_blk_issue_secdiscard_rq(mq, req);
		else
			ret = mmc_blk_issue_discard_rq(mq, req);
	} else if (cmd_flags & REQ_FLUSH) {
		/* complete ongoing async transfer before issuing flush */
		if (card->host->areq)
			mmc_blk_issue_rw_rq(mq, NULL);
		ret = mmc_blk_issue_flush(mq, req);
	} else {
		if (!req && host->areq) {
			spin_lock_irqsave(&host->context_info.lock, flags);
			host->context_info.is_waiting_last_req = true;
			spin_unlock_irqrestore(&host->context_info.lock, flags);
		}
		ret = mmc_blk_issue_rw_rq(mq, req);
	}

//out:
	//if ((!req && !(mq->flags & MMC_QUEUE_NEW_REQUEST)) ||
	    // (cmd_flags & MMC_REQ_SPECIAL_MASK))
		/*
		 * Release host when there are no more requests
		 * and after special request(discard, flush) is done.
		 * In case sepecial request, there is no reentry to
		 * the 'mmc_blk_issue_rq' with 'mqrq_prev->req'.
		 */
		mmc_release_host(card->host);
	return ret;
}

static inline int mmc_blk_readonly(struct mmc_card *card)
{
	return mmc_card_readonly(card) ||
	       !(card->csd.cmdclass & CCC_BLOCK_WRITE);
}

#if 0
static struct mmc_blk_data *mmc_blk_alloc_req(struct mmc_card *card,
					      struct device *parent,
					      sector_t size,
					      bool default_ro,
					      const char *subname,
					      int area_type)
{
	struct mmc_blk_data *md;
	int devidx, ret;

	devidx = find_first_zero_bit(dev_use, max_devices);
	if (devidx >= max_devices)
		return ERR_PTR(-ENOSPC);
	__set_bit(devidx, dev_use);

	md = kzalloc(sizeof(struct mmc_blk_data), GFP_KERNEL);
	if (!md) {
		ret = -ENOMEM;
		goto out;
	}

	/*
	 * !subname implies we are creating main mmc_blk_data that will be
	 * associated with mmc_card with mmc_set_drvdata. Due to device
	 * partitions, devidx will not coincide with a per-physical card
	 * index anymore so we keep track of a name index.
	 */
	if (!subname) {
		md->name_idx = find_first_zero_bit(name_use, max_devices);
		__set_bit(md->name_idx, name_use);
	} else
		md->name_idx = ((struct mmc_blk_data *)
				dev_to_disk(parent)->private_data)->name_idx;

	md->area_type = area_type;

	/*
	 * Set the read-only status based on the supported commands
	 * and the write protect switch.
	 */
	md->read_only = mmc_blk_readonly(card);

	md->disk = alloc_disk(perdev_minors);
	if (md->disk == NULL) {
		ret = -ENOMEM;
		goto err_kfree;
	}

	spin_lock_init(&md->lock);
	INIT_LIST_HEAD(&md->part);
	md->usage = 1;

	ret = mmc_init_queue(&md->queue, card, &md->lock, subname);
	if (ret)
		goto err_putdisk;

	md->queue.issue_fn = mmc_blk_issue_rq;
	md->queue.data = md;

	md->disk->major	= MMC_BLOCK_MAJOR;
	md->disk->first_minor = devidx * perdev_minors;
	md->disk->fops = &mmc_bdops;
	md->disk->private_data = md;
	md->disk->queue = md->queue.queue;
	md->disk->driverfs_dev = parent;
	set_disk_ro(md->disk, md->read_only || default_ro);
	md->disk->flags = GENHD_FL_EXT_DEVT;
	if (area_type & MMC_BLK_DATA_AREA_RPMB)
		md->disk->flags |= GENHD_FL_NO_PART_SCAN;

	/*
	 * As discussed on lkml, GENHD_FL_REMOVABLE should:
	 *
	 * - be set for removable media with permanent block devices
	 * - be unset for removable block devices with permanent media
	 *
	 * Since MMC block devices clearly fall under the second
	 * case, we do not set GENHD_FL_REMOVABLE.  Userspace
	 * should use the block device creation/destruction hotplug
	 * messages to tell when the card is present.
	 */

	snprintf(md->disk->disk_name, sizeof(md->disk->disk_name),
		 "mmcblk%d%s", md->name_idx, subname ? subname : "");

	if (mmc_card_mmc(card))
		blk_queue_logical_block_size(md->queue.queue,
					     card->ext_csd.data_sector_size);
	else
		blk_queue_logical_block_size(md->queue.queue, 512);

	set_capacity(md->disk, size);

	if (mmc_host_cmd23(card->host)) {
		if (mmc_card_mmc(card) ||
		    (mmc_card_sd(card) &&
		     card->scr.cmds & SD_SCR_CMD23_SUPPORT))
			md->flags |= MMC_BLK_CMD23;
	}

	if (mmc_card_mmc(card) &&
	    md->flags & MMC_BLK_CMD23 &&
	    ((card->ext_csd.rel_param & EXT_CSD_WR_REL_PARAM_EN) ||
	     card->ext_csd.rel_sectors)) {
		md->flags |= MMC_BLK_REL_WR;
		blk_queue_flush(md->queue.queue, REQ_FLUSH | REQ_FUA);
	}

	if (mmc_card_mmc(card) &&
	    (area_type == MMC_BLK_DATA_AREA_MAIN) &&
	    (md->flags & MMC_BLK_CMD23) &&
	    card->ext_csd.packed_event_en) {
		if (!mmc_packed_init(&md->queue, card))
			md->flags |= MMC_BLK_PACKED_CMD;
	}

	return md;

 err_putdisk:
	put_disk(md->disk);
 err_kfree:
	kfree(md);
 out:
	return ERR_PTR(ret);
}


static struct mmc_blk_data *mmc_blk_alloc(struct mmc_card *card)
{
	sector_t size;
	struct mmc_blk_data *md;

	if (!mmc_card_sd(card) && mmc_card_blockaddr(card)) {
		/*
		 * The EXT_CSD sector count is in number or 512 byte
		 * sectors.
		 */
		size = card->ext_csd.sectors;
	} else {
		/*
		 * The CSD capacity field is in units of read_blkbits.
		 * set_capacity takes units of 512 bytes.
		 */
		size = card->csd.capacity << (card->csd.read_blkbits - 9);
	}

	md = mmc_blk_alloc_req(card, &card->dev, size, false, NULL,
					MMC_BLK_DATA_AREA_MAIN);
	return md;
}

static int mmc_blk_alloc_part(struct mmc_card *card,
			      struct mmc_blk_data *md,
			      unsigned int part_type,
			      sector_t size,
			      bool default_ro,
			      const char *subname,
			      int area_type)
{
	char cap_str[10];
	struct mmc_blk_data *part_md;

	part_md = mmc_blk_alloc_req(card, disk_to_dev(md->disk), size, default_ro,
				    subname, area_type);
	if (IS_ERR(part_md))
		return PTR_ERR(part_md);
	part_md->part_type = part_type;
	list_add(&part_md->part, &md->part);

	string_get_size((u64)get_capacity(part_md->disk) << 9, STRING_UNITS_2,
			cap_str, sizeof(cap_str));
	pr_info("%s: %s %s partition %u %s\n",
	       part_md->disk->disk_name, mmc_card_id(card),
	       mmc_card_name(card), part_md->part_type, cap_str);
	return 0;
}

/* MMC Physical partitions consist of two boot partitions and
 * up to four general purpose partitions.
 * For each partition enabled in EXT_CSD a block device will be allocatedi
 * to provide access to the partition.
 */

static int mmc_blk_alloc_parts(struct mmc_card *card, struct mmc_blk_data *md)
{
	int idx, ret = 0;

	if (!mmc_card_mmc(card))
		return 0;

	for (idx = 0; idx < card->nr_parts; idx++) {
		if (card->part[idx].size) {
			ret = mmc_blk_alloc_part(card, md,
				card->part[idx].part_cfg,
				card->part[idx].size >> 9,
				card->part[idx].force_ro,
				card->part[idx].name,
				card->part[idx].area_type);
			if (ret)
				return ret;
		}
	}

	return ret;
}

static void mmc_blk_remove_req(struct mmc_blk_data *md)
{
	struct mmc_card *card;

	if (md) {
		card = md->queue.card;
		if (md->disk->flags & GENHD_FL_UP) {
			device_remove_file(disk_to_dev(md->disk), &md->force_ro);
			if ((md->area_type & MMC_BLK_DATA_AREA_BOOT) &&
					card->ext_csd.boot_ro_lockable)
				device_remove_file(disk_to_dev(md->disk),
					&md->power_ro_lock);

			/* Stop new requests from getting into the queue */
			del_gendisk(md->disk);
		}

		/* Then flush out any already in there */
		mmc_cleanup_queue(&md->queue);
		if (md->flags & MMC_BLK_PACKED_CMD)
			mmc_packed_clean(&md->queue);
		mmc_blk_put(md);
	}
}



static void mmc_blk_remove_parts(struct mmc_card *card,
				 struct mmc_blk_data *md)
{
	struct list_head *pos, *q;
	struct mmc_blk_data *part_md;

	__clear_bit(md->name_idx, name_use);
	list_for_each_safe(pos, q, &md->part) {
		part_md = list_entry(pos, struct mmc_blk_data, part);
		list_del(pos);
		mmc_blk_remove_req(part_md);
	}
}

#endif
#if 0
static void tSD_blk_remove_req(struct mmc_blk_data *tSD_device)
{
	int i;
	if(tSD_device)
	{
		for(i = 0; i < partition__logic_incard; i++)
		{
            if( tSD_device->partitions[i].disk != NULL  && tSD_device->partitions[i].disk->flags & GENHD_FL_UP){
				/* Stop new requests from getting into the queue */
				del_gendisk(tSD_device->partitions[i].disk);

			}
		}
		/* Then terminate our worker thread */
		kthread_stop(tSD_device->thread);
		/* Then flush out any already in there */
		tSD_cleanup_queue(&tSD_device->squeue);
		tSD_cleanup_queue(&tSD_device->dqueue);
		tSD_cleanup_queue(&tSD_device->uqueue);

		tSD_blk_put(tSD_device);
	}
}
#endif
#if 0
static int mmc_add_disk(struct mmc_blk_data *md)
{
	int ret;
	struct mmc_card *card = md->queue.card;

	add_disk(md->disk);
	md->force_ro.show = force_ro_show;
	md->force_ro.store = force_ro_store;
	sysfs_attr_init(&md->force_ro.attr);
	md->force_ro.attr.name = "force_ro";
	md->force_ro.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(disk_to_dev(md->disk), &md->force_ro);
	if (ret)
		goto force_ro_fail;

	if ((md->area_type & MMC_BLK_DATA_AREA_BOOT) &&
	     card->ext_csd.boot_ro_lockable) {
		umode_t mode;

		if (card->ext_csd.boot_ro_lock & EXT_CSD_BOOT_WP_B_PWR_WP_DIS)
			mode = S_IRUGO;
		else
			mode = S_IRUGO | S_IWUSR;

		md->power_ro_lock.show = power_ro_lock_show;
		md->power_ro_lock.store = power_ro_lock_store;
		sysfs_attr_init(&md->power_ro_lock.attr);
		md->power_ro_lock.attr.mode = mode;
		md->power_ro_lock.attr.name =
					"ro_lock_until_next_power_on";
		ret = device_create_file(disk_to_dev(md->disk),
				&md->power_ro_lock);
		if (ret)
			goto power_ro_lock_fail;
	}
	return ret;

power_ro_lock_fail:
	device_remove_file(disk_to_dev(md->disk), &md->force_ro);
force_ro_fail:
	del_gendisk(md->disk);

	return ret;
}
#endif

#define CID_MANFID_SANDISK	0x2
#define CID_MANFID_TOSHIBA	0x11
#define CID_MANFID_MICRON	0x13
#define CID_MANFID_SAMSUNG	0x15

static const struct mmc_fixup blk_fixups[] =
{
	MMC_FIXUP("SEM02G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM04G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM08G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM16G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM32G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),

	/*
	 * Some MMC cards experience performance degradation with CMD23
	 * instead of CMD12-bounded multiblock transfers. For now we'll
	 * black list what's bad...
	 * - Certain Toshiba cards.
	 *
	 * N.B. This doesn't affect SD cards.
	 */
	MMC_FIXUP("MMC08G", CID_MANFID_TOSHIBA, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),
	MMC_FIXUP("MMC16G", CID_MANFID_TOSHIBA, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),
	MMC_FIXUP("MMC32G", CID_MANFID_TOSHIBA, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),

	/*
	 * Some Micron MMC cards needs longer data read timeout than
	 * indicated in CSD.
	 */
	MMC_FIXUP(CID_NAME_ANY, CID_MANFID_MICRON, 0x200, add_quirk_mmc,
		  MMC_QUIRK_LONG_READ_TIME),

	/*
	 * On these Samsung MoviNAND parts, performing secure erase or
	 * secure trim can result in unrecoverable corruption due to a
	 * firmware bug.
	 */
	MMC_FIXUP("M8G2FA", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("MAG4FA", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("MBG8FA", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("MCGAFA", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("VAL00M", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("VYL00M", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("KYL00M", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("VZL00M", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),

	END_FIXUP
};

static void mmc_flush(void)
{
    
}

int init_card_proc(void);



static int mmc_blk_probe(struct mmc_card *card)
{
	int i;
	int ret;
	unsigned int size;
	unsigned int mbr_num;
	
	if (!mmc_card_sd(card) && mmc_card_blockaddr(card)) {
		/*
		 * The EXT_CSD sector count is in number or 512 byte
		 * sectors.
		 */
		size = card->ext_csd.sectors;
	} else {
		/*
		 * The CSD capacity field is in units of read_blkbits.
		 * set_capacity takes units of 512 bytes.
		 */
		size = card->csd.capacity << (card->csd.read_blkbits - 9);
	}

	UPGRADE_DBG_INF("card->csd.read_blkbits = %u, card total size = %u sectors\n", card->csd.read_blkbits, size);
	card_total_size = size;
	tSD_card = card;
	
	mmc_fixup_device(card, blk_fixups);
	
	size = tSD_get_logic_cat();
	printk("logic_size %u: phy_size %u\n", size,card_total_size);

	ret = calculate_part_num();
	if(ret == -1){
		printk("%s,%d,get partition error!\n", __FUNCTION__, __LINE__);
		return 0;
	}else{
		printk("%s,%d, partition__logic_incard:%d\n", __FUNCTION__, __LINE__, ret);
	}

	ret = get_afi_configuration();             
	if(ret){
		printk("%s err!\n", __FUNCTION__);
	}

	if(is_for_upgrade)
	{
		AdfuUpdateMbrFromPhyToUsr = UpdateMbrFromPhyToUsr;
		adfu_flush_nand_cache = mmc_flush;
		adfus_nand_read = tSD_adfu_read;
		adfus_nand_write = tSD_adfu_write;
	}

	ret = tSD_queue_init(&tSD_device_md);
	if(ret){
		UPGRADE_DBG_INF("%s err !\n",__FUNCTION__);
	}

	 ret = init_tSD_part_myself();
	 
	 for(i = 0; i < partition__logic_incard; i++)
	{
		ret = tSD_partition_init(&tSD_device_md, i);
		if(ret){
			printk("%s err!,need to check\n", __FUNCTION__);
			break;
		}
	}

	
	if(is_force_format == 1)
	{
		struct uparam adfu_uparam;
		char *buf=NULL;
		buf = kmalloc(FORMATBYTE, GFP_KERNEL);
		if( NULL == buf){
			printk("Err:%s:malloc buf fail\n",__FUNCTION__);
			return -1;
		}
		memset(buf, 0xff, FORMATBYTE);
		//erase 4K boot phy partion head , total 4 mbrc 
		for(mbr_num = 0; mbr_num < MBRC_NUM; mbr_num++){
			__do_adfu_write(mbr_num*BOOT_PHY_SIZE/PSECTBYTE, \
							FORMATBYTE/PSECTBYTE, buf);	
		}
		//erase 4K for partion head
		for (i = 0; i < partition__logic_incard; i++){
			adfu_uparam.flash_partition = i;
			if(tSD_part[i].partsize > 0){
				
				tSD_adfu_write(0, FORMATBYTE/PSECTBYTE, buf, &adfu_uparam);			
			}
	    }
		kfree(buf);
	}
	
	NAND_InitMiscInfo();
	
	return 0;
}


#if 0
static void mmc_blk_remove(struct mmc_card *card)
{
	struct mmc_blk_data *md = mmc_get_drvdata(card);

	mmc_blk_remove_parts(card, md);
	mmc_claim_host(card->host);
	mmc_blk_part_switch(card, md);
	mmc_release_host(card->host);
	mmc_blk_remove_req(md);
	mmc_set_drvdata(card, NULL);
#ifdef CONFIG_MMC_BLOCK_DEFERRED_RESUME
	mmc_set_bus_resume_policy(card->host, 0);
#endif
}
#endif

static void tSD_blk_remove(struct mmc_card *card)
{
	return ;
//	tSD_blk_remove_req(&tSD_device_md);
}

#if 0
#ifdef CONFIG_PM
static int mmc_blk_suspend(struct mmc_card *card)
{
	struct mmc_blk_data *part_md;
	struct mmc_blk_data *md = mmc_get_drvdata(card);

	if (md) {
		mmc_queue_suspend(&md->queue);
		list_for_each_entry(part_md, &md->part, part) {
			mmc_queue_suspend(&part_md->queue);
		}
	}
	return 0;
}

static int mmc_blk_resume(struct mmc_card *card)
{
	struct mmc_blk_data *part_md;
	struct mmc_blk_data *md = mmc_get_drvdata(card);

	if (md) {
		/*
		 * Resume involves the card going into idle state,
		 * so current partition is always the main one.
		 */
		md->part_curr = md->part_type;
		mmc_queue_resume(&md->queue);
		list_for_each_entry(part_md, &md->part, part) {
			mmc_queue_resume(&part_md->queue);
		}
	}
	return 0;
}
#else
#define	mmc_blk_suspend	NULL
#define mmc_blk_resume	NULL
#endif

#endif

static struct mmc_driver mmc_driver = {
	.drv		= {
		.name	= "tsd_card",
	},
	.probe		= mmc_blk_probe,
	.remove		= tSD_blk_remove,
//	.remove		= mmc_blk_remove,
//	.suspend	= mmc_blk_suspend,
//	.resume		= mmc_blk_resume,
	//tsd card don't need to suspend
	.suspend	= NULL,
	.resume		= NULL,
};


static int __init mmc_blk_init(void)
{
	int res;
	unsigned boot_dev=0;
	
	if(owl_set_carddev_match_name()!=0){
		printk("err:%s:tsd set name err\n",__FUNCTION__);	
		res = -1;
		goto out;
	}
	if (perdev_minors != CONFIG_MMC_BLOCK_MINORS)
		pr_info("mmcblk: using %d minors per device\n", perdev_minors);

	max_devices = 256 / perdev_minors;

	if(!card_to_card){
		res = init_card_proc();
    	if(res){
			printk("%s:init_card_proc err\n",__FUNCTION__);
			goto out;
		}
	}
	
	if(card_to_card){
		mmc_driver.drv.name = "card_to_card";
		tsd_major = 94;
		blkdev_name = "card_to_card";	
	}

	res = register_blkdev(tsd_major,blkdev_name);
	if (res)
		goto out;

	res = mmc_register_driver(&mmc_driver);
	if (res)
		goto out2;

	//TODO: register hdcp key get funtion here.
	 boot_dev = owl_get_boot_dev();
     	printk("%s: bootdev 0x%x\n", __FUNCTION__, boot_dev);
     	//only sd->nand ,will not use owl_register_secure_storage
     	if(boot_dev != OWL_BOOTDEV_SD02NAND ) {
			if(0 !=owl_register_secure_storage(&card_secure))
				printk("%s:owl_register_secure_storage fail\n ",__FUNCTION__);
	 }

	return 0;
 out2:
	unregister_blkdev(tsd_major, blkdev_name);
 out:
	return res;
}

static void __exit mmc_blk_exit(void)
{
	owl_unregister_secure_storage(&card_secure);
	/*
	* free the ram space
	*/
	if(capinfo != NULL)
		kfree(capinfo);
	if(tSD_part != NULL)
		kfree(tSD_part);
	if(tSD_device_md.partitions != NULL)
		kfree(tSD_device_md.partitions);

	mmc_unregister_driver(&mmc_driver);
	unregister_blkdev(tsd_major,blkdev_name);
}


/*
*  function for tSD
*/

int tSD_pre_data_transfer(unsigned start, unsigned nsector, void *buf, unsigned blksz, int write)
{
	int  cur_sects;
	int j, i;
	int ret;
	
	UPGRADE_DBG_INF("start = %d, nsector = %d, buf = 0x%x, \
						blksz = %d, write = %d\n", start, nsector, (unsigned int)buf, blksz, write);
	cur_sects = nsector;
	ret = 0;
	i = 0;
	j = nsector/256;
	// single data transfer 128K
	if(j > 0){
		cur_sects = 256;
	}
	
	do
	{ 
		if(0 == cur_sects){
			break;
		}
		UPGRADE_DBG_INF("start = %d, nsector = %d, buf = 0x%x, blksz = %d, write = %d\n",\
						start, cur_sects, (unsigned int)buf, blksz, write);
		ret = tSD_data_transfer(tSD_card, buf, start, cur_sects, blksz, write);
		if(ret){
			break;
		}
		i++;
		start += 256;
		buf += (256*512);

		if(j > i){
			cur_sects = 256;
		}else{
			cur_sects = nsector - (i*256);
		}
	}while(i <= j);  

	return ret;
}

int __do_adfu_read(unsigned start, unsigned nsector, void *buf)
{
	int ret = 0;
//	UPGRADE_DBG_INF("start = %d, nsector = %d, buf = 0x%x\n", start, nsector, (unsigned int)buf);
	if(NULL == tSD_card){
		printk("Err: %s : tSD_card is NULL,please check\n",__FUNCTION__);
		return -1;
	}
	mmc_claim_host(tSD_card->host);
	ret = tSD_pre_data_transfer(start, nsector, buf, TSD_BLOCKSZ, TSD_READ);
	mmc_release_host(tSD_card->host);
	return ret;
}
int __do_adfu_write(unsigned start, unsigned nsector, void * buf)
{
	int ret = 0;
//	UPGRADE_DBG_INF("start = %d, nsector = %d, buf = 0x%x\n", start, nsector, (unsigned int)buf);
	if(NULL == tSD_card){
		printk("Err: %s : tSD_card is NULL,please check\n",__FUNCTION__);
		return -1;
	}
	mmc_claim_host(tSD_card->host);
	ret =tSD_pre_data_transfer(start, nsector, buf, TSD_BLOCKSZ, TSD_WRITE);
	mmc_release_host(tSD_card->host);
	return ret;
}

int tSD_data_transfer(struct mmc_card * card, unsigned char * buf, unsigned start, unsigned  blocks, unsigned blksz, int write)
{
	struct mmc_request mrq = {0};
	struct mmc_command cmd = {0};
	struct mmc_command stop = {0};
	struct mmc_data data = {0};
	struct scatterlist sg ;
	
//	UPGRADE_DBG_INF("start = %d, blocks = %d, buf = 0x%x, blksz = %d ,write = %d\n", start, blocks, (unsigned int)buf, blksz, write);

	data.sg = &sg;
	mrq.cmd = &cmd;
	mrq.data = &data;
	mrq.stop = &stop;

	tSD_prepare_mrq(card, buf, &mrq, start, blocks, blksz, write);

	mmc_wait_for_req(card->host, &mrq);

	tSD_wait_busy(card);

	return tSD_test_check_result(card, &mrq);
}

/*
 * Fill in the mmc_request structure given a set of transfer parameters.
 */
static void tSD_prepare_mrq(struct mmc_card *card, unsigned char *buf,
	struct mmc_request *mrq, unsigned start, unsigned blocks, unsigned blksz, int write)
{

	BUG_ON(!mrq || !mrq->cmd || !mrq->data || !mrq->stop);

	if (blocks > 1) {
		mrq->cmd->opcode = write ?
			MMC_WRITE_MULTIPLE_BLOCK : MMC_READ_MULTIPLE_BLOCK;
	} else {
		mrq->cmd->opcode = write ?
			MMC_WRITE_BLOCK : MMC_READ_SINGLE_BLOCK;
	}

	mrq->cmd->arg = start;
	if (!mmc_card_blockaddr(card))
		mrq->cmd->arg <<= 9;

	mrq->cmd->flags = MMC_RSP_R1 | MMC_CMD_ADTC;

	if (blocks == 1)
		mrq->stop = NULL;
	else {
		mrq->stop->opcode = MMC_STOP_TRANSMISSION;
		mrq->stop->arg = 0;
		mrq->stop->flags = MMC_RSP_R1B | MMC_CMD_AC;
	}

	mrq->data->blksz = blksz;
	mrq->data->blocks = blocks;
	mrq->data->sg_len = 1;
	
	sg_init_one(mrq->data->sg,buf, blksz*blocks);

	mrq->data->flags = write ?
		MMC_DATA_WRITE : MMC_DATA_READ;
	//printk("%s,%d,buf = 0x%x, mrq->data->sg_len = 0x%x\n",__FUNCTION__,__LINE__,(unsigned int)buf,mrq->data->sg_len);
	mmc_set_data_timeout(mrq->data, card);
}




/*
 * Wait for the card to finish the busy state
 */
static int tSD_wait_busy(struct mmc_card *card)
{
	int ret, busy;
	struct mmc_command cmd = {0};

	busy = 0;
	do {
		memset(&cmd, 0, sizeof(struct mmc_command));

		cmd.opcode = MMC_SEND_STATUS;
		cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;

		ret = mmc_wait_for_cmd(card->host, &cmd, 0);
		if (ret)
			break;

		if (!busy && tSD_test_busy(&cmd)) {
			busy = 1;
			if (card->host->caps & MMC_CAP_WAIT_WHILE_BUSY)
				pr_info("%s: Warning: Host did not "
					"wait for busy state to end.\n",
					mmc_hostname(card->host));
		}
	} while (tSD_test_busy(&cmd));

	return ret;
}
static int tSD_test_busy(struct mmc_command *cmd)
{
	return !(cmd->resp[0] & R1_READY_FOR_DATA) ||
		(R1_CURRENT_STATE(cmd->resp[0]) == R1_STATE_PRG);
}
static int tSD_test_check_result(struct mmc_card *card, struct mmc_request *mrq)
{
	int ret;

	BUG_ON(!mrq || !mrq->cmd || !mrq->data);

	ret = 0;

	if (!ret && mrq->cmd->error)
		ret = mrq->cmd->error;
	if (!ret && mrq->data->error)
		ret = mrq->data->error;
	if (!ret && mrq->stop && mrq->stop->error)
		ret = mrq->stop->error;
	if (!ret && mrq->data->bytes_xfered !=
		mrq->data->blocks * mrq->data->blksz)
		ret = 1;

	if (ret == -EINVAL)
		ret = 2;

	return ret;
}

int tSD_single_blk_test(unsigned start, unsigned blocks )
{
	char *r_buf, *w_buf;
	char *pc_char, *pc_temp;
	int i, j, ret;

	UPGRADE_DBG_INF("start = %d, blocks = %d\n",start, blocks);

	w_buf = (char *)kmalloc(TSD_BLOCKSZ, GFP_DMA);
	r_buf = (char *)kmalloc(TSD_BLOCKSZ * blocks, GFP_DMA);
	
	if(r_buf == NULL || w_buf == NULL)
	{
		printk("%s,malloc buf error!\n",__FUNCTION__);
		return -1;
	}
	else
	{
		printk("malloc r_buf at [0x%x]\n",(unsigned int) r_buf);
		printk("malloc w_buf at[0x%x]\n",(unsigned int)w_buf);
	}
	memset(w_buf,0,TSD_BLOCKSZ);
	memset(r_buf,0,TSD_BLOCKSZ * blocks);
	pc_char = w_buf;
	for(i = 0; i < TSD_BLOCKSZ; i++)
	{
		*pc_char = 0xaa;
		pc_char++;
	}
	pc_char = w_buf;
	printk("%s,%d,data in w_buf\n",__FUNCTION__,__LINE__);
	for(i = 0; i < TSD_BLOCKSZ/16; i++)
	{
		printk("w_buf %d  :" ,i);
		for(j=0; j < 16; j++)
		{
			printk(" 0x%x ",*pc_char);
			pc_char++;
		}
		printk("\n");
	}
		
	//write signle data to tSD card
	for(i = 0; i < blocks; i++)
	{
		ret = __do_adfu_write(start + i, 1, w_buf);
		if(ret)
		{
			printk("%s,%d,write data err!i = %d, ret = %d\n",__FUNCTION__,__LINE__, i, ret);
			return ret;
		}
	}

	//read signle data from tSD card
	for(i = 0; i < blocks; i++)
	{
		ret = __do_adfu_read(start + i, 1, r_buf + i*TSD_BLOCKSZ);
		if(ret)
		{
			printk("%s,%d,write data err!i = %d, ret = %d\n",__FUNCTION__,__LINE__, i, ret);
			return ret;
		}
	}
	printk("malloc r_buf at [0x%x]\n",(unsigned int) r_buf);
	pc_char = r_buf;
	printk("%s,%d,data in r_buf\n",__FUNCTION__,__LINE__);
	for(i = 0; i < TSD_BLOCKSZ/16; i++)
	{
		printk("r_buf %d  :" ,i);
		for(j=0; j < 16; j++)
		{
			printk(" 0x%x ",*pc_char);
			pc_char++;
		}
		printk("\n");
	}
	//compare data :read and write
	for(i = 0; i < blocks; i++)
	{
		pc_char = r_buf + i*TSD_BLOCKSZ;
		pc_temp = w_buf;
		for(j = 0; j < TSD_BLOCKSZ; j++)
		{
			if(*pc_char != *pc_temp)
			{
				printk("%s,%d,compare data err! i = %d, j = %d\n",__FUNCTION__,__LINE__,i,j);
				return -1;
			}
			pc_char++;
			pc_temp++;
		}
	}

	printk("%s test OK!\n",__FUNCTION__);
	kfree(r_buf);
	kfree(w_buf);
	return 0;	
	
}
//static int tSD_multiple_blk_test(unsigned start, unsigned blocks, unsigned times)
//{
//	char *r_buf, *w_buf;
//	char *pc_char, *pc_temp;
//	int i, j, ret;
//
//	UPGRADE_DBG_INF("start = %d, blocks = %d, times = %d\n",start, blocks, times);
//
//	w_buf = (char *)kmalloc(TSD_BLOCKSZ * blocks, GFP_DMA);
//	r_buf = (char *)kmalloc(times*(TSD_BLOCKSZ * blocks), GFP_DMA);
//	
//	if( r_buf == NULL || w_buf == NULL)
//	{
//		printk("%s,malloc buf error!\n",__FUNCTION__);
//		return -1;
//	}
//	else
//	{
//		printk("malloc r_buf at [0x%x]\n",(unsigned int) r_buf);
//		printk("malloc w_buf at[0x%x]\n",(unsigned int)w_buf);
//	}
//	memset(w_buf,0,TSD_BLOCKSZ * blocks);
//	memset(r_buf,0,times*(TSD_BLOCKSZ * blocks));
//	pc_char = w_buf;
//	for(i = 0; i < TSD_BLOCKSZ * blocks; i++)
//	{
//		*pc_char = 0x5a;
//		pc_char++;
//	}
//	//write signle data to tSD card
//	for(i = 0; i < times; i++)
//	{
//		ret = __do_adfu_write(start+ i*blocks, blocks, w_buf);
//		if(ret)
//		{
//			printk("%s,%d,write data err!i = %d, ret = %d\n",__FUNCTION__,__LINE__, i, ret);
//			return ret;
//		}
//	}
//	//read signle data from tSD card
//	for(i = 0; i < times; i++)
//	{
//		ret = __do_adfu_read(start + i*blocks, blocks, r_buf + i*(TSD_BLOCKSZ*blocks));
//		if(ret)
//		{
//			printk("%s,%d,write data err!i = %d, ret = %d\n",__FUNCTION__,__LINE__, i, ret);
//			return ret;
//		}
//	}
//	//compare data :read and write
//	for(i = 0; i < times; i++)
//	{
//		pc_char = r_buf + i*(TSD_BLOCKSZ * blocks);
//		pc_temp = w_buf;
//		for(j = 0; j < TSD_BLOCKSZ * blocks; j++)
//		{
//			if(*pc_char != *pc_temp)
//			{
//				printk("%s,%d,compare data err! i = %d, j = %d\n",__FUNCTION__,__LINE__,i,j);
//				return -1;
//			}
//			pc_char++;
//			pc_temp++;
//		}
//	}
//
//	printk("%s test OK!\n",__FUNCTION__);
//	kfree(r_buf);
//	kfree(w_buf);
//	return 0;	
//}
//static void tSD_transfer_interface_test(void)
//{
// 	int ret;
//	unsigned start;
//	unsigned blocks;
//	unsigned times;
//
//	start = 100;
//	blocks =10;
//	times = 1;
//	
//	printk("--------------------\n");
//	printk("tSD_single_blk_test\n");
//	ret = tSD_single_blk_test(100,1);
//	printk("--------------------\n");
//	printk("tSD_multiple_blk_test\n");
//	//ret = tSD_multiple_blk_test(500, 488, 1);
//}

static int tSD_queue_init(struct mmc_blk_data *tSD_device)
{
	int ret;
	int devidx;
	
	UPGRADE_DBG_INF("\n");
	
 	if (tSD_device == NULL){
        		ret = -1;
        		return ret;
    }
	
	tSD_device->area_type = MMC_BLK_DATA_AREA_MAIN;
	tSD_device->usage = 1;
	mmc_set_drvdata(tSD_card, tSD_device);

	devidx = find_first_zero_bit(dev_use, max_devices);
	if (devidx >= max_devices)
		return -28;
	__set_bit(devidx, dev_use);

	tSD_device->name_idx = find_first_zero_bit(name_use, max_devices);
		__set_bit(tSD_device->name_idx, name_use);	

	//init system queue
	spin_lock_init(&tSD_device->slock);
	ret = tSD_init_queue(&tSD_device->squeue, tSD_card, &tSD_device->slock, NULL);	
	tSD_device->squeue.issue_fn = mmc_blk_issue_rq;
	tSD_device->squeue.data = tSD_device;
	//init data queue
	spin_lock_init(&tSD_device->dlock);
	ret = tSD_init_queue(&tSD_device->dqueue, tSD_card, &tSD_device->dlock, NULL);	
	tSD_device->dqueue.issue_fn = mmc_blk_issue_rq;
	tSD_device->dqueue.data = tSD_device;
	//init udisk queue
	spin_lock_init(&tSD_device->ulock);	
	ret = tSD_init_queue(&tSD_device->uqueue, tSD_card, &tSD_device->ulock, NULL);	
	tSD_device->uqueue.issue_fn = mmc_blk_issue_rq;
	tSD_device->uqueue.data = tSD_device;


	
	//init kthread
	sema_init(&tSD_device->thread_sem, 1);
	if(card_to_card){
		tSD_device->thread = kthread_run(tSD_queue_thread, tSD_device, "card_card_thread");
	}else{
		tSD_device->thread = kthread_run(tSD_queue_thread, tSD_device, "tsd_thread");
	}

	if (IS_ERR(tSD_device->thread)) {
		ret = PTR_ERR(tSD_device->thread);
		printk("%s, alloc kthread err!please check!\n",__FUNCTION__);
	}

	return 0;
}

//static int tSD_partition_init(struct mmc_blk_data *tSD_device, int part_num)
//{
//	UPGRADE_DBG_INF("\n");
//	device->partitions[part_num].md = tSD_blk_alloc_md(device->card,
//					device,
//					&device->card->dev,
//					&device->mq,
//					part_num,
//					device->partitions[part_num].size,
//					device->major,
//					device->minorbits,
//					false,
//					NULL,
//					MMC_BLK_DATA_AREA_MAIN);
//
////	mmc_set_drvdata(device->card, device->partitions[part_num].md);
////	mmc_fixup_device(device->card, blk_fixups);
//
//	UPGRADE_DBG_INF("\n");
//
////	if (mmc_add_disk(device->partitions[part_num].md))
////		goto out;
//
//	return 0;
//
// out:
//	mmc_blk_remove_req(device->partitions[part_num].md);
//	return 0;
//
//
//}

static int tSD_partition_init(struct mmc_blk_data *tSD_device, int part_num)
{

	tSD_device->partitions[part_num].num = part_num;
	if(tSD_part[part_num].type != PART_DUMMY){
		tSD_device->partitions[part_num].offset = tSD_part[part_num].off_size;
		tSD_device->partitions[part_num].size = tSD_part[part_num].partsize;
		tSD_device->partitions[part_num].attr = 0;

		printk("tSD_device->partitions[%d]  .offset=%lu, size=%lu\n", part_num
		, tSD_device->partitions[part_num].offset
		, tSD_device->partitions[part_num].size);

		if(tSD_device->partitions[part_num].size == 0){
			printk("%s() %d the disk(%d) is 0 size ,so not alloc disk.\n", __FUNCTION__, __LINE__, part_num);
			return 0;
		}

		tSD_device->partitions[part_num].disk = alloc_disk(1 << tSD_device->minorbits);
		if (!tSD_device->partitions[part_num].disk){
			printk("%s() %d alloc_disk (%d) failed.\n", __FUNCTION__, __LINE__, part_num);
			goto _out;
		}

		tSD_device->partitions[part_num].disk->major = tsd_major;
		tSD_device->partitions[part_num].disk->first_minor = part_num << tSD_device->minorbits;
		tSD_device->partitions[part_num].disk->fops = &mmc_bdops;
		if(tsd_major == 93){ 
			snprintf (tSD_device->partitions[part_num].disk->disk_name, 32, "act%c", 'a'+part_num);
		}else{					
			snprintf (tSD_device->partitions[part_num].disk->disk_name, 32, "burn%c", 'a'+part_num);
		}
		tSD_device->partitions[part_num].disk->private_data = tSD_device;
		set_capacity(tSD_device->partitions[part_num].disk, tSD_device->partitions[part_num].size);

		if (part_num == unassign_partnum){
			tSD_device->partitions[part_num].disk->queue = tSD_device->uqueue.queue;
			blk_queue_logical_block_size(tSD_device->uqueue.queue, 512);
		}else if (part_num == ANDROID_DATA_ACCESS){
			tSD_device->partitions[part_num].disk->queue = tSD_device->dqueue.queue;
			blk_queue_logical_block_size(tSD_device->dqueue.queue, 512);
		}else{
			tSD_device->partitions[part_num].disk->queue = tSD_device->squeue.queue;
			blk_queue_logical_block_size(tSD_device->squeue.queue, 512);
			/*remove waring : WARN_ON_ONCE(q->bypass_depth < 0)*/
			tSD_device->squeue.queue->bypass_depth = 1 ;	
		}

		if (tSD_part[part_num].type == PART_NO_ACCESS){
			tSD_device->partitions[part_num].attr |= NAND_PART_OP_NA;
		}else if (tSD_part[part_num].type == PART_READONLY){
			tSD_device->partitions[part_num].attr |= NAND_PART_OP_RO;
		}else if (tSD_part[part_num].type == PART_WRITEONLY){
			tSD_device->partitions[part_num].attr |= NAND_PART_OP_WO;
		}if (tSD_device->partitions[part_num].attr & NAND_PART_OP_RO){
			set_disk_ro(tSD_device->partitions[part_num].disk, 1);
		}

		add_disk(tSD_device->partitions[part_num].disk);
		printk("add_disk act%c success!\n", 'a'+part_num);
	}

	return 0;
	_out:
	return -1;
}

//static struct mmc_blk_data *tSD_blk_alloc_md(struct mmc_card *card,
//					struct mmc_blk_data *tSD_device,
//					struct device *parent,
//					struct mmc_queue *mq, 
//					int part_num,
//					sector_t partition_size,
//					int major,
//					int minorbits,
//					bool default_ro,
//					const char *subname,
//					int area_type)
//{
//
//	struct mmc_blk_data *md;
//	int ret;
//
//	md = kzalloc(sizeof(struct mmc_blk_data), GFP_KERNEL);
//	if (!md) {
//		ret = -ENOMEM;
//		goto out;
//	}
//
//	md->area_type = area_type;
//
//	/*
//	 * Set the read-only status based on the supported commands
//	 * and the write protect switch.
//	 */
//	md->read_only = mmc_blk_readonly(card);
//
//	md->disk = alloc_disk(1 << minorbits);
//	if (md->disk == NULL) {
//		ret = -ENOMEM;
//		goto err_kfree;
//	}
//
//	md->usage = 1;
//	
//	md->disk->major	= major;
//	md->disk->first_minor = 1 << minorbits;
//	md->disk->fops = &mmc_bdops;
//	md->disk->private_data = tSD_device;
//	md->disk->queue = mq->queue;
//
//	snprintf(md->disk->disk_name, sizeof(md->disk->disk_name),
//		 "act%c", 'w'+part_num);
//
//
//	blk_queue_logical_block_size(mq->queue, 512);
//	set_capacity(md->disk, partition_size);
//
//	UPGRADE_DBG_INF("\n");
//	
//	mmc_add_disk(md);
//	
//	return md;
//
// //err_putdisk:
//	put_disk(md->disk);
// err_kfree:
//	kfree(md);
// out:
//	return ERR_PTR(ret);
//}


static int tSD_blk_ioctl(struct block_device * bdev, fmode_t mode, unsigned int 
					cmd, unsigned long arg)
{
	int ret = 0;

    switch(cmd) 
	{
        case BOOT_ERASE:
        case BOOT_READ:
        case BOOT_WRITE:
        {
            ret = boot_operation(arg, cmd);
            return ret;
        }	        
        case BOOT_GETINFO:
        {
            ret = get_boot_media_info(arg);
            return ret;    
        }
		case ACCESS_MISC_INFO:
        {
        	ret = handle_misc_info(arg);
        	return ret;
        }
    	default:
            break;
    }
    
  //  printk("%s() %d: nand drv miss cmd %x\n", __FUNCTION__, __LINE__, cmd);
    return -ENOTTY; /* unknown command */
}

int handle_misc_info(unsigned int arg)
{
	int ret = 0;
	char *buf;
	MiscInfo_t p;
	ret = copy_from_user(&p, (void*)arg, sizeof(MiscInfo_t));
  if(ret)
  {
     printk("%s err!%d\n", __FUNCTION__, __LINE__);
	}
	    
	//PRINT("handle misc info %d %x %x %x\n", p.dir, p.type, p.buf, p.size);
	printk("[tsd/emmc]handle misc info\n");

	if(p.size < 8192)
	{
		buf = MALLOC(p.size);
		if(buf == NULL)
		{
			printk("%s malloc err,%d\n!", __FUNCTION__, p.size);
			return -1;
		}
	}
	else
	{
		printk("%s ERR\n", __FUNCTION__);
		return 0;
	}

	if(p.dir == 0) //read
	{
		ret = NAND_GetMiscInfo(p.type, buf, p.size);
		if(copy_to_user((void *)p.buf, buf, p.size))
    {
        printk("%s err!%d\n", __FUNCTION__, __LINE__);
        return -1;
	  }
	}
	else if(p.dir == 1)
	{
		if(copy_from_user(buf, p.buf, p.size))
    {
      printk("%s err!%d\n", __FUNCTION__, __LINE__);
      return -1;
	  }
		ret = NAND_WriteMiscInfo(p.type, buf, p.size);
	}

	if(buf)
	{
		FREE(buf);
		buf = NULL;
	}
	printk("[tsd/emmc] %s done\n", __FUNCTION__);
	return ret;
}


int get_boot_media_info(unsigned int arg)
{
	    int ret = 0;
	    unsigned long i = 0;
	    boot_medium_info_t bmi;

      memset(&bmi,0,sizeof(boot_medium_info_t));
        printk("%s %d bmi.medium_type:%d\n", __FUNCTION__, __LINE__, FLASH_TYPE_CARD);
        bmi.medium_type = FLASH_TYPE_CARD;
//	    if (NAND_IS_SMALL_BLOCK)
//	    {
//	        bmi.medium_type = 0;
//	    }
//	    else
//	    {
//	        bmi.medium_type = 1;
//	    }

	    bmi.pageAddrPerBlock = PAGE_PER_BLK;
		bmi.pagePerBlock = PAGE_PER_BLK;
	    bmi.sec_per_page = SEC_PER_PAGE;
	    bmi.sec_per_boot_page = SEC_PER_BOOT_PAGE;
//	    bmi.ecc_bits = NandDevInfo.NandFlashInfo->BromECCUnitParam->ECCBitsPerECCUnit;
//	    bmi.ud_bytes = NandDevInfo.NandFlashInfo->BromECCUnitParam->UserDataBytesPerECCUnit;
//	    bmi.ecc_bits = 0;
//	    bmi.ud_bytes = 0;
//	    bmi.readretry = 0;
//	    MEMSET(bmi.lsb_tbl, 0, 128);
//	    if (NAND_NEED_READ_RETRY!=0)
//	    {
//	        bmi.readretry = 1;
//	        MEMCPY(bmi.lsb_tbl, &(NandStorageInfo.RrStorageInfo.SmodeMap),128);
//	    }
	    
//	    MEMCPY(bmi.chipid, &(NandChipInfo), 64);
	    bmi.data_blk_per_zone = DATA_BLK_NUM_PER_ZONE;
	    i = copy_to_user((void *)arg, &bmi, sizeof(boot_medium_info_t));
	    if(i){
			printk("%s err!\n", __FUNCTION__);
	    }
        printk("%s %d\n", __FUNCTION__, __LINE__);
	    return ret;
}

int boot_operation(unsigned int arg, unsigned int cmd)
{
	    int ret = 0;
	    unsigned long i;
	    boot_op_t bootop;
	    unsigned char * buffer = NULL;
	    unsigned char * usr_buff = NULL;
	    unsigned int  page_size; //in bytes

//	    down(&tSD_device_md.mutex);    
//        printk("%s %d\n", __FUNCTION__, __LINE__);

	    //get param from user space.
	    i = copy_from_user(&bootop, (void*)arg, sizeof(boot_op_t));
	    if(i){
			printk("%s err!%d\n", __FUNCTION__, __LINE__);
	    }
	
	    if((cmd == BOOT_READ) || (cmd == BOOT_WRITE))
	    {
	        page_size = SEC_PER_PAGE * SEC_SIZE;
	        buffer = kmalloc(page_size, GFP_KERNEL);
	        if (buffer == NULL)
	        {
	            ret = -1;
	            goto _out;
	        }
        		usr_buff = bootop.buffer;
	        	//use kernel buffer to read data,
	        	bootop.buffer = buffer;
	    }
		
	    switch(cmd)
	    {
		        case BOOT_READ:		  
		            ret = boot_phy_op_entry(&bootop, BOOT_READ);
		            i = copy_to_user(usr_buff, buffer, page_size);
		            break;
		        case BOOT_WRITE:
		            i = copy_from_user(buffer, usr_buff, page_size);
		            ret = boot_phy_op_entry(&bootop, BOOT_WRITE);		    
		        	   break;
		        case BOOT_ERASE:		 
		            bootop.page = 0;
		            bootop.buffer = 0;
		            ret = boot_phy_op_entry(&bootop, BOOT_ERASE);		
		        break;
		        default:		   
		            printk("%s() %d boot operation cmd error!!!\n", __FUNCTION__, __LINE__);
			   break;		     
	   }

	   if(i){
		printk("%s err!%d\n", __FUNCTION__, __LINE__);
	   }
	   if((cmd == BOOT_READ) || (cmd == BOOT_WRITE))
 	   {
		kfree(buffer);
 	   }
_out:
//    up(&tSD_device_md.mutex);
//    printk("%s %d\n", __FUNCTION__, __LINE__);
    return ret;
}

#define DUMP_BUFFER(buf, start, size)   {\
                                            unsigned char *ptr;\
                                            int i;\
                                            ptr = (char*)buf+start;\
                                            printk("%d: \n", start);\
                                            for(i = 0; i < size; i++)\
                                            {\
                                                if(i % 16 == 0)\
                                                    printk("%d: ", start+i);\
                                                printk("%.2x ", *ptr++);\
                                                if(i % 16 == 15)\
                                                    printk("\n");\
                                            }\
                                            printk("\n");\
                                        }

int boot_phy_op_entry(boot_op_t * op, unsigned int cmd)
{
	int ret = 0;
	unsigned start;
	unsigned nsector;
	unsigned phyblk_num;
	unsigned page_in_blk;
	unsigned char *buffer;

	phyblk_num = op->blk;
	page_in_blk = op->page;
	buffer = op->buffer;

	start =convert_to_sector(phyblk_num, page_in_blk);
	nsector = SEC_PER_PAGE;
	
	switch(cmd)
	{
		case BOOT_READ:
//            printk("BOOT_READ block=%d, page=%d, start=%d, nsector=%d buffer=%p\n", op->blk, op->page, start, nsector, buffer);
			ret = __do_adfu_read(start, nsector, buffer);
			break;
		case BOOT_WRITE:
//		    if(op->blk > 0)
//		    {
//		        ret = 0;
//		        break;
//		    }
		    
//            printk("BOOT_WRITE block=%d, page=%d, start=%d, nsector=%d\n", op->blk, op->page, start, nsector);
//			DUMP_BUFFER(buffer, 0, nsector *512);
			ret = __do_adfu_write(start, nsector, buffer);
			break;
		case BOOT_ERASE:
		    break;
		default:
			printk("%s,%d,err!check!\n",__FUNCTION__,__LINE__);
	}

	return ret;
}

unsigned  convert_to_sector(unsigned blk, unsigned page)
{
	unsigned secter;
	secter = SEC_PER_BLOCK*blk + SEC_PER_PAGE*page + 1; 
	return secter;
}

int calculate_part_num(void)
{
	int ret;
	int i;
	partition_info_t *mbrinfo_partition = NULL;

	if(is_for_upgrade){
		mbrinfo_partition = GetMbrFromUser();
	}else{
		ret = ReadAfinfo();
		if (ret != 0) {
			printk("read mbr form tsd error\n");
			return -1;
		}else{
			printk("%s:ReadAfinfo success\n",__FUNCTION__);
		}
		p_afinfo = (afinfo_t *)(mbrc+AFINFO_OFFSET_IN_MBRC_BIN);
		mbrinfo_partition = (partition_info_t *)(&p_afinfo->partition_info);	
	}

	if(mbrinfo_partition == NULL){
		printk("%s,%d,get mbrinfo error!\n", __FUNCTION__, __LINE__);
		return -2;
	}
	
	for(i = 0; i < MAX_PARTITION; i++){
		printk("ptn=0x%x,part_num=0x%x,reserved=0x%x,part_cap=0x%x\n",	\
		mbrinfo_partition[i].flash_ptn,mbrinfo_partition[i].partition_num,\
		mbrinfo_partition[i].reserved,mbrinfo_partition[i].partition_cap);
	}
	
	i = 0;
	do{		

			if(mbrinfo_partition[i].partition_num == 0xFF){
				i -= 1;
				break;
			}	
			/* boot up will check this*/
			if(i == MAX_PARTITION){
				i = MAX_PARTITION - 1;
				break;
			}
			
			i++;
		
	}while(1);
		
	partition__logic_incard = i;
	partition_inmbr = partition__logic_incard + 1;
	
	printk("%s %d,partition_incard:%d,partition_inmbr:%d\n", \
		   __FUNCTION__, __LINE__,  partition__logic_incard,partition_inmbr);
	//include boot part 
	capinfo = (partition_info_t *) kzalloc (partition_inmbr * sizeof(partition_info_t), GFP_KERNEL);
	//logic part
	tSD_part = (struct tSD_partinfo *) kzalloc (partition__logic_incard * sizeof(struct tSD_partinfo), GFP_KERNEL);
	//logic part
	tSD_device_md.partitions = (tSD_partition_t *) kzalloc (partition__logic_incard * sizeof (tSD_partition_t), GFP_KERNEL); 

	if(capinfo == NULL || tSD_part == NULL || tSD_device_md.partitions == NULL){
		printk("%s, %d, error in alloc zoom!\n", __FUNCTION__, __LINE__);
	}

	return partition__logic_incard;
}


int init_board_cfg(void)
{
    int ret = 0;
    ret = get_afi_configuration();
    return ret;
}
int get_afi_configuration()
{
    int ret = TRUE;
	partition_info_t *mbrinfo_partition;
	
    printk("%s %d\n", __FUNCTION__, __LINE__);

    if(is_for_upgrade == 1)
    {
        printk("%s %d\n", __FUNCTION__, __LINE__);
       	mbrinfo_partition = GetMbrFromUser();
        if(!mbrinfo_partition) 
        {
            printk("GET MBR FROM USER CALLER ERROR!!\n");   
            return ret;
        }
		
		MEMCPY(capinfo, mbrinfo_partition, partition_inmbr * sizeof(partition_info_t));
    }
    else
    {
        printk("%s %d\n", __FUNCTION__, __LINE__);
        ret = ReadAfinfo();
        if (ret != 0) 
        {
            printk("READ MBR FROM NAND ERROR!!\n");
            return ret;
        }
        p_afinfo = (afinfo_t *)(mbrc+AFINFO_OFFSET_IN_MBRC_BIN);
        MEMCPY(capinfo,&(p_afinfo->partition_info), partition_inmbr * sizeof(partition_info_t));  
      	//DATA_BLK_NUM_PER_ZONE = p_afinfo->DataBlkNumInBoot;
    }    

  //  g_pcba_test_flag = p_afinfo->pcba_test;       
//    printk("AFI config: ce=0x%x, ce-ex= 0x%x,clk=%d, paddrv=0x%x.\n",g_nand_ceconfig, g_nand_ceconfig_ex, g_max_clk_config, g_paddrv_config);
    printk("%s %d\n", __FUNCTION__, __LINE__);
    return ret;
}

partition_info_t *GetMbrFromUser(void)
{
	int read_cnt = 0;
	/* path_from_caller = "/usr/mbr_info.bin"; */
	mm_segment_t old_fs;
	struct file *file = NULL;
	partition_info_t * tmp_partition_info;
	UINT8 *tmp_p;   
	mbr_info_t* mbr_info = (mbr_info_t *)MALLOC(MBR_SIZE);    
	char *path_from_caller = "/usr/mbr_info.bin"; //2012-7-7 file does not exist yet
	//char *path_from_caller = "/misc/mbr_info.bin"; //2012-7-7 file does not exist yet
	
	memset(mbr_info,0,MBR_SIZE);   	
	old_fs = get_fs();
	set_fs(get_ds());
	file = filp_open(path_from_caller, O_RDONLY, 0); //2012-7-7 11:54 return 0xfffffffe, instead NULL

	printk("%s %d\n", __FUNCTION__, __LINE__);

	if(IS_ERR(file)){
		printk("%s:OPEN FILE ERROR\n",__FUNCTION__);
		return NULL;
	}
	if (file->f_op->read ==NULL) {
		printk("FILE CAN'T BE READ!!\n");
		return NULL;
	}
	
	read_cnt = file->f_op->read(file, (unsigned char *)mbr_info, MBR_SIZE, &file->f_pos);
	if(read_cnt != MBR_SIZE) {
		printk("ONLY READ %d !!!\n", read_cnt);
		return NULL;    
	}else {
		printk("READ MBR_INFO SUCCESSFULLY !!\n");
		tmp_p = (UINT8*)mbr_info;
		filp_close(file,NULL);
		set_fs(old_fs);  
	}

	tmp_partition_info = mbr_info->partition_info;
	//FREE(mbr_info);

	return tmp_partition_info;
}

unsigned int GetAfiFromUser(afinfo_t * afinfo)
{
	int read_cnt = 0;
	mm_segment_t old_fs;
	struct file *file = NULL;
	unsigned int ret = 0;    

	char * path_from_caller = "/usr/afinfo.bin";
	old_fs = get_fs();
	set_fs(get_ds());
	file = filp_open(path_from_caller, O_RDONLY, 0);
	if (file ==NULL) {
		printk("OPEN FILE ERROR!!\n");
		return -1;
	}
	if (file->f_op->read ==NULL) {
		printk("FILE CAN'T BE READ!!\n");
		return -1;
	}
	read_cnt = file->f_op->read(file, (unsigned char *)afinfo, MBR_SIZE, &file->f_pos);
	if(read_cnt != MBR_SIZE) {
		printk("ONLY READ %d !!!\n", read_cnt);
		return -1;    
	}else {
		printk("READ AFI_INFO SUCCESSFULLY !!\n");
		filp_close(file,NULL);
		set_fs(old_fs);  
	}

	return ret;
}

/*
 * read afinfo from nand(mbrc), and upated to
 * corresponding file(mbrc.bin) on rootfs, when upgrading
 *
 */
unsigned int UpdateMbrFromPhyToUsr(unsigned int *p_nand_part, mbr_info_t *p_mbr_info)
{
	int i,ret = 0, hdcpKey = 0;
	/*
	* chk if hdcp key has been burn, it it does, update /usr/mbrc_info.bin,
	* mark and tell production to halt process
	*/
	if(owl_hdcp_is_burn() == 1){
		printk("%s:hdcp is alrady burn\n",__FUNCTION__);
		hdcpKey = 1 ;
	}

	ret = ReadAfinfo();//read capinfo from phy
	if (ret != 0 || is_force_format == 1) {
		MEMCPY(capinfo,&(p_mbr_info->partition_info), partition_inmbr * sizeof(partition_info_t));
		printk("==================================================\n");
		for(i = 0; i < 13; i++){
			printk("ptn=0x%x,part_num=0x%x,reserved=0x%x,part_cap=0x%x\n",	\
			capinfo[i].flash_ptn,capinfo[i].partition_num,\
			capinfo[i].reserved,capinfo[i].partition_cap);
		}
		printk("No valid AFINFO on nand(%s)\n", __FUNCTION__);
		ret = 1;
	}else{
		p_afinfo = (afinfo_t *)(mbrc+AFINFO_OFFSET_IN_MBRC_BIN);
		MEMCPY(capinfo,&(p_afinfo->partition_info), partition_inmbr * sizeof(partition_info_t));
		ret = UpdateMbrToUsr(capinfo, NULL);
	}

	ret |= init_tSD_part_myself();
	for(i=0; i<partition__logic_incard;i++){
		p_nand_part[i] = tSD_part[i].partsize/(2*1024);
		printk("i=%d, p_nand_part[i]=%d, capinfo[i].partition_cap=%d\n", i, p_nand_part[i], capinfo[i].partition_cap);       
	}

	if(hdcpKey){
		ret = -2; //will be recognized & handled in Production.py
	}
	
	return ret;
}

/*
 * copy/update information from batch production tool
 * and update to mbrc_info.bin in fs, thus the information
 * can be write to nand in the end
 * the upgrade.app in incorporate this information into
 */
unsigned int UpdateMbrToUsr(partition_info_t * partition_info_tbl, afinfo_t *p_afi)
{
    int read_cnt = 0;
    int i,write_cnt = 0, mbrinfo_update_flag=0;
    mm_segment_t old_fs;
    struct file *file = NULL;
//    partition_info_t * tmp_partition_info;
    unsigned int ret = 0;    
    mbr_info_t* mbr_info = (mbr_info_t *)MALLOC(MBR_SIZE);    
    char * path_from_caller = "/usr/mbr_info.bin";

    memset(mbr_info,0,MBR_SIZE);
    old_fs = get_fs();
    set_fs(get_ds());
    file = filp_open(path_from_caller, O_RDWR, 0);
     if (file ==NULL) {
        printk("OPEN FILE ERROR!!\n");
        ret = -1;
        goto out; 
    }
    if (file->f_op->read ==NULL) {
        printk("FILE CAN'T BE READ!!\n");
        ret = -1;
        goto out; 
    }
    read_cnt = file->f_op->read(file, (unsigned char *)mbr_info, MBR_SIZE, &file->f_pos);
    if(read_cnt != MBR_SIZE) {
        printk("ONLY READ %d !!!\n", read_cnt);
        ret = -1;
        goto out; 
    }

    for(i = 0; i < partition_inmbr; i++)
    {
        //update cap from phy to mbr
        if((mbr_info->partition_info[i].partition_cap == 0)
            && (partition_info_tbl[i].partition_cap != 0))
        {
            printk("update cap from phy to mbr,partition[%d],mbr:0x%x,phy:0x%x\n", i, mbr_info->partition_info[i].partition_cap, partition_info_tbl[i].partition_cap);            
            mbr_info->partition_info[i].partition_cap = partition_info_tbl[i].partition_cap;
            mbrinfo_update_flag = 1;        
        }
        
        //update cap from mbr to phy
        if((mbr_info->partition_info[i].partition_cap != 0)
            && (partition_info_tbl[i].partition_cap == 0))
        {
            printk("update cap from mbr to phy,partition[%d],mbr:0x%x,phy:0x%x\n", i, mbr_info->partition_info[i].partition_cap, partition_info_tbl[i].partition_cap);
            partition_info_tbl[i].partition_cap = mbr_info->partition_info[i].partition_cap;
            ret |= 1;
        }        
        
        if(mbr_info->partition_info[i].partition_cap > partition_info_tbl[i].partition_cap)
        {
            printk("too large,partition[%d],mbr:0x%x,phy:0x%x\n", i, mbr_info->partition_info[i].partition_cap, partition_info_tbl[i].partition_cap);
            ret = -1;
            goto out; 
        }
        else
        {
            printk("update cap from phy to mbr,partition[%d],mbr:0x%x,phy:0x%x\n", i, mbr_info->partition_info[i].partition_cap, partition_info_tbl[i].partition_cap);            
            mbr_info->partition_info[i].partition_cap = partition_info_tbl[i].partition_cap;
            mbrinfo_update_flag = 1;        
        }
    }

  /*
     * update hdcp key & serial no, from mbrc afi to mbrc_info.bin
	 * as sn copied to misc info block(BUG00109876) , this code is obsolete 
     */
    if(0)
    {
    	//MEMCPY(&mbr_info->HdcpKey, &p_afi->HdcpKey, 308);
    	MEMCPY(&mbr_info->SerialNo, &p_afi->sn, 16);
    	dump_mem(&p_afi->sn, 16, 0, 1 );
    }
	
    if (file->f_op->write ==NULL) {
        printk("FILE CAN'T BE WRITE!!\n");
        ret = -1;
        goto out;
    }    
    
    if(mbrinfo_update_flag == 1)
    {
        file->f_pos = 0;
        write_cnt = file->f_op->write(file, (unsigned char *)mbr_info, MBR_SIZE, &file->f_pos);
        if(write_cnt != MBR_SIZE) {
            printk("ONLY WRITE %d !!!\n", write_cnt);
            ret = -1;    
        }    
    }
out://    
    filp_close(file,NULL);
    set_fs(old_fs);  
        
    kfree(mbr_info);
    return ret;    
}


int owl_part_table_parse(void)
{
	int i;
	unsigned int fixed_cap = 0;
	sector_t logic_cap;
	
	for (i = 0; i < partition__logic_incard; i++){
		if((capinfo [i + 1].partition_cap == 0xffffffff)&&\
			(capinfo[i+1].partition_num != 0xFF)){
			unassign_partnum = i;
			capinfo[i+1].partition_cap = 0;
			printk ("%s,unassign_partnum : %d\n", __FUNCTION__, unassign_partnum);
		}

		tSD_part[i].partsize = capinfo[i+1].partition_cap;
		tSD_part[i].type = PART_FREE;    

		fixed_cap += tSD_part[i].partsize; // in MBbyte
	}    
  
	fixed_cap = 2 * 1024 * fixed_cap; // in sector
	logic_cap = tSD_get_logic_cat();

	if (fixed_cap >= logic_cap){
		printk("[EMMC/TSD] No enough space for partition(0-%d). Need(0x%08x), Free(0x%08llx).\n", (UDISK_ACCESS - 1), fixed_cap, logic_cap);
		return -1;
	}
   /* 
   	* reserve 2Mb is for hdcp drm etc. key stored 
    * FIXME: has problem ?
   */
	tSD_part[unassign_partnum].partsize = (logic_cap/(2*1024))-(fixed_cap/(2*1024)) ;
	tSD_part[unassign_partnum].type = PART_FREE;
	//resume capinfo partion cap
	capinfo[unassign_partnum+1].partition_cap = tSD_part[unassign_partnum].partsize ;
	
	/* adjust patrition offset */
	for (i = 0; i < partition__logic_incard; i++){
		tSD_part[i].off_size = get_cap_offset(i);		
	}
	
	return 0;
}


int init_tSD_part_myself(void)
{
	int i;
    int ret = 0;

    printk("%s %d\n", __FUNCTION__, __LINE__);

	if(owl_part_table_parse()){
		printk(" err parse owl_part_table_parse \n");
		return -1 ;
	}
		
		
    printk("----------------tSD_part-------------\n");
    for (i = 0; i < partition__logic_incard; i++)
    {
        tSD_part[i].off_size=tSD_part[i].off_size * 2 * 1024;
        tSD_part[i].partsize=tSD_part[i].partsize * 2 * 1024;
        printk("%d(%c).\t offset: 0x%8lx \t size:%lu(MB) \t type:0x%x \n",
            i, i+'a', tSD_part[i].off_size,
            tSD_part[i].partsize / 2048,
            tSD_part[i].type);
    }
    printk("----------------tSD_part-------------\n");


    return ret;

    //dump_mem(tSD_part, sizeof(tSD_part), 0, 4);
}

static unsigned int get_cap_offset(int part)
{
    int i;
    unsigned int part_cap = 0;

    for (i = 1; i < (part+1); i++)
    {
    	part_cap += capinfo[i].partition_cap;
    }
    return part_cap;
}

//get the logic size of the tSD card,not include the phy partition
//todo
unsigned int tSD_get_logic_cat(void)
{
	unsigned  int tSD_logic_size;

	tSD_logic_size = card_total_size;

    if (tSD_logic_size > ((4 * 1024 * 1024) >> 9)+ ((2 * 1024 * 1024) >> 9)) {
        /* reserve 4MB for mbrec */
		

        tSD_logic_size = tSD_logic_size-\
        				(((4 * 1024 * 1024) >> 9) + ((2 * 1024 * 1024) >> 9));
		printk("card_total_siz:%u,tSD_logic_size:%u\n",card_total_size,tSD_logic_size);
    }
	return tSD_logic_size;
}

int tSD_adfu_read(unsigned long start, unsigned long nsector, void *buf, struct uparam * adfu_uparam)
{
	unsigned int flash_part = adfu_uparam->flash_partition;

//    down(&nand_blk_device.mutex);

    if ((start + nsector) > tSD_part[flash_part].partsize) {
//        up(&nand_blk_device.mutex);        
        return -1;
    }
    //todo:add the phy off_size
    start += tSD_part[flash_part].off_size + SEC_PHY_BLOCK;
    
    if (__do_adfu_read(start, nsector, buf))
    {
        printk("read err\n");
//        up(&nand_blk_device.mutex);        
        return -1;
    }
//    up(&nand_blk_device.mutex);        
    return nsector;
}
//EXPORT_SYMBOL(tSD_adfu_read);

int tSD_adfu_write(unsigned long start, unsigned long nsector, void *buf, struct uparam * adfu_uparam)
{
    unsigned int flash_part = adfu_uparam->flash_partition;

//    down(&nand_blk_device.mutex);

    if ((start + nsector) > tSD_part[flash_part].partsize) {
		printk("%s: Error:opearte partion size\n",__FUNCTION__);
        return -1;
    }
    //todo:add the phy partion off_size
    start += tSD_part[flash_part].off_size + SEC_PHY_BLOCK;
    
//    printk("flash_part=%d, adfu_write(start=%d, nsector%d)\n", flash_part, start, nsector);
    if (__do_adfu_write(start, nsector, buf))
    {
        printk("write err\n");
//        up(&nand_blk_device.mutex);        
        return -1;
    }
//    up(&nand_blk_device.mutex);      
    return nsector;
}
//EXPORT_SYMBOL(tSD_adfu_write);

unsigned int tSD_op_read(unsigned long start, unsigned long nsector, void *buf, struct inode * i)
{
    struct uparam adfu_uparam;
    
    adfu_uparam.flash_partition = i->i_bdev->bd_disk->first_minor >> 3;

    tSD_adfu_read(start, nsector, buf, &adfu_uparam);

    return 0;
}

unsigned int tSD_op_write(unsigned long start, unsigned long nsector, void *buf, struct inode * i)
{
    struct uparam adfu_uparam;
    
    adfu_uparam.flash_partition = i->i_bdev->bd_disk->first_minor >> 3;

    tSD_adfu_write(start, nsector, buf, &adfu_uparam);
    
    return 0;
}
unsigned int ReadAfinfo()
{
	int i,j;
	unsigned int ret = -1;
	unsigned int MbrPageNum;
	unsigned short checksum;
	unsigned short  checksum1;
	unsigned short flag ;
	boot_op_t op;

    MbrPageNum = (MBRC_SECTOR_SIZE-1 + SEC_PER_BOOT_PAGE)/SEC_PER_BOOT_PAGE;
    mbrc = kmalloc(MbrPageNum*SEC_PER_BOOT_PAGE*512, GFP_KERNEL); /* mbr_info ocuppies 2 sector*/
    if(mbrc == NULL)
    {
	    printk("Boot Malloc Error!!\n");
	    return 1;
    }
    else
    {
    	printk("Boot Malloc %x, MbrPageNum=%d, %d!!\n",(unsigned int)mbrc, MbrPageNum, MbrPageNum*SEC_PER_BOOT_PAGE*512);
    }

    for (i = 0; i < 4; i++)
    {
    	op.buffer 	= mbrc;
    	op.blk 		= i;
    	for (j = 0;j < MbrPageNum; j++)
    	{
    		op.page = j;
    		ret = boot_phy_op_entry(&op, BOOT_READ);
    		op.buffer += SEC_SIZE * SEC_PER_BOOT_PAGE;
    	}
		
    	checksum = (unsigned int)calCRC(mbrc + 0x400, (MBRC_SIZE-0x400-4), 4) + 0x1234;
    	checksum1 = *(unsigned int*)(mbrc+MBRC_SIZE-4);
    	flag = *(unsigned short*)(mbrc+MBRC_SIZE-6);

    	if ((checksum==checksum1)&&(flag==0x55aa))
    	{
			printk("Read mbrc checksum success\n");
    		ret = 0;
    		break;
    	}
    	else
    	{
    		ret = -1;
    		//printk("Read mbrc checksum failed: calsum:0x%08x,srcsum:0x%08x,flag:0x%08x\n",\
				//	checksum,checksum1,flag);
    	}
    }

    return ret;
}

/**
 * calCRC - cal CRC by nBytes
 * @buf: data buf
 * @length: data length
 * @nBytes: byte ruler,support 2Bytes & 4Bytes
 */
unsigned int calCRC(unsigned char *buf, unsigned int length, unsigned char nBytes)
{
    unsigned int i=0,j=0,checkSum=0;
    unsigned short checkSumShort=0;

    if((length==0) || (nBytes==0))
    {
        return 0;
    }

    if(nBytes == 2)
    {
        for(i=0; i<(length/2); i++)
        {
            checkSumShort += ((unsigned short *)buf)[i];
        }
        return checkSumShort;
    }
    else if(nBytes == 4)
    {
        for(i=0; i<(length/4); i++)
        {
            checkSum += ((unsigned int *)buf)[i];
        }
        return checkSum;
    }
    else
    {
        for(i=0; i<(length/nBytes); i++)
        {
            for(j=0; j<nBytes; j++)
            {
                checkSum += (buf[nBytes*i+j])<<(j*8);
            }
        }
        return checkSum;
    }
}


/**
 * cal_key_checksum - cal CRC by nBytes
 * @buf: data buf
 * @length: data length
 * @nBytes: byte ruler,support 2Bytes & 4Bytes
 */
unsigned int cal_key_checksum(unsigned char *buf, unsigned int length, unsigned char nBytes)
{
    unsigned int i=0,j=0,checkSum=0;
    unsigned short checkSumShort=0;

    if((length==0) || (nBytes==0))
    {
        return 0;
    }


    #if 0
    for(i=0; i<(length/nBytes); i++)
    {
        for(j=0; j<nBytes; j++)
        {
            checkSum += (buf[(nBytes*i)+j]<<(j*8));

            if(nBytes == 2)
            {
                checkSumShort += ((unsigned short)buf[(nBytes*i)+j]<<(j*8));
            }
        }
    }

    if(nBytes == 2)
    {
        return checkSumShort;
    }
    return checkSum;

    #else
    if(nBytes == 2)
    {
        for(i=0; i<(length/2); i++)
        {
            checkSumShort += ((unsigned short *)buf)[i];
        }
        return checkSumShort ^ 0x55aa;
    }
    else if(nBytes == 4)
    {
        for(i=0; i<(length/4); i++)
        {
            checkSum += ((unsigned int *)buf)[i];
        }
        return checkSum ^ 0x55aa;
    }
    else
    {
        for(i=0; i<(length/nBytes); i++)
        {
            for(j=0; j<nBytes; j++)
            {
                checkSum += (buf[nBytes*i+j])<<(j*8);
            }
        }
        return checkSum ^ 0x55aa;
    }

    #endif
}



 #if 0
/*
 *
 */

int NAND_MiscInfoBlkBakup(unsigned char *buf)
{
    int ret = 0;

    __do_adfu_read(card_total_size - 2048 + 1, 32, buf);
    __do_adfu_write(card_total_size - 2048 + 1 + 32, 32, buf);

    return ret;
}
#endif

/*
1: hdcp is burn
others : not burn
*/

static int owl_hdcp_is_burn(void)
{
	int count=0;
	
	unsigned int offset;
	struct MiscInfoType_t Hdcp;

	offset = (unsigned int )(&MiscInfo.Hdcp) - (unsigned int )(&MiscInfo);
	count = do_rw_miscinfo(offset,(char*)(&Hdcp),
						sizeof(struct MiscInfoType_t),MISC_INFO_READ);
	if(count != sizeof(struct MiscInfoType_t)){
		printk("err:%d:read owl_hdcp_is_burn fail\n",count);
		return -1 ;
	}

	if(Hdcp.Burn== BURN_FLAG){
		return 1;
	}else{
		return 0;
	}
	
}

static int owl_miscinfo_is_burn(void)
{
	int count=0;
	
	unsigned int offset;
	unsigned int burn = 0;
	
	offset = (unsigned int )(&MiscInfo.Burn) - (unsigned int )(&MiscInfo);
	count = do_rw_miscinfo(offset,(char*)(&burn),
						sizeof(burn),MISC_INFO_READ);
	if(count != sizeof(burn)){
		printk("err:%d:read miscinfo_is_burn fail\n",count);
		return -1 ;
	}

	if(burn== BURN_FLAG){
		return 1;
	}else{
		return 0;
	}
	
}
/*
 * wr_flag : 1 (read), 0 (write)
 */
static int do_rw_miscinfo (unsigned int offset, char *buf, int size, int wr_flag)
{
	unsigned int mf_offset;
	unsigned int mf_sector_num, mf_sector_cnt;
	unsigned int buf_start;
	char *addr = NULL;
	int ret = 0;
	
	//printk("%s, %s %d bytes in %s\n", __FUNCTION__, (wr_flag ? "read" : "write"), size, mf_type->Name);

	//mf_offset = mf_type->Offset;
	//mf_size = mf_type->Size;

	mf_offset = offset;


	//if (size > mf_size) {
	//	printk (KERN_ERR"wite %d bytes to %s, max_size is %d!\n", size, mf_type->Name, mf_type->Size);
	//	return -ENOMEM;
	//}

	mf_sector_num = mf_offset / 512;
	mf_sector_cnt = size / 512;
	buf_start = mf_offset % 512;
	
	if (mf_offset % 512 || size % 512) {
		if ((mf_offset % 512 + size % 512) > 512)
			mf_sector_cnt += 2;
		else
			mf_sector_cnt += 1;
	}

	addr = (char *)kmalloc(mf_sector_cnt * 512, GFP_KERNEL);	
	if (addr == NULL){
		printk (KERN_ERR"kmalloc %d bytes err!\n", mf_sector_cnt * 512);
		return -ENOMEM;
	}
	ret = __do_adfu_read(miscinfo_start_addr + mf_sector_num, mf_sector_cnt, addr);

	
	//printk ("read from emmc:\n" );
	//dump_mem_f(addr + buf_start,size);
//	
	if (wr_flag == MISC_INFO_READ){
		memcpy(buf, addr + buf_start, size);
	} else {
		memcpy(addr + buf_start, buf, size);
		ret = __do_adfu_write(miscinfo_start_addr + mf_sector_num, mf_sector_cnt, addr);
		//WIRTE BAK 
		ret = __do_adfu_write(miscinfo_start_addr + mf_sector_num + MISC_INFO_SECTERS, mf_sector_cnt, addr);
		
		udelay(10);
		ret = __do_adfu_read(miscinfo_start_addr + mf_sector_num, mf_sector_cnt, addr);
		//printk ("read after write emmc:\n");
		//dump_mem_f(addr + buf_start,size);	
	}
	
	if (addr)
		kfree(addr);

	return size;
}

/*
 * write misc info block
 * [in]     misc info type
 * [in]     requested buffer for the misc info
 * [in]     requested data size
 */



int NAND_WriteMiscInfo(int type, char *buf, int size)
{
	int count=0;
	int ret=0;
	unsigned int offset;
	
	_miscMetuxLock();


	printk("NAND_WriteMiscInfo size %d\n",size);

	switch (type){
	case MISC_INFO_TYPE_SN:
		//wirte misc infor value
		if(size > SN_SIZE){
			size = SN_SIZE;
		}

		count = do_rw_miscinfo(MiscInfo.Sn.Offset,buf,size,MISC_INFO_WRITE);
		if(count!= size){
			printk("err:%d,write Sn do_rw_miscinfo\n",count);
			return -1;
		}
		MiscInfo.Sn.Size = count;
		MiscInfo.Sn.Burn = BURN_FLAG;
		offset = (unsigned int)(&MiscInfo.Sn) - (unsigned int)(&MiscInfo); 
		ret = do_rw_miscinfo(offset,(char*)(&MiscInfo.Sn),\
				sizeof(struct MiscInfoType_t),MISC_INFO_WRITE);
		if(ret != sizeof(struct MiscInfoType_t)){
			printk("err:%d,write Sn struct MiscInfoType_t\n",ret);
			return -1;
		}

	   	break;
	case MISC_INFO_TYPE_DRM:
		//wirte misc infor value
		if(size > DRM_KEY_SIZE){
			size = DRM_KEY_SIZE;
		}

		count = do_rw_miscinfo(MiscInfo.Drm.Offset,buf,size,MISC_INFO_WRITE);
		if(count!= size){
			printk("err:%d,write Drm do_rw_miscinfo\n",count);
			return -1;
		}
		MiscInfo.Drm.Size = count;
		MiscInfo.Drm.Burn = BURN_FLAG;
		offset = (unsigned int)(&MiscInfo.Drm) - (unsigned int)(&MiscInfo); 
		ret = do_rw_miscinfo(offset,(char*)(&MiscInfo.Drm),\
				sizeof(struct MiscInfoType_t),MISC_INFO_WRITE);
		if(ret != sizeof(struct MiscInfoType_t)){
			printk("err:%d,write Drm struct MiscInfoType_t\n",ret);
			return -1;
		}
		
	   	break;
	case MISC_INFO_TYPE_HDCP:
		//wirte misc infor value
		if(size > HDCP_KEY_SIZE){
			size = HDCP_KEY_SIZE;
		}

		count = do_rw_miscinfo(MiscInfo.Hdcp.Offset,buf,size,MISC_INFO_WRITE);
		if(count!= size){
			printk("err:%d,write Hdcp do_rw_miscinfo\n",count);
			return -1;
		}
		MiscInfo.Hdcp.Size = count;
		if(MiscInfo.Hdcp.Size == HDCP_KEY_SIZE){
			MiscInfo.Hdcp.Burn = BURN_FLAG;
		}else{
			MiscInfo.Hdcp.Burn = 0;
		}

		offset = (unsigned int)(&MiscInfo.Hdcp) - (unsigned int)(&MiscInfo); 
		ret = do_rw_miscinfo(offset,(char*)(&MiscInfo.Hdcp),\
				sizeof(struct MiscInfoType_t),MISC_INFO_WRITE);
		if(ret != sizeof(struct MiscInfoType_t)){
			printk("err:%d,write Hdcp struct MiscInfoType_t\n",ret);
			return -1;
		}
	   	break; 
		
	case MISC_INFO_TYPE_DEVNUM:
		//wirte misc infor value
		if(size > DEVNUM_SIZE){
			size = DEVNUM_SIZE;
		}

		count = do_rw_miscinfo(MiscInfo.DevNum.Offset,buf,size,MISC_INFO_WRITE);
		if(count!= size){
			printk("err:%d,write DevNum do_rw_miscinfo\n",count);
			return -1;
		}
		MiscInfo.DevNum.Size = count;
		MiscInfo.DevNum.Burn = BURN_FLAG;
		offset = (unsigned int)(&MiscInfo.DevNum) - (unsigned int)(&MiscInfo); 
		ret = do_rw_miscinfo(offset,(char*)(&MiscInfo.DevNum),\
				sizeof(struct MiscInfoType_t),MISC_INFO_WRITE);
		if(ret != sizeof(struct MiscInfoType_t)){
			printk("err:%d,write DevNum struct MiscInfoType_t\n",ret);
			return -1;
		}
		
		break; 
		
	case MISC_INFO_TYPE_EXT: 	
		//wirte misc infor value
		if(size > EXTSPACE_SIZE){
			size = EXTSPACE_SIZE;
		}

		count = do_rw_miscinfo(MiscInfo.ExtSpace.Offset,buf,size,MISC_INFO_WRITE);
		if(count!= size){
			printk("err:%d,write ExtSpace do_rw_miscinfo\n",count);
			return -1;
		}
		MiscInfo.ExtSpace.Size = count;
		MiscInfo.ExtSpace.Burn = BURN_FLAG;
		offset = (unsigned int)(&MiscInfo.ExtSpace) - (unsigned int)(&MiscInfo); 
		ret = do_rw_miscinfo(offset,(char*)(&MiscInfo.ExtSpace),\
				sizeof(struct MiscInfoType_t),MISC_INFO_WRITE);
		if(ret != sizeof(struct MiscInfoType_t)){
			printk("err:%d,write ExtSpace struct MiscInfoType_t\n",ret);
			return -1;
		}
		
		break;
			
	default:
		printk (KERN_ERR"miscinfo write type not define!\n");
	   	break;
   	}
#if 1
	// chage misc burn flag
	offset = (unsigned int )(&MiscInfo.Burn) - (unsigned int )(&MiscInfo);
	MiscInfo.Burn = BURN_FLAG;
	count = do_rw_miscinfo (offset, (char*)(&MiscInfo.Burn), sizeof(MiscInfo.Burn), MISC_INFO_WRITE);
	if(count < 0){
		printk("%s:err:%d:bakup miscinfor init state fail\n",__FUNCTION__,count);
		return -1;
	}
#endif	
	_miscMetuxUnlock();

	return count;
}

/*
 * must hold semaphore
 */
 int NAND_GetMiscInfo(int type, char *buf, int size)
{
	int count=0;
	int read_size=0;
	int headsize=0; //head_size
	unsigned int offset;
	
	switch (type) {	
	case MISC_INFO_TYPE_SN:
		// get size
		offset = (unsigned int)(&MiscInfo.Sn) - (unsigned int)(&MiscInfo);
		headsize = do_rw_miscinfo (offset,(char*)(&MiscInfo.Sn),\
				sizeof(struct MiscInfoType_t), MISC_INFO_READ);
		if(headsize != sizeof(struct MiscInfoType_t)){
			printk("err:%d,read sn headsize\n",headsize);
			return -1;
		}

		read_size = MiscInfo.Sn.Size;
		if(size < read_size){
			read_size = size;
		}
		printk("sn read size %d\n",read_size);	

		count = do_rw_miscinfo (MiscInfo.Sn.Offset, buf, read_size, MISC_INFO_READ);
		if( count != read_size ){
			printk("err:%d,read sn \n",count);
			return -1;
		}
	  	break;
		
	case MISC_INFO_TYPE_DRM:
		// get size
		offset = (unsigned int)(&MiscInfo.Drm) - (unsigned int)(&MiscInfo);
		headsize = do_rw_miscinfo (offset,(char*)(&MiscInfo.Drm),\
				sizeof(struct MiscInfoType_t), MISC_INFO_READ);
		if(headsize != sizeof(struct MiscInfoType_t)){
			printk("err:%d,read Drm headsize\n",headsize);
			return -1;
		}

		read_size = MiscInfo.Drm.Size;
		if(size < read_size){
			read_size = size;
		}
		printk("Drm read size %d\n",read_size);	
		count = do_rw_miscinfo (MiscInfo.Drm.Offset, buf, read_size, MISC_INFO_READ);
		if( count != read_size ){
			printk("err:%d,read Drm \n",count);
			return -1;
		}
	  	break;
		
	case MISC_INFO_TYPE_HDCP:
		// get size
		offset = (unsigned int)(&MiscInfo.Hdcp) - (unsigned int)(&MiscInfo);
		headsize = do_rw_miscinfo (offset,(char*)(&MiscInfo.Hdcp),\
				sizeof(struct MiscInfoType_t), MISC_INFO_READ);
		if(headsize != sizeof(struct MiscInfoType_t)){
			printk("err:%d,read Hdcp headsize\n",headsize);
			return -1;
		}

		read_size = MiscInfo.Hdcp.Size;	
		if(size < read_size){
			read_size = size;
		}
		printk("Hdcp read size %d\n",read_size);	
		count = do_rw_miscinfo (MiscInfo.Hdcp.Offset, buf, read_size, MISC_INFO_READ);
		if( count != read_size ){
			printk("err:%d,read Hdcp \n",count);
			return -1;
		}
	
	  	break;	
		
	case MISC_INFO_TYPE_DEVNUM:
		// get size
		offset = (unsigned int)(&MiscInfo.DevNum) - (unsigned int)(&MiscInfo);
		headsize = do_rw_miscinfo (offset,(char*)(&MiscInfo.DevNum),\
				sizeof(struct MiscInfoType_t), MISC_INFO_READ);
		if(headsize != sizeof(struct MiscInfoType_t)){
			printk("err:%d,read DevNum headsize\n",headsize);
			return -1;
		}

		read_size = MiscInfo.DevNum.Size;
		if(size < read_size){
			read_size = size;
		}		
		printk("DevNum read size %d\n",read_size);	
		count = do_rw_miscinfo (MiscInfo.DevNum.Offset, buf, read_size, MISC_INFO_READ);
		if( count != read_size ){
			printk("err:%d,read DevNum \n",count);
			return -1;
		}
	  	break;		
		
	case MISC_INFO_TYPE_EXT: 	
		// get size
		offset = (unsigned int)(&MiscInfo.ExtSpace) - (unsigned int)(&MiscInfo);
		headsize = do_rw_miscinfo (offset,(char*)(&MiscInfo.ExtSpace),\
				sizeof(struct MiscInfoType_t), MISC_INFO_READ);
		if(headsize != sizeof(struct MiscInfoType_t)){
			printk("err:%d,read ExtSpace headsize\n",headsize);
			return -1;
		}

		read_size = MiscInfo.ExtSpace.Size;
		if(size < read_size){
			read_size = size;
		}		
		printk("ExtSpace read size %d\n",read_size);	
		count = do_rw_miscinfo (MiscInfo.ExtSpace.Offset, buf, read_size, MISC_INFO_READ);
		if( count != read_size ){
			printk("err:%d,read ExtSpace \n",count);
			return -1;
		}
	  	break;
		
	default:
		break;
	}

	return	count;
}
//EXPORT_SYMBOL(NAND_GetMiscInfo);


/*
* FIXME: pls
*/
void NAND_InitMiscInfo(void)
{
	int ret = 0;
	char *addr = NULL;
	unsigned miscinfo_size;
	unsigned int sector_cnt;
	
	printk("[tsd/emmc]NAND_MISC_INFO ON\n");

	 _miscMetuxInit();
	
	MiscInfo.Drm.Offset  	= sizeof(struct MiscInfoBlk_t);
	MiscInfo.Hdcp.Offset 	= MiscInfo.Drm.Offset+ MiscInfo.Drm.Size;
	MiscInfo.Sn.Offset		= MiscInfo.Hdcp.Offset + MiscInfo.Hdcp.Size;
	MiscInfo.DevNum.Offset	= MiscInfo.Sn.Offset + MiscInfo.Sn.Size;
	MiscInfo.ExtSpace.Offset = MiscInfo.DevNum.Offset + MiscInfo.DevNum.Size;

	if (MiscInfo.TotalSize > (1024 * 1024)){
		printk (KERN_ERR"%s, 1 MB for MiscInfo isn't enough, need: %d\n", __FUNCTION__, MiscInfo.TotalSize);
	} else {
		/* 1M for write infor ,1M for bak infor*/
		miscinfo_start_addr = card_total_size - 2*MISC_INFO_SECTERS ;
		if(owl_miscinfo_is_burn() == 1){
			printk("owl_miscinfo is burn\n");
		}else{
			printk("owl_miscinfo is not burn\n");
		/* init the msic_info space head */
		miscinfo_size = sizeof (struct MiscInfoBlk_t);
		sector_cnt = miscinfo_size / 512;
		if (miscinfo_size % 512)
			sector_cnt++;
		addr = (char *) kmalloc (sector_cnt * 512, GFP_KERNEL);
		if (addr == NULL){
			printk (KERN_ERR"%s, alloc memery for misc_info Fail!\n", __FUNCTION__);
			return;
		}
		memset(addr, 0, miscinfo_size);
		memcpy(addr, &MiscInfo, miscinfo_size);

			ret = __do_adfu_write(miscinfo_start_addr, sector_cnt, addr);
			if (ret)
				printk (KERN_ERR"%s, write misc_info head data err!\n", __FUNCTION__);
			//for backup function
			ret = __do_adfu_write(miscinfo_start_addr+MISC_INFO_SECTERS, sector_cnt, addr); 
			if (ret){
				printk (KERN_ERR"%s, write bak misc_info head data err!\n", __FUNCTION__);
			}
			if (addr){
				kfree(addr);
			}
		}
	}
}

/*
 *
 */
void NAND_ShowMiscInfoAll(void)
{
	char *buf;
	buf = MALLOC(512);
	//NAND_GetMiscInfo(MISC_INFO_TYPE_SN);
	NAND_GetMiscInfo(MISC_INFO_TYPE_HDCP, buf, 308);
	dump_mem(buf, 308, 0 ,1);
}


module_init(mmc_blk_init);
module_exit(mmc_blk_exit);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Multimedia Card (MMC) block device driver");


