#include <asm/io.h>
#include <common.h>
#include <asm/arch/pmu.h>
#include <key_scan.h>
#include <asm/arch/owl_afi.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/actions_reg_owl.h>


#define GPIO_REG_BASE               (GPIO_MFP_PWM_BASE)

#define GPIO_BANK(gpio)             ((gpio) / 32)
#define GPIO_IN_BANK(gpio)          ((gpio) % 32)
#define GPIO_BIT(gpio)              (1 << GPIO_IN_BANK(gpio))

#define GPIO_REG_OUTEN(gpio)    (GPIO_REG_BASE + GPIO_BANK(gpio) * 0xc + 0x0)
#define GPIO_REG_INEN(gpio)     (GPIO_REG_BASE + GPIO_BANK(gpio) * 0xc + 0x4)
#define GPIO_REG_DAT(gpio)      (GPIO_REG_BASE + GPIO_BANK(gpio) * 0xc + 0x8)

static int adc_channel=0;  //  0= AUX0, 1= AUX1, 2= AUX2 ,3 =REMCON;

static void pmu_adc_init(void)
{
	static const u16 pmu_adc_ctl[OWL_PMU_ID_CNT] = {
		[OWL_PMU_ID_ATC2603A] = ATC2603A_PMU_AUXADC_CTL0,
		[OWL_PMU_ID_ATC2603B] = ATC2609A_PMU_AUXADC_CTL0,
		[OWL_PMU_ID_ATC2603C] = ATC2603C_PMU_AUXADC_CTL0,
	};
	int adc_en_bit = 5;
	if ( adc_channel < 3 ) {
		if ( OWL_PMU_ID	 == OWL_PMU_ID_ATC2603C )
			adc_en_bit = 14 - adc_channel; // bit14=aux0, bit13=aux1, bit12= aux2
		else
			adc_en_bit = 15 - adc_channel;// bit15=aux0, bit14=aux1, bit13= aux2
	} else {
		adc_en_bit = 5; // bit5 = REMCON 
	}
	
	if ( OWL_PMU_ID < OWL_PMU_ID_CNT )
		atc260x_set_bits(pmu_adc_ctl[OWL_PMU_ID], (1 << adc_en_bit), (1 << adc_en_bit));

}

static int pmu_adc_read(void)
{
	static const u16 pmu_adc_reg[OWL_PMU_ID_CNT][4] = {
		{ATC2603A_PMU_AUXADC0, ATC2603A_PMU_AUXADC1, ATC2603A_PMU_AUXADC2, ATC2603A_PMU_REMCONADC},
		{ATC2609A_PMU_AUXADC0, ATC2609A_PMU_AUXADC1, ATC2609A_PMU_AUXADC2, ATC2609A_PMU_REMCONADC},
		{ATC2603C_PMU_AUXADC0, ATC2603C_PMU_AUXADC1, ATC2603C_PMU_AUXADC2, ATC2603C_PMU_REMCONADC},
	};
	
	if ( OWL_PMU_ID < OWL_PMU_ID_CNT )
		return atc260x_reg_read(pmu_adc_reg[OWL_PMU_ID][adc_channel]);

    return -1;
}

static int pmu_adckey_read(void)
{
    int adc_val;    
    pmu_adc_init();
    adc_val = pmu_adc_read();
    udelay(1);
    adc_val = pmu_adc_read();
    return adc_val;
}

struct adckey {
    unsigned short adc_min;
    unsigned short adc_max;
    unsigned int key_val;
};

static struct adckey adckey[] = {
    {.adc_min = 0,      .adc_max = 31,      .key_val = KEY_HOME}, 
    {.adc_min = 32,     .adc_max = 143,     .key_val = KEY_MENU}, 
    {.adc_min = 144,    .adc_max = 239,     .key_val = KEY_VOLUMEUP}, 
    {.adc_min = 240,    .adc_max = 335,     .key_val = KEY_VOLUMEDOWN}, 
    {.adc_min = 336,    .adc_max = 447,     .key_val = KEY_RESERVED}, 
    {.adc_min = 448,    .adc_max = 559,     .key_val = KEY_6}, //board option 5
    {.adc_min = 560,    .adc_max = 687,     .key_val = KEY_5}, //board option 4
    {.adc_min = 688,    .adc_max = 799,     .key_val = KEY_4}, //board option 3
    {.adc_min = 800,    .adc_max = 895,     .key_val = KEY_3}, //board option 2
    {.adc_min = 896,    .adc_max = 966,     .key_val = KEY_2}, //board option 1
    {.adc_min = 967,    .adc_max = 1023,    .key_val = KEY_1}, //board option 0
};

static int adckey_index(void)
{
    int i;
    unsigned short adc_val;
    
    adc_val = pmu_adckey_read();
    printf("adc_val=%d\n", adc_val);
    for(i = 0; i < sizeof(adckey)/sizeof(struct adckey); i++) {
        if (adc_val >= adckey[i].adc_min && adc_val <= adckey[i].adc_max) {
            return i;
        }
    }
    
    return 0;
}

DECLARE_GLOBAL_DATA_PTR;

#include <libfdt.h>
#include <fdtdec.h>
static int adckey_init(void)
{
	int	node, ret;
	int keymapsize, i;
	u32 keyval[10], left_adc_val[10], right_adc_val[10];
	const char *str_channel;
	const char *name_channel[4]= {"AUX0", "AUX1", "AUX2", "REMCON"};
	const char *adc_compat [3] = {"actions,atc2603c-adckeypad", "actions,atc2603a-adckeypad", "actions,atc2609a-adckeypad"};

	for ( i = 0 ; i < 3; i++ ) {
		node = fdt_node_offset_by_compatible(gd->fdt_blob, 0, adc_compat[i]);
		if ( node >= 0 )
			break;
	}
	if (i == 3) {
		printf("cannot locate keyboard node\n");
		return node;
	}

	keymapsize = fdtdec_get_int(gd->fdt_blob,
		       node, "keymapsize", 0);
	if (keymapsize <= 0 ) {
		printf("adckey: keymapsize err\n");
		return keymapsize;

	}

	str_channel = fdt_getprop(gd->fdt_blob, node, "adc_channel_name", NULL);
	if ( str_channel != NULL ) {
		for ( i = 0; i < 4; i++ ) {
			if ( 0 == strcmp(name_channel[i], str_channel) ) {
				adc_channel = i;
				break;
			}
		}		
		printf("adckey: %s, channel=%d\n", str_channel, adc_channel);
	}
		
	keymapsize = (keymapsize < 10)?keymapsize:10 ;

	ret = fdtdec_get_int_array(gd->fdt_blob,
			node, "key_val", keyval, keymapsize);
	ret += fdtdec_get_int_array(gd->fdt_blob,
			node, "left_adc_val", left_adc_val,	keymapsize);
	ret += fdtdec_get_int_array(gd->fdt_blob,
			node, "right_adc_val",right_adc_val,keymapsize);
	if ( ret != 0 ) {
		printf("adckey: key or adc val err\n\n");
		return -1;
	}
	
	printf("adckey: keynum=%d\n", keymapsize);
    for(i = 0; i < keymapsize; i++) {
		adckey[i].adc_min =  left_adc_val[i];
		adckey[i].adc_max =  right_adc_val[i];
		adckey[i].key_val =  keyval[i];
    }
	return 0;

}


static int adckey_scan(void)
{
	adckey_init();
    return adckey[adckey_index()].key_val;
}

#define ONOFF_LONG_PRESS		(1 << 13)
#define ONOFF_SHORT_PRESS		(1 << 14)
#define ONOFF_PRESS              (1 << 15)
int count_onoff_short_press(void)
{
    int i, poll_times, on_off_val;

    on_off_val = atc260x_reg_read(ATC2603C_PMU_SYS_CTL2);
    if((on_off_val & ONOFF_PRESS) == 0)
        return 0; 
	
	while ( 1 ) { /*wait key move*/
	    on_off_val = atc260x_reg_read(ATC2603C_PMU_SYS_CTL2);
	    if((on_off_val & ONOFF_PRESS) == 0)
	        break; 
		mdelay(1);
	}
        
    printf("start count onoff times\n");
        
    /* clear On/Off press pending */
    atc260x_set_bits(ATC2603C_PMU_SYS_CTL2, 
        ONOFF_SHORT_PRESS | ONOFF_LONG_PRESS, 
        ONOFF_SHORT_PRESS | ONOFF_LONG_PRESS);
	
    
    for(poll_times = 0; poll_times < 8; poll_times++)
    {
        for(i = 0; i < 2000; i++)
        {
            on_off_val = atc260x_reg_read(ATC2603C_PMU_SYS_CTL2);
            if ((on_off_val & ONOFF_SHORT_PRESS) != 0)
                break;
            mdelay(1);
        }
        
        if(i == 2000)
            break;
            
        atc260x_set_bits(ATC2603C_PMU_SYS_CTL2, 
            ONOFF_SHORT_PRESS | ONOFF_LONG_PRESS, 
            ONOFF_SHORT_PRESS | ONOFF_LONG_PRESS);
    }
    
    printf("Onoff press %d times\n", poll_times);
    return poll_times;
}

void reset_to_adfu(void)
{
	atc260x_pstore_set(ATC260X_PSTORE_TAG_REBOOT_ADFU, 1);
	printf("reset to adfu\n");
	reset_cpu(0);
}

int check_key(void)
{
	int key,value;
	int onoff_shortpress_times;

	key = adckey_scan(); 
	if ( key != KEY_VOLUMEUP && key != KEY_VOLUMEDOWN ) {
		if(owl_get_boot_mode() != (int)BOOT_MODE_PRODUCE){
			onoff_shortpress_times = count_onoff_short_press();
			
			if((onoff_shortpress_times >= 2) && (onoff_shortpress_times <= 3) )
				key = KEY_VOLUMEDOWN ;
			else if(onoff_shortpress_times >= 7)
				key = KEY_VOLUMEUP ;
			else if ((onoff_shortpress_times >= 4) && (onoff_shortpress_times <= 6))
				 gd->flags |= GD_FLG_CMDLINE ;
		}
	}

	clrbits_le32(GPIO_REG_OUTEN(41), GPIO_BIT(41));
        setbits_le32(GPIO_REG_INEN(41), GPIO_BIT(41));

        value = (readl(GPIO_REG_DAT(41)) & GPIO_BIT(41));
        if(!value) {
                printf("touch gpio key, enter adfu\n");
                reset_to_adfu();
        }
	
/*	switch(key) {
	case KEY_VOLUMEUP:
		printf("touch v+ key, enter adfu\n");
		reset_to_adfu();
		break;
	case KEY_VOLUMEDOWN:
		printf("touch v- key, enter recovery\n");
		setup_recovery_env();
		break;
	}
*/
	return 0;
}

