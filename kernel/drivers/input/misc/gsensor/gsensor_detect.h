#ifndef _GSENSOR_DETECT_H_
#define _GSENSOR_DETECT_H_

struct gsensor_device
{
    char * name;            //0.IC名称
    char * ko_name;         //1.ko名称
    bool has_sa0;             //2.有sa0 pin
    unsigned char i2c_addr;    //3.i2c地址
    bool has_chipid;        //4.有chipid
    unsigned char chipid_reg; //5.chipid寄存器
    unsigned char chipid[2];   //6.chipid 
    bool need_detect;	     //7.是否扫描
};

//每增加一款IC，往这个列表里添加
//注意如果两个ic的i2c地址相同，把有chipid的放在前面。
struct gsensor_device gsensor_device_list[]=
{
    // AFA750
    {
        "afa750",                    //0.IC名称
        "gsensor_afa750.ko",    //1.ko名称
        true,                           //2.有sa0 pin
        0x3c,                          //3.i2c地址
        true,                           //4.有chipid
        0x37,                          //5.chipid寄存器
        {0x3c, 0x3d},              //6.chipid
        true,                           //7.是否扫描
    },
    // BMA220
    {
        "bma220",                    //0.IC名称
        "gsensor_bma220.ko",    //1.ko名称
        false,                           //2.有sa0 pin
        0x0a,                          //3.i2c地址
        true,                           //4.有chipid
        0x00,                          //5.chipid寄存器
        {0xdd, 0xdd},              //6.chipid
        true,                           //7.是否扫描
    },
    
    // BMA222 bma223
    {
        "bma222",                    //0.IC名称
        "gsensor_bma222.ko",    //1.ko名称
        false,                           //2.有sa0 pin
        0x18,                          //3.i2c地址
        true,                           //4.有chipid
        0x00,                          //5.chipid寄存器
        {0x02, 0xf8},              //6.chipid
        true,                           //7.是否扫描
    },
    
    // BMA250/BMA250E
    {
        "bma250",                    //0.IC名称
        "gsensor_bma250.ko",    //1.ko名称
        false,                           //2.有sa0 pin
        0x18,                          //3.i2c地址
        true,                           //4.有chipid
        0x00,                          //5.chipid寄存器
        {0x03, 0xf9},              //6.chipid
        //true,                           //7.是否扫描
        false,                           //7.是否扫描
    },
    // DMARD10
    {
        "dmard10",                    //0.IC名称
        "gsensor_dmard10.ko",    //1.ko名称
        false,                           //2.有sa0 pin
        0x18,                          //3.i2c地址
        false,                           //4.有chipid
        0x00,                          //5.chipid寄存器
        {0x00, 0x00},              //6.chipid
        true,                           //7.是否扫描
    },
    
    // kxtj9-1007
    {
        "kxtj9",                    //0.IC名称
        "gsensor_kionix_accel.ko",    //1.ko名称
        true,                           //2.有sa0 pin
        0x0e,                          //3.i2c地址
        true,                           //4.有chipid
        0x0f,                          //5.chipid寄存器
        {0x08, 0x08},              //6.chipid
        true,                           //7.是否扫描
    },
    
    // lis3dh
    {
        "lis3dh",                    //0.IC名称
        "gsensor_lis3dh_acc.ko",    //1.ko名称
        true,                           //2.有sa0 pin
        0x18,                          //3.i2c地址
        true,                           //4.有chipid
        0x0f,                          //5.chipid寄存器
        {0x33, 0x33},              //6.chipid
        true,                           //7.是否扫描
    },
    
    // mc3210
    {
        "mc3210",                    //0.IC名称
        "gsensor_mc3210.ko",    //1.ko名称
        false,                           //2.有sa0 pin
        0x4c,                          //3.i2c地址
        true,                           //4.有chipid
        0x3b,                          //5.chipid寄存器
        {0x90, 0x90},              //6.chipid
        true,                           //7.是否扫描
    },

    // mc3232
    {
        "mc3232",                    //0.IC名称
        "gsensor_mc3232.ko",    //1.ko名称
        false,                           //2.有sa0 pin
        0x4c,                          //3.i2c地址
        true,                           //4.有chipid
        0x3b,                          //5.chipid寄存器
        {0x19, 0x19},              //6.chipid
        true,                           //7.是否扫描
    },

    // mc3236
    {
        "mc3236",                    //0.IC名称
        "gsensor_mc3236.ko",    //1.ko名称
        false,                           //2.有sa0 pin
        0x4c,                          //3.i2c地址
        true,                           //4.有chipid
        0x3b,                          //5.chipid寄存器
        {0x60, 0x60},              //6.chipid
        true,                           //7.是否扫描
    },

    // mma7660
    {
        "mma7660",                    //0.IC名称
        "gsensor_mma7660.ko",    //1.ko名称
        false,                           //2.有sa0 pin
        0x4c,                          //3.i2c地址
        false,                           //4.有chipid
        0x00,                          //5.chipid寄存器
        {0x00, 0x00},              //6.chipid
        true,                           //7.是否扫描
    },
    
    // mma8452
    {
        "mma8452",                    //0.IC名称
        "gsensor_mma8452.ko",    //1.ko名称
        true,                           //2.有sa0 pin
        0x1c,                          //3.i2c地址
        true,                           //4.有chipid
        0x0d,                          //5.chipid寄存器
        {0x2a, 0x2a},              //6.chipid
        true,                           //7.是否扫描
    },
    
    // stk8312
    {
        "stk8312",                    //0.IC名称
        "gsensor_stk8312.ko",    //1.ko名称
        false,                           //2.有sa0 pin
        0x3d,                          //3.i2c地址
        false,                           //4.有chipid
        0x00,                          //5.chipid寄存器
        {0x00, 0x00},              //6.chipid
        true,                           //7.是否扫描
    },
    
    // stk8313
    {
        "stk8313",                    //0.IC名称
        "gsensor_stk8313.ko",    //1.ko名称
        false,                           //2.有sa0 pin
        0x22,                          //3.i2c地址
        false,                           //4.有chipid
        0x00,                          //5.chipid寄存器
        {0x00, 0x00},              //6.chipid
        true,                           //7.是否扫描
    },
};


#endif
