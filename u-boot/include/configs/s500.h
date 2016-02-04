/*
 * Configuration for Actions S500.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_S500_H
#define __CONFIG_S500_H

#define CONFIG_BOOTCOMMAND "run ramboot;"

#define CONFIG_EXTRA_ENV_SETTINGS			\
	"stdin=serial\0"				\
	"stdout=serial\0"				\
	"stderr=serial\0"				\
	"splashpos=m,m\0"				\
	"verify=yes\0"					\
	"bootpart=1\0"					\
	"emmcdev=1\0"					\
	"mmcdev=0\0"					\
	"nanddev=0\0"					\
	"kernel_addr_r=0x7fc0\0"				\
	"ramdisk_addr_r=0x1ffffc0\0"			\
	"fdt_addr_r=0x04000000\0"                     \
	"fdt_high=0xffffffff\0"				\
	"initrd_high=0xffffffff\0"			\
	"scriptaddr=0x04400000\0" \
	"bootenv=uEnv.txt\0" \
	"bootscr=boot.scr\0" \
	"console=ttyS2,115200\0" \
	"loglevel=7\0" \
	"loadkernel=fatload ${devtype} ${devpart} ${kernel_addr_r} uImage\0"	\
	"loadramdisk=fatload ${devtype} ${devpart} ${ramdisk_addr_r} ramdisk.img\0" \
	"loadfdt=fatload ${devtype} ${devpart} ${fdt_addr_r} kernel.dtb\0" \
	"loadbootscr=fatload ${devtype} ${devpart} ${scriptaddr} ${bootscr} \0" \
	"loadbootenv=fatload ${devtype} ${devpart} ${scriptaddr} ${bootenv} \0" \
	"mmcargs=setenv devtype mmc; setenv devpart ${mmcdev}:${bootpart}\0" \
	"emmcargs=setenv devtype mmc; setenv devpart ${emmcdev}:${bootpart}\0"	\
	"nandargs=setenv devtype nand; setenv devpart ${nanddev}:${bootpart}\0" \
	"mboot=run loadkernel; run loadramdisk; run loadfdt;" \
		"bootm ${kernel_addr_r} ${ramdisk_addr_r} ${fdt_addr_r}\0"	\
	"setbootenv=" \
		"if run loadbootenv; then " \
			"echo Loaded environment from ${bootenv};" \
			"env import -t ${scriptaddr} ${filesize};" \
		"fi;" \
		"if test -n \\\"${uenvcmd}\\\"; then " \
			"echo Running uenvcmd ...;" \
			"run uenvcmd;" \
		"fi;" \
		"if run loadbootscr; then " \
			"echo Jumping to ${bootscr};" \
			"source ${scriptaddr};" \
		"fi;" \
		"run mboot;\0" \
	"mmcboot=echo boot from mmc card ...; "		\
		"run mmcargs; run setbootenv\0"				\
	"emmcboot=echo boot from emmc card ...; "		\
		"run emmcargs; run setbootenv\0"				\
	"nandboot=echo boot from nand card ...; "	\
		"run nandargs; run setbootenv\0"		\
	"ramboot=bootm ${kernel_addr_r} ${ramdisk_addr_r} ${fdt_addr_r}\0"


#define CONFIG_SYS_HZ            1000

#define CONFIG_SYS_NO_FLASH

#define CONFIG_IDENT_STRING		    "Actions-Semi"

#define CONFIG_NR_DRAM_BANKS	1

#define CONFIG_SYS_SDRAM_BASE		0x0
/* reserve the last 16MB for secure environment */
#define CONFIG_SYS_SDRAM_SIZE		((512 - 16) * 1024 * 1024)


#define CONFIG_SYS_MALLOC_LEN		(32 * 1024 * 1024 + CONFIG_ENV_SIZE)

/* Link Definitions */
#define CONFIG_SYS_TEXT_BASE		0x08000040
#define CONFIG_SYS_LOAD_ADDR		0x08000000
#define CONFIG_SYS_INIT_SP_ADDR     (CONFIG_SYS_LOAD_ADDR + 0x7fff0)

/* Flat Device Tree Definitions */
#define CONFIG_OF_LIBFDT
#define CONFIG_FIT

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

#define CONFIG_BOOTDELAY		2	/* autoboot after 2 seconds */ 
#define CONFIG_AUTOBOOT
#define CONFIG_AUTOBOOT_KEYED
#define CONFIG_AUTOBOOT_DELAY_STR	"s500"
#define CONFIG_AUTOBOOT_PROMPT		\
	"Enter %s to abort autoboot in %d seconds\n", CONFIG_AUTOBOOT_DELAY_STR, bootdelay

/* MMC */
#define CONFIG_CMD_MMC	
#define CONFIG_MMC			1
#define CONFIG_GENERIC_MMC	1
#define CONFIG_OWL_MMC		1
#define CONFIG_MISC_INFO	1

#define CONFIG_SYS_VSNPRINTF
#define CONFIG_CMD_ECHO


#define CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ	(4 * 1024)
#define CONFIG_STACKSIZE_FIQ	(1 * 1024)


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

#define CONFIG_OWL_SPI


#define CONFIG_ENV_IS_IN_MMC

/* Do not preserve environment */
#if defined(CONFIG_ENV_IS_IN_MMC) && defined(CONFIG_MMC)
#define CONFIG_SYS_MMC_ENV_DEV		1	/* SLOT2: eMMC(1) */
#define CONFIG_ENV_SIZE			(128 << 10)
#define CONFIG_ENV_OFFSET		0x5000
#define CONFIG_ENV_OFFSET_REDUND	(CONFIG_ENV_OFFSET + CONFIG_ENV_SIZE)
#define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#elif defined(CONFIG_ENV_IS_IN_FAT)
#define CONFIG_ENV_SIZE			(128 << 10)
#define FAT_ENV_INTERFACE		"mmc"
#define FAT_ENV_DEVICE_AND_PART	"1:1"
#define FAT_ENV_FILE			"uboot.env"
#elif defined(CONFIG_ENV_IS_IN_SPI_FLASH)
#define CONFIG_ENV_OFFSET		0x140000
#define CONFIG_ENV_SIZE			0x2000
#define CONFIG_ENV_SECT_SIZE	0x10000
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_WINBOND
#define CONFIG_SPI_FLASH_MACRONIX
#define CONFIG_SF_DEFAULT_SPEED        25000000
#define CONFIG_SPI_FLASH_SIZE          (2 << 20)
#define CONFIG_SPI_BUS                 2
#define CONFIG_SPI_CS                  0
#define CONFIG_CMD_SF
#else
#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_SIZE			0x1000
#endif

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

#define CONFIG_ATM7059A
#define CONFIG_OWL_PINCTRL

#define CONFIG_OWL_GPIO_GENERIC
#define CONFIG_OWL_GPIO

#define CONFIG_SYS_I2C_OWL

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
#define CONFIG_FASTBOOT_FLASH
#define CONFIG_FASTBOOT_FLASH_MMC_DEV  1
#define CONFIG_BOOTLOADER_ADDR         0x200200
#define CONFIG_UBOOT_ADDR              0x300000
/* function: usb_ether */
#define CONFIG_USB_ETHER
#define CONFIG_USB_ETH_RNDIS
#define CONFIG_CMD_NET
#define CONFIG_CMD_PING

/* function:ethernet */
#define CONFIG_ACTS_OWL_MAC
#define CONFIG_LIB_RAND

#ifndef CONFIG_IPADDR
#define CONFIG_IPADDR			192.168.90.246
#endif

#ifndef	CONFIG_SERVERIP
#define CONFIG_SERVERIP			192.168.90.103
#endif

#define CONFIG_USBNET_DEV_ADDR		"de:ad:be:ef:00:01"
#define CONFIG_USBNET_HOST_ADDR	"de:ad:be:ef:00:02"


/* USB Host*/
#define CONFIG_USB_XHCI
#define CONFIG_USB_XHCI_OWL

#define CONFIG_CMD_USB
#define CONFIG_USB_STORAGE
#define CONFIG_SYS_USB_XHCI_MAX_ROOT_PORTS     2


#define CONFIG_ANDROID_RECOVERY

#if 1
#define CONFIG_OWL_PWM
#define CONFIG_OWL_PWM_BACKLIGHT
#define CONFIG_VIDEO
#define CONFIG_VIDEO_OWL
#define CONFIG_CFB_CONSOLE
#define CONFIG_VGA_AS_SINGLE_DEVICE
#define CONFIG_VIDEO_LOGO
#define CONFIG_SPLASH_SCREEN
#define CONFIG_SPLASH_SCREEN_ALIGN
#define CONFIG_VIDEO_BMP_LOGO
#define CONFIG_VIDEO_BMP_GZIP
#define CONFIG_SYS_VIDEO_LOGO_MAX_SIZE		(10 * 1024 * 1024)
#define CONFIG_SYS_VIDEO_LOGO_NAME		    "boot_logo.bmp.gz"
#define CONFIG_SYS_BATTERY_LOW_NAME		    "battery_low.bmp.gz"
#define CONFIG_SYS_CHARGER_LOGO_NAME		"charger_logo.bmp.gz"
#define CONFIG_SYS_RECOVERY_LOGO_NAME		"recovery_logo.bmp.gz"
#define CONFIG_SYS_CHARGER_FRAME_NAME		"charger_frame.bmp.gz"
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define CONFIG_CONSOLE_MUX
#define CONFIG_OWL_DISPLAY_LCD
#define CONFIG_OWL_DISPLAY_DSI
#define CONFIG_OWL_DISPLAY_HDMI
#define CONFIG_OWL_DISPLAY_CVBS
#define	CONFIG_LCD_FRAMEBUF_SIZE (1024*1024*10)

/*gamma support*/
#define CONFIG_SYS_GAMMA_NAME		"new_gamma_data"
#define CONFIG_SYS_GAMMA_SIZE		(256 * 3)
/*
#define EXT4_CACHE_DEVICE         6
#define EXT4_CACHE_PART           0
*/
#define RECOVERY_MMC_DEV "2"
#define RECOVERY_NAND_DEV "2"
#define RECOVERY_EMMC_DEV "2"
#define FAT_ENV_DEVICE			1
#define FAT_ENV_PART			0
#define FAT_MISC_DEV		1
#define FAT_RECOVERY_DEV	2
#endif

#define CONFIG_BOOT_POWER
#define CONFIG_GAUGE_BQ27441
#define CONFIG_CHECK_KEY

#endif /* __CONFIG_S500_H */
