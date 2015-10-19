/*
 * Configuration for Actions ATM7059 EVB.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_ATM7059_H
#define __CONFIG_ATM7059_H



#define CONFIG_EXTRA_ENV_SETTINGS			\
	"stdin=serial\0"				\
	"stdout=serial\0"				\
	"stderr=serial\0"				\
	"splashpos=m,m\0"				\
	"verify=yes\0"					\
	"loadaddr=0x7fc0\0"				\
	"ramdiskaddr=0x1ffffc0\0"			\
	"ft2test_addr=0x00300000\0"                     \
	"fdt_high=0xffffffff\0"				\
	"initrd_high=0xffffffff\0"			\
	"emmcpart=1:1\0"					\
	"mmcpart=0:1\0"					\
	"nandpart=0:1\0"					\
	"fdtaddr2=0x04000000\0"                     \
	"loaduimage=fatload ${devtype} ${devpart} ${loadaddr} uImage\0"	\
	"loadramdisk=fatload ${devtype} ${devpart} ${ramdiskaddr} ramdisk.img\0" \
	"loadfdt2=fatload ${devtype} ${devpart} ${fdtaddr2} kernel.dtb\0" \
	"loadft2test=fatload ${devtype} ${devpart} ${ft2test_addr} ft2_test.bin\0"	\
	"loadtestdata=fatload mmc ${mmcpart} 0x0 img_dat.dat\0"	\
	"mmcargs=setenv devtype mmc; setenv devpart ${mmcpart}\0"	\
	"emmcargs=setenv devtype mmc; setenv devpart ${emmcpart}\0"	\
	"nandargs=setenv devtype nand; setenv devpart ${nandpart}\0"	\
	"mboot=run loaduimage; run loadramdisk; run loadfdt2;"	\
		"bootm ${loadaddr} ${ramdiskaddr} ${fdtaddr2}\0"	\
	"mmcboot=echo boot from mmc card ...; "		\
		"run mmcargs; run mboot\0"				\
	"emmcboot=echo boot from emmc card ...; "		\
		"run emmcargs; run mboot\0"				\
	"nandboot=echo boot from nand card ...; "	\
		"run nandargs; run mboot\0"		\
	"ramboot=bootm ${loadaddr} ${ramdiskaddr} ${fdtaddr2}\0"	



#define CONFIG_SYS_HZ            1000


/*#define CONFIG_REMAKE_ELF*/

#define CONFIG_SYS_NO_FLASH



#define CONFIG_IDENT_STRING		    "ATM7059 EVB"

#define CONFIG_NR_DRAM_BANKS	1

#define CONFIG_SYS_SDRAM_BASE		0x0
/* reserve the last 16MB for secure environment */
#define CONFIG_SYS_SDRAM_SIZE		((512 - 16) * 1024 * 1024)


#define CONFIG_SYS_MALLOC_LEN		(32 * 1024 * 1024 + CONFIG_ENV_SIZE)

/* Link Definitions */
#define CONFIG_SYS_TEXT_BASE		0x08000040
#define CONFIG_SYS_LOAD_ADDR		(CONFIG_SYS_TEXT_BASE-0x40) 

#define CONFIG_SYS_INIT_SP_ADDR     (CONFIG_SYS_LOAD_ADDR + 0x7fff0)

/* Flat Device Tree Definitions */
#define CONFIG_OF_LIBFDT

#define CONFIG_SYS_GENERIC_BOARD

#define CONFIG_OWL

#define CONFIG_OWL_SERIAL
#define CONFIG_BAUDRATE			115200

#define CPU_RELEASE_ADDR		0xe0228050

#define CONFIG_CMD_BOOTI
#define CONFIG_CMD_BOOTZ
#define CONFIG_CMD_MEMORY
#define CONFIG_CMD_RUN
#define CONFIG_ANDROID_BOOT_IMAGE

#define CONFIG_BOOTCOMMAND		"run ramboot;"
#define CONFIG_BOOTDELAY		0	/* autoboot after 0 seconds */ 

/* MMC */

#define CONFIG_CMD_MMC	
#define CONFIG_MMC			1
#define CONFIG_GENERIC_MMC		1
#define CONFIG_OWL_MMC		1
#define CONFIG_MISC_INFO	1


#define CONFIG_ATM7059_NAND
#define CONFIG_SYS_VSNPRINTF
#define CONFIG_CMD_ECHO

/*
 * Common filesystems support.  When we have removable storage we
 * enabled a number of useful commands and support.
 */
#if defined(CONFIG_MMC) || defined(CONFIG_USB_STORAGE) || defined(CONFIG_ATM9009_NAND)
#define CONFIG_DOS_PARTITION
#define CONFIG_CMD_FAT
#define CONFIG_FAT_WRITE
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_EXT4
#define CONFIG_CMD_FS_GENERIC
#define CONFIG_EFI_PARTITION
#define CONFIG_PARTITION_UUIDS
#endif

/* smc call test */
#define CONFIG_CMD_SMC		1

/* Generic Timer Definitions */
#define COUNTER_FREQUENCY		(0x1800000)	/* 24MHz */

/* Generic Interrupt Controller Definitions */
#define GICD_BASE			(0xe00f1000)
#define GICC_BASE			(0xe00f2000)


/* Do not preserve environment */
#define CONFIG_ENV_IS_NOWHERE		1
#define CONFIG_ENV_SIZE			0x1000


/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE		512	/* Console I/O Buffer Size */
#define CONFIG_SYS_PROMPT		"owl> "
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE
#define CONFIG_SYS_LONGHELP
#define CONFIG_SYS_MAXARGS		16	/* max command args */

/*======usb dwc3 gadge support=====*/
#define CONFIG_SYS_CACHELINE_SIZE	64

#define	CONFIG_USB_GADGET

#define	CONFIG_USB_GADGET_ACTIONS
#define CONFIG_USB_GADGET_DUALSPEED
#define CONFIG_USB_GADGET_VBUS_DRAW 400
#define CONFIG_USBD_HS

#define	CONFIG_USBDOWNLOAD_GADGET
#define CONFIG_G_DNL_VENDOR_NUM 0x18d1
#define CONFIG_G_DNL_PRODUCT_NUM 0x0c02
#define CONFIG_G_DNL_UMS_VENDOR_NUM 0x10d6
#define CONFIG_G_DNL_UMS_PRODUCT_NUM	0x0C02
#define CONFIG_G_DNL_MANUFACTURER "Actions-Semi"

/* function: gaget mass storage */
#define	CONFIG_USB_GADGET_MASS_STORAGE
#define	CONFIG_CMD_USB_MASS_STORAGE

/* function: fastboot */
#define	CONFIG_CMD_FASTBOOT
#define	CONFIG_USB_FASTBOOT_BUF_SIZE (1024*1024*700)
#define	CONFIG_USB_FASTBOOT_BUF_ADDR 0x100

/* function: usb_ether */
#define CONFIG_USB_ETHER
#define CONFIG_USB_ETH_RNDIS
#define CONFIG_CMD_NET
#define CONFIG_CMD_PING

#ifndef CONFIG_IPADDR
#define CONFIG_IPADDR			192.168.90.246
#endif

#ifndef	CONFIG_SERVERIP
#define CONFIG_SERVERIP			192.168.90.103
#endif

#define CONFIG_USBNET_DEV_ADDR		"de:ad:be:ef:00:01"
#define CONFIG_USBNET_HOST_ADDR	"de:ad:be:ef:00:02"

#endif /* __CONFIG_S500_EVB_H */
