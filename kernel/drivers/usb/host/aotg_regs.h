/*
 * for Actions AOTG 
 *
 */

#ifndef  __AOTG_REGS_H__
#define  __AOTG_REGS_H__

//#include <mach/hardware.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
//#include <mach/irqs.h>
#include <mach/hardware.h>

#define USBH_BASE0				0xB0600000
#define USBH_BASE1				0xB0700000

#define AOTG_REGS_SIZE		(64*1024)    //64k

#define	    HCIN0BC             0x00000000
#define	    HCOUT0BC            0x00000001
#define     EP0CS           	0x00000002

#define     HCIN1BC         	0x00000008
#define     HCIN2BC         	0x00000010
#define     HCIN3BC         	0x00000018
#define     HCIN4BC         	0x00000020
#define     HCIN5BC         	0x00000028
#define     HCIN6BC         	0x00000030
#define     HCIN7BC         	0x00000038
#define     HCIN8BC         	0x00000040
#define     HCIN9BC         	0x00000048
#define     HCIN10BC            0x00000050
#define     HCIN11BC            0x00000058
#define     HCIN12BC            0x00000060
#define     HCIN13BC            0x00000068
#define     HCIN14BC            0x00000070
#define     HCIN15BC            0x00000078

#define     HCIN1BCL            0x00000008
#define     HCIN2BCL            0x00000010
#define     HCIN3BCL            0x00000018
#define     HCIN4BCL            0x00000020
#define     HCIN5BCL            0x00000028
#define     HCIN6BCL            0x00000030
#define     HCIN7BCL            0x00000038
#define     HCIN8BCL            0x00000040
#define     HCIN9BCL            0x00000048
#define     HCIN10BCL           0x00000050
#define     HCIN11BCL           0x00000058
#define     HCIN12BCL           0x00000060
#define     HCIN13BCL           0x00000068
#define     HCIN14BCL           0x00000070
#define     HCIN15BCL           0x00000078

#define     HCIN1BCH            0x00000009
#define     HCIN2BCH            0x00000011
#define     HCIN3BCH            0x00000019
#define     HCIN4BCH            0x00000021
#define     HCIN5BCH            0x00000029
#define     HCIN6BCH            0x00000031
#define     HCIN7BCH            0x00000039
#define     HCIN8BCH            0x00000041
#define     HCIN9BCH            0x00000049
#define     HCIN10BCH           0x00000051
#define     HCIN11BCH           0x00000059
#define     HCIN12BCH           0x00000061
#define     HCIN13BCH           0x00000069
#define     HCIN14BCH           0x00000071
#define     HCIN15BCH           0x00000079

#define     HCIN1CON            0x0000000A
#define     HCIN2CON            0x00000012
#define     HCIN3CON            0x0000001A
#define     HCIN4CON            0x00000022
#define     HCIN5CON            0x0000002A
#define     HCIN6CON            0x00000032
#define     HCIN7CON            0x0000003A
#define     HCIN8CON            0x00000042
#define     HCIN9CON            0x0000004A
#define     HCIN10CON           0x00000052
#define     HCIN11CON           0x0000005A
#define     HCIN12CON           0x00000062
#define     HCIN13CON           0x0000006A
#define     HCIN14CON           0x00000072
#define     HCIN15CON           0x0000007A

#define     HCIN1CS         0x0000000B
#define     HCIN2CS         0x00000013
#define     HCIN3CS         0x0000001B
#define     HCIN4CS         0x00000023
#define     HCIN5CS         0x0000002B
#define     HCIN6CS         0x00000033
#define     HCIN7CS         0x0000003B
#define     HCIN8CS         0x00000043
#define     HCIN9CS         0x0000004B
#define     HCIN10CS            0x00000053
#define     HCIN11CS            0x0000005B
#define     HCIN12CS            0x00000063
#define     HCIN13CS            0x0000006B
#define     HCIN14CS            0x00000073
#define     HCIN15CS            0x0000007B

#define     HCOUT1BC            0x0000000C
#define     HCOUT2BC            0x00000014
#define     HCOUT3BC            0x0000001C
#define     HCOUT4BC            0x00000024
#define     HCOUT5BC            0x0000002C
#define     HCOUT6BC            0x00000034
#define     HCOUT7BC            0x0000003C
#define     HCOUT8BC            0x00000044
#define     HCOUT9BC            0x0000004C
#define     HCOUT10BC           0x00000054
#define     HCOUT11BC           0x0000005C
#define     HCOUT12BC           0x00000064
#define     HCOUT13BC           0x0000006C
#define     HCOUT14BC           0x00000074
#define     HCOUT15BC           0x0000007C

#define     HCOUT1BCL           0x0000000C
#define     HCOUT2BCL           0x00000014
#define     HCOUT3BCL           0x0000001C
#define     HCOUT4BCL           0x00000024
#define     HCOUT5BCL           0x0000002C
#define     HCOUT6BCL           0x00000034
#define     HCOUT7BCL           0x0000003C
#define     HCOUT8BCL           0x00000044
#define     HCOUT9BCL           0x0000004C
#define     HCOUT10BCL          0x00000054
#define     HCOUT11BCL          0x0000005C
#define     HCOUT12BCL          0x00000064
#define     HCOUT13BCL          0x0000006C
#define     HCOUT14BCL          0x00000074
#define     HCOUT15BCL          0x0000007C

#define     HCOUT1BCH           0x0000000D
#define     HCOUT2BCH           0x00000015
#define     HCOUT3BCH           0x0000001D
#define     HCOUT4BCH           0x00000025
#define     HCOUT5BCH           0x0000002D
#define     HCOUT6BCH           0x00000035
#define     HCOUT7BCH           0x0000003D
#define     HCOUT8BCH           0x00000045
#define     HCOUT9BCH           0x0000004D
#define     HCOUT10BCH          0x00000055
#define     HCOUT11BCH          0x0000005D
#define     HCOUT12BCH          0x00000065
#define     HCOUT13BCH          0x0000006D
#define     HCOUT14BCH          0x00000075
#define     HCOUT15BCH          0x0000007D

#define     HCOUT1CON           0x0000000E
#define     HCOUT2CON           0x00000016
#define     HCOUT3CON           0x0000001E
#define     HCOUT4CON           0x00000026
#define     HCOUT5CON           0x0000002E
#define     HCOUT6CON           0x00000036
#define     HCOUT7CON           0x0000003E
#define     HCOUT8CON           0x00000046
#define     HCOUT9CON           0x0000004E
#define     HCOUT10CON          0x00000056
#define     HCOUT11CON          0x0000005E
#define     HCOUT12CON          0x00000066
#define     HCOUT13CON          0x0000006E
#define     HCOUT14CON          0x00000076
#define     HCOUT15CON          0x0000007E

#define     HCOUT1CS            0x0000000F
#define     HCOUT2CS            0x00000017
#define     HCOUT3CS            0x0000001F
#define     HCOUT4CS            0x00000027
#define     HCOUT5CS            0x0000002F
#define     HCOUT6CS            0x00000037
#define     HCOUT7CS            0x0000003F
#define     HCOUT8CS            0x00000047
#define     HCOUT9CS            0x0000004F
#define     HCOUT10CS           0x00000057
#define     HCOUT11CS           0x0000005F
#define     HCOUT12CS           0x00000067
#define     HCOUT13CS           0x0000006F
#define     HCOUT14CS           0x00000077
#define     HCOUT15CS           0x0000007F

#define     HCEP0CTRL           0x000000C0
#define     HCOUT1CTRL          0x000000C4
#define     HCOUT2CTRL          0x000000C8
#define     HCOUT3CTRL          0x000000CC
#define     HCOUT4CTRL          0x000000D0
#define     HCOUT5CTRL          0x000000D4
#define     HCOUT6CTRL          0x000000D8
#define     HCOUT7CTRL          0x000000DC
#define     HCOUT8CTRL          0x000000E0
#define     HCOUT9CTRL          0x000000E4
#define     HCOUT10CTRL         0x000000E8
#define     HCOUT11CTRL         0x000000EC
#define     HCOUT12CTRL         0x000000F0
#define     HCOUT13CTRL         0x000000F4
#define     HCOUT14CTRL         0x000000F8
#define     HCOUT15CTRL         0x000000FC

#define     HCOUT0ERR           0x000000C1
#define     HCOUT1ERR           0x000000C5
#define     HCOUT2ERR           0x000000C9
#define     HCOUT3ERR           0x000000CD
#define     HCOUT4ERR           0x000000D1
#define     HCOUT5ERR           0x000000D5
#define     HCOUT6ERR           0x000000D9
#define     HCOUT7ERR           0x000000DD
#define     HCOUT8ERR           0x000000E1
#define     HCOUT9ERR           0x000000E5
#define     HCOUT10ERR          0x000000E9
#define     HCOUT11ERR          0x000000ED
#define     HCOUT12ERR          0x000000F1
#define     HCOUT13ERR          0x000000F5
#define     HCOUT14ERR          0x000000F9
#define     HCOUT15ERR          0x000000FD

#define     HCIN1CTRL           0x000000C6
#define     HCIN2CTRL           0x000000CA
#define     HCIN3CTRL           0x000000CE
#define     HCIN4CTRL           0x000000D2
#define     HCIN5CTRL           0x000000D6
#define     HCIN6CTRL           0x000000DA
#define     HCIN7CTRL           0x000000DE
#define     HCIN8CTRL           0x000000E2
#define     HCIN9CTRL           0x000000E6
#define     HCIN10CTRL          0x000000EA
#define     HCIN11CTRL          0x000000EE
#define     HCIN12CTRL          0x000000F2
#define     HCIN13CTRL          0x000000F6
#define     HCIN14CTRL          0x000000FA
#define     HCIN15CTRL          0x000000FE

#define     HCIN0ERR            0x000000C3
#define     HCIN1ERR            0x000000C7
#define     HCIN2ERR            0x000000CB
#define     HCIN3ERR            0x000000CF
#define     HCIN4ERR            0x000000D3
#define     HCIN5ERR            0x000000D7
#define     HCIN6ERR            0x000000DB
#define     HCIN7ERR            0x000000DF
#define     HCIN8ERR            0x000000E3
#define     HCIN9ERR            0x000000E7
#define     HCIN10ERR           0x000000EB
#define     HCIN11ERR           0x000000EF
#define     HCIN12ERR           0x000000F3
#define     HCIN13ERR           0x000000F7
#define     HCIN14ERR           0x000000FB
#define     HCIN15ERR           0x000000FF

#define     EP0INDATA_W0            0x00000100
#define     EP0INDATA_W1            0x00000104
#define     EP0INDATA_W2            0x00000108
#define     EP0INDATA_W3            0x0000010C
#define     EP0INDATA_W4            0x00000110
#define     EP0INDATA_W5            0x00000114
#define     EP0INDATA_W6            0x00000118
#define     EP0INDATA_W7            0x0000011C
#define     EP0INDATA_W8            0x00000120
#define     EP0INDATA_W9            0x00000124
#define     EP0INDATA_W10           0x00000128
#define     EP0INDATA_W11           0x0000012C
#define     EP0INDATA_W12           0x00000130
#define     EP0INDATA_W13           0x00000134
#define     EP0INDATA_W14           0x00000138
#define     EP0INDATA_W15           0x0000013C
#define     EP0OUTDATA_W0           0x00000140
#define     EP0OUTDATA_W1           0x00000144
#define     EP0OUTDATA_W2           0x00000148
#define     EP0OUTDATA_W3           0x0000014C
#define     EP0OUTDATA_W4           0x00000150
#define     EP0OUTDATA_W5           0x00000154
#define     EP0OUTDATA_W6           0x00000158
#define     EP0OUTDATA_W7           0x0000015C
#define     EP0OUTDATA_W8           0x00000160
#define     EP0OUTDATA_W9           0x00000164
#define     EP0OUTDATA_W10          0x00000168
#define     EP0OUTDATA_W11          0x0000016C
#define     EP0OUTDATA_W12          0x00000170
#define     EP0OUTDATA_W13          0x00000174
#define     EP0OUTDATA_W14          0x00000178
#define     EP0OUTDATA_W15          0x0000017C
#define     SETUPDATA_W0            0x00000180
#define     SETUPDATA_W1            0x00000184


#if 0
/////////////////////////////////////////////////////
/*******  for device.  *****************************/
/////////////////////////////////////////////////////
#define OUT0BC                  0x00000000
#define IN0BC                   0x00000001

#define OUT1BCL                 0x00000008
#define OUT1BCH                 0x00000009
#define OUT1CON                 0x0000000A
#define OUT1CS                  0x0000000B
#define IN1BCL                  0x0000000C
#define IN1BCH                  0x0000000D
#define IN1CON                  0x0000000E
#define IN1CS                   0x0000000F
#define OUT2BCL                 0x00000010
#define OUT2BCH                 0x00000011
#define OUT2CON                 0x00000012
#define OUT2CS                  0x00000013
#define IN2BCL                  0x00000014
#define IN2BCH                  0x00000015
#define IN2CON                  0x00000016
#define IN2CS                   0x00000017
#define OUT3BCL                 0x00000018
#define OUT3BCH                 0x00000019
#define OUT3CON                 0x0000001A
#define OUT3CS                  0x0000001B
#define IN3BCL                  0x0000001C
#define IN3BCH                  0x0000001D
#define IN3CON                  0x0000001E
#define IN3CS                   0x0000001F
#define OUT4BCL                 0x00000020
#define OUT4BCH                 0x00000021
#define OUT4CON                 0x00000022
#define OUT4CS                  0x00000023
#define IN4BCL                  0x00000024
#define IN4BCH                  0x00000025
#define IN4CON                  0x00000026
#define IN4CS                   0x00000027
#define OUT5BCL                 0x00000028
#define OUT5BCH                 0x00000029
#define OUT5CON                 0x0000002A
#define OUT5CS                  0x0000002B
#define IN5BCL                  0x0000002C
#define IN5BCH                  0x0000002D
#define IN5CON                  0x0000002E
#define IN5CS                   0x0000002F

//#define FIFO1DATA               0x00000084
//#define FIFO2DATA               0x00000088
//#define FIFO3DATA               0x0000008C
//#define FIFO4DATA               0x00000090
//#define FIFO5DATA               0x00000094
//#define EP0INDAT                0x00000100
//#define HCEP0OUTDAT             0x00000100
#endif


#if 0
/***************  for 0-7 ep to 0-15 ep. *****************/
//#define IN07IRQ                 0x00000188
//#define HCOUT07IRQ              0x00000188
//#define OUT07IRQ                0x0000018A
//#define HCIN07IRQ               0x0000018A
//#define USBIRQ                  0x0000018C
//#define OUT07PNGIRQ             0x0000018E
//#define INTXKIRQ                0x00000190
//#define OUTXTOKIRQ              0x00000190
//#define OUT07EMPTIRQ            0x00000191
//#define HCIN07EMPTIRQ           0x00000191
//#define IN07IEN                 0x00000194
//#define HCOUT07IEN              0x00000194
//#define OUT07IEN                0x00000196
//#define HCIN07IEN               0x00000196
//#define USBIEN                  0x00000198
//#define OUT07PNGIEN             0x0000019A
//#define INTXKIEN                0x0000019C
//#define OUTXTOKIEN              0x0000019D
#endif

#define     HCOUTxIRQ0          0x00000188
#define     HCOUTxIRQ1          0x00000189
#define     HCINxIRQ0           0x0000018A
#define     HCINxIRQ1           0x0000018B
#define     USBIRQ              0x0000018C
#define     HCINxPNGIRQ0            0x0000018E
#define     HCINxPNGIRQ1            0x0000018F
#define     HCOUTxTOKIRQ0           0x00000190
#define     HCOUTxTOKIRQ1           0x00000191
#define     HCINxTOKIRQ0            0x00000192
#define     HCINxTOKIRQ1            0x00000193
#define     HCOUTxIEN0          0x00000194
#define     HCOUTxIEN1          0x00000195
#define     HCINxIEN0           0x00000196
#define     HCINxIEN1           0x00000197
#define     USBIEN              0x00000198
#define     HCINxPNGIEN0            0x0000019A
#define     HCINxPNGIEN1            0x0000019B
#define     HCOUTxTOKIEN0           0x0000019C
#define     HCOUTxTOKIEN1           0x0000019D
#define     HCINxTOKIEN0            0x0000019E
#define     HCINxTOKIEN1            0x0000019F

#define IVECT                   0x000001A0
#define ENDPRST                 0x000001A2
//#define HCENDPRST               0x000001A2
#define USBCS                   0x000001A3
#define FRMNRL                  0x000001A4
#define     FRMNRH              0x000001A5
//#define FRMNFH                  0x000001A5
#define FNADDR                  0x000001A6
#define CLKGATE                 0x000001A7
//#define FIFOCTRL                0x000001A8
#define HCTRAINTERVAL		0x000001A8
#define HCPORTCTRL              0x000001AB
#define HCFRMNRL                0x000001AC
#define HCFRMNRH                0x000001AD
#define HCFRMREMAINL            0x000001AE
#define HCFRMREMAINH            0x000001AF

#if 0
/************** ep0-7 to ep0-15 ******************/
//#define HCIN07ERRIRQ            0x000001B4
//#define HCOUT07ERRIRQ           0x000001B6
//#define HCIN07ERRIEN            0x000001B8
//#define HCOUT07ERRIEN           0x000001BA
#endif
#define     HCINxERRIRQ0            0x000001B4
#define     HCINxERRIRQ1            0x000001B5
#define     HCOUTxERRIRQ0           0x000001B6
#define     HCOUTxERRIRQ1           0x000001B7
#define     HCINxERRIEN0            0x000001B8
#define     HCINxERRIEN1            0x000001B9
#define     HCOUTxERRIEN0           0x000001BA
#define     HCOUTxERRIEN1           0x000001BB

#define OTGIRQ                  0x000001BC
#define OTGSTATE                0x000001BD
#define OTGCTRL                 0x000001BE
#define OTGSTATUS               0x000001BF
#define OTGIEN                  0x000001C0
#define TAAIDLBDIS              0x000001C1
#define TAWAITBCON              0x000001C2
#define TBVBUSPLS               0x000001C3
//#define TBVBUSDISCHPLS          0x000001C7
#define     TBVBUSDISPLS            0x000001C7

#define     HCIN0MAXPCK         0x000001E0
#define     HCIN1MAXPCK         0x000001E2
#define     HCIN2MAXPCK         0x000001E4
#define     HCIN3MAXPCK         0x000001E6
#define     HCIN4MAXPCK         0x000001E8
#define     HCIN5MAXPCK         0x000001EA
#define     HCIN6MAXPCK         0x000001EC
#define     HCIN7MAXPCK         0x000001EE
#define     HCIN8MAXPCK         0x000001F0
#define     HCIN9MAXPCK         0x000001F2
#define     HCIN10MAXPCK            0x000001F4
#define     HCIN11MAXPCK            0x000001F6
#define     HCIN12MAXPCK            0x000001F8
#define     HCIN13MAXPCK            0x000001FA
#define     HCIN14MAXPCK            0x000001FC
#define     HCIN15MAXPCK            0x000001FE
#define     HCIN1MAXPCKL            0x000001E2
#define     HCIN2MAXPCKL            0x000001E4
#define     HCIN3MAXPCKL            0x000001E6
#define     HCIN4MAXPCKL            0x000001E8
#define     HCIN5MAXPCKL            0x000001EA
#define     HCIN6MAXPCKL            0x000001EC
#define     HCIN7MAXPCKL            0x000001EE
#define     HCIN8MAXPCKL            0x000001F0
#define     HCIN9MAXPCKL            0x000001F2
#define     HCIN10MAXPCKL           0x000001F4
#define     HCIN11MAXPCKL           0x000001F6
#define     HCIN12MAXPCKL           0x000001F8
#define     HCIN13MAXPCKL           0x000001FA
#define     HCIN14MAXPCKL           0x000001FC
#define     HCIN15MAXPCKL           0x000001FE
#define     HCIN1MAXPCKH            0x000001E3
#define     HCIN2MAXPCKH            0x000001E5
#define     HCIN3MAXPCKH            0x000001E7
#define     HCIN4MAXPCKH            0x000001E9
#define     HCIN5MAXPCKH            0x000001EB
#define     HCIN6MAXPCKH            0x000001ED
#define     HCIN7MAXPCKH            0x000001EF
#define     HCIN8MAXPCKH            0x000001F1
#define     HCIN9MAXPCKH            0x000001F3
#define     HCIN10MAXPCKH           0x000001F5
#define     HCIN11MAXPCKH           0x000001F7
#define     HCIN12MAXPCKH           0x000001F9
#define     HCIN13MAXPCKH           0x000001FB
#define     HCIN14MAXPCKH           0x000001FD
#define     HCIN15MAXPCKH           0x000001FF

#define     HCEP0BINTERVAL          0x00000200
#define     HCIN1BINTERVAL          0x00000208
#define     HCIN2BINTERVAL          0x00000210
#define     HCIN3BINTERVAL          0x00000218
#define     HCIN4BINTERVAL          0x00000220
#define     HCIN5BINTERVAL          0x00000228
#define     HCIN6BINTERVAL          0x00000230
#define     HCIN7BINTERVAL          0x00000238
#define     HCIN8BINTERVAL          0x00000240
#define     HCIN9BINTERVAL          0x00000248
#define     HCIN10BINTERVAL     0x00000250
#define     HCIN11BINTERVAL     0x00000258
#define     HCIN12BINTERVAL     0x00000260
#define     HCIN13BINTERVAL     0x00000268
#define     HCIN14BINTERVAL     0x00000270
#define     HCIN15BINTERVAL     0x00000278
#define     HCEP0ADDR           0x201
#define     HCIN1ADDR           0x00000209
#define     HCIN2ADDR           0x00000211
#define     HCIN3ADDR           0x00000219
#define     HCIN4ADDR           0x00000221
#define     HCIN5ADDR           0x00000229
#define     HCIN6ADDR           0x00000231
#define     HCIN7ADDR           0x00000239
#define     HCIN8ADDR           0x00000241
#define     HCIN9ADDR           0x00000249
#define     HCIN10ADDR          0x00000251
#define     HCIN11ADDR          0x00000259
#define     HCIN12ADDR          0x00000261
#define     HCIN13ADDR          0x00000269
#define     HCIN14ADDR          0x00000271
#define     HCIN15ADDR          0x00000279
#define     HCEP0PORT           0x00000202
#define     HCIN1PORT           0x0000020A
#define     HCIN2PORT           0x00000212
#define     HCIN3PORT           0x0000021A
#define     HCIN4PORT           0x00000222
#define     HCIN5PORT           0x0000022A
#define     HCIN6PORT           0x00000232
#define     HCIN7PORT           0x0000023A
#define     HCIN8PORT           0x00000242
#define     HCIN9PORT           0x0000024A
#define     HCIN10PORT          0x00000252
#define     HCIN11PORT          0x0000025A
#define     HCIN12PORT          0x00000262
#define     HCIN13PORT          0x0000026A
#define     HCIN14PORT          0x00000272
#define     HCIN15PORT          0x0000027A
#define     HCEP0SPILITCS           0x00000203
#define     HCIN1SPILITCS           0x0000020B
#define     HCIN2SPILITCS           0x00000213
#define     HCIN3SPILITCS           0x0000021B
#define     HCIN4SPILITCS           0x00000223
#define     HCIN5SPILITCS           0x0000022B
#define     HCIN6SPILITCS           0x00000233
#define     HCIN7SPILITCS           0x0000023B
#define     HCIN8SPILITCS           0x00000243
#define     HCIN9SPILITCS           0x0000024B
#define     HCIN10SPILITCS          0x00000253
#define     HCIN11SPILITCS          0x0000025B
#define     HCIN12SPILITCS          0x00000263
#define     HCIN13SPILITCS          0x0000026B
#define     HCIN14SPILITCS          0x00000273
#define     HCIN15SPILITCS          0x0000027B
#define     HCOUT1BINTERVAL     0x00000288
#define     HCOUT2BINTERVAL     0x00000290
#define     HCOUT3BINTERVAL     0x00000298
#define     HCOUT4BINTERVAL     0x000002A0
#define     HCOUT5BINTERVAL     0x000002A8
#define     HCOUT6BINTERVAL     0x000002B0
#define     HCOUT7BINTERVAL     0x000002B8
#define     HCOUT8BINTERVAL     0x000002C0
#define     HCOUT9BINTERVAL     0x000002C8
#define     HCOUT10BINTERVAL        0x000002D0
#define     HCOUT11BINTERVAL        0x000002D8
#define     HCOUT12BINTERVAL        0x000002E0
#define     HCOUT13BINTERVAL        0x000002E8
#define     HCOUT14BINTERVAL        0x000002F0
#define     HCOUT15BINTERVAL        0x000002F8
#define     HCOUT1ADDR          0x00000289
#define     HCOUT2ADDR          0x00000291
#define     HCOUT3ADDR          0x00000299
#define     HCOUT4ADDR          0x000002A1
#define     HCOUT5ADDR          0x000002A9
#define     HCOUT6ADDR          0x000002B1
#define     HCOUT7ADDR          0x000002B9
#define     HCOUT8ADDR          0x000002C1
#define     HCOUT9ADDR          0x000002C9
#define     HCOUT10ADDR         0x000002D1
#define     HCOUT11ADDR         0x000002D9
#define     HCOUT12ADDR         0x000002E1
#define     HCOUT13ADDR         0x000002E9
#define     HCOUT14ADDR         0x000002F1
#define     HCOUT15ADDR         0x000002F9
#define     HCOUT1PORT          0x0000028A
#define     HCOUT2PORT          0x00000292
#define     HCOUT3PORT          0x0000029A
#define     HCOUT4PORT          0x000002A2
#define     HCOUT5PORT          0x000002AA
#define     HCOUT6PORT          0x000002B2
#define     HCOUT7PORT          0x000002BA
#define     HCOUT8PORT          0x000002C2
#define     HCOUT9PORT          0x000002CA
#define     HCOUT10PORT         0x000002D2
#define     HCOUT11PORT         0x000002DA
#define     HCOUT12PORT         0x000002E2
#define     HCOUT13PORT         0x000002EA
#define     HCOUT14PORT         0x000002F2
#define     HCOUT15PORT         0x000002FA
#define     HCOUT1SPILITCS          0x0000028B
#define     HCOUT2SPILITCS          0x00000293
#define     HCOUT3SPILITCS          0x0000029B
#define     HCOUT4SPILITCS          0x000002A3
#define     HCOUT5SPILITCS          0x000002AB
#define     HCOUT6SPILITCS          0x000002B3
#define     HCOUT7SPILITCS          0x000002BB
#define     HCOUT8SPILITCS          0x000002C3
#define     HCOUT9SPILITCS          0x000002CB
#define     HCOUT10SPILITCS     0x000002D3
#define     HCOUT11SPILITCS     0x000002DB
#define     HCOUT12SPILITCS     0x000002E3
#define     HCOUT13SPILITCS     0x000002EB
#define     HCOUT14SPILITCS     0x000002F3
#define     HCOUT15SPILITCS     0x000002FB

#if 0
/*************  replaced by new register define. ************/
#define OUT1STARTADDRESS        0x00000304
#define OUT1STARTADDRESSL       0x00000304
#define OUT1STARTADDRESSH       0x00000305
#define OUT2STARTADDRESS        0x00000308
#define OUT2STARTADDRESSL       0x00000308

#define OUT3STADDR              0x0000030C
#define OUT4STADDR              0x00000310
#define OUT5STADDR              0x00000314

#define OUT2STARTADDRESSH       0x00000309
#define IN1STARTADDRESS         0x00000344
#define IN1STARTADDRESSL        0x00000344
#define IN1STARTADDRESSH        0x00000345
#define IN2STARTADDRESS         0x00000348
#define IN2STARTADDRESSL        0x00000348
#define IN2STARTADDRESSH        0x00000349

#define IN3STADDR               0x0000034C
#define IN4STADDR               0x00000350
#define IN5STADDR               0x00000354

#define HCOUT0MAXPCK            0x000003E0
#define HCOUT1MAXPCKL           0x000003E2
#define HCOUT1MAXPCKH           0x000003E3
#define HCOUT2MAXPCKL           0x000003E4
#define HCOUT2MAXPCKH           0x000003E5
#define HCOUT3MAXPCKL           0x000003E6
#define HCOUT3MAXPCKH           0x000003E7
#define HCOUT4MAXPCKL           0x000003E8
#define HCOUT4MAXPCKH           0x000003E9
#define HCOUT5MAXPCKL           0x000003EA
#define HCOUT5MAXPCKH           0x000003EB

//#define USBERESET               0x00000400
#define TA_BCON_COUNT           0x00000401
#define VBUSDBCTIMERL           0x00000402
#define VBUSDBCTIMERH           0x00000403
#define VDCTRL                  0x00000404
#define VDSTATE                 0x00000405
#define BKDOOR                  0x00000406
#define DBGMODE                 0x00000407
#define SRPCTRL                 0x00000408
//#define USBEIRQ                 0x0000040A
#define USBEIEN                 0x0000040C
#define UDMAIRQ                 0x0000040E
#define UDMAIEN                 0x0000040F
#define OUTXSHORTPCKIRQ         0x00000410
#define OUTXSHORTPCKIEN         0x00000412
#define OUTXNAKCTRL             0x00000414
#define HCINXSTART              0x00000416
#define HCINXENDIRQ             0x00000418
#define HCINXENDIEN             0x0000041A

/* HCINxCounter寄存器的写入地址为0x420,0x424,0x428,0x42c,0x430,读取地址为0x420，0x422,0x424,0x426,0x428。*/
//#define HCIN1_COUNTL            0x00000420
//#define HCIN1_COUNTH            0x00000421
//#define HCIN2_COUNTL            0x00000422
//#define HCIN2_COUNTH            0x00000423
//#define HCIN3_COUNTL            0x00000424
//#define HCIN3_COUNTH            0x00000425
//#define HCIN4_COUNTL            0x00000426
//#define HCIN4_COUNTH            0x00000427
//#define HCIN5_COUNTL            0x00000428
//#define HCIN5_COUNTH            0x00000429
#define HCIN1_COUNTL            0x00000420
#define HCIN1_COUNTH            0x00000421
#define HCIN2_COUNTL            0x00000424
#define HCIN2_COUNTH            0x00000425
#define HCIN3_COUNTL            0x00000428
#define HCIN3_COUNTH            0x00000429
#define HCIN4_COUNTL            0x0000042c
#define HCIN4_COUNTH            0x0000042d
#define HCIN5_COUNTL            0x00000430
#define HCIN5_COUNTH            0x00000431

#define INXBUFEMPTYIRQ          0x00000440
#define INXBUFEMPTYIEN          0x00000442
#define INXBUFEMPTYCTRL         0x00000444
#define UDMA1MEMADDR            0x00000450
#define UDMA1EPSEL              0x00000454
#define UDMA1COM                0x00000455
#define UDMA1CNTL               0x00000458
#define UDMA1CNTM               0x00000459
#define UDMA1CNTH               0x0000045A
#define UDMA1REML               0x0000045C
#define UDMA1REMM               0x0000045D
#define UDMA1REMH               0x0000045E
#define UDMA2MEMADDR            0x00000460
#define UDMA2EPSEL              0x00000464
#define UDMA2COM                0x00000465
#define UDMA2CNTL               0x00000468
#define UDMA2CNTM               0x00000469
#define UDMA2CNTH               0x0000046A
#define UDMA2REML               0x0000046C
#define UDMA2REMM               0x0000046D
#define UDMA2REMH               0x0000046E
#endif

#define     HCIN1STADDR         0x00000304
#define     HCIN2STADDR         0x00000308
#define     HCIN3STADDR         0x0000030C
#define     HCIN4STADDR         0x00000310
#define     HCIN5STADDR         0x00000314
#define     HCIN6STADDR         0x00000318
#define     HCIN7STADDR         0x0000031C
#define     HCIN8STADDR         0x00000320
#define     HCIN9STADDR         0x00000324
#define     HCIN10STADDR            0x00000328
#define     HCIN11STADDR            0x0000032C
#define     HCIN12STADDR            0x00000330
#define     HCIN13STADDR            0x00000334
#define     HCIN14STADDR            0x00000338
#define     HCIN15STADDR            0x0000033C
#define     HCOUT1STADDR            0x00000344
#define     HCOUT2STADDR            0x00000348
#define     HCOUT3STADDR            0x0000034C
#define     HCOUT4STADDR            0x00000350
#define     HCOUT5STADDR            0x00000354
#define     HCOUT6STADDR            0x00000358
#define     HCOUT7STADDR            0x0000035C
#define     HCOUT8STADDR            0x00000360
#define     HCOUT9STADDR            0x00000364
#define     HCOUT10STADDR           0x00000368
#define     HCOUT11STADDR           0x0000036C
#define     HCOUT12STADDR           0x00000370
#define     HCOUT13STADDR           0x00000374
#define     HCOUT14STADDR           0x00000378
#define     HCOUT15STADDR           0x0000037C
#define     HCOUT1MAXPCK            0x000003E2
#define     HCOUT2MAXPCK            0x000003E4
#define     HCOUT3MAXPCK            0x000003E6
#define     HCOUT4MAXPCK            0x000003E8
#define     HCOUT5MAXPCK            0x000003EA
#define     HCOUT6MAXPCK            0x000003EC
#define     HCOUT7MAXPCK            0x000003EE
#define     HCOUT8MAXPCK            0x3F0
#define     HCOUT9MAXPCK            0x000003F2
#define     HCOUT10MAXPCK           0x000003F4
#define     HCOUT11MAXPCK           0x000003F6
#define     HCOUT12MAXPCK           0x000003F8
#define     HCOUT13MAXPCK           0x000003FA
#define     HCOUT14MAXPCK           0x000003FC
#define     HCOUT15MAXPCK           0x000003FE
#define     HCOUT1MAXPCKL           0x000003E2
#define     HCOUT2MAXPCKL           0x000003E4
#define     HCOUT3MAXPCKL           0x000003E6
#define     HCOUT4MAXPCKL           0x000003E8
#define     HCOUT5MAXPCKL           0x000003EA
#define     HCOUT6MAXPCKL           0x000003EC
#define     HCOUT7MAXPCKL           0x000003EE
#define     HCOUT8MAXPCKL           0x3F0
#define     HCOUT9MAXPCKL           0x000003F2
#define     HCOUT10MAXPCKL          0x000003F4
#define     HCOUT11MAXPCKL          0x000003F6
#define     HCOUT12MAXPCKL          0x000003F8
#define     HCOUT13MAXPCKL          0x000003FA
#define     HCOUT14MAXPCKL          0x000003FC
#define     HCOUT15MAXPCKL          0x000003FE
#define     HCOUT1MAXPCKH           0x000003E3
#define     HCOUT2MAXPCKH           0x000003E5
#define     HCOUT3MAXPCKH           0x000003E7
#define     HCOUT4MAXPCKH           0x000003E9
#define     HCOUT5MAXPCKH           0x000003EB
#define     HCOUT6MAXPCKH           0x000003ED
#define     HCOUT7MAXPCKH           0x000003EF
#define     HCOUT8MAXPCKH           0x3F1
#define     HCOUT9MAXPCKH           0x000003F3
#define     HCOUT10MAXPCKH          0x000003F5
#define     HCOUT11MAXPCKH          0x000003F7
#define     HCOUT12MAXPCKH          0x000003F9
#define     HCOUT13MAXPCKH          0x000003FB
#define     HCOUT14MAXPCKH          0x000003FD
#define     HCOUT15MAXPCKH          0x000003FF
#define     HCINxDMASTART0          0x00000400
#define     HCINxDMASTART1          0x00000401
#define     HCINDMAERROR            0x00000402
#define     HCINxDMAIRQ0            0x00000404
#define     HCINxDMAIRQ1            0x00000405
#define     HCINxDMAIEN0            0x00000406
#define     HCINxDMAIEN1            0x00000407
#define     HCIN1DMASTADDR          0x00000408
#define     HCIN2DMASTADDR          0x00000410
#define     HCIN3DMASTADDR          0x00000418
#define     HCIN4DMASTADDR          0x00000420
#define     HCIN5DMASTADDR          0x00000428
#define     HCIN6DMASTADDR          0x00000430
#define     HCIN7DMASTADDR          0x00000438
#define     HCIN8DMASTADDR          0x00000440
#define     HCIN9DMASTADDR          0x00000448
#define     HCIN10DMASTADDR     0x00000450
#define     HCIN11DMASTADDR     0x00000458
#define     HCIN12DMASTADDR     0x00000460
#define     HCIN13DMASTADDR     0x00000468
#define     HCIN14DMASTADDR     0x00000470
#define     HCIN15DMASTADDR     0x00000478
#define     HCIN1DMACOUNTER     0x0000040C
#define     HCIN2DMACOUNTER     0x00000414
#define     HCIN3DMACOUNTER     0x0000041C
#define     HCIN4DMACOUNTER     0x00000424
#define     HCIN5DMACOUNTER     0x0000042C
#define     HCIN6DMACOUNTER     0x00000434
#define     HCIN7DMACOUNTER     0x0000043C
#define     HCIN8DMACOUNTER     0x00000444
#define     HCIN9DMACOUNTER     0x0000044C
#define     HCIN10DMACOUNTER        0x00000454
#define     HCIN11DMACOUNTER        0x0000045C
#define     HCIN12DMACOUNTER        0x00000464
#define     HCIN13DMACOUNTER        0x0000046C
#define     HCIN14DMACOUNTER        0x00000474
#define     HCIN15DMACOUNTER        0x0000047C
#define     HCOUTxDMASTART0     0x00000480
#define     HCOUTxDMASTART1     0x00000481
#define     HCOUTxDMAIRQ0           0x00000484
#define     HCOUTxDMAIRQ1           0x00000485
#define     HCOUTxDMAIEN0           0x00000486
#define     HCOUTxDMAIEN1           0x00000487
#define     HCOUT1DMASTADDR     0x00000488
#define     HCOUT2DMASTADDR     0x00000490
#define     HCOUT3DMASTADDR     0x00000498
#define     HCOUT4DMASTADDR     0x000004A0
#define     HCOUT5DMASTADDR     0x000004A8
#define     HCOUT6DMASTADDR     0x000004B0
#define     HCOUT7DMASTADDR     0x000004B8
#define     HCOUT8DMASTADDR     0x000004C0
#define     HCOUT9DMASTADDR     0x000004C8
#define     HCOUT10DMASTADDR        0x000004D0
#define     HCOUT11DMASTADDR        0x000004D8
#define     HCOUT12DMASTADDR        0x000004E0
#define     HCOUT13DMASTADDR        0x000004E8
#define     HCOUT14DMASTADDR        0x000004F0
#define     HCOUT15DMASTADDR        0x000004F8
#define     HCOUT1DMACOUNTER        0x0000048C
#define     HCOUT2DMACOUNTER        0x00000494
#define     HCOUT3DMACOUNTER        0x0000049C
#define     HCOUT4DMACOUNTER        0x000004A4
#define     HCOUT5DMACOUNTER        0x000004AC
#define     HCOUT6DMACOUNTER        0x000004B4
#define     HCOUT7DMACOUNTER        0x000004BC
#define     HCOUT8DMACOUNTER        0x000004C4
#define     HCOUT9DMACOUNTER        0x000004CC
#define     HCOUT10DMACOUNTER       0x000004D4
#define     HCOUT11DMACOUNTER       0x000004DC
#define     HCOUT12DMACOUNTER       0x000004E4
#define     HCOUT13DMACOUNTER       0x000004EC
#define     HCOUT14DMACOUNTER       0x000004F4
#define     HCOUT15DMACOUNTER       0x000004FC
#define     USBERESET           0x00000500
#define     TA_BCON_COUNT           0x00000501
#define     VBUSDBCTIMERL           0x00000502
#define     VBUSDBCTIMERH           0x00000503
#define     VDCTRL              0x00000504
#define     VDSTATUS            0x00000505
#define     BKDOOR              0x00000506
#define     DBGMODE         0x00000507
#define     SRPCTRL         0x00000508
#define     USBEIRQ         0x0000050A
#define     USBEIEN         0x0000050B
#define     HCINxSHORTPCKIRQ0       0x00000510
#define     HCINxSHORTPCKIRQ1       0x00000511
#define     HCINxSHORTPCKIEN0       0x00000512
#define     HCINxSHORTPCKIEN1       0x00000513
#define     HCINxZEROPCKIRQ0        0x00000514
#define     HCINxZEROPCKIRQ1        0x00000515
#define     HCINxZEROPCKIEN0        0x00000516
#define     HCINxZEROPCKIEN1        0x00000517
#define     HCOUTxBUFEMPTYIRQ0      0x00000518
#define     HCOUTxBUFEMPTYIRQ1      0x00000519
#define     HCOUTxBUFEMPTYIEN0      0x0000051A
#define     HCOUTxBUFEMPTYIEN1      0x0000051B
#define     HCOUTxBUFEMPTYCTRL0     0x0000051C
#define     HCOUTxBUFEMPTYCTRL1     0x0000051D


//linklist regs
#define	HCDMABCKDOOR	0x00000800
#define	HCDMAxOVERFLOWIRQ	0x00000808
#define HCDMAxOVERFLOWIEN	0x0000080C

#define	HCOUT1DMALINKADDR	0x00000910
#define	HCOUT2DMALINKADDR	0x00000920
#define	HCOUT3DMALINKADDR	0x00000930
#define	HCOUT4DMALINKADDR	0x00000940
#define	HCOUT5DMALINKADDR	0x00000950
#define	HCOUT6DMALINKADDR	0x00000960
#define	HCOUT7DMALINKADDR	0x00000970
#define	HCOUT8DMALINKADDR	0x00000980
#define	HCOUT9DMALINKADDR	0x00000990
#define	HCOUT10DMALINKADDR	0x000009A0
#define	HCOUT11DMALINKADDR	0x000009B0
#define	HCOUT12DMALINKADDR	0x000009C0
#define	HCOUT13DMALINKADDR	0x000009D0
#define	HCOUT14DMALINKADDR	0x000009E0
#define	HCOUT15DMALINKADDR	0x000009F0

#define HCOUT1DMACURADDR	0x00000914
#define HCOUT2DMACURADDR	0x00000924
#define HCOUT3DMACURADDR	0x00000934
#define HCOUT4DMACURADDR	0x00000944
#define HCOUT5DMACURADDR	0x00000954
#define HCOUT6DMACURADDR	0x00000964
#define HCOUT7DMACURADDR	0x00000974
#define HCOUT8DMACURADDR	0x00000984
#define HCOUT9DMACURADDR	0x00000994
#define HCOUT10DMACURADDR	0x000009A4
#define HCOUT11DMACURADDR	0x000009B4
#define HCOUT12DMACURADDR	0x000009C4
#define HCOUT13DMACURADDR	0x000009D4
#define HCOUT14DMACURADDR	0x000009E4
#define HCOUT15DMACURADDR	0x000009F4

#define	HCOUT1DMACTRL	0x00000918
#define	HCOUT2DMACTRL	0x00000928
#define	HCOUT3DMACTRL	0x00000938
#define	HCOUT4DMACTRL	0x00000948
#define	HCOUT5DMACTRL	0x00000958
#define	HCOUT6DMACTRL	0x00000968
#define	HCOUT7DMACTRL	0x00000978
#define	HCOUT8DMACTRL	0x00000988
#define	HCOUT9DMACTRL	0x00000998
#define	HCOUT10DMACTRL	0x000009A8
#define	HCOUT11DMACTRL	0x000009B8
#define	HCOUT12DMACTRL	0x000009C8
#define	HCOUT13DMACTRL	0x000009D8
#define	HCOUT14DMACTRL	0x000009E8
#define	HCOUT15DMACTRL	0x000009F8

#define	HCOUT1DMACOMPLETECNT	0x0000091C
#define	HCOUT2DMACOMPLETECNT	0x0000092C
#define	HCOUT3DMACOMPLETECNT	0x0000093C
#define	HCOUT4DMACOMPLETECNT	0x0000094C
#define	HCOUT5DMACOMPLETECNT	0x0000095C
#define	HCOUT6DMACOMPLETECNT	0x0000096C
#define	HCOUT7DMACOMPLETECNT	0x0000097C
#define	HCOUT8DMACOMPLETECNT	0x0000098C
#define	HCOUT9DMACOMPLETECNT	0x0000099C
#define	HCOUT10DMACOMPLETECNT	0x000009AC
#define	HCOUT11DMACOMPLETECNT	0x000009BC
#define	HCOUT12DMACOMPLETECNT	0x000009CC
#define	HCOUT13DMACOMPLETECNT	0x000009DC
#define	HCOUT14DMACOMPLETECNT	0x000009EC
#define	HCOUT15DMACOMPLETECNT	0x000009FC

#define	HCIN1DMALINKADDR	0x00000810
#define	HCIN2DMALINKADDR	0x00000820
#define	HCIN3DMALINKADDR	0x00000830
#define	HCIN4DMALINKADDR	0x00000840
#define	HCIN5DMALINKADDR	0x00000850
#define	HCIN6DMALINKADDR	0x00000860
#define	HCIN7DMALINKADDR	0x00000870
#define	HCIN8DMALINKADDR	0x00000880
#define	HCIN9DMALINKADDR	0x00000890
#define	HCIN10DMALINKADDR	0x000008A0
#define	HCIN11DMALINKADDR	0x000008B0
#define	HCIN12DMALINKADDR	0x000008C0
#define	HCIN13DMALINKADDR	0x000008D0
#define	HCIN14DMALINKADDR	0x000008E0
#define	HCIN15DMALINKADDR	0x000008F0

#define	HCIN1DMACURADDR		0x00000814
#define	HCIN2DMACURADDR		0x00000824
#define	HCIN3DMACURADDR		0x00000834
#define	HCIN4DMACURADDR		0x00000844
#define	HCIN5DMACURADDR		0x00000854
#define	HCIN6DMACURADDR		0x00000864
#define	HCIN7DMACURADDR		0x00000874
#define	HCIN8DMACURADDR		0x00000884
#define	HCIN9DMACURADDR		0x00000894
#define	HCIN10DMACURADDR	0x000008A4
#define	HCIN11DMACURADDR	0x000008B4
#define	HCIN12DMACURADDR	0x000008C4
#define	HCIN13DMACURADDR	0x000008D4
#define	HCIN14DMACURADDR	0x000008E4
#define	HCIN15DMACURADDR	0x000008F4

#define	HCIN1DMACTRL		0x00000818
#define	HCIN2DMACTRL		0x00000828
#define	HCIN3DMACTRL		0x00000838
#define	HCIN4DMACTRL		0x00000848
#define	HCIN5DMACTRL		0x00000858
#define	HCIN6DMACTRL		0x00000868
#define	HCIN7DMACTRL		0x00000878
#define	HCIN8DMACTRL		0x00000888
#define	HCIN9DMACTRL		0x00000898
#define	HCIN10DMACTRL		0x000008A8
#define	HCIN11DMACTRL		0x000008B8
#define	HCIN12DMACTRL		0x000008C8
#define	HCIN13DMACTRL		0x000008D8
#define	HCIN14DMACTRL		0x000008E8
#define	HCIN15DMACTRL		0x000008F8

#define HCIN1DMACOMPLETECNT		0x0000081C
#define HCIN2DMACOMPLETECNT		0x0000082C
#define HCIN3DMACOMPLETECNT		0x0000083C
#define HCIN4DMACOMPLETECNT		0x0000084C
#define HCIN5DMACOMPLETECNT		0x0000085C
#define HCIN6DMACOMPLETECNT		0x0000086C
#define HCIN7DMACOMPLETECNT		0x0000087C
#define HCIN8DMACOMPLETECNT		0x0000088C
#define HCIN9DMACOMPLETECNT		0x0000089C
#define HCIN10DMACOMPLETECNT	0x000008AC
#define HCIN11DMACOMPLETECNT	0x000008BC
#define HCIN12DMACOMPLETECNT	0x000008CC
#define HCIN13DMACOMPLETECNT	0x000008DC
#define HCIN14DMACOMPLETECNT	0x000008EC
#define HCIN15DMACOMPLETECNT	0x000008FC



/******************************************************************************/
/* DMA LINK_LIST */
/******************************************************************************/
#define DMACTRL_DMACS	(1 << 0)
#define DMACTRL_DMACC	(1 << 1)

/******************************************************************************/
/*OTG external Registers USBEIRQ, USBEIEN. */
/******************************************************************************/
#define USBEIRQ_USBIRQ          (0x1 << 7)
#define USBEIRQ_USBIEN          (0x1 << 7)

/* USBERESET USBERES*/
#define USBERES_USBRESET        (1 << 0)

/*VDCTRL*/
#define VDCTRL_VLOAD            (1 << 4)
#define VDCTRL_VCONTROL(x)      ((x) & 0xf)

/******************************************************************************/
/*OTG SFR Registers*/
/******************************************************************************/
/*OTGIRQ*/
#define	OTGIRQ_PERIPH	            (1<<4)
#define	OTGIRQ_VBUSEER		    (1<<3)
#define	OTGIRQ_LOCSOF	            (1<<2)
#define	OTGIRQ_SRPDET		    (1<<1)
#define	OTGIRQ_IDLE		    (1<<0)

/*bit 7:5 reserved*/
/*OTGSTATE*/
#define	A_IDLE				        (0x00)
#define	A_WAIT_VRISE		        (0x01)
#define	A_WAIT_BCON		            (0x02)
#define	A_HOST				        (0x03)
#define	A_SUSPEND			        (0x04)
#define	A_PHERIPHERAL	            (0x05)
#define	A_VBUS_ERR		            (0x06)
#define	A_WAIT_VFAL		            (0x07)
#define B_IDLE				        (0x08)
#define	B_PHERIPHERAL	            (0x09)
#define	B_WAIT_ACON		            (0x0a)
#define	B_HOST				        (0x0b)
#define	B_SRP_INIT1		            (0x0c)
#define	B_SRP_INIT2		            (0x0d)
#define	B_DISCHRG1		            (0x0e)
#define	B_DISCHRG2		            (0x0f)

/* OTGSTATE value. */
/* extra dual-role default-b states */
/* dual-role default-a */
#define AOTG_STATE_A_IDLE		0
#define AOTG_STATE_A_WAIT_VRISE		1
#define AOTG_STATE_A_WAIT_BCON		2
#define AOTG_STATE_A_HOST		3
#define AOTG_STATE_A_SUSPEND		4
#define AOTG_STATE_A_PERIPHERAL		5
#define AOTG_STATE_A_VBUS_ERR		6
#define AOTG_STATE_A_WAIT_VFALL		7
/* single-role peripheral, and dual-role default-b */
#define AOTG_STATE_B_IDLE		8
#define AOTG_STATE_B_PERIPHERAL		9
#define AOTG_STATE_B_WAIT_ACON		10
#define AOTG_STATE_B_HOST		11
#define AOTG_STATE_B_SRP_INIT		12
#define AOTG_STATE_UNDEFINED		17

/*bit 7:4 reserved*/
/*OTGCTRL*/
#define	OTGCTRL_FORCEBCONN	        (1 << 7)
/*bit 6 reserved*/
#define	OTGCTRL_SRPDATDETEN 		(1 << 5)
#define	OTGCTRL_SRPVBUSDETEN 		(1 << 4)
#define	OTGCTRL_BHNPEN				(1 << 3)
#define	OTGCTRL_ASETBHNPEN			(1 << 2)
#define	OTGCTRL_ABUSDROP			(1 << 1)
#define	OTGCTRL_BUSREQ				(1 << 0)

/*OTGSTATUS*/
/*bit 7 reserved*/
#define	OTGSTATUS_ID	        	(1 << 6)
#define	OTGSTATUS_AVBUSSVAL 		(1 << 5)
#define	OTGSTATUS_BSESSEND			(1 << 4)
#define	OTGSTATUS_ASESSVAL			(1 << 3)
#define	OTGSTATUS_BSESSVAL			(1 << 2)
#define	OTGSTATUS_CONN				(1 << 1)
#define	OTGSTATUS_BSE0SRP			(1 << 0)

/*OTGIEN*/
/*bit 7:5 reserved*/
#define	OTGIEN_PERIPH	            (1 << 4)
#define	OTGIEN_VBUSEER		        (1 << 3)
#define	OTGIEN_LOCSOF	            (1 << 2)
#define	OTGIEN_SRPDET		        (1 << 1)
#define	OTGIEN_IDLE			        (1 << 0)

/*HCEP0CTRL*/
#define HCEP0CTRL_ENDPNR(x)         (((x) & 0xf) << 0)
/*bit 7:4 reserved*/

/* HCINxERR */
#define	HCINxERR_TYPE_MASK		(0xf << 2)
/*
 * 0000 – reserved (no error)
 * 0001 – CRC error
 * 0010 – data toggle mismatch
 * 0011 – endpoint sent STALL handshake
 * 0100 – no endpoint handshake (timeout)
 * 0101 – PID error (pid check=error or unknown PID)
 * 0110 – Data Overrun (too long packet – babble)
 * 0111 – Data Underrun (packet shorter than MaxPacketSize)
 * 1xxx – Spilit Transaction Error, reference to HCOUTxSPILITCS register
*/
#define	HCINxERR_NO_ERR			(0x0 << 2)
#define	HCINxERR_CRC_ERR		(0x1 << 2)
#define	HCINxERR_TOG_ERR		(0x2 << 2)
#define	HCINxERR_STALL			(0x3 << 2)
#define	HCINxERR_TIMEOUT		(0x4 << 2)
#define	HCINxERR_PID_ERR		(0x5 << 2)
#define	HCINxERR_OVER_RUN		(0x6 << 2)
#define	HCINxERR_UNDER_RUN		(0x7 << 2)
#define	HCINxERR_SPLIET			(0x8 << 2)
#define	HCINxERR_RESEND			(1 << 6)

/*HCOUTXCTRL*/
//#define HCOUTXCTRL(x)               (((x) & 0xf) << 0)
/*bit 7:4 reserved*/


/******************************************************************************/
/*Device Mode Special Function Registers*/
/******************************************************************************/
/*EP0CS*/
/*bit 7 reserved*/
#define EP0CS_HCSETTOOGLE           (1 << 6)
#define EP0CS_HCCLRTOOGLE           (1 << 5)
#define EP0CS_HCSET                 (1 << 4)
#define EP0CS_HCINBSY		        (1 << 3)
#define	EP0CS_HCOUTBSY	            (1 << 2)
#define EP0CS_OUTBSY	            (1 << 3)
#define	EP0CS_INBSY	                (1 << 2)
#define	EP0CS_HSNAK	                (1 << 1)
#define	EP0CS_STALL	                (1 << 0)

/*EPXCS host & device*/
/* HCIN1CS */
/*bit 7 reserved*/
//#define	EPCS_AUTO_IN		        (1 << 4)
//#define	EPCS_AUTO_OUT		        (1 << 4)
#define EPCS_NPAK                   	(0x3 << 2)
#define	EPCS_BUSY		       (1 << 1)
#define	EPCS_ERR		       (1 << 0)

/*EPXCON host & device*/
#define	EPCON_VAL	                (1 << 7)
#define	EPCON_STALL	                (1 << 6)
#define	EPCON_TYPE                  (0x3 << 2)
#define	EPCON_TYPE_INT		        (0x3 << 2)
#define	EPCON_TYPE_BULK	            (0x2 << 2)
#define	EPCON_TYPE_ISO		        (0x1 << 2)
#define	EPCON_BUF                   (0x03)
#define	EPCON_BUF_QUAD	            (0x03)
#define	EPCON_BUF_TRIPLE	        (0x02)
#define	EPCON_BUF_DOUBLE	        (0x01)
#define	EPCON_BUF_SINGLE	        (0x00)

/*OUTXIRQ*/
//#define OUTXIRQ(x)                  (1 << (x))
/*INXIRQ*/
//#define INXIRQ(x)                   (1 << (x))

/*USBIRQ*/
#define	USBIRQ_HS	                (1 << 5)
#define	USBIRQ_URES	                (1 << 4)
#define	USBIRQ_SUSP	                (1 << 3)
#define	USBIRQ_SUTOK	            (1 << 2)
#define	USBIRQ_SOF	                (1 << 1)
#define	USBIRQ_SUDAV	            (1 << 0)

/*OUTXIEN*/
//#define OUTXIEN(x)                  (1 << (x))
/*INXIEN*/
//#define INXIEN(x)                   (1 << (x))

/*USBIEN*/
/*bit 7:6 reserved*/
#define	USBIEN_HS	                (1 << 5)
#define	USBIEN_URES               	(1 << 4)
#define	USBIEN_SUSP             	(1 << 3)
#define	USBIEN_SUTOK            	(1 << 2)
#define	USBIEN_SOF	                (1 << 1)
#define	USBIEN_SUDAV            	(1 << 0)

/* IVECT, USB Interrupt Vector. */
//#define UIV_OTGIRQ          (0xd8)

#define UIV_SUDAV           0x01
#define UIV_SOF             0x02
#define UIV_SUTOK           0x03
#define UIV_SUSPEND         0x04
#define UIV_USBRESET        0x05
#define UIV_HSPEED          0x06

/* otg status. */
#define UIV_IDLE            0x07
#define UIV_SRPDET          0x08
#define UIV_LOCSOF          0x09
#define UIV_VBUSERR         0x0a
#define UIV_PERIPH          0x0b

#define UIV_HCOUT0ERR       0x10
#define UIV_EP0IN           0x20
#define UIV_HCEP0OUT        0x20
#define UIV_IN0TOKEN        0x30
#define UIV_HCIN0ERR        0x40
#define UIV_EP0OUT          0x50
#define UIV_HCEP0IN         0x50
#define UIV_OUT0TOKEN       0x60
#define UIV_EP0PING         0x70

#define UIV_HCOUT1ERR       0x11
#define UIV_EP1IN           0x21
#define UIV_HCEP1OUT        0x21
#define UIV_IN0T1KEN        0x31
#define UIV_HCIN1ERR        0x41
#define UIV_EP1OUT          0x51
#define UIV_HCEP1IN         0x51
#define UIV_OUT1TOKEN       0x61
#define UIV_EP1PING         0x71

#define UIV_HCOUT2ERR       0x12
#define UIV_EP2IN           0x22
#define UIV_HCEP2OUT        0x22
#define UIV_IN2TOKEN        0x32
#define UIV_HCIN2ERR        0x42
#define UIV_EP2OUT          0x52
#define UIV_HCEP2IN         0x52
#define UIV_OUT2TOKEN       0x62
#define UIV_EP2PING         0x72

#define UIV_HCOUT3ERR       0x13
#define UIV_EP3IN           0x23
#define UIV_HCEP3OUT        0x23
#define UIV_IN3TOKEN        0x33
#define UIV_HCIN3ERR        0x43
#define UIV_EP3OUT          0x53
#define UIV_HCEP3IN         0x53
#define UIV_OUT3TOKEN       0x63
#define UIV_EP3PING         0x73

#define UIV_HCOUT4ERR       0x14
#define UIV_EP4IN           0x24
#define UIV_HCEP4OUT        0x24
#define UIV_IN4TOKEN        0x34
#define UIV_HCIN4ERR        0x44
#define UIV_EP4OUT          0x54
#define UIV_HCEP4IN         0x54
#define UIV_OUT4TOKEN       0x64
#define UIV_EP4PING         0x74

#define UIV_HCOUT5ERR       0x15
#define UIV_EP5IN           0x25
#define UIV_HCEP5OUT        0x25
#define UIV_IN5TOKEN        0x35
#define UIV_HCIN5ERR        0x45
#define UIV_EP5OUT          0x55
#define UIV_HCEP5IN         0x55
#define UIV_OUT5TOKEN       0x65
#define UIV_EP5PING         0x75

#define UIV_HCOUT6ERR       0x16
#define UIV_EP6IN           0x26
#define UIV_HCEP6OUT        0x26
#define UIV_IN6TOKEN        0x36
#define UIV_HCIN6ERR        0x46
#define UIV_EP6OUT          0x56
#define UIV_HCEP6IN         0x56
#define UIV_OUT6TOKEN       0x66
#define UIV_EP6PING         0x76

#define UIV_HCOUT7ERR       0x17
#define UIV_EP7IN           0x27
#define UIV_HCEP7OUT        0x27
#define UIV_IN7TOKEN        0x37
#define UIV_HCIN7ERR        0x47
#define UIV_EP7OUT          0x57
#define UIV_HCEP7IN         0x57
#define UIV_OUT7TOKEN       0x67
#define UIV_EP7PING         0x77

#define UIV_HCOUT8ERR       0x18
#define UIV_EP8IN           0x28
#define UIV_HCEP8OUT        0x28
#define UIV_IN8TOKEN        0x38
#define UIV_HCIN8ERR        0x48
#define UIV_EP8OUT          0x58
#define UIV_HCEP8IN         0x58
#define UIV_OUT8TOKEN       0x68
#define UIV_EP8PING         0x78

#define UIV_HCOUT9ERR       0x19
#define UIV_EP9IN           0x29
#define UIV_HCEP9OUT        0x29
#define UIV_IN9TOKEN        0x39
#define UIV_HCIN9ERR        0x49
#define UIV_EP9OUT          0x59
#define UIV_HCEP9IN         0x59
#define UIV_OUT9TOKEN       0x69
#define UIV_EP9PING         0x79

#define UIV_HCOUT10ERR       0x1a
#define UIV_EP10IN           0x2a
#define UIV_HCEP10OUT        0x2a
#define UIV_IN10TOKEN        0x3a
#define UIV_HCIN10ERR        0x4a
#define UIV_EP10OUT          0x5a
#define UIV_HCEP10IN         0x5a
#define UIV_OUT10TOKEN       0x6a
#define UIV_EP10PING         0x7a

#define UIV_HCOUT11ERR       0x1b
#define UIV_EP11IN           0x2b
#define UIV_HCEP11OUT        0x2b
#define UIV_IN11TOKEN        0x3b
#define UIV_HCIN11ERR        0x4b
#define UIV_EP11OUT          0x5b
#define UIV_HCEP11IN         0x5b
#define UIV_OUT11TOKEN       0x6b
#define UIV_EP11PING         0x7b

#define UIV_HCOUT12ERR       0x1c
#define UIV_EP12IN           0x2c
#define UIV_HCEP12OUT        0x2c
#define UIV_IN12TOKEN        0x3c
#define UIV_HCIN12ERR        0x4c
#define UIV_EP12OUT          0x5c
#define UIV_HCEP12IN         0x5c
#define UIV_OUT12TOKEN       0x6c
#define UIV_EP12PING         0x7c

#define UIV_HCOUT13ERR       0x1d
#define UIV_EP13IN           0x2d
#define UIV_HCEP13OUT        0x2d
#define UIV_IN13TOKEN        0x3d
#define UIV_HCIN13ERR        0x4d
#define UIV_EP13OUT          0x5d
#define UIV_HCEP13IN         0x5d
#define UIV_OUT13TOKEN       0x6d
#define UIV_EP13PING         0x7d 

#define UIV_HCOUT14ERR       0x1e
#define UIV_EP14IN           0x2e
#define UIV_HCEP14OUT        0x2e
#define UIV_IN14TOKEN        0x3e
#define UIV_HCIN14ERR        0x4e
#define UIV_EP14OUT          0x5e
#define UIV_HCEP14IN         0x5e
#define UIV_OUT14TOKEN       0x6e
#define UIV_EP14PING         0x7e 

#define UIV_HCOUT15ERR       0x1f
#define UIV_EP15IN           0x2f
#define UIV_HCEP15OUT        0x2f
#define UIV_IN15TOKEN        0x3f
#define UIV_HCIN15ERR        0x4f
#define UIV_EP15OUT          0x5f
#define UIV_HCEP15IN         0x5f
#define UIV_OUT15TOKEN       0x6f
#define UIV_EP15PING         0x7f 


/*ENDPRST*/
#define ENDPRST_EPX(x)              ((x) & 0xf)
#define ENDPRST_IO                  (1 << 4)
#define ENDPRST_FIFORST             (1 << 5)
#define ENDPRST_TOGRST              (1 << 6)
#define ENDPRST_TOGRST_R            (0x3 << 6)
/*bit 7 reserved*/

/*USBCS*/
/*bit 7 reserved*/
#define	USBCS_DISCONN		        (1 << 6)
#define	USBCS_SIGRSUME		        (1 << 5)
#define	USBCS_HFMODE                (1 << 1)
#define USBCS_LSMODE                (1 << 0)

#if 0 /******** need to check bits. ***********************/
/*FIFOCTRL*/
/*bit 7:6 reserved*/
#define FIFOCTRL_EPX(x)             ((x) & 0xf)
#define	FIFOCTRL_FIFOAUTO	        (1 << 5)
#define	FIFOCTRL_IO			        (1 << 4)

#endif


static void inline usb_writeb(u8 val, volatile void __iomem *reg)
{
	writeb(val, reg);
}

static void inline usb_writew(u16 val, volatile void __iomem *reg)
{
	writew(val, reg);
}

static void inline usb_writel(u32 val, volatile void __iomem *reg)
{
	writel(val, reg);
}

/******************************************************************************/
static inline u8 usb_readb(volatile void __iomem *reg)
{
	return readb(reg);
}

static inline u16 usb_readw(volatile void __iomem *reg)
{
	return readw(reg);
}

static inline u32 usb_readl(volatile void __iomem *reg)
{
	return readl(reg);
}

/******************************************************************************/
static void inline usb_setb(u8 val, volatile void __iomem *reg)
{
	//act_setb(val, (u32)reg);
	writeb(readb(reg) | val, reg);
}

static void inline usb_setw(u16 val,volatile void __iomem *reg)
{
	//act_setw(val, (u32)reg);
	writew(readw(reg) | val, reg);
}

static void inline usb_setl(u32 val,volatile void __iomem *reg)
{
	//act_setl(val, (u32)reg);
	writel(readl(reg) | val, reg);
}

/******************************************************************************/
static void inline usb_clearb(u8 val,volatile void __iomem *reg)
{
	//act_clearb(val, (u32)reg);
	writeb(readb(reg)&(~val),reg);
}

static void inline usb_clearw(u16 val,volatile void __iomem *reg)
{
	//act_clearw(val, (u32)reg);
	writew(readw(reg)&(~val),reg);
}

static void inline usb_clearl(u32 val,volatile void __iomem *reg)
{
	//act_clearl(val, (u32)reg);
	writel(readl(reg)&(~val),reg);
}

/*********************** old define. *****************************************/

static inline void usb_setbitsb(u8 mask, volatile void __iomem *mem)
{
	writeb(readb(mem) | mask, mem);
}

static inline void usb_setbitsw(u16 mask, volatile void __iomem *mem)
{
	writew(readw(mem) | mask, mem);
}

static inline void usb_setbitsl(ulong mask, volatile void __iomem *mem)
{
	writel(readl(mem) | mask, mem);
}

static inline void usb_clearbitsb(u8 mask, volatile void __iomem *mem)
{
	writeb(readb(mem) & ~mask, mem);
}

static inline void usb_clearbitsw(u16 mask, volatile void __iomem *mem)
{
	writew(readw(mem) & ~mask, mem);
}

static inline void usb_clearbitsl(ulong mask, volatile void __iomem *mem)
{
	writel(readl(mem) & ~mask, mem);
}

#endif  /* __AOTG_REGS_H__ */

