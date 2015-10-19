/*
 * Misc info Access driver
 *
 * Copyright 2015 Actions Company
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 *
 * The driver is used to save the important device hardware information,
 * ie. Serial No, HDCP Key and etc.
 * The misc info area is 1M Byte, start from 4M to 5M in storage like
 * NandFlash or e-MMC and etc.
 * The driver has 2 copy, one is in kernel, the other is in u-boot.
 *
 * Author:  Alex Sun
 *          28 Aug 2015
 */

#ifdef OS_LINUX
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/capability.h>
#include <linux/compat.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
//#include <linux/mutex.h>
#else
#include <common.h>
#include <errno.h>
//#include <mmc.h>
//#include <malloc.h>
#include <asm/arch/owl_afi.h>
#endif

#include "misc_info.h"

int read_misc_info(void *buf, unsigned int count);
int write_misc_info(void *buf, unsigned int count);

#ifdef OS_LINUX
extern int mi_debug_init(void);
extern int mi_debug_exit(void);

const char *mi_file[4]=
{
	"/dev/nand0",
	"/dev/block/nand0",
	"/dev/mmcblk0",
	"/dev/block/mmcblk0"
};

/*
 * read storage
 *
 * 1.ret < 0  read fail
 * 2.ret >= 0 read success, ret is read length
 * 3.
 */
int read_storage(void *buf, int start, int size)
{
	int i, ret = -1;
	off_t offset;
	int count;
	mm_segment_t old_fs;
	struct file *file = NULL;

	PRINT_DBG("%s, line %d, buf 0x%p, start %d, size %d\n",__FUNCTION__, __LINE__, buf, start, size);
	if(!buf || size <= 0){
		PRINT_ERR("invalid param\n");
		goto OUT;
	}
	old_fs = get_fs();
    set_fs(get_ds());

	for(i = 0; i < 4; i++){
		file = filp_open(mi_file[i], O_RDONLY, 0);
		if (IS_ERR(file) || file->f_op == NULL || file->f_op->read == NULL){
			if(!IS_ERR(file))
				filp_close(file, NULL);
			if(i < 3){
				continue;
			}else{
				PRINT_ERR("no blk dev\n");
				ret = -EAGAIN;
				goto OUT;
			}
		}else{
			PRINT_DBG("open file %s success\n", mi_file[i]);
			break;
		}
	}
	offset = file->f_op->llseek(file, (MISC_INFO_OFFSET + start), 0);
	if(offset != (MISC_INFO_OFFSET + start)){
		PRINT_ERR("lseek failed, offset %d\n", (int)offset);
		ret = -1;
		goto OUT;
	}
	count = file->f_op->read(file, (unsigned char *)buf, size, &file->f_pos);
	//dump_mem(buf, 0, count);
    if(count != size)
    {
        PRINT_ERR("should read %d, but only read %d!\n", size, count);
    }
	PRINT_DBG("read count:%d\n", count);
	ret = count;

	filp_close(file, NULL);
    set_fs(old_fs);

OUT:
	return ret;
}

/*
 * write storage
 *
 * 1.ret < 0  write fail
 * 2.ret >= 0 write success, ret is write length
 * 3.
 */
int write_storage(void *buf, int start, int size)
{
	int i, ret = -1;
	off_t offset;
	int count;
	mm_segment_t old_fs;
	struct file *file = NULL;

	PRINT_DBG("%s, line %d, buf 0x%p, start %d, size %d\n",__FUNCTION__, __LINE__, buf, start, size);
	if(!buf || size <= 0){
		PRINT_ERR("invalid param\n");
		goto OUT;
	}

	old_fs = get_fs();
    set_fs(get_ds());

	for(i = 0; i < 4; i++){
		file = filp_open(mi_file[i], O_RDWR, 0);
		if (IS_ERR(file) || file->f_op == NULL || file->f_op->write == NULL){
			if(!IS_ERR(file))
				filp_close(file, NULL);
			if(i < 3){
				continue;
			}else{
				PRINT_ERR("no blk dev\n");
				ret = -EAGAIN;
				goto OUT;
			}
		}else{
			PRINT_DBG("open file %s success\n", mi_file[i]);
			break;
		}
	}

	offset = file->f_op->llseek(file, (MISC_INFO_OFFSET + start), 0);
	if(offset != (MISC_INFO_OFFSET + start)){
		PRINT_ERR("lseek failed, offset %d\n", (int)offset);
		ret = -1;
		goto OUT;
	}
	count = file->f_op->write(file, (unsigned char *)buf, size, &file->f_pos);
	//dump_mem(buf, 0, count);
    if(count != size)
    {
        PRINT_ERR("should write %d, but only write %d!\n", size, count);
    }
	PRINT_DBG("write count:%d\n", count);
	ret = count;

	filp_close(file, NULL);
    set_fs(old_fs);

OUT:
	return ret;
}

#else

#define BLOCK_SIZE 				 512

extern int owl_get_boot_dev(void);
extern int LDL_DeviceOpReadSectors(unsigned int start, unsigned int nsector, void *buf, int diskNo);
extern int LDL_DeviceOpWriteSectors(unsigned int start, unsigned int nsector, void *buf, int diskNo);
extern ulong mmc_bread(int dev_num, lbaint_t start, lbaint_t blkcnt, void *dst);
extern ulong mmc_bwrite(int dev_num, lbaint_t start, lbaint_t blkcnt, const void *src);

static int blk_read(void *buf, unsigned int start, unsigned int blkcnt)
{
	int ret = -1;

	ret = owl_get_boot_dev();
	if(ret == OWL_BOOTDEV_NAND){
//		ret = LDL_DeviceOpReadSectors(start, blkcnt, buf, 0);
	}else{
		if(ret == OWL_BOOTDEV_SD0)
			ret = mmc_bread(0, start, blkcnt, buf);
		else if(ret == OWL_BOOTDEV_SD2)
			ret = mmc_bread(1, start, blkcnt, buf);
		else
			ret = -1;
		if(ret != blkcnt)
			ret = -1;
		else
			ret = 0;
	}
	return ret;
}

static int blk_write(void *buf, unsigned int start, unsigned int blkcnt)
{
	int ret = -1;

	ret = owl_get_boot_dev();
	if(ret == OWL_BOOTDEV_NAND){
//		ret = LDL_DeviceOpWriteSectors(start, blkcnt, buf, 0);
	}else{
		if(ret == OWL_BOOTDEV_SD0)
			ret = mmc_bwrite(0, start, blkcnt, buf);
		else if(ret == OWL_BOOTDEV_SD2)
			ret = mmc_bwrite(1, start, blkcnt, buf);
		else
			ret = -1;
		if(ret != blkcnt)
			ret = -1;
		else
			ret = 0;
	}
	return ret;
}

/*
 * read storage
 *
 * 1.ret < 0  read fail
 * 2.ret >= 0 read success, ret is read length
 * 3.
 */
int read_storage(void *buf, int start, int size)
{
	int start_blk, start_in_blk, count;
	char *blk_buf;
	int ret = size;

	PRINT_DBG("%s, line %d, buf 0x%p, start %d, size %d\n",__FUNCTION__, __LINE__, buf, start, size);

	if(!buf || size <= 0){
		PRINT_ERR("invalid param\n");
		ret = -1;
		goto OUT_NULL;
	}

	blk_buf = MALLOC(BLOCK_SIZE);
	if(!blk_buf){
		PRINT_ERR("MALLOC FAILED\n");
		ret = -ENOMEM;
		goto OUT_NULL;
	}
	memset(blk_buf, 0, BLOCK_SIZE);

	if(start % BLOCK_SIZE != 0){
		start_in_blk = (MISC_INFO_OFFSET + start) % BLOCK_SIZE;
		start_blk = (MISC_INFO_OFFSET + start) / BLOCK_SIZE;//from sector 0
		if(blk_read(blk_buf, start_blk, 1) < 0){
			PRINT_ERR("blk_read failed, start blk %d, blkcnt 1\n", start_blk);
			ret = -1;
			goto OUT_FAILED;
		}
		count = size > (BLOCK_SIZE - start_in_blk) ? (BLOCK_SIZE - start_in_blk) : size;
		memcpy(buf, blk_buf + start_in_blk, count);
		buf += count;
		start += count;
		size -= count;
	}
	while(size > BLOCK_SIZE){
		start_blk = (MISC_INFO_OFFSET + start) / BLOCK_SIZE;//from sector 0
		if(blk_read(blk_buf, start_blk, 1) < 0){
			PRINT_ERR("blk_read failed, start blk %d, blkcnt 1\n", start_blk);
			ret = -1;
			goto OUT_FAILED;
		}
		memcpy(buf, blk_buf, BLOCK_SIZE);
		buf += BLOCK_SIZE;
		start += BLOCK_SIZE;
		size -= BLOCK_SIZE;
	}
	if(size > 0){
		start_blk = (MISC_INFO_OFFSET + start) / BLOCK_SIZE;//from sector 0
		if(blk_read(blk_buf, start_blk, 1) < 0){
			PRINT_ERR("blk_read failed, start blk %d, blkcnt 1\n", start_blk);
			ret = -1;
			goto OUT_FAILED;
		}
		memcpy(buf, blk_buf, size);
	}

OUT_FAILED:
	if(blk_buf){
		FREE(blk_buf);
		blk_buf = NULL;
	}
OUT_NULL:
	return ret;
}
/*
 * write storage
 *
 * 1.ret < 0  write fail
 * 2.ret >= 0 write success, ret is write length
 * 3.
 */
int write_storage(void *buf, int start, int size)
{
	int start_blk, start_in_blk, count;
	char *blk_buf;
	int ret = size;

	PRINT_DBG("%s, line %d, buf 0x%p, start %d, size %d\n",__FUNCTION__, __LINE__, buf, start, size);

	if(!buf || size <= 0){
		PRINT_ERR("invalid param\n");
		ret = -1;
		goto OUT_NULL;
	}

	blk_buf = MALLOC(BLOCK_SIZE);
	if(!blk_buf){
		PRINT_ERR("MALLOC FAILED\n");
		ret = -ENOMEM;
		goto OUT_NULL;
	}
	memset(blk_buf, 0, BLOCK_SIZE);

	if(start % BLOCK_SIZE != 0){
		start_in_blk = (MISC_INFO_OFFSET + start) % BLOCK_SIZE;
		start_blk = (MISC_INFO_OFFSET + start) / BLOCK_SIZE;//from sector 0
		if(blk_read(blk_buf, start_blk, 1) < 0){
			PRINT_ERR("blk_read failed, start blk %d, blkcnt 1\n", start_blk);
			ret = -1;
			goto OUT_FAILED;
		}
		count = size > (BLOCK_SIZE - start_in_blk) ? (BLOCK_SIZE - start_in_blk) : size;
		memcpy(blk_buf + start_in_blk, buf, count);
		if(blk_write(blk_buf, start_blk, 1) < 0){
			PRINT_ERR("blk_write failed, start blk %d, blkcnt 1\n", start_blk);
			ret = -1;
			goto OUT_FAILED;
		}
		buf += count;
		start += count;
		size -= count;
	}
	while(size > BLOCK_SIZE){
		start_blk = (MISC_INFO_OFFSET + start) / BLOCK_SIZE;//from sector 0
		memcpy(blk_buf, buf, BLOCK_SIZE);
		if(blk_write(blk_buf, start_blk, 1) < 0){
			PRINT_ERR("blk_write failed, start blk %d, blkcnt 1\n", start_blk);
			ret = -1;
			goto OUT_FAILED;
		}
		buf += BLOCK_SIZE;
		start += BLOCK_SIZE;
		size -= BLOCK_SIZE;
	}
	if(size > 0){
		start_blk = (MISC_INFO_OFFSET + start) / BLOCK_SIZE;//from sector 0
		if(blk_read(blk_buf, start_blk, 1) < 0){
			PRINT_ERR("blk_read failed, start blk %d, blkcnt 1\n", start_blk);
			ret = -1;
			goto OUT_FAILED;
		}
		memcpy(blk_buf, buf, size);
		if(blk_write(blk_buf, start_blk, 1) < 0){
			PRINT_ERR("blk_write failed, start blk %d, blkcnt 1\n", start_blk);
			ret = -1;
			goto OUT_FAILED;
		}
	}

OUT_FAILED:
	if(blk_buf){
		FREE(blk_buf);
		blk_buf = NULL;
	}
OUT_NULL:
	return ret;
}
#endif


/*
 * read misc info head
 *
 * 1.ret < 0  read fail
 * 2.ret == 0 read data invalid
 * 3.ret > 0  read success
 */
int read_mi_head(misc_info_head_t * head)
{
	int ret, head_length;
	misc_info_head_t *tmp_head = NULL;
	char *head_buf = NULL;

	PRINT_DBG("%s, line %d\n",__func__, __LINE__);
	head_length = sizeof(misc_info_head_t);
	head_buf = MALLOC(head_length);
	if(!head_buf){
		PRINT_ERR("MALLOC head failed\n");
		ret = -ENOMEM;
		goto OUT_NULL;
	}
	memset(head_buf, 0, head_length);

	ret = read_storage(head_buf, 0, head_length);
	if(ret < 0){
		PRINT_ERR("read MiscInfoHeader failed\n");
		goto OUT_FAILED;
	}
	if(ret != head_length){
		PRINT_ERR("read_storage return %d, but should be %d\n", ret, head_length);
		goto OUT_FAILED;
	}
	tmp_head = (misc_info_head_t *)head_buf;
	if(tmp_head->magic != MISC_INFO_MAGIC || tmp_head->length > MISC_INFO_MAX_SIZE
		|| tmp_head->item_num > MISC_INFO_MAX_ITEM_NUM){
		PRINT_ERR("%s, line %d, magic 0x%x, length %d, item num %d\n", __FUNCTION__, __LINE__,
			tmp_head->magic, tmp_head->length, tmp_head->item_num);
		ret = 0;
		goto OUT_FAILED;
	}
	memcpy(head, (misc_info_head_t *)head_buf, head_length);
	ret = head_length;
	if(debug_enable)
		dump_mem(head, 0, head_length);

OUT_FAILED:
	if(head_buf){
		FREE(head_buf);
		head_buf = NULL;
	}
OUT_NULL:
	return ret;
}

/*
 * read misc info item
 *
 * 1.ret < 0  read fail
 * 2.ret > 0  read success, ret is read length
 *
 */
int read_mi_item(char *name, void *buf, unsigned int count)
{
	int ret = -1, item;
	unsigned short chksum_calc, chksum_rec;
	misc_info_head_t *head = NULL;
	unsigned char *data = NULL;

	PRINT_DBG("%s, line %d, buf 0x%p, count %d\n", __func__, __LINE__, buf, count);
	if(!name || !buf || count == 0){
		PRINT_ERR("%s, line %d, err\n", __func__, __LINE__);
		goto OUT_NULL;
	}

	head = MALLOC(sizeof(misc_info_head_t));
	if(!head){
		PRINT_ERR("MALLOC head failed\n");
		ret = -ENOMEM;
		goto OUT_NULL;
	}
	memset(head, 0, sizeof(misc_info_head_t));

	ret = read_mi_head(head);
	if(ret < 0){
		PRINT_ERR("read Head failed\n");
		goto OUT_FAILED;
	}else if(ret == 0){
		PRINT_ERR("read Head null\n");
		ret = -1;
		goto OUT_FAILED;
	}
	PRINT_DBG("%s, line %d, misc info: length %d, item num %d\n", __func__, __LINE__, head->length, head->item_num);

	if(strlen(name) > sizeof(head->item_head[0].name)){
		PRINT_ERR("invalid name, too large\n");
		goto OUT_FAILED;
	}
	for(item = 0; item < head->item_num; item++)
	{
		if(memcmp(name, head->item_head[item].name, strlen(name)) == 0){
			break;
		}
	}
	if(item == head->item_num){
		PRINT_ERR("can not find %s\n", name);
		goto OUT_FAILED;
	}
	PRINT_DBG("%s, line %d, name %s, size %d, offset %d, checksum 0x%x\n", __func__, __LINE__,
	 head->item_head[item].name, head->item_head[item].size,
	 head->item_head[item].offset, head->item_head[item].chk_sum);

	data = MALLOC(head->item_head[item].size);
	if(!data){
		PRINT_ERR("MALLOC head failed\n");
		ret = -ENOMEM;
		goto OUT_FAILED;
	}
	memset(data, 0, head->item_head[item].size);
	ret = read_storage(data, head->item_head[item].offset, head->item_head[item].size);
	if(ret < 0){
		PRINT_ERR("read item %s data failed\n", name);
		goto OUT_FAILED;
	}
	if(debug_enable)
		dump_mem(data, 0, head->item_head[item].size);
	chksum_rec = head->item_head[item].chk_sum;
	chksum_calc = get_checksum((unsigned short *)data, head->item_head[item].size/2);
	if(chksum_rec != chksum_calc)
	{
		PRINT_ERR("get %s chksum failed, calc:0x%x rec:0x%x\n", name, chksum_calc, chksum_rec);
		//dump_mem(data, 0, head->item_head[item].size);
		ret = -1;
		goto OUT_INVALID;
	}
	ret = (count > head->item_head[item].size) ? head->item_head[item].size : count;
	memcpy(buf, data, ret);
	PRINT("%s, line %d, success\n", __func__, __LINE__);

OUT_INVALID:
	if(data){
		FREE(data);
		data = NULL;
	}
OUT_FAILED:
	if(head){
		FREE(head);
		head = NULL;
	}
OUT_NULL:
	return ret;
}

/*
 * write misc info item
 *
 * 1.ret < 0  write fail
 * 2.ret >= 0 write success, ret is write length
 *
 */
int write_mi_item(char *name, void *buf, unsigned int count)
{
	int ret = -1, pack_len;
    usb_packet_t *packet = NULL;

	PRINT_DBG("%s, line %d, buf 0x%p, count %d\n", __func__, __LINE__, buf, count);
	if(!name || !buf || count == 0){
		PRINT_ERR("%s, line %d, err\n", __func__, __LINE__);
		goto OUT_NULL;
	}
	pack_len = sizeof(int) + sizeof(packet_item_t) + count;

    packet = MALLOC(pack_len);
	if(!packet){
		PRINT_ERR("MALLOC packet failed\n");
		ret = -ENOMEM;
		goto OUT_NULL;
	}
	memset(packet, 0, pack_len);

	packet->length = pack_len;
	packet->item[0].magic = MISC_INFO_MAGIC;// FIXME
	if(strlen(name) > sizeof(packet->item[0].name)){
		PRINT_ERR("invalid name, too large, must < 7 Byte\n");
		goto OUT_FAILED;
	}
	memcpy(packet->item[0].name, name, strlen(name));
	packet->item[0].size = count;
	memcpy(packet->item[0].data, buf, count);

	ret = write_misc_info(packet, packet->length);
	if(ret != packet->length){
		PRINT_ERR("write item failed, ret %d\n", ret);
	}
	PRINT("%s, line %d, success\n", __func__, __LINE__);

OUT_FAILED:
	if(packet){
		FREE(packet);
		packet = NULL;
	}
OUT_NULL:
	return ret;
}

/*
 * print all item info
 *
 */
void print_mi_items(misc_info_head_t * head)
{
	int item;
	if(!head){
		PRINT_ERR("%s, line %d, buf is NULL\n", __func__, __LINE__);
		return;
	}

	for(item = 0; item < head->item_num; item++){
		PRINT("[%02d] name:%s size:%d\n", item, head->item_head[item].name,
			  head->item_head[item].size);
	}
}

/*
 * get item size
 *
 */
int get_item_size(misc_info_head_t * head, char *name)
{
	int ret = -1, item;
	if(!head || !name){
		PRINT_ERR("%s, line %d, buf is NULL\n", __func__, __LINE__);
		return ret;
	}

	if(strlen(name) > sizeof(head->item_head[0].name)){
		PRINT_ERR("invalid name, too large, must < 7 Byte\n");
		return ret;
	}
	for(item = 0; item < head->item_num; item++){
		if(memcmp(head->item_head[item].name, name, strlen(name)) == 0){
			return head->item_head[item].size;
		}
	}
	return ret;
}

/*
 * read misc info
 *
 * 1.the data is read to pc tool through USB.
 * 2.
 */
int read_misc_info(void *buf, unsigned int count)
{
	int ret = -1, i;
	int head_length, data_length, pack_length, data_offset;
	unsigned short chksum_calc, chksum_rec;
	misc_info_head_t *head = NULL;
	unsigned char *data = NULL;
	usb_packet_t *packet = NULL;
	packet_item_t *item = NULL;

	PRINT_DBG("%s, line %d, buf 0x%p, count %d\n", __func__, __LINE__, buf, count);

	if(!buf){
		PRINT_ERR("%s, line %d, buf is NULL\n", __func__, __LINE__);
		return ret;
	}

	head_length = sizeof(misc_info_head_t);
	head = MALLOC(head_length);
	if(!head){
		PRINT_ERR("MALLOC head failed\n");
		ret = -ENOMEM;
		goto MALLOC_FAILED;
	}
	memset(head, 0, head_length);

	ret = read_mi_head(head);
	if(ret < 0){
		PRINT_ERR("read Head failed\n");
		ret = -1;
		goto READ_HEAD_FAILED;
	}else if(ret == 0){
		memset(head, 0, head_length);
	}else{
		PRINT_DBG("%s, line %d, misc info: length %d, item num %d\n", __func__, __LINE__, head->length, head->item_num);
		data_length = head->length - head_length;
		data = MALLOC(data_length);
		if(!data){
			PRINT_ERR("MALLOC data failed\n");
			ret = -ENOMEM;
			goto READ_HEAD_FAILED;
		}
		memset(data, 0, data_length);

		if(read_storage(data, head_length, data_length) < 0){
			PRINT_ERR("read data failed\n");
			goto READ_DATA_FAILED;
		}
		if(debug_enable)
			dump_mem(data, 0, data_length);
	}

	pack_length = sizeof(int);
	for(i=0; i<head->item_num; i++)
	{
		pack_length += sizeof(packet_item_t) + head->item_head[i].size;//sizeof(packet_item_t)==4+8+4?
	}
	PRINT_DBG("%s, line %d, packet length %d\n", __func__, __LINE__, pack_length);

	packet = (usb_packet_t *) MALLOC(pack_length);
	if(!packet){
		PRINT_ERR("MALLOC packet failed\n");
		ret = -ENOMEM;
		goto READ_DATA_FAILED;
	}
	memset(packet, 0, pack_length);
	item = packet->item;

	packet->length = pack_length;
	for(i=0; i<head->item_num; i++){
		item->magic = head->item_head[i].magic;
		memcpy(item->name, head->item_head[i].name, strlen(head->item_head[i].name));
		item->size = head->item_head[i].size;
		data_offset = head->item_head[i].offset - head_length;
		memcpy(item->data, data + data_offset, item->size);

		chksum_rec = head->item_head[i].chk_sum;
		chksum_calc = get_checksum((unsigned short *)item->data, item->size/2);
		if(chksum_rec != chksum_calc){
			PRINT_ERR("get %s chksum failed calc:%x rec:%x\n", item->name, chksum_calc, chksum_rec);
			//dump_mem(item->data, 0, item->size);
		}
		item = (packet_item_t *)((char *)item + sizeof(packet_item_t) + item->size);
	}
	dump_mem(packet, 0, packet->length);

	ret = (count > packet->length) ? packet->length : count;
	memcpy(buf, packet, ret);
	PRINT("%s, line %d, success\n", __func__, __LINE__);

	if(packet){
		FREE(packet);
		packet = NULL;
	}
READ_DATA_FAILED:
	if(data){
		FREE(data);
		data = NULL;
	}
READ_HEAD_FAILED:
	if(head){
		FREE(head);
		head = NULL;
	}
MALLOC_FAILED:
	return ret;
}

/*
 * write misc info
 *
 * 1.the data is written from pc tool through USB.
 * 2.if the data is incomplete, write part of it.
 * 3.item will be merged, item with same name will override the old.
 */
int write_misc_info(void *buf, unsigned int count)
{
    int ret = -1, offset = 0, item_size, head_length;
	int index = 0, pack_item_num, pack_pos, i, j;
    usb_packet_t *packet = NULL;
    packet_item_t *pack_item = NULL;
	misc_info_t *old_misc = NULL, *new_misc = NULL;
	misc_info_head_t *new_head = NULL, *old_head = NULL;
	unsigned char *new_data = NULL, *old_data = NULL;

	PRINT_DBG("%s, line %d, buf 0x%p, count %d\n", __FUNCTION__, __LINE__, buf, count);
	if(buf == NULL){
		PRINT_ERR("%s, line %d, buf is null\n",__FUNCTION__, __LINE__);
		return -1;
	}
	if(debug_enable)
		dump_mem(buf, 0, count);

    packet = (usb_packet_t *)buf;

	if(count <= sizeof(int) + sizeof(packet_item_t)){
		return 0;
	}

	old_misc = (misc_info_t *) MALLOC(MISC_INFO_MAX_SIZE);
	if(!old_misc){
		PRINT_ERR("MALLOC old_misc failed\n");
		ret = -ENOMEM;
		goto MALLOC_OLD_MISC_FAILED;
	}
	new_misc = (misc_info_t *) MALLOC(MISC_INFO_MAX_SIZE);
	if(!new_misc){
		PRINT_ERR("MALLOC new_misc failed\n");
		ret = -ENOMEM;
		goto MALLOC_NEW_MISC_FAILED;
	}
	memset(old_misc, 0, MISC_INFO_MAX_SIZE);
	memset(new_misc, 0, MISC_INFO_MAX_SIZE);

    head_length = sizeof(misc_info_head_t);
	new_head = &new_misc->head;
	new_data = (char *)new_misc + head_length;////!!!!(char *) is must, so easy to forget
	PRINT_DBG("new_head 0x%p, new_data 0x%p\n",new_head,new_data);

    pack_item = packet->item;
    pack_pos = sizeof(packet->length);
    PRINT_DBG("%s, line %d, length %d\n", __FUNCTION__, __LINE__, packet->length);

	//parse packet
    while(pack_pos < packet->length)
	{
		//check packet data completable
		if(count < pack_pos + sizeof(packet_item_t) + pack_item->size){
			PRINT_ERR("%s, line %d, pack_pos %d, sizeof(packet_item_t) %d, pack_item->size %d\n",
			__FUNCTION__, __LINE__, pack_pos, sizeof(packet_item_t), pack_item->size);
			break;
		}
		new_head->item_head[index].magic = pack_item->magic;
		memcpy(new_head->item_head[index].name, pack_item->name, strlen(pack_item->name));
		new_head->item_head[index].size = pack_item->size;
		new_head->item_head[index].offset = head_length + offset;//offset is from begining
		offset += new_head->item_head[index].size;
		new_head->item_head[index].chk_sum = get_checksum((unsigned short *)pack_item->data, pack_item->size/2);
		//copy
		memcpy(new_data, pack_item->data, new_head->item_head[index].size);
		new_data += new_head->item_head[index].size;

		item_size = sizeof(packet_item_t) + pack_item->size;
        pack_pos += item_size;
        pack_item = (packet_item_t *)((unsigned char *)pack_item + item_size);
		index++;
	}
	ret = pack_pos;
	PRINT_DBG("pack_pos %d\n", pack_pos);
	pack_item_num = index;

	//read misc info to merge with packet data
	if(read_storage((unsigned char*)old_misc, 0, MISC_INFO_MAX_SIZE) < 0){
		PRINT_ERR("readMiscInfo failed\n");
		ret = -1;
		goto OUT_END;
	}
	old_head = &old_misc->head;
	old_data = (char *)old_misc + head_length;////!!!!(char *) is must, so easy to forget
	if(old_head->magic != MISC_INFO_MAGIC || old_head->length > MISC_INFO_MAX_SIZE
		|| old_head->item_num > MISC_INFO_MAX_ITEM_NUM){
		PRINT_ERR("%s, line %d, magic 0x%x, length %d, item num %d\n", __FUNCTION__, __LINE__,
			old_head->magic, old_head->length, old_head->item_num);
		PRINT_ERR("invalid old misc info\n");
	}else{
		PRINT_DBG("%s, line %d, old misc info: length %d, item num %d\n", __func__, __LINE__, old_head->length, old_head->item_num);
		for(i=0; i<old_head->item_num; i++)
		{
			for(j=0; j<pack_item_num; j++)
			{
				if(memcmp(old_head->item_head[i].name, new_head->item_head[j].name, sizeof(old_head->item_head[i].name)) == 0){
					break;
				}
			}
			if(j < pack_item_num)
				continue;

			memcpy(&new_head->item_head[index], &old_head->item_head[i], sizeof(item_head_t));
			new_head->item_head[index].offset = head_length + offset;
			offset += new_head->item_head[index].size;

			//copy data
			//!!!old_misc->data is wrong
			old_data = (char *)old_misc + old_head->item_head[i].offset;
			memcpy(new_data, old_data, old_head->item_head[i].size);
			new_data += new_head->item_head[index].size;

			index++;
		}
	}

	//build head
	new_head->magic = MISC_INFO_MAGIC;
	new_head->item_num = index;
	new_head->length = head_length;
	for(i=0; i<new_head->item_num; i++){
		new_head->length += new_head->item_head[i].size;
	}
	PRINT_DBG("%s, line %d, new misc info: length %d, item num %d\n", __func__, __LINE__, new_head->length, new_head->item_num);

	//check total length
	if(new_head->length > MISC_INFO_MAX_SIZE){
		PRINT_ERR("new misc info length > MISC_INFO_MAX_SIZE, do nothing\n");
		dump_mem(new_misc, 0, new_head->length);
		ret = -1;
		goto OUT_END;
	}

	if(debug_enable)
		dump_mem(new_misc, 0, new_head->length);

	//write
	if(write_storage(new_misc, 0, new_head->length) < 0){
		PRINT_ERR("%s, line %d, write_storage failed\n", __FUNCTION__, __LINE__);
		ret = -1;
	}
	PRINT("%s, line %d, success\n", __func__, __LINE__);

OUT_END:
MALLOC_NEW_MISC_FAILED:
	if(new_misc){
		FREE(new_misc);
		new_misc = NULL;
	}
MALLOC_OLD_MISC_FAILED:
	if(old_misc){
		FREE(old_misc);
		old_misc = NULL;
	}

	return ret;
}

int format_misc_info(void)
{
	unsigned char *buf = NULL;

	PRINT_DBG("%s, line %d\n",__func__, __LINE__);
	buf = MALLOC(MISC_INFO_MAX_SIZE);
	if(!buf){
		PRINT_ERR("MALLOC buf failed\n");
		return -ENOMEM;
	}
	memset(buf, 0, MISC_INFO_MAX_SIZE);

	if(write_storage(buf, 0, MISC_INFO_MAX_SIZE) < 0){
		PRINT_ERR("%s, line %d, write_storage failed\n", __func__, __LINE__);
		return -1;
	}

	if(buf){
		FREE(buf);
		buf = NULL;
	}
	PRINT("%s, line %d, success\n", __func__, __LINE__);

	return 0;
}

#ifdef OS_LINUX

static int mi_open(struct inode *inode, struct file *filp)
{
	if(nonseekable_open(inode, filp))
	{
		PRINT_ERR ("misc nonseekable_open failed\n");
		return -1;
	}
    return 0;
}

static int mi_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static long mi_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = -1;
	ioctl_item_t item;
	char *buf = NULL;

	switch(cmd){
	case GET_ITEM_DATA:
		if(copy_from_user(&item, (void*)arg, sizeof(ioctl_item_t)) != 0){
			PRINT_ERR("copy_from_user failed\n");
			goto OUT_NULL;
		}
		PRINT_DBG("item.name %s, len %d\n", item.name, strlen(item.name));
		PRINT_DBG("item.size %d\n", item.size);
		PRINT_DBG("item.data 0x%p\n", item.data);

		buf = MALLOC(item.size);
		if(!buf){
			PRINT_ERR("MALLOC buf failed\n");
			ret = -ENOMEM;
			goto OUT_NULL;
		}
		ret = read_mi_item(item.name, buf, item.size);
		if(ret <= 0){
			PRINT_ERR("read_mi_item failed\n");
			goto OUT_FAILED;
		}
		PRINT_DBG("read_mi_item ret %d\n", ret);
		if(debug_enable)
			dump_mem(buf, 0, ret);
		if(copy_to_user(item.data, buf, ret) != 0){
			PRINT_ERR("copy_to_user failed\n");
			ret = -1;
			goto OUT_FAILED;
		}
		break;
	case SET_ITEM_DATA:
		if(copy_from_user(&item, (void*)arg, sizeof(ioctl_item_t)) != 0){
			PRINT_ERR("copy_from_user failed\n");
			goto OUT_NULL;
		}
		PRINT_DBG("item.name %s, len %d\n", item.name, strlen(item.name));
		PRINT_DBG("item.size %d\n", item.size);
		PRINT_DBG("item.data 0x%p\n", item.data);

		buf = MALLOC(item.size);
		if(!buf){
			PRINT_ERR("MALLOC buf failed\n");
			ret = -ENOMEM;
			goto OUT_NULL;
		}
		if(copy_from_user(buf, item.data, item.size) != 0){
			PRINT_ERR("copy_from_user failed\n");
			ret = -1;
			goto OUT_FAILED;
		}
		if(debug_enable)
			dump_mem(buf, 0, item.size);
		ret = write_mi_item(item.name, buf, item.size);
		if(ret < 0){
			PRINT_ERR("write_mi_item failed\n");
			goto OUT_FAILED;
		}
		break;
	case FORMAT_MISC_INFO:
		if(format_misc_info() < 0){
			PRINT_ERR("format misc info failed\n");
			ret = -1;
		}
		break;
	}
	return ret;

OUT_FAILED:
	if(buf){
		FREE(buf);
		buf = NULL;
	}
OUT_NULL:
	return ret;
}

static struct file_operations mi_fops =
{
    .owner   		= THIS_MODULE,
    .unlocked_ioctl = mi_ioctl,
    .open    		= mi_open,
    .release 		= mi_release
};

static struct miscdevice mi_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "misc_info",
    .fops = &mi_fops,
};

static int __init misc_info_init(void)
{
	PRINT("%s, line %d, %s %s\n", __FUNCTION__, __LINE__, __DATE__, __TIME__);

	if(misc_register(&mi_dev) < 0)
		return -1;
	mi_debug_init();

	return 0;
}

static void __exit misc_info_exit(void)
{
    PRINT("%s, line %d\n", __FUNCTION__, __LINE__);

	misc_deregister(&mi_dev);
    mi_debug_exit();
	return;
}

module_init(misc_info_init);
module_exit(misc_info_exit);

EXPORT_SYMBOL_GPL(read_misc_info);
EXPORT_SYMBOL_GPL(write_misc_info);
EXPORT_SYMBOL_GPL(format_misc_info);
EXPORT_SYMBOL_GPL(read_mi_item);
EXPORT_SYMBOL_GPL(write_mi_item);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alex.Sun");
MODULE_DESCRIPTION("MISC INFO access driver");

#endif
