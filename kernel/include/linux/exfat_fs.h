
#ifndef _LINUX_EXFAT_FS_H
#define _LINUX_EXFAT_FS_H

#include <linux/types.h>

#define EXFAT_OEM_ID		"EXFAT   "
#define BOOT_SIG		0xaa55

#define EXFAT_SB_BLOCKS		12
#define EXFAT_SB_DATA_BLOCKS	11
#define EXFAT_SB_CKSUM_BLOCK	11

/* State flags. */
#define EXFAT_SB_DIRTY		0x0002

struct exfat_boot_sector 
{
    __u8	ignored[3];             /* jump code */
    __u8	system_id[8];           /* Name - can be used to special case */
    __u8	reserved1[53];          /* reserved */
    __u64	partition_offset;       /* offset of MBR or offset of disk fist sector*/
    __u64	total_secs;             /* partition sectors */
    __u32	fat_offset;             /* FAT offset of dbr */
    __u32	fat_length;             /* fat length */
    __u32	fat_heap_offset;        /* offset of dbr */
    __u32	total_clusters;         /* total clusters of partition */
    __u32	root_cluster;           /* the first cluster of root dir */
    __u32	id;                     /* partition id */
    __u16	version;                /* fs version */
    __u16	flags;                  /*  */
    __u8	bytes_per_sec_shift;    /* sector size */
    __u8	secs_per_cluster_shift; /* cluster size */
    __u8	fats;                   /* number fat */
    __u8	driver;                 /* */
    __u8	percent;                /* dirty percent */
    __u8	reserved2[7];           
    __u8	reserved3[390];
    __u16	sign;                   /* 0x55 0xaa */
};


/* FAT entry size */
#define EXFAT_ENT_BITS		2
#define EXFAT_ENT_SIZE		(1 << EXFAT_ENT_BITS)

/* Start of data cluster's entry (number of reserved clusters) */
#define EXFAT_START_ENT		2

/* Values for FAT entry. */
#define EXFAT_ENT_FREE		0x00000000	/* free entry */
#define EXFAT_ENT_BAD		0xfffffff7	/* bad cluster */
#define EXFAT_ENT_EOF		0xffffffff	/* end of entry */

/* Chunk size in directory. */
#define EXFAT_CHUNK_BITS	5
#define EXFAT_CHUNK_SIZE	(1 << EXFAT_CHUNK_BITS)

/* Type of chunk in directory. */
#define EXFAT_TYPE_VALID	0x80
#define EXFAT_TYPE_SUBCHUNK	0x40

#define EXFAT_TYPE_EOD		0x00
#define __EXFAT_TYPE_BITMAP	0x01
#define EXFAT_TYPE_BITMAP	(__EXFAT_TYPE_BITMAP | EXFAT_TYPE_VALID)
#define __EXFAT_TYPE_UPCASE	0x02
#define EXFAT_TYPE_UPCASE	(__EXFAT_TYPE_UPCASE | EXFAT_TYPE_VALID)
#define __EXFAT_TYPE_LABEL	0x03
#define EXFAT_TYPE_LABEL	(__EXFAT_TYPE_LABEL | EXFAT_TYPE_VALID)
#define __EXFAT_TYPE_DIRENT	0x05
#define EXFAT_TYPE_DIRENT	(__EXFAT_TYPE_DIRENT | EXFAT_TYPE_VALID)

#define EXFAT_TYPE_GUID		0xA0
/* Type of sub chunk in directory. */
//Stream Extension Directory Entry
#define __EXFAT_TYPE_DATA	(0x00 | EXFAT_TYPE_SUBCHUNK)
#define EXFAT_TYPE_DATA		(__EXFAT_TYPE_DATA | EXFAT_TYPE_VALID)

#define __EXFAT_TYPE_NAME	(0x01 | EXFAT_TYPE_SUBCHUNK)
#define EXFAT_TYPE_NAME		(__EXFAT_TYPE_NAME | EXFAT_TYPE_VALID)

#define EXFAT_TYPE_VEDE		0xE0   //Vendor Extension Directory Entry
#define EXFAT_TYPE_VADE		0xE1   //Vendor Allocation Directory Entry



#define EXFAT_VALID_MODE (S_IFREG | S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO)

/* Chunk for free space bitmap. */
struct exfat_chunk_bitmap {
	__u8	type;			/* 0x01 */
	__u8	flag;
	__u8	reserved[18];		
	__le32	clusnr;			/* start cluster number of bitmap */
	__le64	size;			/* size of bitmap */
};

/* Chunk for upper-case table. */
struct exfat_chunk_upcase {
	__u8	type;			/* 0x02 */
	__u8	reserved01[3];		
	__le32	checksum;		/* checksum of upcase table */
	__u8	reserved02[12];		
	__le32	clusnr;			/* start cluster number of table */
	__le64	size;			/* size of table */
};

/* Maximum name length in a chunk. */
#define EXFAT_CHUNK_NAME_SIZE	15

/* Chunk for volume lable. */
struct exfat_chunk_label {
	__u8	type;			/* 0x03 */
	__u8	len;			/* length of volume lable */
	__u8	name[22];/* name of volume label */
	__u8    reserved[8];
};

/* Attribute flags. */
#define EXFAT_ATTR_RO		0x0001
#define EXFAT_ATTR_HIDDEN	0x0002
#define EXFAT_ATTR_SYSTEM	0x0004
#define EXFAT_ATTR_VOLUME	0x0008
#define EXFAT_ATTR_DIR		0x0010
#define EXFAT_ATTR_ARCH		0x0020

/* Chunk for directory entry. */
struct exfat_chunk_dirent {
	__u8	type;			/* 0x05 */
	__u8	sub_chunks;		/* number of sub chunks */
	__le16	checksum;		/* checksum of these chunks */
	__le16	attrib;			/* attribute flags */
	__u8	reserved01[2];		
	__le16	crtime, crdate;		/* creation time/date */
	__le16	mtime, mdate;		/* modification time/date */
	__le16	atime, adate;		/* access time/date */
	__u8	crtime_cs;		/* creation centi seconds time */
	__u8	mtime_cs;		/* modification centi seconds time */
	__u8	crtime_utc;
	__u8	mtime_utc;
	__u8	atime_utc;		/*Utc Offset*/
	__u8	reserved02[7];		
};

/* Flag for cluster number. */
#define EXFAT_DATA_NORMAL	0x01	/* clusnr == 0, or FAT has these
					 * cluster number */
#define EXFAT_DATA_CONTIGUOUS	0x03	/* clusters are contiguous (FAT may not
					 * be updated yet) */

/* Maximum filename length */
#define EXFAT_MAX_NAMELEN	255U

/* Sub chunk for file name info and data clusters info. */
struct exfat_chunk_data {
	__u8	type;			/* 0x40 */
	__u8	flag;			/* ??? (0x01 or 0x03) */
	__u8	reserved01;			
	__u8	name_len;		/* length of filename */
	__le16	hash;			/* hash of up-cased filename */
	__u8	reserved2[2];			
	__le64	size_valid;			
	__u8	reserved03[4];			
	__le32	clusnr;			/* start cluster number of data */
	__le64	size;			/* size of data */
};

/* Sub chunk for file name. */
struct exfat_chunk_name {
	__u8	type;			/* 0x41 */
	__u8	flag;			
	__u8	name[30];/* file name */
};

static inline __u32 __exfat_checksum32(__u32 sum, __u8 val)
{
	return ((sum << 31) | (sum >> 1)) + val;
}

static inline __u16 __exfat_checksum16(__u16 sum, __u8 val)
{
	return ((sum << 15) | (sum >> 1)) + val;
}

#endif /* !_LINUX_EXFAT_FS_H */
