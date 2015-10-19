/* vi: set sw=4 ts=4: */
/*
 * mkfs_vfat: utility to create FAT32 filesystem
 * inspired by dosfstools
 *
 * Busybox'ed (2009) by Vladimir Dronnikov <dronnikov@gmail.com>
 *
 * Licensed under GPLv2, see file LICENSE in this tarball for details.
 */


/*
#ifndef __USE_FILE_OFFSET64
#define __USE_FILE_OFFSET64
#endif

#ifndef __USE_LARGEFILE64
#define __USE_LARGEFILE64
#endif
*/


#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#include <unistd.h>

#include <linux/hdreg.h> /* HDIO_GETGEO */
#include <linux/fd.h>    /* FDGETPRM */
#include <linux/fs.h>   

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <utime.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>



#define SECTOR_SIZE             512
//#define BLOCK_SIZE		     1024

//#define SECTORS_PER_BLOCK	(BLOCK_SIZE / SECTOR_SIZE)

// M$ says the high 4 bits of a FAT32 FAT entry are reserved
#define EOF_FAT32       0x0FFFFFF8
#define BAD_FAT32       0x0FFFFFF7
#define MAX_CLUST_32    0x0FFFFFF0

#define ATTR_VOLUME     8

#define	NUM_FATS        2



#define cpu_to_le32(x)  x
#define cpu_to_le16(x)  x

typedef unsigned int uint32_t;
typedef unsigned short int uint16_t;
typedef unsigned char uint8_t;

enum {
	info_sector_number = 1,
	backup_boot_sector = 3,
	reserved_sect      = 6,
};

// how many blocks we try to read while testing
#define TEST_BUFFER_BLOCKS      16

struct msdos_dir_entry {
	char     name[11];       /* 000 name and extension */
	uint8_t  attr;           /* 00b attribute bits */
	uint8_t  lcase;          /* 00c case for base and extension */
	uint8_t  ctime_cs;       /* 00d creation time, centiseconds (0-199) */
	uint16_t ctime;          /* 00e creation time */
	uint16_t cdate;          /* 010 creation date */
	uint16_t adate;          /* 012 last access date */
	uint16_t starthi;        /* 014 high 16 bits of cluster in FAT32 */
	uint16_t time;           /* 016 time */
	uint16_t date;           /* 018 date */
	uint16_t start;          /* 01a first cluster */
	uint32_t size;           /* 01c file size in bytes */
} __attribute__ ((packed));

/* Example of boot sector's beginning:
0000  eb 58 90 4d 53 57 49 4e  34 2e 31 00 02 08 26 00  |...MSWIN4.1...&.|
0010  02 00 00 00 00 f8 00 00  3f 00 ff 00 3f 00 00 00  |........?...?...|
0020  54 9b d0 00 0d 34 00 00  00 00 00 00 02 00 00 00  |T....4..........|
0030  01 00 06 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
0040  80 00 29 71 df 51 e0 4e  4f 20 4e 41 4d 45 20 20  |..)q.Q.NO NAME  |
0050  20 20 46 41 54 33 32 20  20 20 33 c9 8e d1 bc f4  |  FAT32   3.....|
*/
struct msdos_volume_info { /* (offsets are relative to start of boot sector) */
	uint8_t  drive_number;    /* 040 BIOS drive number */
	uint8_t  reserved;        /* 041 unused */
	uint8_t  ext_boot_sign;	  /* 042 0x29 if fields below exist (DOS 3.3+) */
	uint32_t volume_id32;     /* 043 volume ID number */
	char     volume_label[11];/* 047 volume label */
	char     fs_type[8];      /* 052 typically "FATnn" */
} __attribute__ ((packed));       /* 05a end. Total size 26 (0x1a) bytes */

struct msdos_boot_sector {
	char     boot_jump[3];       /* 000 short or near jump instruction */
	char     system_id[8];       /* 003 name - can be used to special case partition manager volumes */
	uint16_t bytes_per_sect;     /* 00b bytes per logical sector */
	uint8_t  sect_per_clust;     /* 00d sectors/cluster */
	uint16_t reserved_sect;      /* 00e reserved sectors (sector offset of 1st FAT relative to volume start) */
	uint8_t  fats;               /* 010 number of FATs */
	uint16_t dir_entries;        /* 011 root directory entries */
	uint16_t volume_size_sect;   /* 013 volume size in sectors */
	uint8_t  media_byte;         /* 015 media code */
	uint16_t sect_per_fat;       /* 016 sectors/FAT */
	uint16_t sect_per_track;     /* 018 sectors per track */
	uint16_t heads;              /* 01a number of heads */
	uint32_t hidden;             /* 01c hidden sectors (sector offset of volume within physical disk) */
	uint32_t fat32_volume_size_sect; /* 020 volume size in sectors (if volume_size_sect == 0) */
	uint32_t fat32_sect_per_fat; /* 024 sectors/FAT */
	uint16_t fat32_flags;        /* 028 bit 8: fat mirroring, low 4: active fat */
	uint8_t  fat32_version[2];   /* 02a major, minor filesystem version (I see 0,0) */
	uint32_t fat32_root_cluster; /* 02c first cluster in root directory */
	uint16_t fat32_info_sector;  /* 030 filesystem info sector (usually 1) */
	uint16_t fat32_backup_boot;  /* 032 backup boot sector (usually 6) */
	uint32_t reserved2[3];       /* 034 unused */
	struct msdos_volume_info vi; /* 040 */
	char     boot_code[0x200 - 0x5a - 2]; /* 05a */
#define BOOT_SIGN 0xAA55
	uint16_t boot_sign;          /* 1fe */
} __attribute__ ((packed));

#define FAT_FSINFO_SIG1 0x41615252
#define FAT_FSINFO_SIG2 0x61417272
struct fat32_fsinfo {
	uint32_t signature1;         /* 0x52,0x52,0x41,0x61, "RRaA" */
	uint32_t reserved1[128 - 8];
	uint32_t signature2;         /* 0x72,0x72,0x61,0x41, "rrAa" */
	uint32_t free_clusters;      /* free cluster count.  -1 if unknown */
	uint32_t next_cluster;       /* most recently allocated cluster */
	uint32_t reserved2[3];
	uint16_t reserved3;          /* 1fc */
	uint16_t boot_sign;          /* 1fe */
} __attribute__ ((packed));

struct bug_check {
	char BUG1[sizeof(struct msdos_dir_entry  ) == 0x20 ? 1 : -1];
	char BUG2[sizeof(struct msdos_volume_info) == 0x1a ? 1 : -1];
	char BUG3[sizeof(struct msdos_boot_sector) == 0x200 ? 1 : -1];
	char BUG4[sizeof(struct fat32_fsinfo     ) == 0x200 ? 1 : -1];
};

static const char boot_code[]  =
	"\x0e"          /* 05a:         push  cs */
	"\x1f"          /* 05b:         pop   ds */
	"\xbe\x77\x7c"  /*  write_msg:  mov   si, offset message_txt */
	"\xac"          /* 05f:         lodsb */
	"\x22\xc0"      /* 060:         and   al, al */
	"\x74\x0b"      /* 062:         jz    key_press */
	"\x56"          /* 064:         push  si */
	"\xb4\x0e"      /* 065:         mov   ah, 0eh */
	"\xbb\x07\x00"  /* 067:         mov   bx, 0007h */
	"\xcd\x10"      /* 06a:         int   10h */
	"\x5e"          /* 06c:         pop   si */
	"\xeb\xf0"      /* 06d:         jmp   write_msg */
	"\x32\xe4"      /*  key_press:  xor   ah, ah */
	"\xcd\x16"      /* 071:         int   16h */
	"\xcd\x19"      /* 073:         int   19h */
	"\xeb\xfe"      /*  foo:        jmp   foo */
	/* 077: message_txt: */
	"This is not a bootable disk\r\n";


#define MARK_CLUSTER(cluster, value) \
	((uint32_t *)fat)[cluster] = cpu_to_le32(value)


#define STORE_LE(field, value) \
do { \
	if (sizeof(field) == 4) \
		field = cpu_to_le32(value); \
	else if (sizeof(field) == 2) \
		field = cpu_to_le16(value); \
	else if (sizeof(field) == 1) \
		field = (value); \
	else \
		printf("field fail\n"); \
} while (0)



int mkfs_vfat(const char * bpath)
{
	int dev;
	struct stat st;
	const char *volume_label = "NONAME";
	char *buf;
	const char *device_name;
	off64_t volume_size_bytes;
	off64_t volume_size_sect;
	uint32_t total_clust;
	uint32_t volume_id;
	unsigned bytes_per_sect;
	unsigned sect_per_fat;
  	unsigned opts;
	uint16_t sect_per_track;
	uint8_t media_byte;
	uint8_t sect_per_clust;
	uint8_t heads;
	int act_reserve = 64;
	int reserve_rest;
    int secters_rest;

	// cache device name
	device_name = bpath;
	volume_id = time(NULL);

	dev = open(device_name,  O_RDWR|O_LARGEFILE, 0666);
	if ( dev < 0 ) {
		printf("mkfs_vfat: open %s fail\n", device_name);
		return -1;
	}
	
	if (fstat(dev, &st) < 0) {
		printf("mkfs_vfat: fstat fail\n");
		return -1;
	}	

	//
	// Get image size and sector size
	//
	volume_size_bytes = st.st_size;
	
	printf("2.volume size=%lld b\n", volume_size_bytes);
	bytes_per_sect = SECTOR_SIZE;
	
	if (!S_ISBLK(st.st_mode)) {
		printf("mkfs_vfat: not block dev\n");
		return -1;
	} else {
		int min_bytes_per_sect;
		// more portable than BLKGETSIZE[64]
		volume_size_bytes = lseek64(dev, 0, SEEK_END);
		if ( volume_size_bytes == (off64_t)-1 ) {
			printf("mkfs_vfat: lseek  fail,err=%s\n",strerror(errno));
			return -1;
		}
		printf("volume size=%lld b\n", volume_size_bytes);
		lseek64(dev, 0, SEEK_SET);
		// get true sector size
		// (parameter must be int*, not long* or size_t*)
		ioctl(dev, BLKSSZGET, &min_bytes_per_sect);
		if (min_bytes_per_sect > SECTOR_SIZE) {
			bytes_per_sect = min_bytes_per_sect;
			printf("for this device sector size is %d", min_bytes_per_sect);
		}
	}

	volume_size_sect = volume_size_bytes / bytes_per_sect;
	printf("volume_size_sect=%lld\n", volume_size_sect);

	media_byte = 0xf8;
	heads = 255;
	sect_per_track = 63;
	sect_per_clust = 1;
	{
		struct hd_geometry geometry;
		// size (in sectors), sect (per track), head
		struct floppy_struct param;

		// N.B. whether to use HDIO_GETGEO or HDIO_REQ?
		if (ioctl(dev, HDIO_GETGEO, &geometry) == 0 && geometry.sectors && geometry.heads ) {
			// hard drive
			sect_per_track = geometry.sectors;
			heads = geometry.heads;
			printf("HDIO_GETGEO\n");

 set_cluster_size:
			/* For FAT32, try to do the same as M$'s format command
			 * (see http://www.win.tue.nl/~aeb/linux/fs/fat/fatgen103.pdf p. 20):
			 * fs size <= 260M: 0.5k clusters
			 * fs size <=   8G: 4k clusters
			 * fs size <=  16G: 8k clusters
			 * fs size >   16G: 16k clusters
			 */
			sect_per_clust = 1;
			if (volume_size_bytes >= 260*1024*1024) {
				sect_per_clust = 8;		
				/* fight gcc: */
				/* "error: integer overflow in expression" */
				/* "error: right shift count >= width of type" */
				//if (sizeof(off_t) > 4) {
				#if 1
				unsigned t = (volume_size_bytes >> 31 >> 1);
				if (t >= 8/4)
					sect_per_clust = 16;
				if (t >= 16/4)
					sect_per_clust = 32;
				#endif
				//}
			}
		} else {
			printf("not_floppy\n");
			// floppy, loop, or regular file
			int not_floppy = ioctl(dev, FDGETPRM, &param);
			if (not_floppy == 0) {
				// floppy disk
				sect_per_track = param.sect;
				heads = param.head;
				volume_size_sect = param.size;
				volume_size_bytes = param.size * SECTOR_SIZE;
				printf("2.volume_size_sect=%d, volume size=%d kb\n", (int)volume_size_sect, (int)(volume_size_bytes/1024));
			}
			// setup the media descriptor byte
			switch (volume_size_sect) {
			case 2*360:	// 5.25", 2, 9, 40 - 360K
				media_byte = 0xfd;
				break;
			case 2*720:	// 3.5", 2, 9, 80 - 720K
			case 2*1200:	// 5.25", 2, 15, 80 - 1200K
				media_byte = 0xf9;
				break;
			default:	// anything else
				if (not_floppy)
					goto set_cluster_size;
			case 2*1440:	// 3.5", 2, 18, 80 - 1440K
			case 2*2880:	// 3.5", 2, 36, 80 - 2880K
				media_byte = 0xf0;
				break;
			}
			// not floppy, but size matches floppy exactly.
			// perhaps it is a floppy image.
			// we already set media_byte as if it is a floppy,
			// now set sect_per_track and heads.
			heads = 2;
			sect_per_track = (unsigned)volume_size_sect / 160;
			if (sect_per_track < 9)
				sect_per_track = 9;
		}
	}
	printf("2.volume_size_sect=%d \n", (int)(volume_size_sect));

	//
	// Calculate number of clusters, sectors/cluster, sectors/FAT
	// (an initial guess for sect_per_clust should already be set)
	//
	// "mkdosfs -v -F 32 image5k 5" is the minimum:
	// 2 sectors for FATs and 2 data sectors
	//if ((off_t)(volume_size_sect - reserved_sect) < 4) //wanghao
	if ((off64_t)(volume_size_sect - act_reserve) < 4) {    
		printf("the image is too small for FAT32");
		return -1;
	}
	sect_per_fat = 1;
	while (1) {
		while (1) {
			int spf_adj;
			//off_t tcl = (volume_size_sect - reserved_sect - NUM_FATS * sect_per_fat) / sect_per_clust; //wanghao
			off_t tcl = (volume_size_sect - act_reserve - NUM_FATS * sect_per_fat) / sect_per_clust;
			// tcl may be > MAX_CLUST_32 here, but it may be
			// because sect_per_fat is underestimated,
			// and with increased sect_per_fat it still may become
			// <= MAX_CLUST_32. Therefore, we do not check
			// against MAX_CLUST_32, but against a bigger const:
			if (tcl > 0x7fffffff)
				goto next;
			total_clust = tcl; // fits in uint32_t
			spf_adj = ((total_clust+2) + (bytes_per_sect/4)-1) / (bytes_per_sect/4) - sect_per_fat;

			if (spf_adj <= 0) {
				// do not need to adjust sect_per_fat.
				// so, was total_clust too big after all?
				if (total_clust <= MAX_CLUST_32)
					goto found_total_clust; // no
				// yes, total_clust is _a bit_ too big
				goto next;
			}
			// adjust sect_per_fat, go back and recalc total_clust
			// (note: just "sect_per_fat += spf_adj" isn't ok)
			sect_per_fat += ((unsigned)spf_adj / 2) | 1;
		}
 next:
		if (sect_per_clust == 128)
			printf("can't make FAT32 with >128 sectors/cluster");
		sect_per_clust *= 2;
		sect_per_fat = (sect_per_fat / 2) | 1;
	}
 found_total_clust:

	//
	// Print info
	//

	printf("Device '%s':\n"
		"heads:%d, sectors/track:%d, bytes/sector:%d\n"
		"media descriptor:%02x\n"
		"total sectors:%u, clusters:%d, sectors/cluster:%d\n"
		"FATs:2, sectors/FAT:%d\n"
		"volumeID:%08x, label:'%s'\n",
		device_name,
		heads, sect_per_track, bytes_per_sect,
		(int)media_byte,
		(int)volume_size_sect, (int)total_clust, (int)sect_per_clust,
		sect_per_fat,
		(int)volume_id, volume_label
	);

    printf("%s, %d, sect_per_fat=%d\n", __FUNCTION__, __LINE__, sect_per_fat);
    
    act_reserve = sect_per_fat + sect_per_fat;
    if ( (act_reserve & 0x1f) != 0 )
    {
        act_reserve = 64 - (act_reserve & 0x1f);
    }
    else
    {
        act_reserve = 32;
    }
    // check reserved_res + free_sects > sect_per_clust, if so, send them all to the free sects to avoid fschk fail; yuyonghui.
    reserve_rest = 64 - act_reserve;
    secters_rest = (volume_size_sect - 64 - sect_per_fat * 2 + sect_per_clust * 2) % sect_per_clust; //fat32_root_cluster
    if ((reserve_rest + secters_rest) >= sect_per_clust)
    {
        volume_size_sect -= reserve_rest;
    }
    printf("%s, %d, volume_size_sect%lld, act_reserve=%d, secters_rest=%d\n", __FUNCTION__, __LINE__, volume_size_sect, act_reserve, secters_rest);
    
	//
	// Write filesystem image sequentially (no seeking)
	//
	{
		// (a | b) is poor man's max(a, b)
		//unsigned bufsize = reserved_sect; //wanghao
		unsigned bufsize = act_reserve;
		//bufsize |= sect_per_fat; // can be quite large
		bufsize |= 2; // use this instead
		bufsize |= sect_per_clust;
		buf = (char*)malloc(bufsize * bytes_per_sect);			
		memset(buf, 0, bufsize * bytes_per_sect);
	}

	{ // boot and fsinfo sectors, and their copies
		struct msdos_boot_sector *boot_blk = (void*)buf;
		struct fat32_fsinfo *info = (void*)(buf + bytes_per_sect);

		strncpy(boot_blk->boot_jump, "\xeb\x58\x90", 3);		
		strncpy(boot_blk->system_id, "mkdosfs", 8);
		//strcpy(boot_blk->boot_jump, "\xeb\x58\x90" "mkdosfs"); // system_id[8] included :)
		STORE_LE(boot_blk->bytes_per_sect, bytes_per_sect);
		STORE_LE(boot_blk->sect_per_clust, sect_per_clust);
		//STORE_LE(boot_blk->reserved_sect, reserved_sect); //wanghao
		STORE_LE(boot_blk->reserved_sect, act_reserve);
		STORE_LE(boot_blk->fats, 2);
		//STORE_LE(boot_blk->dir_entries, 0); // for FAT32, stays 0		
		if (volume_size_sect <= 0xffff)
			STORE_LE(boot_blk->volume_size_sect, volume_size_sect);
		STORE_LE(boot_blk->media_byte, media_byte);
		// wrong: this would make Linux think that it's fat12/16:
		//if (sect_per_fat <= 0xffff)
		//	STORE_LE(boot_blk->sect_per_fat, sect_per_fat);
		// works:
		//STORE_LE(boot_blk->sect_per_fat, 0);
		STORE_LE(boot_blk->sect_per_track, sect_per_track);
		STORE_LE(boot_blk->heads, heads);
		//STORE_LE(boot_blk->hidden, 0);
		STORE_LE(boot_blk->fat32_volume_size_sect, volume_size_sect);
		STORE_LE(boot_blk->fat32_sect_per_fat, sect_per_fat);
		//STORE_LE(boot_blk->fat32_flags, 0);
		//STORE_LE(boot_blk->fat32_version[2], 0,0);
		STORE_LE(boot_blk->fat32_root_cluster, 2);
		STORE_LE(boot_blk->fat32_info_sector, info_sector_number);
		STORE_LE(boot_blk->fat32_backup_boot, backup_boot_sector);
		//STORE_LE(boot_blk->reserved2[3], 0,0,0);
		STORE_LE(boot_blk->vi.ext_boot_sign, 0x29);
		STORE_LE(boot_blk->vi.volume_id32, volume_id);
		strncpy(boot_blk->vi.fs_type, "FAT32   ", sizeof(boot_blk->vi.fs_type));
		strncpy(boot_blk->vi.volume_label, volume_label, sizeof(boot_blk->vi.volume_label));
		memcpy(boot_blk->boot_code, boot_code, sizeof(boot_code));
		STORE_LE(boot_blk->boot_sign, BOOT_SIGN);

		STORE_LE(info->signature1, FAT_FSINFO_SIG1);
		STORE_LE(info->signature2, FAT_FSINFO_SIG2);
		// we've allocated cluster 2 for the root dir
		STORE_LE(info->free_clusters, (total_clust - 1));
		STORE_LE(info->next_cluster, 2);
		STORE_LE(info->boot_sign, BOOT_SIGN);

		// 1st copy
		write(dev, buf, bytes_per_sect * backup_boot_sector);
		// 2nd copy and possibly zero sectors
		//xwrite(dev, buf, bytes_per_sect * (reserved_sect - backup_boot_sector)); //wanghao
		write(dev, buf, bytes_per_sect * (act_reserve - backup_boot_sector));
	}

	{ // file allocation tables
		unsigned i,j;
		unsigned char *fat = (void*)buf;

		memset(buf, 0, bytes_per_sect * 2);
		// initial FAT entries
		MARK_CLUSTER(0, 0x0fffff00 | media_byte);
		MARK_CLUSTER(1, 0xffffffff);
		// mark cluster 2 as EOF (used for root dir)
		MARK_CLUSTER(2, EOF_FAT32);
		for (i = 0; i < NUM_FATS; i++) {
			write(dev, buf, bytes_per_sect);
			for (j = 1; j < sect_per_fat; j++)
				write(dev, buf + bytes_per_sect, bytes_per_sect);
		}
	}
	// root directory
	// empty directory is just a set of zero bytes
	memset(buf, 0, sect_per_clust * bytes_per_sect);
	if (volume_label[0]) {
		// create dir entry for volume_label
		struct msdos_dir_entry *de;

		de = (void*)buf;		
		memset(de->name, 0, sizeof(de->name));
		memcpy(de->name, volume_label, 6);
		//strncpy(de->name, volume_label, sizeof(de->name)-1);
		STORE_LE(de->attr, ATTR_VOLUME);

	}
	write(dev, buf, sect_per_clust * bytes_per_sect);

	free(buf);
	close(dev);
	printf("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}


	
