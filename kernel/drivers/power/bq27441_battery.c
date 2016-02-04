
#if 1
#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/moduleparam.h>

#if 1
#define DEPEND
#define STORE_BQ27441_INFO 	1
#else
#define STORE_BQ27441_INFO 	0
#endif


//#define USED_EXT_TEMP

//config for bq27441
#define TI_CONFIG_STATUS "okay"
#define TI_CONFIG_REG 0x55
#define TI_CONFIG_CAP 3500
#define TI_CONFIG_TERMVOL 3400
#define TI_CONFIG_TERMCUR 250
#define TI_CONFIG_TAPERVOL 4200
#define TI_CONFIG_QMAXCELL 10369
#define TI_CONFIG_LOADSEL 0x81
#define TI_CONFIG_DESIGNENERGY 12950
#define TI_CONFIG_DSGCUR 333
#define TI_CONFIG_CHGCUR 267
#define TI_CONFIG_QUITCUR 500
//#define TI_CONFIG_UPDATAFLAG 0

int TI_CONFIG_RATABLE[] = {106, 106, 107, 119, 85, 72, 79, 85, 77, 73, 94, 116, 116, 291, 467};


#ifdef TI_DEBUG 
#define print_info(fmt, args...)   \
        do{                              \
                printk(fmt, ##args);     \
        }while(0)
#else
#define print_info(fmt, args...)
#endif

#define CONTROL_STATUS	0x0
#define DEVICE_TYPE 		0x1
#define FW_VERSION		0x2
#define DM_CODE               0x4
#define SET_CFGUPDATE	0x13
#define SET_SOFT_RESET	0x42
#define SEALED_CMD		0x20
#define BAT_INSERT		0x0C
#define RESET_CMD		0x41
#define ITPOR				(1 << 5)
#define DRIVER_VESION	1
#define CAP_STEP	1
#define INCREASE_CAP_COUNT		25
#define DEDUCE_CAP_COUNT		30
#define DEDUCE_CAP_COUNT_QUICK	15
#define CAP_HOLD_COUNT			25


#define OP_CONFIG_SUBID 0x40

enum REGISTER_TYPE {
    TYPE_INVALID,
    TYPE_8Bit,
    TYPE_16Bit,
};

enum REG_TYPE
{
	PMU_SYS_CTL9,
};

static int register_atc2603c[] =
{
	0x09, /*0:PMU_SYS_CTL9*/
};

#if STORE_BQ27441_INFO
struct bq27441_info {
	int act_vol;		/* battery voltage */
	int act_cur; 		/* battery current */
	int act_cap;    		/* actually capacity read from eg2801*/
	int remain;    		/* remain capacity */
	int full;    			/* full capacity */
	int flag;
	int health;
	int temp;
	int temp_in;
	int status;
	int control;
	int vir_cap;
};

static struct bq27441_info bq27441_infos;
static struct kobject *hw_gauge_kobj; 
static int first_store_pmu_info_flag = 1;
static bool ti_updataflag_config = false;

static int bq27441_info_store_usb(struct bq27441_info* p_info);
#endif

static struct i2c_client *gauge_client;
static int open_debug;
static int open_save;
static int cap_last_time;
static int cap_chg_count;
static int cap_hold_count = 0;
int gauge = 0;
static int deduce_vir_cap_count = 0;
static int increase_vir_cap_count = 0;
static int duringconfig = 0;


extern void act260x_set_get_hw_cap_point(void *ptr);
extern void act260x_set_get_hw_volt_point(void *ptr);
extern void act260x_set_get_hw_cur_point(void *ptr);
extern void act260x_set_get_hw_temp_point(void *ptr);

extern int pmu_reg_read(unsigned short reg);
extern int pmu_reg_write(unsigned short reg, unsigned short val);

static int bq27441_config(struct i2c_client *client);
static int bq27441_config_check(void);
//extern void system_powerdown_withgpio(void);
extern int pmu_reg_read(unsigned short);

static int isDuringConfig(){
	return duringconfig;
}

static void SetDuringConfig(int val){
	duringconfig = val;
}

#if STORE_BQ27441_INFO
/*
	store log into /data/
*/
static int bq27441_info_store_usb(struct bq27441_info* p_info)
{
	u8 buf[200];
	struct file *filp;
	mm_segment_t fs;
	int offset = 0;
	int h, ms, ms_m, ms_s;

	fs = get_fs();
	set_fs(KERNEL_DS);	
		
	filp = filp_open("/data/bq27441_info.log", O_CREAT | O_RDWR, 0644); //  /mnt/sd-ext/ /mnt/uhost/
	if (IS_ERR(filp)) {
		print_info("[bq27441] can't accessed USB bq27441_info.log.\n");
		return 0;
	}

    if (first_store_pmu_info_flag == 1) {
        memset(buf,0,200);
        offset = sprintf(buf, "time,act_vol, act_cur, act_cap, remain, full, flag, health, temp, temp_in,status, control\t\n");
        filp->f_op->llseek(filp, 0, SEEK_END);
        filp->f_op->write(filp, (char *)buf, offset + 1, &filp->f_pos);
        first_store_pmu_info_flag = 0;
    }
    	
    filp->f_op->llseek(filp, 0, SEEK_END);

    memset(buf,0,200);
    offset = sprintf(buf, "%02d:%02d:%02d,%04d,%04d,%d,%d,%d,0x%04x,0x%04x,%02d, %02d, 0x%04x, 0x%04x, %d\t\n",
                h, ms_m, ms_s,
                p_info->act_vol,
                p_info->act_cur,
                p_info->act_cap,
                p_info->remain,
		p_info->full,
		p_info->flag ,
		p_info->health,
		p_info->temp,
		p_info->temp_in,
		p_info->status,
		p_info->control,
		p_info->vir_cap);
    
    filp->f_op->write(filp, (char *)buf, offset + 1, &filp->f_pos);
    
    set_fs(fs);
    filp_close(filp, NULL);
    return 0;
}
#endif

static int bq27441_get_regs(struct i2c_client *client, 
                int addr, u8 *buff, int count)
{
	struct i2c_msg msg[2];
	int err;
	u8 address;

	address = addr & 0xff;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].buf = &address;
	msg[0].len = 1;
	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = buff;
	msg[1].len = count;

	err = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	if ( err != 2 ) {
		print_info("[bq27441] transfer failed(%d)\n", err);
		return -EFAULT;
	}

	return 0;
}

static int bq27441_set_reg(struct i2c_client *client, 
    enum REGISTER_TYPE type, int addr, u16 data)
{
	unsigned char buffer[3];
	int ret, count;

	switch( type ) {
	    case TYPE_8Bit:
	        buffer[0] = addr & 0xff;
	        buffer[1] = data & 0xff;
	        count = 2;
	        break;
	    case TYPE_16Bit:
	        buffer[0] = addr & 0xff;
	        buffer[1] = (data >> 8) & 0xff;
	        buffer[2] = data & 0xff;
	        count = 3;
	        break;
	    default:
	        return -EFAULT;
	}
    
	ret = i2c_master_send(client, buffer, count);
	if ( ret != count) {
		print_info("[bq27441] set reg failed.\n");
		return -EFAULT;
	}

	return 0;
}

static int bq27441_set_subcmd(struct i2c_client *client, u8 sub_cmd)
{
	unsigned char buffer[3];
	int ret, count;

	buffer[0] = 0x00;
	buffer[1] = sub_cmd & 0xff;
	buffer[2] = 0x00;
	count = 3;

	ret = i2c_master_send(client, buffer, count);
	if ( ret != count) {
		pr_err("[bq27441] set reg failed.\n");
		return -EFAULT;
	}

	return 0;
}

static void store_first_product(void)
{
	int data;	
	data = pmu_reg_read(register_atc2603c[PMU_SYS_CTL9]);
	data |= 0x8000;
	pmu_reg_write(register_atc2603c[PMU_SYS_CTL9], data);
}

/*
	get first product flag
*/
static bool get_first_product(void)
{
	int pmu_sys_ctl9 = pmu_reg_read(register_atc2603c[PMU_SYS_CTL9]);
	printk(KERN_ERR "[%s]:pmu_sys_ctl9:0x%x\n", __func__, pmu_sys_ctl9);
	if (pmu_sys_ctl9 & 0x8000)	
	{
		return false;
	}
	else 
	{
		return true;
	}
}

/*
	get_pmu_battery_volatge
	get voltage from pmu
*/
static int get_pmu_battery_volatge()
{
	int tmp = 0 ;	
	int pmu_bat_v = 0;
	tmp = pmu_reg_read(0x40);
	pmu_bat_v = tmp * 2930 * 2 / 1000;
	//print_info("get_pmu_battery_volatge %d\n",tmp);
	return pmu_bat_v;
}

/*
get_battery_healty
if battery voltage < 3200 return 0 else return 1
*/
static int get_battery_healty()
{
	int pmu_bat_v = 0;

	pmu_bat_v = get_pmu_battery_volatge();
	if(pmu_bat_v > 3200)
		return 1;
	else
		return 0;
}

/*
	check battery error
*/
static int setError = 0;
static int bq27441_check_error()
{
	int err = 0;
	int temperature = 0;
	int volatge = 0;
	u8 vbuffer[2];
	u8 tbuffer[2];
	u8 ebuffer[2];
	if(setError == 1)
		return -1;
	
	if(get_battery_healty() == 0 || isDuringConfig() == 1)
		return -1;

	err = bq27441_set_subcmd(gauge_client, DM_CODE);
	err = bq27441_get_regs(gauge_client, 0x0, ebuffer, 2);

	//print_info("version[%x][%x][%x] \n", ((ebuffer[0]) | ebuffer[1]<<8),ebuffer[0],ebuffer[1]);

	memset(vbuffer,sizeof(vbuffer),0);
	err = bq27441_get_regs(gauge_client, 0x04, vbuffer, 2);
	if ( err ) 
	{ 
		//print_info("[bq27441] check err : i2c error\n"); 
		//print_info("volatge[%x][%x][%x] \n", ((vbuffer[0]) | vbuffer[1]<<8),vbuffer[0],vbuffer[1]);
		return -1;
	} 
	volatge = ((vbuffer[0]) | vbuffer[1]<<8);
	
	// 273.16K = 0C
	memset(tbuffer,sizeof(tbuffer),0);
	err = bq27441_get_regs(gauge_client, 0x1e, tbuffer, 2);
	if ( err ) 
	{ 
		//print_info("[eg2801] check err : i2c error\n"); 
		//print_info("temperature [%x][%x][%x]  \n", ((tbuffer[0]) | tbuffer[1]<<8),tbuffer[0],tbuffer[1]);

		return -1;
	} 
	temperature = ((tbuffer[0]) | tbuffer[1]<<8);

	if(volatge < 2048 || temperature	 < 2048 ||  ((ebuffer[0]) | ebuffer[1]<<8) != 0x48)
	{
		//print_info("[eg2801] check err : date error\n");
		//print_info("temperature [%x][%x][%x] volatge[%x][%x][%x] \n",temperature,tbuffer[0],tbuffer[1],volatge,vbuffer[0],vbuffer[1]);
		return -2;
	}
	
	//print_info("temperature [%x][%x][%x] volatge[%x][%x][%x] \n",volatge,tbuffer[0],tbuffer[1],volatge,vbuffer[0],vbuffer[1]);
	return 0;
}


static int bq27441_get_real_capacity(void)
{
	u8 data[2];
	int ret = 0;
	int capacity = 0;
	
	if(get_battery_healty()==0 || isDuringConfig() == 1)
		return 0;
	
	ret = bq27441_get_regs(gauge_client, 0x1c, data, 2);
	if (ret) {
		print_info("[bq27441] bq27441_get_real_voltage failed\n"); 
		return -1; 
	} 

	capacity =((data[0]) | data[1]<<8);
	return capacity;
}


static int bq27441_get_Flags_FC(void){
	u8 data[2];
	int ret = 0;
	int Flags = 0;
	int mask = 0xFDFF;
	if(get_battery_healty()==0 || isDuringConfig() == 1)
		return 0;
	
	ret = bq27441_get_regs(gauge_client, 0x06, data, 2);
	if (ret) {
		print_info("[bq27441] bq27441_get_Flag_FC failed\n"); 
		return -1; 
	} 

	Flags = ((data[0]) | data[1]<<8);

	if(Flags&(~mask) != 0){
		return 1;
	}
	
	print_info("Flags = %x , [%x][%x] [%x]",Flags,data[1],data[0],Flags&(~mask));
		
	return 0;

}

static int testTemp = 0;
static int bq27441_get_real_temperature(void)
{
	u8 data[2];
	int ret = 0;
	int temperature = 0;

	if(testTemp != 0){
		return testTemp;
	}

	if(get_battery_healty()==0 || isDuringConfig() == 1)
		return 0;
	
	ret = bq27441_get_regs(gauge_client, 0x1e, data, 2);

	if (ret) {
		print_info("[bq27441] bq27441_get_real_temperature failed\n"); 
		return -1; 
	} 
	temperature = ((data[0]) | data[1]<<8);
	temperature = temperature / 10 - 273;
	print_info("[bq27441] temp = %d data[0]=%d data[1]=%d\n",temperature,data[0],data[1]); 
	return temperature;
}

static int bq27441_get_real_voltage(void)
{
	u8 data[2];
	int ret = 0;
	int voltage = 0;
	
	if(get_battery_healty()==0 || isDuringConfig() == 1)
		return 0;
	
	ret = bq27441_get_regs(gauge_client, 0x4, data, 2);
	if (ret) {
		print_info("[bq27441] bq27441_get_real_voltage failed\n"); 
		return -1; 
	} 

	voltage = ((data[0]) | data[1]<<8);
	
	return voltage;
}

static int bq27441_get_real_current(void)
{
	u8 data[2];
	int ret = 0, bat_current;
	
	if(get_battery_healty()==0 || isDuringConfig() == 1)
		return 0;
	
	ret = bq27441_get_regs(gauge_client, 0x10, data, 2);
	if (ret) { 
		print_info("[bq27441] bq27441_get_real_current failed\n"); 
		return -1; 
	} 

	bat_current = ((data[0]) | data[1]<<8);
	if (data[1] & 0x80)
		bat_current |= 0xFFFF0000;

	return bat_current;
}

static int bq27441_get_type(void)
{
	return 1;
}

static int bq27441_get_capacity(void)
{
	int act_cap = 0, vir_cap, real_cap, i, deduce_count;
	int act_vol = 0;
	int act_cur = 0;
	int ret;
	u8 data[2];
	int remain, full, health, temp, temp_in, flag, status, control;
	//return 0;
	if(get_battery_healty()==0 || isDuringConfig() == 1)
		return 0;
	
#if STORE_BQ27441_INFO
	struct bq27441_info* p_info = &bq27441_infos;
#endif

	for (i = 0; i < 16; i++) {
		ret = bq27441_get_regs(gauge_client, 0x06, data, 2);
		if (!ret) 
			break;
		if (i == 15) {
			pr_err("[bq27441]get_caoacity flags failed,cap_last_time=%d , system_powerdown_withgpio\n", cap_last_time);
			//system_powerdown_withgpio();
			return cap_last_time;
		}
	}

	ret = ((data[0]) | (data[1]<<8));
	if (ret & ITPOR) {
		ret = bq27441_get_regs(gauge_client, 0x06, data, 2);
		if (ret) {
			pr_err("[bq27441]get_caoacity flags failed,cap_last_time=%d \n", cap_last_time);
			return cap_last_time;
		}
		ret = ((data[0]) | data[1]<<8);
		if (ret & ITPOR) {
			print_info("[bq27441][RECONFIG] WATCH DOG HAS DEAD,RECONFIG \n");
			pr_err("[bq27441]reconfig, flags=0x%x\n", ret);
			ret = bq27441_config(gauge_client);
			if ( ret < 0) {
				pr_err("[bq27441] First config failed\n");
				msleep(1000);
				ret = bq27441_config(gauge_client);
				if ( ret < 0) {
					pr_err("[bq27441] Second config failed\n");
					msleep(5000);
					ret = bq27441_config(gauge_client);
					if ( ret < 0) {
						pr_err("[bq27441] Third config failed\n");
						return cap_last_time;
					}
				}
			}
		}
	}

	act_cap = bq27441_get_real_capacity();
	act_vol = bq27441_get_real_voltage();
	act_cur = bq27441_get_real_current();

	bq27441_get_regs(gauge_client, 0x0, data, 2);
	control = ((data[0]) | data[1]<<8);

	bq27441_get_regs(gauge_client, 0x2, data, 2);
	temp = ((data[0]) | data[1]<<8) - 2731;

	bq27441_get_regs(gauge_client, 0x06, data, 2);
	flag = ((data[0]) | data[1]<<8);

	bq27441_get_regs(gauge_client, 0x0C, data, 2);
	remain = ((data[0]) | data[1]<<8);

	bq27441_get_regs(gauge_client, 0x0E, data, 2);
	full = ((data[0]) | data[1]<<8);

	bq27441_get_regs(gauge_client, 0x1E, data, 2);
	temp_in = ((data[0]) | data[1]<<8) - 2731;

	bq27441_get_regs(gauge_client, 0x20, data, 2);
	health = ((data[0]) | data[1]<<8);

	bq27441_set_subcmd(gauge_client, CONTROL_STATUS);
	bq27441_get_regs(gauge_client, 0x0, data, 2);
	status = ((data[0]) | data[1]<<8);

	vir_cap = act_cap;

CAP_REORT:

	if (open_debug) {
		print_info(KERN_ERR"[cap]  %d\t%d\t%d\t%d\t%d\t0x%x\t%d\t%d\t%d\t0x%x\t0x%x\t%d\n",
			act_vol, act_cur, act_cap, remain, full, flag, health, temp, temp_in,status, control, vir_cap);
	}

#if STORE_BQ27441_INFO
		if (open_save) {
			memset(&bq27441_infos, 0, sizeof(bq27441_infos));
			p_info->act_vol = act_vol;
			p_info->act_cur = act_cur;
			p_info->act_cap = act_cap;
			p_info->remain = remain;
			p_info->full = full;
			p_info->flag = flag;
			p_info->health = health;
			p_info->temp = temp;
			p_info->temp_in = temp_in;
			p_info->status = status;
			p_info->control = control;
			p_info->vir_cap = vir_cap;
			bq27441_info_store_usb(p_info);
		}
#endif
#if 0
	if(vir_cap == 100){
		if(bq27441_get_Flags_FC()||(act_vol > 4250 && act_cur < 200)){
			vir_cap = 100;
			//print_info("FC Hase Set UP  return 100\n");
		}
		else{
			vir_cap = 99;
			//print_info("FC Hase Not Set UP  return 99 (vol %d | cur %d )\n",act_vol,act_cur);
		}
	}
#endif
	return vir_cap;
}

//1860302360潘
/*
	bq27441_config
	init bq27441
*/
static int bq27441_config(struct i2c_client *client)
{
	int ret, len, tmp, new_tmp, flag, index, addr,reg_value, i;
	u8 old_csum ,new_csum_first, new_csum_sec, new_csum_third, data[2];
	u16 old_capacity, old_term_vol, old_taper_rate, old_load_sel, dsg_cur, chg_cur, quit_cur, old_taper_vol, old_design_energy, old_reserve_cap, old_term_vdatle;
	u16 new_capacity, new_term_vol, new_term_cur, new_taper_rate, new_load_sel,new_qmax_cell, new_taper_vol, new_design_energy, new_reserve_cap;
	u16 new_dsg_cur, new_chg_cur, new_quit_cur, new_term_vdatle, old_qmax_cell;
	int a_x = 0;
	struct device_node *node;
	const __be32 *property;
	int new_version = DRIVER_VESION;
	node = client->dev.of_node;
	
	SetDuringConfig(1);

	ret = bq27441_set_subcmd(client, CONTROL_STATUS);
	ret |= bq27441_get_regs(client, 0x0, data, 2);
	if (ret) {
		pr_err("[bq27441] get control status register failed\n");
		goto ERROR; 
      }
	print_info("[bq27441] CONTROL_STATUS: 0x%x%x\n", data[1],data[0]);

	if (data[1] & 0x20) {
		ret   = bq27441_set_reg(client, TYPE_8Bit, 0x00, 0x00);
		ret   |= bq27441_set_reg(client, TYPE_8Bit, 0x01, 0x80);
		ret   |= bq27441_set_reg(client, TYPE_8Bit, 0x00, 0x00);
		ret   |= bq27441_set_reg(client, TYPE_8Bit, 0x01, 0x80);
		if (ret) {
			pr_err("[bq27441] set device usealed failed\n");
			goto ERROR; 
	        }
	}

	bq27441_set_subcmd(client, RESET_CMD);
	msleep(5000);

	ret = bq27441_set_subcmd(client, SET_CFGUPDATE);
	msleep(1200);
	ret |= bq27441_get_regs(client, 0x6, data, 2);

	flag = data[0] & 0x10;
	if (!flag) {
		print_info("[bq27441] CFGUPDATE mode failed! 0x%x%x,\n", ret ,data[0],data[1]);
		goto ERROR; 
	}

SET_BLOCK_FIRST:
	
#if USED_EXT_TEMP
	ret   = bq27441_set_reg(client, TYPE_8Bit, 0x61, 0x00);
	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3E, 0x40);
	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3F, 0x00);
	if (ret) {
		pr_err("[bq27441] set first 32 bytes in block 0x40 failed\n");
		goto ERROR; 
	}

	mdelay(50);

	ret = bq27441_get_regs(client, 0x60, data, 1);
	if (!ret) {
		old_csum = data[0];
		print_info("[bq27441] old csum_blk(40) = 0x%x\n", old_csum);
	} else {
		pr_err("[bq27441] get old csum_blk(40) failed\n");
		goto ERROR; 
	}

	ret = bq27441_get_regs(client, 0x40, data,2);
	print_info("[bq27441][OP_CONFIG] 0x40 is:[%x] [%x]\n",data[0],data[1]);
	ret = bq27441_get_regs(client, 0x3a, data,2);
	print_info("[bq27441][OP_CONFIG] 0x3a is:[%x] [%x]\n",data[0],data[1]);

	ret = bq27441_get_regs(client, 0x60, data, 1);
	if (!ret) {
		print_info("[bq27441] old csum_blk(40) = 0x%x\n", data[0]);
	} else {
		pr_err("[bq27441] get old csum_blk(40) failed\n");
		goto ERROR; 
	}

	ret  =  bq27441_set_reg(client, TYPE_16Bit, 0x40, 0x25f8);
	if (ret) {
		pr_err("[bq27441] set opConfig failed\n");
		goto ERROR; 
	}
	
	ret = bq27441_set_reg(client, TYPE_8Bit, 0x60, 0x6A);
	print_info("[bq27441] count new csum_blk(40) = 0x%x data[0]=%x data[1]=%x \n", new_csum_first,data[0] ,data[1] );

	ret = bq27441_get_regs(client, 0x40, data,2);
	print_info("[bq27441][OP_CONFIG] 0x40 is:[%x] [%x]\n",data[0],data[1]);
	ret = bq27441_get_regs(client, 0x3a, data,2);
	print_info("[bq27441][OP_CONFIG] 0x3a is:[%x] [%x]\n",data[0],data[1]);

#endif

	ret   = bq27441_set_reg(client, TYPE_8Bit, 0x61, 0x00);
	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3E, 0x24);
	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3F, 0x00);
	if (ret) {
		pr_err("[bq27441] set first 32 bytes in block 0x24 failed\n");
		goto ERROR; 
	}

	mdelay(50);

	ret = bq27441_get_regs(client, 0x60, data, 1);
	if (!ret) {
		old_csum = data[0];
		print_info("[bq27441] old csum_blk(36) = 0x%x\n", old_csum);
	} else {
		pr_err("[bq27441] get old csum_blk(36) failed\n");
		goto ERROR; 
	}

	if (0xD3 != old_csum) {
		ret  =  bq27441_set_reg(client, TYPE_16Bit, 0x47, 0xc8);
		if (ret) {
			pr_err("[bq27441] set DODatEOC Delta T failed\n");
			goto ERROR; 
		}
		ret = bq27441_set_reg(client, TYPE_8Bit, 0x60, 0xD3);
		mdelay(50);
	}

SET_BLOCK_SECOND:

	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x61, 0x00);
	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3E, 0x52);
	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3F, 0x00);
	if (ret) {
		pr_err("[bq27441] set first 32 bytes in block 0x52  failed\n");
		goto ERROR; 
        }

	mdelay(50);

	ret = bq27441_get_regs(client, 0x60, data, 1);
	if (!ret) {
		old_csum = data[0];
		print_info("[bq27441] old csum_blk(82) = 0x%x\n", old_csum);
        } else {
		pr_err("[bq27441] get check sum failed\n");
		goto ERROR; 
        }

	ret |= bq27441_get_regs(client, 0x40, data, 2);
	old_qmax_cell = (data[0]<<8) |data[1];
	tmp = data[0] + data[1];

	ret |= bq27441_get_regs(client, 0x45, data, 1);
	old_load_sel = data[0];
	tmp += data[0];

	ret |= bq27441_get_regs(client, 0x4A, data, 2);
	old_capacity = (data[0]<<8) |data[1];
	tmp += data[0] + data[1];
	
	#ifdef TI_CONFIG_DESIGNENERGY
		property = TI_CONFIG_DESIGNENERGY;
		new_design_energy = property;
		ret |= bq27441_get_regs(client, 0x4C, data, 2);
		old_design_energy = (data[0]<<8) |data[1];
		tmp += data[0] + data[1];
	#else
		new_design_energy = 0x00;
	#endif 
	
	ret |= bq27441_get_regs(client, 0x50, data, 2);
	old_term_vol = (data[0]<<8) |data[1];
	tmp += data[0] + data[1];

	ret |= bq27441_get_regs(client, 0x52, data, 2);
	old_term_vdatle = (data[0]<<8) |data[1];
	tmp += data[0] + data[1];

	ret |= bq27441_get_regs(client, 0x5B, data, 2);
	old_taper_rate = (data[0]<<8) |data[1];
	tmp += data[0] + data[1];

	ret |= bq27441_get_regs(client, 0x5D, data, 2);
	old_taper_vol = (data[0]<<8) |data[1];
	tmp += data[0] + data[1];

	#ifdef TI_CONFIG_RESERVECAP
		new_reserve_cap = TI_CONFIG_RESERVECAP;
		ret |= bq27441_get_regs(client, 0x43, data, 2);
		old_reserve_cap = (data[1]<<8) |data[0];
		tmp += data[0] + data[1];
	#else
		new_reserve_cap = 0x00;
	#endif
	
	tmp += old_csum;

	if (ret) {
		pr_err("[bq27441] get blk(82) old config failed\n");
		goto ERROR; 
	} else {
		print_info("[bq27441] old config: cap=%dmAh, term_vol=%dmV, taper_rate=%d, old_qmax_cell=%d, old_taper_vol=%d, \
old_design_energy=%d, old_term_vdatle=%d, old_load_sel=%d\n",
			old_capacity, old_term_vol, old_taper_rate, old_qmax_cell, old_taper_vol, old_design_energy, old_term_vdatle, old_load_sel);
	}
	
	#ifdef TI_CONFIG_CAP
		new_capacity = TI_CONFIG_CAP;
	#else
		new_capacity = 5700;
	#endif

	#ifdef TI_CONFIG_TERMVOL
		new_term_vol = TI_CONFIG_TERMVOL;
	#else
		new_term_vol = 3400;
	#endif

	#ifdef TI_CONFIG_TERMCUR
		new_term_cur = TI_CONFIG_TERMCUR;
	#else
		new_term_cur = 350;
	#endif
	
	#ifdef TI_CONFIG_QMAXCELL
		new_qmax_cell = TI_CONFIG_QMAXCELL;
	#else
		pr_err("[bq27441] get qmax_cell from dts failed\n");
		goto ERROR; 
	#endif

	#ifdef TI_CONFIG_TAPERVOL
		new_taper_vol = TI_CONFIG_TAPERVOL;
	#else
		new_taper_vol = 4140;
	#endif

	#ifdef TI_CONFIG_LOADSEL
		new_load_sel = TI_CONFIG_LOADSEL;
	#else
		new_load_sel = 0x81;
	#endif
	
	new_taper_rate = (new_capacity*10)/new_term_cur;
	new_term_vdatle = 100;

	new_tmp = ((new_capacity >> 8)&0xFF) + (new_capacity&0xFF);
	new_tmp += ((new_term_vol >> 8)&0xFF) + (new_term_vol&0xFF);
	new_tmp += ((new_taper_rate >> 8)&0xFF) + (new_taper_rate&0xFF);
	new_tmp += ((new_qmax_cell >> 8)&0xFF) + (new_qmax_cell&0xFF);
	new_tmp += ((new_taper_vol >> 8)&0xFF) + (new_taper_vol&0xFF);
	new_tmp += new_term_vdatle;
	new_tmp += new_load_sel;
	if (new_reserve_cap)
		new_tmp += ((new_reserve_cap >> 8)&0xFF) + (new_reserve_cap&0xFF);
	
	if (new_design_energy)
		new_tmp += ((new_design_energy >> 8)&0xFF) + (new_design_energy&0xFF);

	print_info("[bq27441] new config: cap=%dmAh, term_vol=%dmV, new_taper_rate=%d, qmax_cell=%d, \
new_taper_vol=%d, new_design_energy=%d, new_term_vdatle=%d\n",
		new_capacity, new_term_vol, new_taper_rate, new_qmax_cell, new_taper_vol, new_design_energy, new_term_vdatle);

	print_info("[bq27441]tmp: 0x%x, new_tmp=%x\n", tmp, new_tmp);
	new_csum_first= ~((~(tmp&0xFF) + new_tmp)&0xFF);
	print_info("[bq27441] new csum_blk(82) =0x%x !\n", new_csum_first);

	if (new_csum_first == old_csum) {
		print_info("[bq27441] FW blk(82) have already configed!\n");
	} else {
		ret  =  bq27441_set_reg(client, TYPE_16Bit, 0x40, new_qmax_cell);
		ret |= bq27441_set_reg(client, TYPE_8Bit, 0x45, new_load_sel);
		ret |= bq27441_set_reg(client, TYPE_16Bit, 0x4A, new_capacity);
		ret |= bq27441_set_reg(client, TYPE_16Bit, 0x50, new_term_vol);
		ret |= bq27441_set_reg(client, TYPE_16Bit, 0x52, new_term_vdatle);
		ret |= bq27441_set_reg(client, TYPE_16Bit, 0x5B, new_taper_rate);
		ret |= bq27441_set_reg(client, TYPE_16Bit, 0x5D, new_taper_vol);
		if (new_reserve_cap) {
			ret |= bq27441_set_reg(client, TYPE_16Bit, 0x43, new_reserve_cap);
		}

		if (new_design_energy) {
			ret |= bq27441_set_reg(client, TYPE_16Bit, 0x4C, new_design_energy);
		}

		if (ret) {
			pr_err("[bq27441] set new config failed\n");
			goto ERROR; 
		}

		ret = bq27441_set_reg(client, TYPE_8Bit, 0x60, new_csum_first);
		mdelay(50);
	}

SET_BLOCK_THIRD:
	ret   = bq27441_set_reg(client, TYPE_8Bit, 0x61, 0x00);
	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3E, 0x51);
	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3F, 0x00);
	if (ret) {
		pr_err("[bq27441] set first 32 bytes in block 0x51  failed\n");
		goto ERROR; 
        }

	mdelay(50);

	ret = bq27441_get_regs(client, 0x60, data, 1);
	if (!ret) {
		old_csum = data[0];
		print_info("[bq27441] old csum_blk(81) = 0x%x\n", old_csum);
        } else {
		pr_err("[bq27441] get csum_blk(81) failed\n");
		goto ERROR; 
        }

	ret = bq27441_get_regs(client, 0x40, data, 2);
	dsg_cur = (data[1]<<8) |data[0];
	tmp = data[0] + data[1];

	ret |= bq27441_get_regs(client, 0x42, data, 2);
	chg_cur = (data[1]<<8) |data[0];
	tmp += data[0] + data[1];

	ret |= bq27441_get_regs(client, 0x44, data, 2);
	quit_cur = (data[1]<<8) |data[0];
	tmp += data[0] + data[1];

	tmp += old_csum;

	if (ret) {
		pr_err("[bq27441] get blk(36) old config failed\n");
		goto ERROR; 
	} else {
		pr_err("[bq27441] old config: Dsg_cur=%dmA, Chg_cur=%dmA, Quit_cur=%dmA\n",
			dsg_cur, chg_cur, quit_cur);
	}

	#ifdef TI_CONFIG_DSGCUR
		new_dsg_cur = TI_CONFIG_DSGCUR;
	#else
		new_dsg_cur = 167;
	#endif

	#ifdef TI_CONFIG_CHGCUR
		new_chg_cur = TI_CONFIG_CHGCUR;
	#else
		new_chg_cur = 100;
	#endif

	#ifdef TI_CONFIG_QUITCUR
		new_quit_cur = TI_CONFIG_QUITCUR;
	#else
		new_quit_cur = 250;
	#endif
	
	print_info("[bq27441] new_dsg_cur: %d, new_chg_cur=%d, new_quit_cur=%d\n", new_dsg_cur, new_chg_cur, new_quit_cur);
	new_tmp = ((new_dsg_cur >> 8)&0xFF) + (new_dsg_cur&0xFF);
	new_tmp += ((new_chg_cur >> 8)&0xFF) + (new_chg_cur&0xFF);
	new_tmp += ((new_quit_cur >> 8)&0xFF) + (new_quit_cur&0xFF);
	
	//print_info("[bq27441]tmp: 0x%x, new_tmp=%x\n", tmp, new_tmp);
	new_csum_sec= ~((~(tmp&0xFF) + new_tmp)&0xFF);
	print_info("[bq27441] new csum_blk(81)=0x%x !(0x8D)\n", new_csum_sec);

	if (new_csum_sec == old_csum) {
		print_info("[bq27441] FW blk(81) have already configed!\n");
	} else {
		ret  =  bq27441_set_reg(client, TYPE_16Bit, 0x40, new_dsg_cur);
		ret |= bq27441_set_reg(client, TYPE_16Bit, 0x42, new_chg_cur);
		ret |= bq27441_set_reg(client, TYPE_16Bit, 0x44, new_quit_cur);

		if (ret) {
			pr_err("[bq27441] set new config failed\n");
			goto ERROR; 
		}
			
		ret = bq27441_set_reg(client, TYPE_8Bit, 0x60, new_csum_sec);
		mdelay(50);
	}

SET_BLOCK_FOUTH:
	//TI_CONFIG_RATABLE
		//of_property_read_u32_array(node, "ra_table", ra_table, 15);
		print_info("GET NEW R_TABLE:\n[bq27441] ");
		for (index = 0; index < 15; index++) {
			print_info("%d\t", TI_CONFIG_RATABLE[index]);
		}

		ret   = bq27441_set_reg(client, TYPE_8Bit, 0x61, 0x00);
		ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3E, 0x59);
		ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3F, 0x00);
		if (ret) {
			pr_err("[bq27441] set first 32 bytes in block 0x59 failed\n");
			goto ERROR; 
		}

		mdelay(50);

		ret = bq27441_get_regs(client, 0x60, data, 1);
		if (!ret) {
			old_csum = data[0];
			printk(KERN_ERR "\n[bq27441] old csum_blk(89)= 0x%x\n", old_csum);
		} else {
			pr_err("\n[bq27441] get csum_blk(89) failed\n");
			goto ERROR; 
		}

		print_info("OLD R_TABLE:\n[bq27441] ");
		for (index = 0, addr = 0x40, tmp = 0; index < 15; index++, addr+=2) {
			ret = bq27441_get_regs(client, addr, data, 2);
			if (ret) {
				pr_err("[bq27441] get old config blk(89) failed\n");
				goto ERROR; 
			}
			tmp += data[0] + data[1];
			reg_value = (data[0]<<8) + data[1];
			print_info("%d\t", reg_value);
		}

		tmp += old_csum;
		
		for (index = 0, new_tmp = 0; index < 15; index++) {
			new_tmp += ((TI_CONFIG_RATABLE[index]>> 8)&0xFF) + (TI_CONFIG_RATABLE[index]&0xFF);
		}
		
		print_info("\n[bq27441]tmp: 0x%x, new_tmp=%x\n", tmp, new_tmp);
		new_csum_third= ~((~(tmp&0xFF) + new_tmp)&0xFF);
		print_info("[bq27441] new csum_blk(89)=0x%x !\n", new_csum_third);

		if (new_csum_third == old_csum) {
			printk(KERN_ERR "[bq27441] FW blk(89) have already configed!\n");
		} else {
			printk(KERN_ERR "[bq27441] FW blk(89) not have configed!\n");
			for (index = 0, addr = 0x40; index < 15; index++, addr+=2) {
				print_info("[bq27441] set r_table index =%d\n", index);
				mdelay(100);
				ret = bq27441_set_reg(client, TYPE_16Bit, addr, TI_CONFIG_RATABLE[index]);
				if (ret) {
					pr_err("[bq27441] set old config blk(89) failed\n");
					goto ERROR; 
				}
			}

			ret = bq27441_set_reg(client, TYPE_8Bit, 0x60, new_csum_third);
			if (ret) {
				pr_err("[bq27441] set new csum_blk(89) failed\n");
				goto ERROR; 
			}
			mdelay(50);
		}
			
		ret |= bq27441_set_subcmd(client, SET_SOFT_RESET);
		if (ret) {
			pr_err("[bq27441] SET_SOFT_RESET failed\n");
			goto ERROR; 
		}

		msleep(2000);
		
		ret = bq27441_get_regs(client, 0x6, data, 2);
		flag = data[0] & 0x10;
		if (flag) {
			print_info("[bq27441] CFGUPDATE mode uclear! 0x%x%x,\n", ret ,data[0],data[1]);
			goto ERROR; 
		}

	bq27441_config_check();

	bq27441_set_subcmd(client, SEALED_CMD);
	msleep(1000);
	ERROR:
	SetDuringConfig(0);
	
	return 0;
}

/*
	check bq27441 init whether success
*/
static int bq27441_config_check(void)
{
	int ret, i, addr, tmp;
	u8 data[2];
	u16 dsg_cur, chg_cur, quit_cur;
	struct i2c_client *client = gauge_client;
	SetDuringConfig(1);
	ret = bq27441_set_subcmd(client, CONTROL_STATUS);
	ret |= bq27441_get_regs(client, 0x0, data, 2);
	if (ret) {
		pr_err("[bq27441] get control status register failed\n");
		goto ERROR; 
        }
	print_info("[bq27441] CONTROL_STATUS: 0x%x%x\n", data[1],data[0]);

	if (data[1] & 0x20) {
		ret   = bq27441_set_reg(client, TYPE_8Bit, 0x00, 0x00);
		ret   |= bq27441_set_reg(client, TYPE_8Bit, 0x01, 0x80);
		ret   |= bq27441_set_reg(client, TYPE_8Bit, 0x00, 0x00);
		ret   |= bq27441_set_reg(client, TYPE_8Bit, 0x01, 0x80);
		if (ret) {
			pr_err("[bq27441] set device usealed failed\n");
			goto ERROR; 
	        }
	}

	ret   = bq27441_set_reg(client, TYPE_8Bit, 0x00, 0x13);
	msleep(1200);

	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x61, 0x00);
	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3E, 0x24);
	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3F, 0x00);
	if (ret) {
		pr_err("[bq27441] set first 32 bytes in block 0x59 failed\n");
		goto ERROR; 
	}

	mdelay(50);

	ret = bq27441_get_regs(client, 0x60, data, 1);
	if (!ret) {
		print_info("[GETSUM bq27441] Now, csum_blk(36)=0x%x\n", data[0]);
	} else {
		pr_err("[bq27441] get csum_blk(36) failed\n");
		goto ERROR; 
	}

	ret = bq27441_get_regs(client, 0x47, data, 2);
	if (!ret) {
		print_info("[bq27441] Now, DODatEOC Delta T=0x%x\n", ((data[1]<<8) |data[0]));
	} else {
		pr_err("[bq27441] get DODatEOC Delta T failed\n");
	}

	mdelay(50);

	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x61, 0x00);
	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3E, 0x52);
	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3F, 0x00);
	if (ret) {
		pr_err("[bq27441] set first 32 bytes in block 0x59 failed\n");
		goto ERROR; 
	}

	mdelay(50);

	ret = bq27441_get_regs(client, 0x60, data, 1);
	if (!ret) {
		print_info("[GETSUM bq27441] Now, csum_blk(82) = 0x%x\n", data[0]);
	} else {
		pr_err("[bq27441] get new csum_blk(82) failed\n");
		goto ERROR; 
	}

	ret |= bq27441_get_regs(client, 0x40, data, 2);
	pr_err("qmax_cell =%d", ((data[0]<<8) |data[1]));

	ret |= bq27441_get_regs(client, 0x45, data, 1);
	pr_err("load_sel =0x%x", data[0]);

	ret |= bq27441_get_regs(client, 0x4A, data, 2);
	pr_err("capacity =%d", ((data[0]<<8) |data[1]));

	ret |= bq27441_get_regs(client, 0x4C, data, 2);
	pr_err("design_energy =%d", ((data[0]<<8) |data[1]));

	ret |= bq27441_get_regs(client, 0x50, data, 2);
	pr_err("term_vol =%d", ((data[0]<<8) |data[1]));

	ret |= bq27441_get_regs(client, 0x52, data, 2);
	pr_err("term_vdatle =%d", ((data[0]<<8) |data[1]));

	ret |= bq27441_get_regs(client, 0x5B, data, 2);
	pr_err("taper_rate =%d", ((data[0]<<8) |data[1]));

	ret |= bq27441_get_regs(client, 0x5D, data, 2);
	pr_err("taper_vol =%d\n", ((data[0]<<8) |data[1]));

	print_info("[bq27441] "); 
	for (i=0; i <32;i++) {
		bq27441_get_regs(client, (0x40+i), data, 1);
		print_info("0x%x\t",data[0]);
	}
	print_info("\n");

	mdelay(50);

	ret   = bq27441_set_reg(client, TYPE_8Bit, 0x61, 0x00);
	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3E, 0x51);
	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3F, 0x00);
	if (ret) {
		pr_err("[bq27441] set first 32 bytes in block 0x52 failed\n");
		goto ERROR; 
	}

	mdelay(50);

	ret = bq27441_get_regs(client, 0x60, data, 1);
	if (!ret) {
		print_info("[GETSUM bq27441] Now, csum_blk(81)= 0x%x\n", data[0]);
	} else {
		pr_err("[bq27441] get csum_blk(81) failed\n");
		goto ERROR; 
	}

	ret = bq27441_get_regs(client, 0x40, data, 2);
	dsg_cur = (data[1]<<8) |data[0];
	
	ret |= bq27441_get_regs(client, 0x42, data, 2);
	chg_cur = (data[1]<<8) |data[0];
	
	ret |= bq27441_get_regs(client, 0x44, data, 2);
	quit_cur = (data[1]<<8) |data[0];

	pr_err("[bq27441] check: csum_blk(81): Dsg_cur=0x%x, Chg_cur=0x%x, Quit_cur=0x%x\n",
		dsg_cur, chg_cur, quit_cur);

	mdelay(50);

	ret  = bq27441_set_reg(client, TYPE_8Bit, 0x61, 0x00);
	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3E, 0x59);
	ret |= bq27441_set_reg(client, TYPE_8Bit, 0x3F, 0x00);
	if (ret) {
		pr_err("[bq27441] set first 32 bytes in block 0x51 failed\n");
		goto ERROR; 
	}

	mdelay(50);
	
	ret = bq27441_get_regs(client, 0x60, data, 1);
	if (!ret) {
		print_info("[GETSUM bq27441] Now, csum_blk(89) = 0x%x\n", data[0]);
	} else {
		pr_err("[bq27441] get csum_third failed\n");
		goto ERROR; 
	}

	print_info("NOW R_TABLE:\n[bq27441] ");
	for (i = 0, addr = 0x40, tmp = 0; i < 15; i++, addr+=2) {
		ret = bq27441_get_regs(client, addr, data, 2);
		if (ret) {
			pr_err("[bq27441] get old config blk(89) failed\n");
			goto ERROR; 
		}
		tmp += data[0] + data[1];
		ret = (data[0]<<8) + data[1];
		print_info("%d\t", ret);
	}
	print_info("\n");
	
	ERROR:
	
	SetDuringConfig(0);

	return -EAGAIN;
}

//check bq27441 init whether success
static ssize_t bq27441_csum_check(struct device *dev, struct device_attribute *attr,
									 const char *buf, size_t count)
{
	bq27441_config_check();

	bq27441_set_subcmd(gauge_client, SEALED_CMD);

	return count;
}

static ssize_t store_debug_open(struct device *dev, struct device_attribute *attr,
                                 const char *buf, size_t count)
{
	unsigned int status=0;
	unsigned short tmp = 0;
	char *end_ptr;
	 
	status = simple_strtoul(buf, &end_ptr, 16);
	if ((buf == end_ptr) || (status > 0x1))
	{
	    print_info("\n error at %s %d", __FUNCTION__, __LINE__);
	    goto out;
	}
	
	if(status != 0)
		open_debug = 1;
	else
		open_debug = 0;

	print_info("\n %s status:0x%x, open_debug = 0x%x\n", __FUNCTION__, status, open_debug);

out:
	return count;    
}

static ssize_t store_save_open(struct device *dev, struct device_attribute *attr,
                                 const char *buf, size_t count)
{
	unsigned int status=0;
	unsigned short tmp = 0;
	char *end_ptr;

	status = simple_strtoul(buf, &end_ptr, 16);
	if ((buf == end_ptr) || (status > 0x1))
	{
	    print_info("\n error at %s %d", __FUNCTION__, __LINE__);
	    goto out;
	}
	
	if(status != 0)
		open_save = 1;
	else
		open_save = 0;

	print_info("\n %s status:0x%x, open_save = 0x%x\n", __FUNCTION__, status, open_save);

out:
	return count;    
}

/*
static struct device_attribute bq27441_attrs[] = {
	__ATTR(debug_open, 0644, NULL, store_debug_open),
	__ATTR(save_open, 0644, NULL, store_save_open),
	__ATTR(check_config, 0644, NULL, bq27441_csum_check),
};
*/ 
static int bq27441_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int flags, ret, i, device_version, updata_flag, len, act_cap;
	u8 data[2];
	struct device_node *node;

	printk("[bq27441] %s is probing .......  \n", __func__);
	
	gauge_client = client;
	node = client->dev.of_node;
	print_info("[bq27441] probe in, address=0x%0x\n",  client->addr);
	ret = i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_WRITE_BYTE_DATA  | I2C_FUNC_SMBUS_READ_BYTE);
	if ( !ret ) {
		ret = -EFAULT;
		print_info("i2c_check_functionality err\n");
		goto error;
	}
	
	ti_updataflag_config = get_first_product();
	store_first_product();
	
	/*check bq27441 device type*/
	ret = bq27441_set_subcmd(client, DEVICE_TYPE);
	
	ret |= bq27441_get_regs(client, 0x0, data, 2);
	if (ret) {
		pr_err("[bq27441] get device type register failed\n");
		goto error;
      }
		
	if ((data[0] != 0x21) || (data[1] != 0x4)) {
		pr_err("[bq27441] ID error! ID= 0x%x%x\n",data[1],data[0]);
		//goto error;
	} else {
		print_info("[bq27441] dect success!\n");
	}

#if 0
	#ifdef TI_CONFIG_UPDATAFLAG
		updata_flag = TI_CONFIG_UPDATAFLAG;
	#else
		updata_flag = 0;
	#endif
#endif

	updata_flag = ti_updataflag_config;
	
	pr_err("updata_flag = %d\n", updata_flag);


	ret = bq27441_get_regs(gauge_client, 0x06, data, 2);
	if (ret) {
		pr_err("[bq27441] probe get flags failed\n");
		goto error;
        }	
	
     //ret = bq27441_check_updata(gauge_client,0xd3,0xb9,0x5e,0x33);
	//if(ret == 1) {
		//updata_flag = 1;
	//	print_info("need to updata \n");
	//}

	flags = ((data[0]) | data[1]<<8);
	 
	if ((flags & ITPOR) || (1 == updata_flag)) {
		if(flags & ITPOR)
			printk(KERN_ERR "[bq27441][RECONFIG] TI REG ITOP NEED TO RESET \n");
		if(updata_flag == 1 )
			printk(KERN_ERR "[bq27441][RECONFIG] SYSTEM SET TO RESET \n");
		
		printk(KERN_ERR "[bq27441] first config!\n");
		ret = bq27441_config(client);
		if ( ret < 0) {
			printk("[bq27441] first config failed\n");
			ret = bq27441_config(client);
			if ( ret < 0) {
				printk("[bq27441] second config failed\n");
				ret = bq27441_config(client);
				if ( ret < 0) {
					printk("[bq27441] third config failed\n");
					goto error;
				}
			}
		}

	} else {
		printk("[bq27441] ITPOR = 0\n");
	}
	
	PASS:

	
#ifdef DEPEND
	/*注册电源模块接口*/
	print_info("[bq27441] call back function register. \n");
	act260x_set_get_hw_cap_point((void *)bq27441_get_capacity);
	act260x_set_get_hw_volt_point((void *)bq27441_get_real_voltage);
	act260x_set_get_hw_cur_point((void *)bq27441_get_real_current);
	act260x_set_get_hw_temp_point((void *)bq27441_get_real_temperature);
	//act260x_set_get_detect_for_hd_point((void *)bq27441_check_error);
#endif
/*
	for (i = 0; i < ARRAY_SIZE(bq27441_attrs); i++) {
		ret = device_create_file(&(client->dev), &bq27441_attrs[i]);
		if (ret)
			return ret;
	}*/

	do {
		act_cap = bq27441_get_real_capacity();
		if (act_cap < 0 || act_cap >100) {
			pr_err("[bq27441] get init cap failed!\n");
			continue;
		} else
			break;
	} while(1);
#if 0
	if (act_cap > 85) {
                cap_last_time = 90 + (act_cap - 85)*2/3;
        } else if (act_cap > 60) {
                cap_last_time = 60 + (act_cap - 60)*6/5;
        } /*else if (act_cur < 0 && bat_vol <= 3650 && bat_low_cap) {
			vir_cap = (bat_vol -3500) * bat_low_cap/(bat_low_vol - 3500);
	} */ else {
		cap_last_time = act_cap;
	}
#endif
	//pr_err("[bq27441] cap_last_time = %d\n", cap_last_time);

	return 0;

error:
	return ret;
}

static int bq27441_suspend(struct i2c_client *client, pm_message_t mesg)
{
	return 0;
}

static int bq27441_resume(struct i2c_client *client)
{
	int act_cap;

	do {
		act_cap = bq27441_get_real_capacity();
		if (act_cap < 0 || act_cap >100) {
			pr_err("[bq27441] get init cap failed!\n");
			continue;
		} else
			break;
	} while(1);
#if 0
	if (act_cap > 85) {
                cap_last_time = 90 + (act_cap - 85)*2/3;
        } else if (act_cap > 60) {
                cap_last_time = 60 + (act_cap - 60)*6/5;
        } /*else if (act_cur < 0 && bat_vol <= 3650 && bat_low_cap) {
			vir_cap = (bat_vol -3500) * bat_low_cap/(bat_low_vol - 3500);
	} */ else {
		cap_last_time = act_cap;
	}
#endif
	//pr_err("[bq27441] return from str, cap_last_time = %d,  act_cap =%d\n", cap_last_time, act_cap);
	return 0;
}

static struct i2c_device_id bq27441_id[] = {
    {"bq27441", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, bq27441_id);

static const struct of_device_id atc260x_bq27441_match[] = {
	{ .compatible = "actions,bq27441", },
	{},
};
MODULE_DEVICE_TABLE(of, atc260x_bq27441_match);

static struct i2c_client * pClient = NULL;

static struct i2c_board_info bq27441_device = {
    .type	= "bq27441",
	.addr   = 0x55,
};

//show volt from ba27441 @/sys/HW_GAUGE/
static ssize_t show_VoltageHW(struct device *dev, struct device_attribute *attr, char *buf, size_t count) 
{
	int ret=0;
	ret=bq27441_get_real_voltage();
	return sprintf(buf, "%d\n", ret); 
}

static ssize_t store_VoltageHW(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
    return count;
}

//show volt from atc260x @/sys/HW_GAUGE/
static ssize_t show_VoltagePMU(struct device *dev, struct device_attribute *attr, char *buf, size_t count) 
{
	int ret=0;
	ret=get_pmu_battery_volatge();
	return sprintf(buf, "%d\n", ret); 
}

static ssize_t store_VoltagePMU(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
    return count;
}

//show cap from bq27441 @/sys/HW_GAUGE/
static ssize_t show_Capacity(struct device *dev, struct device_attribute *attr, char *buf, size_t count) 
{
	int ret=0;
	ret=bq27441_get_real_capacity();
	return sprintf(buf, "%d\n", ret); 
}

static ssize_t store_Capacity(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
    return count;
}

//show current temp  @/sys/HW_GAUGE/
static ssize_t show_Temperature(struct device *dev, struct device_attribute *attr, char *buf, size_t count) 
{
	int ret=0;
	ret=bq27441_get_real_temperature();
	return sprintf(buf, "%d\n", ret); 
}

static ssize_t store_Temperature(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
    return count;
}

//show i2c transfer error @/sys/HW_GAUGE/
static ssize_t show_ErrorCheck(struct device *dev, struct device_attribute *attr, char *buf, size_t count) 
{
	int ret=0;
	ret=bq27441_check_error();
	return sprintf(buf, "%d\n", ret); 
}

static ssize_t store_ErrorCheck(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
    return count;
}

//dump all regiest for bq27441 @/sys/HW_GAUGE/
static ssize_t show_DumpReg(struct device *dev, struct device_attribute *attr, char *buf, size_t count) 
{
	int ret=0;
	int data[30];
	memset(data,0,30*4);
	bq27441_get_regs(gauge_client, 0x00, data, 30);

	return sprintf(buf, "Ctl:%x Tem:%x Tem:%x Vol:%x Flg:%x NACap:%x FACap:%x RCap:%x FCap:%x ACur:%x SCur:%x MLCur:%x APow:%x SOChg:%x ITem:%x SOH:%x \n"
										, (data[0]|data[1]<<8),(data[2]|data[3]<<8),(data[4]|data[5]<<8)
										,(data[6]|data[7]<<8),(data[8]|data[9]<<8),(data[10]|data[11]<<8)
										,(data[12]|data[13]<<8),(data[14]|data[15]<<8),(data[16]|data[17]<<8)
										,(data[18]|data[19]<<8),(data[20]|data[21]<<8),(data[22]|data[23]<<8)
										,(data[24]|data[25]<<8),(data[26]|data[27]<<8),(data[28]|data[29]<<8)
										); 
}

static ssize_t store_DumpReg(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
    return count;
}

//reinit bq27441 @/sys/HW_GAUGE/
static ssize_t show_ReConfig(struct device *dev, struct device_attribute *attr, char *buf, size_t count) 
{
	int ret=0;
	return sprintf(buf, "%d\n", ret); 
}

static ssize_t store_ReConfig(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
	unsigned int status=0;
	unsigned short tmp = 0;
	char *end_ptr;

	status = simple_strtoul(buf, &end_ptr, 16);
	if ((buf == end_ptr) || (status > 0x1))
	{
	    print_info("\n error at %s %d", __FUNCTION__, __LINE__);
	    goto out;
	}
	
	if(status != 0)
		bq27441_config(gauge_client);
		
	out:

    	return count;
	
}

//show current @/sys/HW_GAUGE/
static ssize_t show_Current(struct device *dev, struct device_attribute *attr, char *buf, size_t count) 
{
	int ret=0;
	ret=bq27441_get_real_current();
	return sprintf(buf, "%d\n", ret); 
}

static ssize_t store_Current(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
    return count;
}

//enable save log to /data/bq27441_info.log @/sys/HW_GAUGE/
static ssize_t show_SetDebugLog(struct device *dev, struct device_attribute *attr, char *buf, size_t count) 
{
	int ret=open_save;
	
	return sprintf(buf, "%d\n", ret); 
}

static ssize_t store_SetDebugLog(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{

	unsigned int status=0;
	unsigned short tmp = 0;
	char *end_ptr;

	status = simple_strtoul(buf, &end_ptr, 16);
	if ((buf == end_ptr) || (status > 0x1))
	{
	    print_info("\n error at %s %d", __FUNCTION__, __LINE__);
	    goto out;
	}
	
	if(status != 0)
		open_save = 1;
	else
		open_save = 0;
	
	out:
    	return count;
}

//for test @/sys/HW_GAUGE/
static ssize_t show_SetError(struct device *dev, struct device_attribute *attr, char *buf, size_t count) 
{
	int ret=setError;
	
	return sprintf(buf, "%d\n", ret); 
}

static ssize_t store_SetError(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
	unsigned int status=0;
	unsigned short tmp = 0;
	char *end_ptr;

	status = simple_strtoul(buf, &end_ptr, 16);
	if ((buf == end_ptr) || (status > 0x1))
	{
	    print_info("\n error at %s %d", __FUNCTION__, __LINE__);
	    goto out;
	}
	
	if(status != 0)
		setError = 1;
	else
		setError = 0;
	
	out:
    	return count;
}

//for test @/sys/HW_GAUGE/
static ssize_t show_SetTemp(struct device *dev, struct device_attribute *attr, char *buf, size_t count) 
{
	int ret=testTemp;
	
	return sprintf(buf, "%d\n", ret); 
}

static ssize_t store_SetTemp(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{

	unsigned int status=0;
	unsigned short tmp = 0;
	char *end_ptr;

	status = simple_strtoul(buf, end_ptr, 10);
	if ((buf == end_ptr)/* || (status > 0x1)*/)
	{
	    print_info("\n error at %s %d", __FUNCTION__, __LINE__);
	    goto out;
	}
	
	testTemp = status;

	out:
    	return count;
}



static DEVICE_ATTR(ErrorCheck, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
       show_ErrorCheck, store_ErrorCheck);
static DEVICE_ATTR(Capacity, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        show_Capacity,store_Capacity);
static DEVICE_ATTR(VoltageHW, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        show_VoltageHW, store_VoltageHW);
static DEVICE_ATTR(VoltagePMU, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        show_VoltagePMU, store_VoltagePMU);
static DEVICE_ATTR(DumpReg, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        show_DumpReg, store_DumpReg);
static DEVICE_ATTR(ReConfig, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        show_ReConfig, store_ReConfig);
static DEVICE_ATTR(SetDebugLog, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        show_SetDebugLog, store_SetDebugLog);
static DEVICE_ATTR(Temperature, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        show_Temperature, store_Temperature);
static DEVICE_ATTR(SetError, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        show_SetError, store_SetError);
static DEVICE_ATTR(SetTemp, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        show_SetTemp, store_SetTemp);
static DEVICE_ATTR(Current, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        show_Current, store_Current);

static struct attribute *hd_gauge_attributes[] = {
    &dev_attr_ErrorCheck.attr,
    &dev_attr_VoltageHW.attr,
    &dev_attr_Capacity.attr,
    &dev_attr_Temperature.attr,
    &dev_attr_VoltagePMU.attr,
    &dev_attr_DumpReg.attr,
    &dev_attr_ReConfig.attr,
    &dev_attr_SetDebugLog.attr,
    &dev_attr_SetError.attr,
    &dev_attr_SetTemp.attr,
    &dev_attr_Current.attr,
    NULL
};

static struct attribute_group hd_gauge_attribute_group = {
    .attrs = hd_gauge_attributes
};

static int bq27441_remove(struct i2c_client *client)
{
	int i;
/*
	for (i = 0; i < ARRAY_SIZE(bq27441_attrs); i++) {
		device_remove_file(&(client->dev), &bq27441_attrs[i]);
	}
*/
#ifdef DEPEND
	act260x_set_get_hw_cap_point((void *)NULL);
	act260x_set_get_hw_volt_point((void *)NULL);
	act260x_set_get_hw_cur_point((void *)NULL); 
	act260x_set_get_hw_temp_point((void *)NULL);
#endif

	sysfs_remove_group(hw_gauge_kobj,&hd_gauge_attribute_group);
	return 0;
}

static struct i2c_driver bq27441_driver = {
	.driver = {
	    .owner = THIS_MODULE,
	    .name = "bq27441",
	},

	.class = I2C_CLASS_HWMON,
	.probe = bq27441_probe,
	.remove = bq27441_remove,
	.suspend = bq27441_suspend,
	.resume = bq27441_resume,
	.id_table = bq27441_id,
};

static int bq27441_driver_init(void)
{
	int ret;
	struct i2c_client *client = NULL;
    struct i2c_adapter *adap;
	printk("[bq27441] %s is start .......  \n", __func__);
	adap = i2c_get_adapter(2);
	hw_gauge_kobj = kobject_create_and_add("HW_GAUGE", NULL);  
	if(hw_gauge_kobj == NULL){  
		ret = -ENOMEM;  
		return ret;
	}  

    	ret = sysfs_create_group(hw_gauge_kobj,&hd_gauge_attribute_group);
	
	print_info("[bq27441] %s version: %s, 2013-8-17\n", THIS_MODULE->name, THIS_MODULE->version);	
   	pClient = i2c_new_device(adap, &bq27441_device); 
	ret = i2c_add_driver(&bq27441_driver);
	if (ret) {
		print_info("i2c_add_driver err %d \n",ret);
		return ret;
	}
	return 0;
}

static void  bq27441_driver_exit(void)
{
	struct i2c_client *client = NULL;
	printk("[bq27441] %s is exit .......  \n", __func__);
	i2c_del_driver(&bq27441_driver);
      i2c_unregister_device(pClient);
}

module_param(gauge, int, S_IRUGO);
module_init(bq27441_driver_init);
module_exit(bq27441_driver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("semi-actions co.");
MODULE_VERSION("1.0.0");
#endif
