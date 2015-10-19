#ifndef _CTP_DETECT_H_
#define _CTP_DETECT_H_

struct ctp_device
{
	char * name;			//0.IC名称
	char * ko_name; 		//1.ko名称
	bool need_detect;		//2.是否扫描
	unsigned int i2c_addr;	//3.i2c地址
	bool has_chipid;		//4.有chipid
	unsigned int chipid_req;//5.chipid寄存器
	unsigned int chipid;	//6.chipid
};

//每增加一款IC，往这个列表里添加
struct ctp_device ctp_device_list[]=
{
//注意如果两个ic的i2c地址相同，把有chipid的放在前面。
	//ICN83XX
	{
		"ICN83XX",			//0.IC名称
		"ctp_icn83xx.ko", //"ctp_icn838x_ts_iic.ko"	//1.ko名称
		true,				//2.是否扫描
		0x40,				//3.i2c地址
		true,				//4.有chipid
		0x0a,				//5.chipid寄存器
		0x83,				//6.chipid
	},
	//GSL1680,GSL3670,GSL3680,因这三款通道数不同，一般不会混用，
	//且1680有D版E版，3680有A版B版，chipid均不同，全部扫描太麻烦，这里不再区分。
	{
		"GSLX6X0",			//0.IC名称
		"ctp_gslX680.ko",	//1.ko名称
		true,				//2.是否扫描
		0x40,				//3.i2c地址
		false,				//4.有chipid
		0xfc,					//5.chipid寄存器
		0x0,//1680:0x0			//6.chipid
	},
	//FT5206,FT5406
	{
		"FT52-406",			//0.IC名称
		"ctp_ft5X06.ko",	//1.ko名称
		true,				//2.是否扫描
		0x38,				//3.i2c地址
		true,				//4.有chipid
		0xA3,				//5.chipid寄存器
		0x55,				//6.chipid
	},
	//FT5606
	{
		"FT5606",			//0.IC名称
		"ctp_ft5X06.ko",	//1.ko名称
		true,				//2.是否扫描
		0x38,				//3.i2c地址
		true,				//4.有chipid
		0xA3,				//5.chipid寄存器
		0x08,				//6.chipid
	},
	//GT813
	{
		"GT813",			//0.IC名称
		"ctp_goodix_touch.ko",	//1.ko名称
		true,				//2.是否扫描
		0x5d,				//3.i2c地址
		true,				//4.有chipid
		0xf7d,				//5.chipid寄存器
		0x13,				//6.chipid
	},
	//AW5206
	{
		"AW5206",			//0.IC名称
		"ctp_aw5306.ko",	//1.ko名称
		true,				//2.是否扫描
		0x38,				//3.i2c地址
		true,				//4.有chipid
		0x01,				//5.chipid寄存器
		0xA8,				//6.chipid
	},
	//AW5209
	{
		"AW5209",			//0.IC名称
		"ctp_aw5209.ko",	//1.ko名称
		true,				//2.是否扫描
		0x38,				//3.i2c地址
		true,				//4.有chipid
		0x01,				//5.chipid寄存器
		0xB8,				//6.chipid
	},
	//CT36X
	{
		"CT36X",			//0.IC名称
		"ctp_ct36x_i2c_ts.ko",	//1.ko名称
		true,				//2.是否扫描
		0x01,				//3.i2c地址
		true,				//4.有chipid
		0x00,//???				//5.chipid寄存器
		0x00,//0x02:CT360,0x01:CT363,CT365	//6.chipid
	},
	//HL3X06
	{
		"HL3X06",			//0.IC名称
		"ctp_hl3x06.ko",	//1.ko名称
		true,				//2.是否扫描
		0x3B,				//3.i2c地址
		false,				//4.有chipid
		0x00,				//5.chipid寄存器
		0x30,				//6.chipid
	},
	//ILITEK
	{
		"ILITEK",			//0.IC名称
		"ctp_ilitek_aimvF.ko",	//1.ko名称
		true,				//2.是否扫描
		0x41,				//3.i2c地址
		false,	//确实无			//4.有chipid
		0,				//5.chipid寄存器
		0,				//6.chipid
	},
	//ili2672
	{
		"ili2672",			//0.IC名称
		"ctp_ili2672.ko",	//1.ko名称
		true,				//2.是否扫描
		0x41,				//3.i2c地址
		false,	//确实无			//4.有chipid
		0,				//5.chipid寄存器
		0,				//6.chipid
	},
	//ft5x06
	{
		"ft5x06",			//0.IC名称
		"ctp_ft5x06.ko",	//1.ko名称
		true,				//2.是否扫描
		0x38,				//3.i2c地址
		false,	//确实无			//4.有chipid
		0,				//5.chipid寄存器
		0,				//6.chipid
	},	
	//MT395
	{
		"MT395",			//0.IC名称
		"ctp_mt395.ko",	//1.ko名称
		true,				//2.是否扫描
		0x67,				//3.i2c地址
		false,				//4.有chipid
		0,				//5.chipid寄存器
		0,				//6.chipid
	},	
	//NOVATEK
	{
		"NT1100X",			//0.IC名称
		"ctp_Novatek_TouchDriver.ko",	//1.ko名称
		true,				//2.是否扫描
		0x01,				//3.i2c地址
		false,				//4.有chipid
		0x00,	//???		//5.chipid寄存器
		0,	//???			//6.chipid
	},	
	//SSD254X
	{
		"SSD254X",			//0.IC名称
		"ctp_ssd254x.ko",	//1.ko名称
		true,				//2.是否扫描
		0x48,				//3.i2c地址
		true,				//4.有chipid
		0x02,					//5.chipid寄存器
		0x25,	//0x2541,0x2543			//6.chipid
	},		
};


#endif
