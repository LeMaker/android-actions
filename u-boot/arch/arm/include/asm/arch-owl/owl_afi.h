#ifndef __OWL_AFINFO_FUNC_H__
#define __OWL_AFINFO_FUNC_H__


typedef struct
{
	/*! magic:'A''F''I'*/
	char magic[4];
	unsigned char boot_mode;
	unsigned char boot_dev;	
	unsigned char uart_index;		 //0: uart0; 1: uart1; 2: uart2. >7 diable
	unsigned char uart_pad;
	unsigned int uart_baudrate;       //38400(0x9600)	
	unsigned int ddr_size;  // 
} afinfo_t;

#define BOOT_MODE_NORMAL     	0
#define BOOT_MODE_PRODUCE    	1
#define BOOT_MODE_MINICHARGE 	2
#define BOOT_MODE_RECOVERY   	3
#define BOOT_MODE_LOWPWR     	4
#define BOOT_MODE_CRITIAL_PWR  	5
#define BOOT_MODE_FASTBOOT   	6
#define BOOT_MODE_ADFU       	7
#define BOOT_MODE_CMDLINE    	8

#define OWL_BOOTDEV_NAND       (0x00)
#define OWL_BOOTDEV_SD0        (0x20)
#define OWL_BOOTDEV_SD1        (0x21)
#define OWL_BOOTDEV_SD2        (0x22)
#define OWL_BOOTDEV_SD02NAND   (0x30)   //nand for cardburn 
#define OWL_BOOTDEV_SD02SD2    (0x31)	//emmc for cardburn 
#define OWL_BOOTDEV_NOR        (0x40)   //spinor

void owl_set_boot_mode(int mode);
int owl_get_boot_mode(void);
int owl_get_boot_dev(void);
int owl_afi_get_serial_number(void);
int owl_afi_get_serial_baudrate(void);
int owl_afi_get_serial_pad(void);
unsigned long owl_get_ddr_size(void);

int owl_afi_init(void);

#endif
