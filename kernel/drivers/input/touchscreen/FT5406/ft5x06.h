#ifndef FT5X06_H
#define FT5X06_H
#include "ft5x06_reg.h"

#define CFG_FT_USE_CONFIG  1

#define FT5X06_X_MAX            ( 1280 )
#define FT5X06_Y_MAX            ( 800 )

#define TP_ROTATE_DEFAULT 90
#define TP_XREVERT 0
#define TP_YREVERT 0
#define TP_XYSWAP 0

//#define FT5X06_MAX_POINT  ( 5 )
//#define FT5X0X_DOWNLOAD_FIRM  ( 1 )
//#define UPGRADE_ID2   ( 0x3 )
//#define FIRM_I_FILE_NAME  "NOVO7_VER0X0E_20130124_app.i"

#if CFG_FT_USE_CONFIG
#define FT5X06_IRQ              ( OWL_EXT_IRQ_SIRQ0 )
#endif

//#define FT5X06_RESET_PIN        OWL_GPIO_PORTB(3)
#define FT5X06_I2C_ADAPTER      ( 1 )
#define FT5X06_I2C_ADDR         ( 0x38 ) // Don't contained r/w bit

//#define FT5X06_MAX_POINT        ( R_TP_MAX_POINT )
#define FT5X06_POWER_ID			("ldo5")
#define FT5X06_POWER_MIN_VOL	(3100000)
#define FT5X06_POWER_MAX_VOL	(3110000)

#define FT5X06_RESET_PIN        OWL_GPIO_PORTB(3)
#define I2C_CTPM_ADDRESS 0x38
//#define FT5X0X_DOWNLOAD_FIRM    (R_TP_DOWNLOAD_FIRM)
//#define FIRM_I_FILE_NAME	(R_TP_I_FILE_NAME)

enum ft5x0x_ts_regs {
	FT5X0X_REG_THGROUP					= 0x80,     /* touch threshold, related to sensitivity */
	FT5X0X_REG_THPEAK						= 0x81,
	FT5X0X_REG_THCAL						= 0x82,
	FT5X0X_REG_THWATER					= 0x83,
	FT5X0X_REG_THTEMP					= 0x84,
	FT5X0X_REG_THDIFF						= 0x85,				
	FT5X0X_REG_CTRL						= 0x86,
	FT5X0X_REG_TIMEENTERMONITOR			= 0x87,
	FT5X0X_REG_PERIODACTIVE				= 0x88,      /* report rate */
	FT5X0X_REG_PERIODMONITOR			= 0x89,
	FT5X0X_REG_HEIGHT_B					= 0x8a,
	FT5X0X_REG_MAX_FRAME					= 0x8b,
	FT5X0X_REG_DIST_MOVE					= 0x8c,
	FT5X0X_REG_DIST_POINT				= 0x8d,
	FT5X0X_REG_FEG_FRAME					= 0x8e,
	FT5X0X_REG_SINGLE_CLICK_OFFSET		= 0x8f,
	FT5X0X_REG_DOUBLE_CLICK_TIME_MIN	= 0x90,
	FT5X0X_REG_SINGLE_CLICK_TIME			= 0x91,
	FT5X0X_REG_LEFT_RIGHT_OFFSET		= 0x92,
	FT5X0X_REG_UP_DOWN_OFFSET			= 0x93,
	FT5X0X_REG_DISTANCE_LEFT_RIGHT		= 0x94,
	FT5X0X_REG_DISTANCE_UP_DOWN		= 0x95,
	FT5X0X_REG_ZOOM_DIS_SQR				= 0x96,
	FT5X0X_REG_RADIAN_VALUE				=0x97,
	FT5X0X_REG_MAX_X_HIGH                       	= 0x98,
	FT5X0X_REG_MAX_X_LOW             			= 0x99,
	FT5X0X_REG_MAX_Y_HIGH            			= 0x9a,
	FT5X0X_REG_MAX_Y_LOW             			= 0x9b,
	FT5X0X_REG_K_X_HIGH            			= 0x9c,
	FT5X0X_REG_K_X_LOW             			= 0x9d,
	FT5X0X_REG_K_Y_HIGH            			= 0x9e,
	FT5X0X_REG_K_Y_LOW             			= 0x9f,
	FT5X0X_REG_AUTO_CLB_MODE			= 0xa0,
	FT5X0X_REG_LIB_VERSION_H 				= 0xa1,
	FT5X0X_REG_LIB_VERSION_L 				= 0xa2,		
	FT5X0X_REG_CIPHER						= 0xa3,
	FT5X0X_REG_MODE						= 0xa4,
	FT5X0X_REG_PMODE						= 0xa5,	  /* Power Consume Mode		*/	
	FT5X0X_REG_FIRMID						= 0xa6,   /* Firmware version */
	FT5X0X_REG_STATE						= 0xa7,
	FT5X0X_REG_FT5201ID					= 0xa8,
	FT5X0X_REG_ERR						= 0xa9,
	FT5X0X_REG_CLB						= 0xaa,
};


typedef enum
{
    ERR_OK,
    ERR_MODE,
    ERR_READID,
    ERR_ERASE,
    ERR_STATUS,
    ERR_ECC,
    ERR_DL_ERASE_FAIL,
    ERR_DL_PROGRAM_FAIL,
    ERR_DL_VERIFY_FAIL
}E_UPGRADE_ERR_TYPE;


typedef unsigned char         FTS_BYTE;     //8 bit
typedef unsigned short        FTS_WORD;    //16 bit
typedef unsigned int          FTS_DWRD;    //16 bit
typedef unsigned char         FTS_BOOL;    //8 bit

#define FTS_NULL                0x0
#define FTS_TRUE                0x01
#define FTS_FALSE              0x0

//#define GPIO_TO_IRQ
#ifdef GPIO_TO_IRQ
#define FT5X06_IRQ_GPIO        OWL_GPIO_PORTA(24)
#define FT5x06_IRQ_NAME        "ft5x06_irq"
#endif

#define ft5x06_debug
#define ft5x06_warnning

extern int debug_switch;

#ifdef ft5x06_debug
#define FT5X06_DEBUG(fmt, args...) \
    if ( debug_switch)  \
        printk("[FT5X06-INFO]"fmt"\n", ##args)
#else
#define FT5X06_DEBUG(fmt, args...) \
    do {} while(0)
#endif

#ifdef  ft5x06_warnning
#define FT5X06_WARNNING(fmt, args...) \
    printk("[FT5X06-WARN]"fmt"\n", ##args)
#else
#define FT5X06_WARNNING(fmt, args...) \
    do {} while(0)
#endif

#endif
