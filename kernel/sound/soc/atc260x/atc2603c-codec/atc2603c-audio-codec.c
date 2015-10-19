#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/clk.h>			/* clk_enable */
#include "../sndrv-owl.h"
#include "atc2603c-audio-regs.h"
#include "../common-regs-owl.h"
#include <mach/clkname.h>
#include <mach/module-owl.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

#include <linux/io.h>
#include <linux/ioport.h>

#include <linux/suspend.h>
#include <linux/earlysuspend.h>

#include <linux/mfd/atc260x/atc260x.h>
#include <linux/interrupt.h>
#include <linux/switch.h>


static int direct_drive_disable;//0:直驱， 1:非直驱

static int adc_detect_mode; //0:earphone irq or gpio detect, 1: earphone adc detect

static const char *audio_device_node = "actions,atc2603c-audio";
static const char *snd_earphone_output_mode = "earphone_output_mode";
static const char *snd_mic_num = "mic_num";
static const char *snd_mic0_gain = "mic0_gain";
static const char *snd_speaker_gain = "speaker_gain";
static const char *snd_earphone_gain = "earphone_gain";
/*
static const char *snd_speaker_volume = "speaker_volume";
static const char *snd_earphone_volume = "earphone_volume";
static const char *snd_earphone_detect_mode = "earphone_detect_mode";
*/
static const char *speaker_ctrl_name = "speaker_en_gpios";
static const char *earphone_detect_gpio = "earphone_detect_gpios";
//20141202 new_code by yuchen: add new item in dts file to config mic mode differential or single end
static const char *snd_mic_mode = "mic_mode";
static const char *snd_earphone_detect_method = "earphone_detect_method";
static const char *snd_adc_plugin_threshold = "adc_plugin_threshold";
static const char *snd_adc_level = "adc_level";


static int speaker_gpio_num;
static int earphone_gpio_num;

static enum of_gpio_flags speaker_gpio_level;
static int speaker_gpio_active;

static audio_hw_cfg_t audio_hw_cfg;

static int atc2603c_open_count;
static unsigned int user_lock = 1;
static volatile unsigned int hw_init_flag = false;
static DEFINE_MUTEX(atc2603c_pa_down_lock);

//20141013 yuchen: to check pmu ic type
static int atc2603c_ictype = PMU_NOT_USED;
static int earphone_irq = -1;

static int earphone_poll_ms = 50;


struct reg_val {
	int reg;
	unsigned short val;
	short delay; /* ms */
};

/*
typedef struct ear_detect_dev {
	struct cdev chrdev;
	int dev_idx;
	int irq;
} ear_detect_dev_t;
*/

struct atc2603c_priv_data {
	int mode;
};
static struct atc260x_dev *atc260x;
struct snd_soc_codec *atc2603c_codec;

static struct switch_dev headphone_sdev;

#define   ATC2603C_AIF			0
#define   REG_BASE				ATC2603C_AUDIO_OUT_BASE

#define     AUDIOINOUT_CTL                                                    (0x0)
#define     AUDIO_DEBUGOUTCTL                                                 (0x1)
#define     DAC_DIGITALCTL                                                    (0x2)
#define     DAC_VOLUMECTL0                                                    (0x3)
#define     DAC_ANALOG0                                                       (0x4)
#define     DAC_ANALOG1                                                       (0x5)
#define     DAC_ANALOG2                                                       (0x6)
#define     DAC_ANALOG3                                                       (0x7)

//--------------Bits Location------------------------------------------//
//--------------AUDIO_IN-------------------------------------------//


//--------------Register Address---------------------------------------//

#define     ADC_DIGITALCTL                                                    (0x8)
#define     ADC_HPFCTL                                                        (0x9)
#define     ADC_CTL                                                           (0xa)
#define     AGC_CTL0                                                          (0xb)
#define     AGC_CTL1                                                          (0xc)
#define     AGC_CTL2                                                          (0xd)
#define     ADC_ANALOG0                                                       (0xe)
#define     ADC_ANALOG1                                                       (0xf)

/*
#define	ATC2603C_CMU_DEVRST		(ATC2603C_CMU_CONTROL_BASE+0x01) //0xc1
#define	ATC2603C_PAD_EN			(ATC2603C_MFP_BASE+0x6)		//0xd6
#define	ATC2603C_MFP_CTL		(ATC2603C_MFP_BASE+0x00)		//0xd0
#define	ATC2603C_PMU_BDG_CTL		(ATC2603C_PMU_BASE+0x51)	//0x51
*/

static const u16 atc2603c_reg[ADC_ANALOG1+1] = {
	[AUDIOINOUT_CTL] = 0x00,
	[AUDIO_DEBUGOUTCTL] = 0x00,
	[DAC_DIGITALCTL] = 0x03,
	[DAC_VOLUMECTL0] = 0x00,
	[DAC_ANALOG0] = 0x00,
	[DAC_ANALOG1] = 0x00,
	[DAC_ANALOG2] = 0x00,
	[DAC_ANALOG3] = 0x00,
	[ADC_DIGITALCTL] = 0x00,
	[ADC_HPFCTL] = 0x00,
	[ADC_CTL] = 0x00,
	[AGC_CTL0] = 0x00,
	[AGC_CTL1] = 0x00,
	[AGC_CTL2] = 0x00,
	[ADC_ANALOG0] = 0x00,
	[ADC_ANALOG1] = 0x00,
};

/*
struct reg_val atc2603c_pa_up_list[] = {
	{DAC_VOLUMECTL0, 0xbebe, 0},
	{DAC_ANALOG0, 0x00, 0},
	{DAC_ANALOG1, 0x00, 0},
	{DAC_ANALOG2, 0x00, 0},
	{DAC_ANALOG3, 0x00, 0},
	{DAC_ANALOG4, 0x00, 0},
	{AUDIOINOUT_CTL, 0x02, 0},
	{DAC_DIGITALCTL, 0x03, 0},
	{DAC_ANALOG4, 0x08c0, 0},
	{DAC_ANALOG0, 0x26b3, 0},
	{DAC_ANALOG3, 0x8b0b, 0},
	{DAC_ANALOG2, 0x07, 0},
	{DAC_ANALOG3, 0x8b0f, 100},
	{DAC_ANALOG2, 0x0f, 0},
	{DAC_ANALOG4, 0x88c0, 0},
	{DAC_ANALOG2, 0x1f, 600},
	{DAC_ANALOG2, 0x17, 0},
};

struct reg_val atc2603c_pa_down_list[] = {
	{DAC_VOLUMECTL0, 0xbebe, 0},
	{DAC_ANALOG4, 0x08c0, 0},
	{DAC_ANALOG2, 0x1f, 0},
	{DAC_ANALOG2, 0x0f, 0},
};
*/

struct asoc_codec_resource {
    void __iomem    *base[MAX_RES_NUM];/*virtual base for every resource*/
    void __iomem    *baseptr; /*pointer to every virtual base*/
    struct clk      *clk;
    int             irq;
    unsigned int    setting;
};

//codec resources
//static struct asoc_codec_resource codec_res;

static int earphone_is_in_for_irq(void);
static irqreturn_t earphone_detect_irq_handler(int irq, void *data);

static struct delayed_work dwork_adc_detect;


/* 
static void set_dai_reg_base(int num)
{
	codec_res.baseptr = codec_res.base[num];
}

static u32 snd_dai_readl(u32 reg)
{
	return readl(codec_res.baseptr + reg);
}
	 
static void snd_dai_writel(u32 val, u32 reg)
{
	writel(val, codec_res.baseptr + reg);
}
*/

/*
static void snd_codec_writel_debug(unsigned int  val, unsigned int  reg)
{
	snd_dai_writel(val, reg);
	snd_dbg("%s: reg[0x%x]=[0x%x][0x%x]\n"
		, __func__, reg, val, snd_dai_readl(reg));
}
*/

static int atc2603c_write_pmu(struct snd_soc_codec *codec, unsigned int reg,
		unsigned int value)
{
	struct atc260x_dev *atc260x = snd_soc_codec_get_drvdata(codec);
	int ret;
	
	ret = atc260x_reg_write(atc260x, reg, value);
	if(ret < 0)
	{
		snd_err("atc2603c_write: reg = %#X, ret = %d \n", reg, ret);
	}

//	snd_dbg("%s: reg[0x%x]=[0x%x][0x%x]\n"
//		, __func__, reg, value, atc260x_reg_read(atc260x, reg));

	return ret;
}
static int atc2603c_read_pmu(struct snd_soc_codec *codec, unsigned int reg)
{
	struct atc260x_dev *atc260x = snd_soc_codec_get_drvdata(codec);
	int ret;
	
	ret = atc260x_reg_read(atc260x, reg);
	if(ret < 0)
	{
		snd_err("atc2603c_read: reg = %#X, ret = %d \n", reg, ret);
	}

	return ret;	
}


static int snd_soc_update_bits_pmu(struct snd_soc_codec *codec, unsigned short reg,
				unsigned int mask, unsigned int value)
{
	bool change;
	unsigned int old, new;
	int ret;

	{
		ret = atc2603c_read_pmu(codec, reg);
		if (ret < 0)
			return ret;

		old = ret;
		new = (old & ~mask) | (value & mask);
		change = old != new;
		if (change)
			ret = atc2603c_write_pmu(codec, reg, new);
	}

	if (ret < 0)
		return ret;

	return change;

}


static void ramp_undirect(unsigned int begv, unsigned int endv) {
	unsigned int val = 0;
	int count = 0;
	
	set_dai_reg_base(I2S_SPDIF_NUM);
	while (endv < begv) {
		count++;
		if ((count & 0x7F) == 0) {
			mdelay(1);
		}
		val = snd_dai_readl(I2S_FIFOCTL);
		while ((val & (0x1 << 8)) != 0) {
			val = snd_dai_readl(I2S_FIFOCTL);
		};
		snd_dai_writel(endv, I2STX_DAT);
		endv -= 0x36000;
	}
	while (begv <= endv) {
		count++;
		if ((count & 0x7F) == 0) {
			mdelay(1);
		}
		val = snd_dai_readl(I2S_FIFOCTL);
		while ((val & (0x1 << 8)) != 0) {
			val = snd_dai_readl(I2S_FIFOCTL);
		};
		snd_dai_writel(endv, I2STX_DAT);
		endv -= 0x36000;
	}
}

static int atc2603c_write(struct snd_soc_codec *codec, unsigned int reg,
		unsigned int value)
{
	int ret = 0;

	struct atc260x_dev *atc260x = snd_soc_codec_get_drvdata(codec);
/*
	if (!snd_soc_codec_volatile_register(codec, reg) &&
			(reg) < codec->driver->reg_cache_size &&
			!codec->cache_bypass) {
		ret = snd_soc_cache_write(codec, reg, value);
		snd_dbg("%s: reg[0x%x]=[0x%x]\r\n", __func__, reg, value);
		if (ret < 0)
			return -1;
	}

	if (codec->cache_only) {
		codec->cache_sync = 1;
		return 0;
	}
*/

	ret = atc260x_reg_write(atc260x, reg + REG_BASE, value);
	//snd_err("%s: reg[0x%x]=[0x%x]\r\n", __FUNCTION__, reg + REG_BASE, value);
	if (ret < 0)
		snd_err("atc2603c_write: reg = %#X, ret = %d failed\n",
		reg, ret);

//	snd_dbg("%s: reg[0x%x]=[0x%x][0x%x]\n"
//		, __func__, reg + REG_BASE, value, atc260x_reg_read(atc260x, reg + REG_BASE));

	return ret;
}

static unsigned int atc2603c_read(struct snd_soc_codec *codec, unsigned int reg)
{
	int ret = 0;
/*
	if (!snd_soc_codec_volatile_register(codec, reg) &&
			(reg) < codec->driver->reg_cache_size &&
			!codec->cache_bypass) {
		ret = snd_soc_cache_read(codec, reg, &val);
		snd_dbg("%s: reg[0x%x]\r\n", __func__, reg);
		if (ret < 0) {
			snd_err("atc2603c_read: reg=%#X,ret=%d\n",
				reg, ret);
			return ret;
		}
		return val;
	}
*/
	ret = atc260x_reg_read(atc260x, reg + REG_BASE);
//	snd_dbg("%s: reg[0x%x]\r\n", __func__, reg + REG_BASE);
	
	if (ret < 0)
	   snd_err("atc2603c_read: reg = %#X, ret = %d failed\n", reg, ret);

	return ret;
}

static int atc2603c_volatile_register(
	struct snd_soc_codec *codec, unsigned int reg)
{
	return 0;
}

static int atc2603c_readable_register(
	struct snd_soc_codec *codec, unsigned int reg)
{
	return 1;
}

#define ADC0_GAIN_MAX	25

static int atc2603c_adc_gain_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int val;
	//snd_soc_cache_sync(codec);
	val = snd_soc_read(codec, mc->reg);
	ucontrol->value.integer.value[0] =
		((val & AGC_CTL0_AMP1GL_MSK) >> mc->shift);
	ucontrol->value.integer.value[1] =
		(val & AGC_CTL0_AMP1GR_MSK) >> mc->rshift;

	return 0;
}

static int atc2603c_adc_gain_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int val, val1, val2;

	val = snd_soc_read(codec, mc->reg);
	val1 = ucontrol->value.integer.value[0];
	val2 = val1;
	val = val & (~AGC_CTL0_AMP1GL_MSK);
	val = val & (~AGC_CTL0_AMP1GR_MSK);
	val = val | (val1<< mc->shift);
	val = val | (val2 << mc->rshift);

	snd_soc_update_bits_locked(codec, mc->reg,
		AGC_CTL0_AMP1GL_MSK | AGC_CTL0_AMP1GR_MSK,
		val);
	//snd_soc_cache_sync(codec);

	return 0;
}

static int atc2603c_mic_gain_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] =
		audio_hw_cfg.mic0_gain[0];
	ucontrol->value.integer.value[1] =
		audio_hw_cfg.mic0_gain[1];

	return 0;
}

static int atc2603c_earphone_gain_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] =
		audio_hw_cfg.earphone_gain[0];
	ucontrol->value.integer.value[1] =
		audio_hw_cfg.earphone_gain[1];

	return 0;
}

static int atc2603c_speaker_gain_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] =
		audio_hw_cfg.speaker_gain[0];
	ucontrol->value.integer.value[1] =
		audio_hw_cfg.speaker_gain[1];

	return 0;
}

static int atc2603c_speaker_volume_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] =
		audio_hw_cfg.speaker_volume;

	return 0;
}

static int atc2603c_earphone_volume_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] =
		audio_hw_cfg.earphone_volume;

	return 0;
}

static int atc2603c_mic_num_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] =
		audio_hw_cfg.mic_num;

	return 0;
}

//20141202 by yuchen: new_code, get mic mode config from dts, 1 for differential, 2 for single end
static int atc2603c_mic_mode_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	
	ucontrol->value.integer.value[0] =
		audio_hw_cfg.mic_mode;

	return 0;	
}

static int atc2603c_earphone_detect_method_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	if(earphone_gpio_num < 0)
	{
		//no gpio, we use irq
		ucontrol->value.integer.value[0] = 0;
	}
	else
	{
		//with gpio, we use gpio
		ucontrol->value.integer.value[0] = 1;
	}
	return 0;	
}



const char *atc2603c_mic0_mode[] = {
	"Differential", "Single ended"};

static const SOC_ENUM_SINGLE_DECL(
		atc2603c_mic0_mode_enum, ADC_CTL,
		ADC_CTL_MIC0FDSE_SFT, atc2603c_mic0_mode);

static const char *pa_output_swing[] = {
	"Vpp2.4", "Vpp1.6"};

static const SOC_ENUM_SINGLE_DECL(
		pa_output_swing_enum, DAC_ANALOG1,
		DAC_ANALOG1_PASW_SFT, pa_output_swing);

const struct snd_kcontrol_new atc2603c_snd_controls[] = {

	SOC_DOUBLE_EXT_TLV("Dummy mic Gain",
		0, 0, 1, 0xf, 0,
		atc2603c_mic_gain_get,
		NULL,
		NULL),

	SOC_DOUBLE_EXT_TLV("Dummy earphone gain",
		0, 0, 1, 0xff, 0,
		atc2603c_earphone_gain_get,
		NULL,
		NULL),

	SOC_DOUBLE_EXT_TLV("Dummy speaker gain",
		0, 0, 1, 0xff, 0,
		atc2603c_speaker_gain_get,
		NULL,
		NULL),

	SOC_SINGLE_EXT_TLV("Dummy speaker volume",
		0, 0, 0x28, 0,
		atc2603c_speaker_volume_get,
		NULL,
		NULL),

	SOC_SINGLE_EXT_TLV("Dummy earphone volume",
		0, 0, 0x28, 0,
		atc2603c_earphone_volume_get,
		NULL,
		NULL),

	SOC_SINGLE_EXT_TLV("Dummy mic num",
		0, 0, 0x2, 0,
		atc2603c_mic_num_get,
		NULL,
		NULL),

	SOC_DOUBLE_EXT_TLV("Adc0 Gain",
		AGC_CTL0,
		AGC_CTL0_AMP1G0L_SFT,
		AGC_CTL0_AMP1G0R_SFT,
		0xf,
		0,
		atc2603c_adc_gain_get,
		atc2603c_adc_gain_put,
		NULL),

	SOC_SINGLE_TLV("AMP1 Gain boost Range select",
		AGC_CTL0,
		AGC_CTL0_AMP0GR1_SET,
		0x7,
		0,
		NULL),
		
	SOC_SINGLE_TLV("ADC0 Digital Gain control",
		ADC_DIGITALCTL,
		ADC_DIGITALCTL_ADGC0_SFT,
		0xF,
		0,
		NULL),	
		
	SOC_ENUM("Mic0 Mode Mux", atc2603c_mic0_mode_enum),
	
	SOC_SINGLE_EXT_TLV("Dummy mic mode",
		0, 0, 1, 0,
		atc2603c_mic_mode_get,
		NULL,
		NULL),

	SOC_SINGLE_EXT_TLV("Dummy earphone detect method",
		0, 0, 1, 0,
		atc2603c_earphone_detect_method_get,
		NULL,
		NULL),
		
	
	
	SOC_ENUM("PA Output Swing Mux", pa_output_swing_enum),
	
	SOC_SINGLE_TLV("DAC PA Volume",
		DAC_ANALOG1,
		DAC_ANALOG1_VOLUME_SFT,
		0x28,
		0,
		NULL),
		
	SOC_SINGLE("DAC FL FR PLAYBACK Switch",
		DAC_ANALOG1,
		DAC_ANALOG1_DACFL_FRMUTE_SFT,
		1,
		0),
		
	SOC_SINGLE_TLV("DAC FL Gain",
		DAC_VOLUMECTL0,
		DAC_VOLUMECTL0_DACFL_VOLUME_SFT,
		0xFF,
		0,
		NULL),
		
	SOC_SINGLE_TLV("DAC FR Gain",
		DAC_VOLUMECTL0,
		DAC_VOLUMECTL0_DACFR_VOLUME_SFT,
		0xFF,
		0,
		NULL),
		
	SOC_SINGLE("DAC PA Switch",
		DAC_ANALOG3,
		DAC_ANALOG3_PAEN_FR_FL_SFT,
		1,
		0),
		
	SOC_SINGLE("DAC PA OUTPUT Stage Switch",
		DAC_ANALOG3,
		DAC_ANALOG3_PAOSEN_FR_FL_SFT,
		1,
		0),
		
	SOC_DOUBLE("DAC Digital FL FR Switch",
		DAC_DIGITALCTL,
		DAC_DIGITALCTL_DEFL_SFT,
		DAC_DIGITALCTL_DEFR_SFT,
		1,
		0),

	SOC_SINGLE("Internal Mic Power Switch", 
		AGC_CTL0,
		AGC_CTL0_VMICINEN_SFT,
		1,
		0),
		
	SOC_SINGLE("External Mic Power Switch",
		AGC_CTL0,
		AGC_CTL0_VMICEXEN_SFT,
		1,
		0),
		
	SOC_SINGLE_TLV("External MIC Power Voltage",
		AGC_CTL0,
		AGC_CTL0_VMICEXST_SFT,
		0x3,
		0,
		NULL),
		
	SOC_SINGLE_TLV("Adc0 Digital Gain",
		ADC_DIGITALCTL,
		ADC_DIGITALCTL_ADGC0_SFT,
		0xf, 0, NULL),

};
#ifdef CONFIG_SND_UBUNTU
static int atc2603c_playback_set_controls(struct snd_soc_codec *codec)
{
    snd_soc_update_bits(codec, DAC_ANALOG1, 0x3f << DAC_ANALOG1_VOLUME_SFT, 0x28 << DAC_ANALOG1_VOLUME_SFT);
    snd_soc_update_bits(codec, DAC_VOLUMECTL0, 0xff << DAC_VOLUMECTL0_DACFL_VOLUME_SFT, 0xb5 << DAC_VOLUMECTL0_DACFL_VOLUME_SFT);
    snd_soc_update_bits(codec, DAC_VOLUMECTL0, 0xff << DAC_VOLUMECTL0_DACFR_VOLUME_SFT, 0xb5 << DAC_VOLUMECTL0_DACFR_VOLUME_SFT);
    snd_soc_update_bits(codec, DAC_ANALOG3, 0x1 << DAC_ANALOG3_PAEN_FR_FL_SFT, 0x01 << DAC_ANALOG3_PAEN_FR_FL_SFT);
    snd_soc_update_bits(codec, DAC_ANALOG3, 0x1 << DAC_ANALOG3_PAOSEN_FR_FL_SFT, 0x01 << DAC_ANALOG3_PAOSEN_FR_FL_SFT);
    snd_soc_update_bits(codec, DAC_DIGITALCTL, 0x03 << DAC_DIGITALCTL_DEFL_SFT, 0x03 << DAC_DIGITALCTL_DEFL_SFT);
    snd_soc_update_bits(codec, DAC_ANALOG1, 0x01 << DAC_ANALOG1_PASW_SFT, 0x01 << DAC_ANALOG1_PASW_SFT);
    snd_soc_update_bits(codec, DAC_ANALOG3, 0x03 << 0, 0x03);
}

static int atc2603c_capture_set_controls(struct snd_soc_codec *codec)
{
    snd_soc_update_bits(codec, DAC_ANALOG1, 0x1 << DAC_ANALOG1_DACMICMUTE_SFT, 0x0 << DAC_ANALOG1_DACMICMUTE_SFT);
    snd_soc_update_bits(codec, DAC_ANALOG1, 0x1 << DAC_ANALOG1_DACFMMUTE_SFT, 0x0 << DAC_ANALOG1_DACFMMUTE_SFT);
    snd_soc_update_bits(codec, ADC_CTL, 0x3 << 6, 0x3 << 6);
    snd_soc_update_bits(codec, ADC_CTL, 0x1f << 0, 0x12 << 0);

    snd_soc_update_bits(codec, AGC_CTL0, 0xf << AGC_CTL0_AMP1G0L_SFT, 0x7 << AGC_CTL0_AMP1G0L_SFT);
    snd_soc_update_bits(codec, AGC_CTL0, 0xf << AGC_CTL0_AMP1G0R_SFT, 0x7 << AGC_CTL0_AMP1G0R_SFT);
    snd_soc_update_bits(codec, ADC_DIGITALCTL, 0xF << ADC_DIGITALCTL_ADGC0_SFT, 0x3 << ADC_DIGITALCTL_ADGC0_SFT);
    snd_soc_update_bits(codec, AGC_CTL0, 0x7 << AGC_CTL0_AMP0GR1_SET, 0x07 << AGC_CTL0_AMP0GR1_SET);
    snd_soc_update_bits(codec, AGC_CTL0, 0x1 << AGC_CTL0_VMICINEN_SFT, 0x1 << AGC_CTL0_VMICINEN_SFT);
    snd_soc_update_bits(codec, AGC_CTL0, 0x1 << AGC_CTL0_VMICEXEN_SFT, 0x1 << AGC_CTL0_VMICEXEN_SFT);
    snd_soc_update_bits(codec, AGC_CTL0, 0x3 << AGC_CTL0_VMICEXST_SFT, 0x1 << AGC_CTL0_VMICEXST_SFT);

    if(audio_hw_cfg.mic_mode == 1){  //if differential.
        snd_soc_update_bits(codec, ADC_CTL, 0x1 << ADC_CTL_MIC0FDSE_SFT, 0x0 << ADC_CTL_MIC0FDSE_SFT);
        //FIXME no fd on 705a for now
        snd_soc_update_bits_pmu(codec, ATC2603C_MFP_CTL,
                0x3<<9 , 0x2<<9);//MICINL&MICINR
        snd_soc_update_bits_pmu(codec, ATC2603C_MFP_CTL,
                0x01 , 0x01);//MICINL&MICINR
    }else{ //if single end. mic_mode=2
        snd_soc_update_bits(codec, ADC_CTL, 0x1 << ADC_CTL_MIC0FDSE_SFT, 0x0 << ADC_CTL_MIC0FDSE_SFT);
        snd_soc_update_bits_pmu(codec, ATC2603C_MFP_CTL,
                0x03 , 0);//MICINL&MICINR
    }
}
#endif
/*FIXME on input source selection */
const char *atc2603c_adc0_src[] = {"None", "FM","MIC0", "FM MIC0", "PAOUT"};

static const SOC_ENUM_SINGLE_DECL(
	atc2603c_adc0_enum, ADC_CTL,
	ADC_CTL_ATAD_MTA_FTA_SFT, atc2603c_adc0_src);

const struct snd_kcontrol_new atc2603c_adc0_mux =
SOC_DAPM_ENUM("ADC0 Source", atc2603c_adc0_enum);


const struct snd_kcontrol_new atc2603c_dac_lr_mix[] = {
	SOC_DAPM_SINGLE("FL FR Switch", DAC_ANALOG1,
			DAC_ANALOG1_DACFL_FRMUTE_SFT, 1, 0),
	SOC_DAPM_SINGLE("MIC Switch", DAC_ANALOG1,
			DAC_ANALOG1_DACMICMUTE_SFT, 1, 0),
	SOC_DAPM_SINGLE("FM Switch", DAC_ANALOG1,
			DAC_ANALOG1_DACFMMUTE_SFT, 1, 0),
};

static int atc2603c_mic0_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	unsigned int val;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		val = snd_soc_read(codec, ADC_CTL);
		/* single end or full differential */
		if (val & ADC_CTL_MIC0FDSE) {//if single end
			snd_soc_update_bits_pmu(codec, ATC2603C_MFP_CTL,
					0x03 , 0);//MICINL&MICINR
		}
		else
		{
			//FIXME no fd on 705a for now
			snd_soc_update_bits_pmu(codec, ATC2603C_MFP_CTL,
					0x3<<9 , 0x2<<9);//MICINL&MICINR
			snd_soc_update_bits_pmu(codec, ATC2603C_MFP_CTL,
					0x01 , 0x01);//MICINL&MICINR			
			
		}
		//snd_soc_update_bits(codec, ADC1_CTL,
		//		0x3 << 7, 0x3 << 7);//(VRDN output0 enable) | (VRDN output1 enable)
		snd_soc_update_bits(codec, ADC_HPFCTL,
				0x3, 0x3);//(High Pass filter0 L enable) | (High Pass filter0 R enable)

		break;

	case SND_SOC_DAPM_POST_PMD:
		break;

	default:
		return 0;
	}

	return 0;
}

static void i2s_clk_disable(void)
{
}

static int i2s_clk_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		break;

	case SND_SOC_DAPM_PRE_PMD:
		i2s_clk_disable();
		break;

	default:
		return 0;
	}

	return 0;
}

static const struct snd_soc_dapm_widget atc2603c_dapm_widgets[] = {
	/* Input Lines */
	SND_SOC_DAPM_INPUT("MICIN0LP"),
	SND_SOC_DAPM_INPUT("MICIN0LN"),
	SND_SOC_DAPM_INPUT("MICIN0RP"),
	SND_SOC_DAPM_INPUT("MICIN0RN"),
	SND_SOC_DAPM_INPUT("FMINL"),
	SND_SOC_DAPM_INPUT("FMINR"),

	SND_SOC_DAPM_SUPPLY("I2S_CLK", SND_SOC_NOPM,
		0, 0, i2s_clk_event,
		SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMU),
//	SND_SOC_DAPM_PGA("MICIN0L", ADC_CTL,
//		ADC_CTL_MIC0LEN_SFT, 0, NULL, 0),
//	SND_SOC_DAPM_PGA("MICIN0R", ADC_CTL,
//		ADC_CTL_MIC0REN_SFT, 0, NULL, 0),
	SND_SOC_DAPM_PGA("FM L", ADC_CTL,
		ADC_CTL_FMLEN_SFT, 0, NULL, 0),
	SND_SOC_DAPM_PGA("FM R", ADC_CTL,
		ADC_CTL_FMREN_SFT, 0, NULL, 0),

	SND_SOC_DAPM_MIC("MICIN0", atc2603c_mic0_event),
	SND_SOC_DAPM_MIC("FM", NULL),
	/* ADC0 MUX */
	SND_SOC_DAPM_MUX("ADC0 Mux", SND_SOC_NOPM, 0, 0,
		&atc2603c_adc0_mux),
	/* ADCS */
	SND_SOC_DAPM_ADC("ADC0 L", NULL, ADC_CTL,
			ADC_CTL_AD0LEN_SFT, 0),
	SND_SOC_DAPM_ADC("ADC0 R", NULL, ADC_CTL,
			ADC_CTL_AD0REN_SFT, 0),
	/* DAC Mixer */
	SND_SOC_DAPM_MIXER("AOUT FL FR Mixer",
			SND_SOC_NOPM, 0, 0,
			atc2603c_dac_lr_mix,
			ARRAY_SIZE(atc2603c_dac_lr_mix)),
	SND_SOC_DAPM_DAC("DAC FL", NULL, DAC_ANALOG3,
		DAC_ANALOG3_DACEN_FL_SFT, 0),
	SND_SOC_DAPM_DAC("DAC FR", NULL, DAC_ANALOG3,
		DAC_ANALOG3_DACEN_FR_SFT, 0),

	SND_SOC_DAPM_AIF_IN("AIFRX", "AIF Playback", 0,
			SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("AIFTX", "AIF Capture", 0,
			SND_SOC_NOPM, 0, 0),

	/* output lines */
	SND_SOC_DAPM_OUTPUT("HP"),
	SND_SOC_DAPM_OUTPUT("SP"),
};

static const struct snd_soc_dapm_route atc2603c_dapm_routes[] = {
//	{"MICIN0L", NULL, "MICIN0LP"},
//	{"MICIN0L", NULL, "MICIN0LN"},
//	{"MICIN0R", NULL, "MICIN0RP"},
//	{"MICIN0R", NULL, "MICIN0RN"},
	{"FM L", NULL, "FMINL"},
	{"FM R", NULL, "FMINR"},
//	{"MICIN0", NULL, "MICIN0L"},
//	{"MICIN0", NULL, "MICIN0R"},
	{"FM", NULL, "FM L"},
	{"FM", NULL, "FM R"},
	{"ADC0 Mux", "MIC0", "MICIN0"},
	{"ADC0 Mux", "FM", "FM"},
	{"ADC0 Mux", "AOUT MIXER", "AOUT FL FR Mixer"},
	{"ADC0 L", NULL, "ADC0 Mux"},
	{"ADC0 R", NULL, "ADC0 Mux"},
	{"AIFTX", NULL, "ADC0 L"},
	{"AIFTX", NULL, "ADC0 R"},
	{"AOUT FL FR Mixer", "FL FR Switch", "AIFRX"},
	{"AOUT FL FR Mixer", "MIC Switch", "MICIN0"},
	{"AOUT FL FR Mixer", "FM Switch", "FM"},
	{"DAC FL", NULL, "AOUT FL FR Mixer"},
	{"DAC FR", NULL, "AOUT FL FR Mixer"},
	{"HP", NULL, "DAC FL"},
	{"HP", NULL, "DAC FR"},
	{"SP", NULL, "DAC FL"},
	{"SP", NULL, "DAC FR"},
};

static void pa_up(struct snd_soc_codec *codec) {
	/* DAC充电的方法:非直驱
	a)	所有audio analog寄存器都写0，主控TX和TXFIFO都使能。
	b)	向主控里面写最小值（0x80000000）
	c)	主控和atc2603c选择N wire模式，2.0 channel模式
	d)	atc2603c的DAC_D&A都使能
	e)	PA BIAS EN ，PA EN，LOOP2 EN，atc2603c_DAC_ANALOG1写全0
	f)	Delay10ms
	g)	DAC 开始放ramp数据（从0X80000000到0x7fffffff，每隔0x36000写一次
	，使用mips查询写5201的I2S_TX FIFO）,同时开启ramp connect
	h)	等待约500 ms
	i)	开启pa输出级en，断开ramp connect，LOOP2 Disable。
	*/

	set_dai_reg_base(I2S_SPDIF_NUM);
	/* i2stx fifo en */
	snd_dai_writel(snd_dai_readl(I2S_FIFOCTL) | 0x3, I2S_FIFOCTL);
	/* i2s tx en */
	snd_dai_writel(snd_dai_readl(I2S_CTL) | 0x3, I2S_CTL);
	snd_soc_write(codec, DAC_VOLUMECTL0, 0xbebe);
	snd_soc_write(codec, DAC_ANALOG0, 0);
	snd_soc_write(codec, DAC_ANALOG1, 0);
	snd_soc_write(codec, DAC_ANALOG2, 0);
	snd_soc_write(codec, DAC_ANALOG3, 0);

	if (direct_drive_disable == 0) {
		snd_dai_writel(0x1 << 31, I2STX_DAT);
		snd_dai_writel(0x1 << 31, I2STX_DAT);
	} else {
		snd_dai_writel(0x7ffffe00, I2STX_DAT);
		snd_dai_writel(0x7ffffe00, I2STX_DAT);
	}

	/* 2.0-Channel Mode */
	snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x7 << 4), I2S_CTL);
	
	if (direct_drive_disable != 0) 
	{	
		/* I2S OUTPUT input ENABLE,SEL 4WIRE MODE,0x1a6:4 wire,tx 4.0,rx 5.1 */
		/* I2S input en */
		snd_soc_update_bits(codec, AUDIOINOUT_CTL, 0x01 << 1, 0x01 << 1);
		/* I2S output en */
		snd_soc_update_bits(codec, AUDIOINOUT_CTL, 0x03 << 5, 0x01 << 5);
		/* I2S TX&RX MODE, SEL 4WIRE MODE*/
		//snd_soc_update_bits(codec, AUDIOINOUT_CTL, 0x01 << 8, 0x01 << 8);
		/* dac_2ch FL&FR enable */
		snd_soc_update_bits(codec, DAC_DIGITALCTL, 0x3, 0x3);
	}
	/* da_a  EN,PA EN,all bias en */
	snd_soc_write(codec, DAC_ANALOG3, 0x4b7);
	/* DAC PA BIAS */
	snd_soc_write(codec, DAC_ANALOG0, 0x26b3);
	/* PA VOLUME=0 */
	snd_soc_write(codec, DAC_ANALOG1, 0x0000);
	/* */
	snd_soc_write(codec, DAC_ANALOG2, 0x03);
	
	if (direct_drive_disable != 0) 
	{	
		/* da_a  EN,PA EN,OUTPSTAGE DIS,all bias en,loop2 en*/
		snd_soc_write(codec, DAC_ANALOG3, 0x6b7);
	}
	else
	{
		/*PA EN,OUTPSTAGE en,all bias en,loop2en*/
		snd_soc_write(codec, DAC_ANALOG3, 0x6bc);		
	}

	snd_dai_writel(((snd_dai_readl(I2S_CTL) & ~(0x3 << 11)) | (0x1 << 11)), I2S_CTL);
	if (direct_drive_disable == 0) {
		//direct mode
		msleep(100);
		/* antipop_VRO Resistant Connect */
		snd_soc_write(codec, DAC_ANALOG2, 0x0b);
		/* out stage en */
		snd_soc_update_bits(codec, DAC_ANALOG2 , 0x1<<4, 0x1<<4);
		/* DAC_OPVRO enable */
		snd_soc_update_bits(codec, DAC_ANALOG2, 0x1<<3, 0);
		//ramp_direct(0x80000000, 0x7ffffe00);
	} else {
		//non direct mode
		msleep(100);
		/* PA RAMP2 CONNECT */
		snd_soc_update_bits(codec, DAC_ANALOG2 , 0x1<<4, 0x1<<4);
		
		ramp_undirect(0x80000000, 0x7ffffe00);
	}
	
	if (direct_drive_disable != 0) 
	{
		//non direct mode
		msleep(400);
		/* out stage en */
		/*EN,PA EN,OUTPSTAGE EN,all bias en,loop2 en*/
		snd_soc_update_bits(codec, DAC_ANALOG3, 0x1<<3, 0x1<<3);
		msleep(100);
		
		/*EN,PA EN,OUTPSTAGE EN,all bias en,loop2 DISABLE */
		snd_soc_update_bits(codec, DAC_ANALOG3, 0x1<<9, 0);
		/* ramp disconnect */
		snd_soc_update_bits(codec, DAC_ANALOG2, 0x1<<9, 0);
	}
	
	snd_dai_writel(0x0, I2STX_DAT);
	snd_dai_writel(0x0, I2STX_DAT);
	msleep(20);
}


static void atc2603c_pa_down(struct snd_soc_codec *codec)
{
	if(hw_init_flag == true)
	{	
        	if (direct_drive_disable == 0) {
        		/* antipop_VRO Resistant Connect */
        		snd_soc_update_bits(codec, DAC_ANALOG2, 0x3<<3, 0x3<<3);
        		/* PA EN,OUTPSTAGE disen,all bias en,loop2 en */
        		snd_soc_write(codec, DAC_ANALOG2, 0x6b4);
        		/* bit3 vro antipop res connect,vro disen */
        		snd_soc_update_bits(codec, DAC_ANALOG2, 0x1<<4, 0);
        		/* bit3 vro antipop res disconnect,vro disen */
        		snd_soc_update_bits(codec, DAC_ANALOG2, 0x1<<3, 0);
        		
        	} else {
        		snd_dai_writel(snd_dai_readl(I2S_FIFOCTL) | 0x3, I2S_FIFOCTL);
        		snd_dai_writel(snd_dai_readl(I2S_CTL) | 0x3, I2S_CTL);
        		//act_snd_writel(act_snd_readl(CO_AUDIOINOUT_CTL) | (0x1 << 1), CO_AUDIOINOUT_CTL);
        		
        		snd_soc_write(codec, DAC_VOLUMECTL0, 0xbebe);
        		
        		/* PA LOOP2 en */
        		snd_soc_update_bits(codec, DAC_ANALOG3, 0x1<<9, 0x1<<9);
        		/* 隔直电容Discharge 开启 */
        		snd_soc_update_bits(codec, DAC_ANALOG2, 0x1<<8, 0x1<<8);
        		
        		snd_dai_writel(0x7ffffe00, I2STX_DAT);
        		snd_dai_writel(0x7ffffe00, I2STX_DAT);
        		msleep(300);
        		
        		/* ramp Connect EN */
        		snd_soc_update_bits(codec, DAC_ANALOG2, 0x1 << 9, 0x1<<9);
        		ramp_undirect(0x80000000, 0x7ffffe00);
			msleep(600);
        	}
		snd_soc_write(codec, DAC_VOLUMECTL0, 0);
		hw_init_flag = false;
	}
}

static void atc2603c_adckeypad_config(struct snd_soc_codec *codec)
{
	//external mic power enable
	snd_soc_update_bits(codec, AGC_CTL0, 0x1 << 6, 0x1 << 6); 
	snd_soc_update_bits(codec, AGC_CTL0, 0x3 << 4, 0x3 << 4);
	//printk("atc2603_ADC0_CTL:0x%x\n",act_snd_readl(atc2603_ADC0_CTL));
	
	//enable auxadc2
	snd_soc_update_bits_pmu(codec, ATC2603C_PMU_AUXADC_CTL0, 0x1 << 12, 0x1 << 12);
    
	//GL5201_ADCKEY_INFO("open:AuxADC_CTL0 = 0x%x\n",act_snd_readl(atc2603_PMU_AuxADC_CTL0));                
}

static int detect_dep = 0;
static int old_adc_state = SPEAKER_ON;
static int now_adc_state = SPEAKER_ON;
static int earphone_is_in_for_adc(void)
{
	int adc_val = 0;
	
	if(audio_hw_cfg.adc_level == 1)
	{
		
		adc_val = 1024-atc2603c_read_pmu(atc2603c_codec, ATC2603C_PMU_AUXADC2);
		//printk("direct 0x%x\n", adc_val);
        	if(adc_val < audio_hw_cfg.adc_plugin_threshold)
        	{
        		return HEADSET_NO_MIC;
        	}
        	
        	return SPEAKER_ON;
	}
	else
	{
		adc_val = atc2603c_read_pmu(atc2603c_codec, ATC2603C_PMU_AUXADC2);
		//printk("undirect 0x%x\n", adc_val);
        	if(adc_val < audio_hw_cfg.adc_plugin_threshold)
        	{
        		return HEADSET_NO_MIC;
        	}
        	
        	return SPEAKER_ON;
	}
	
	
}


static void adc_poll(struct work_struct *data) 
{
    	now_adc_state = earphone_is_in_for_adc();

    	if(now_adc_state != old_adc_state)
    	{
    		if(detect_dep >= 4)
    		{
    			//state is stable
    			detect_dep = 0;
    		} 	
    		else
    		{
    			//printk("detect_dep %d\n", detect_dep);
    			detect_dep++;
    			goto POLL_EXIT;
    		}
    	}
    	else //now_state == old_state
    	{
    		if(detect_dep > 0)
    		{
    			//state is not stable yet
    			detect_dep = 0;
    		}
    		
    		goto POLL_EXIT;
    	}
    	
    	        	
        //we get here because state changed and stable	
    	if (now_adc_state == HEADSET_NO_MIC) {
            printk("sndrv: speaker off \n");
        	/* 1:headset with mic;  2:headset without mic */
            switch_set_state(&headphone_sdev, HEADSET_NO_MIC);
                        
        } 
        else
        {
            printk("sndrv: speaker on state %d, old_state %d\n",now_adc_state, old_adc_state);
            switch_set_state(&headphone_sdev, SPEAKER_ON);
        }

	old_adc_state = now_adc_state;        

POLL_EXIT:	
	schedule_delayed_work(&dwork_adc_detect, msecs_to_jiffies(earphone_poll_ms));
}


void atc2603c_pa_up_all(struct snd_soc_codec *codec)
{
	int ret;

	/* 标识classd的状态是否被设置为固定不需要改变，缺省是TRUE */
	static int classd_flag = 1;
	struct clk *apll_clk;

	module_clk_disable(MOD_ID_I2SRX);
	module_clk_disable(MOD_ID_I2STX);
	module_clk_enable(MOD_ID_I2SRX);
	module_clk_enable(MOD_ID_I2STX);

	/*for the fucked bug of motor shaking while startup,
	because GPIOB(1) is mfp with I2S_LRCLK1*/
	set_dai_reg_base(I2S_SPDIF_NUM);
	snd_dai_writel(snd_dai_readl(GPIO_BOUTEN) | 2, GPIO_BOUTEN);

	if(((snd_dai_readl(I2S_CTL) & 0x3) == 0x0)) {
		/* disable i2s tx&rx */
		snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x3 << 0), I2S_CTL);

		/* avoid sound while reset fifo */
		snd_soc_update_bits(codec,DAC_ANALOG1, 0x1 << 10, 0);

		/* reset i2s rx&&tx fifo, avoid left & right channel wrong */
		snd_dai_writel(snd_dai_readl(I2S_FIFOCTL) & ~(0x3 << 9) & ~0x3, I2S_FIFOCTL);
		snd_dai_writel(snd_dai_readl(I2S_FIFOCTL) | (0x3 << 9) | 0x3, I2S_FIFOCTL);

		/* this should before enable rx/tx, or after suspend, data may be corrupt */
		snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x3 << 11),I2S_CTL);
		snd_dai_writel(snd_dai_readl(I2S_CTL) | (0x1 << 11),I2S_CTL);
		/* set i2s mode I2S_RX_ClkSel==1 */
		snd_dai_writel(snd_dai_readl(I2S_CTL) | (0x1 << 10), I2S_CTL);

		/* enable i2s rx/tx at the same time */
		snd_dai_writel(snd_dai_readl(I2S_CTL) | 0x3, I2S_CTL);

		apll_clk = clk_get(NULL, CLKNAME_AUDIOPLL);
		clk_prepare(apll_clk);
		clk_enable(apll_clk);

		/* i2s rx 00: 2.0-Channel Mode */
		snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x3 << 8), I2S_CTL);
		snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x3 << 4), I2S_CTL);
	}

    	/* EXTIRQ pad enable, 使中断信号能送到主控 */
    	snd_soc_update_bits_pmu(codec, ATC2603C_PAD_EN, 0x73, 0x73);
    	/* I2S: 2.0 Channel, SEL 4WIRE MODE */
    	snd_soc_update_bits(codec, AUDIOINOUT_CTL, 0x122, 0);

    	snd_soc_update_bits(codec, AUDIOINOUT_CTL, 0x20, 0x20);
    	
    	//FIXME: it's default to differential here, how to config single end?
	//snd_soc_update_bits_pmu(codec, ATC2603C_MFP_CTL, 0xf << 3, 0);
	if(audio_hw_cfg.mic_mode == 1)
	{
		snd_soc_update_bits_pmu(codec, ATC2603C_MFP_CTL, 0x3<<9 , 0x2<<9);//MICINL&MICINR
		snd_soc_update_bits_pmu(codec, ATC2603C_MFP_CTL, 0x01 , 0x01);//MICINL&MICINR			
	}
	else
	{
		snd_soc_update_bits_pmu(codec, ATC2603C_MFP_CTL, 0x3 , 0);//MICINL&MICINR		
	}
	

	/* 打开内部的基准滤波器,减小底噪，提升audio性能 */
	/*set_bdg_ctl();*/
	/* for atc2603c those burn Efuse */
	/* bit4:0 should not be changed, otherwise VREF isn't accurate */
	ret = snd_soc_update_bits_pmu(codec, ATC2603C_PMU_BDG_CTL, 0x01 << 6, 0x01 << 6);
	ret = snd_soc_update_bits_pmu(codec, ATC2603C_PMU_BDG_CTL, 0x01 << 5, 0);

	pa_up(codec);

	//snd_soc_update_bits(codec, DAC_DIGITALCTL, 0x3<<2, 0x3<<2);
			
	if (direct_drive_disable == 0)
	{
		snd_soc_update_bits(codec, DAC_ANALOG3, 0x1<<10, 0x1<<10);
		snd_soc_update_bits(codec, DAC_ANALOG2, 0x7, 0x3);
		snd_soc_update_bits(codec, DAC_ANALOG3, 0x7<<4, 0x3<<4);						
	}
			
	if (classd_flag == 1) 
	{
		// DAC&PA Bias enable
		snd_soc_update_bits(codec, DAC_ANALOG3, 0x1 << 10, 0x1 << 10);
		// pa zero cross enable 
		snd_soc_update_bits(codec, DAC_ANALOG2, 0x1 << 15, 0x1 << 15);
		snd_soc_update_bits(codec, DAC_ANALOG3, 0x1 << 13, 0x1 << 13);
	}


	//after pa_up, the regs status should be the same as outstandby?
	snd_soc_update_bits(codec, DAC_ANALOG1, DAC_ANALOG1_DACFL_FRMUTE, 0);//mute
	snd_soc_update_bits(codec, AUDIOINOUT_CTL, 0x01 << 1, 0);//I2S input disable
	snd_soc_update_bits(codec, DAC_ANALOG1, DAC_ANALOG1_PASW, DAC_ANALOG1_PASW);//1.6v
	snd_soc_update_bits(codec, DAC_DIGITALCTL, 0x3<<4, 0x2<<4);
	snd_soc_update_bits(codec, DAC_ANALOG1, 0x3f, 40);//
	snd_soc_write(codec, DAC_VOLUMECTL0, 0xb5b5);//0//0xbebe

/*
	if (direct_drive_disable == 0) {
	}
	else {
		snd_soc_write(codec, DAC_ANALOG2, 0x8400);
		snd_soc_write(codec, DAC_ANALOG3, 0xaa0b);
	}
*/

	snd_dai_writel(snd_dai_readl(I2S_CTL) & ~0x3, I2S_CTL);
	snd_dai_writel(snd_dai_readl(I2S_FIFOCTL) & ~(0x3 << 9) & ~0x3, I2S_FIFOCTL);
	
	
	//FIXME: for earphone irq detect and gpio detect compatibility
	//we need these 2 bits always set to keep irq detect always enabled when no gpio detect available
	//if we use dapm to control these 2 bits, irq detect cant always be enabled
	//and if we dont use dapm and dont set these bits here, mic recording wont function 
	//in gpio detect scenario.
	//so we always set these 2 bits here and delete related dapm configs
	//need to check any negative results
        snd_soc_update_bits(codec, ADC_CTL, 0x1 << 6, 0x1 << 6);
        snd_soc_update_bits(codec, ADC_CTL, 0x1 << 7, 0x1 << 7);
	
	//FIXME: set earphone irq detect
	//enable pmu interupts mask for audio
	//ret = snd_soc_update_bits_pmu(codec, ATC2603C_INTS_MSK, 0x1, 0x1);
	//headset or earphone INOUT DECTECT IRQ enable
	if((earphone_gpio_num < 0) && (adc_detect_mode == 0) && (earphone_irq != -1))	
	{        	
        	snd_soc_update_bits(codec, AUDIOINOUT_CTL, 0x1 << 10, 0x1 << 10);
        	//  External MIC Power VMIC enabled
        	//snd_soc_update_bits(codec, AGC_CTL0, 0x1 << 3, 0x1 << 3);
        	snd_soc_update_bits(codec, AGC_CTL0, 0x1 << 6, 0x1 << 6);
        	snd_soc_update_bits(codec, AGC_CTL0, 0x1 << 7, 0x1 << 7);
        	
        	snd_soc_update_bits(codec, ADC_ANALOG1, 0x1 << 8, 0x1 << 8);
	}
//	snd_soc_update_bits(codec, AUDIOINOUT_CTL, 0x1 << 2, 0);
//	snd_soc_update_bits(codec, AUDIOINOUT_CTL, 0x1 << 3, 0);	
}
EXPORT_SYMBOL_GPL(atc2603c_pa_up_all);

static int atc2603c_audio_set_dai_digital_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;

	if (mute) {
		snd_soc_update_bits(codec, DAC_ANALOG1,
			DAC_ANALOG1_DACFL_FRMUTE, 0);
	} else {
		snd_soc_update_bits(codec, DAC_ANALOG1,
			DAC_ANALOG1_DACFL_FRMUTE,
			DAC_ANALOG1_DACFL_FRMUTE);
	}

	return 0;
}
static int atc2603c_audio_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	/* enable the atc2603c i2s input function */
	struct snd_soc_codec *codec = dai->codec;

	//printk("atc2603c_audio_hw_params %d", hw_init_flag);
	if(hw_init_flag == false) {
		//printk("request IRQ %d!!!!", earphone_irq);
				
		//printk("atc2603c_pa_up_all %d!!!!", hw_init_flag);
		atc2603c_pa_up_all(codec);
		hw_init_flag = true;
		
	}
	//snd_err("%s %d\n", __FILE__,__LINE__);
	snd_soc_update_bits(codec, AUDIOINOUT_CTL, 0x03 << 5, 0x01 << 5);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
/*		
		if (direct_drive_disable == 0) {

		} else {
			snd_soc_write(codec,	DAC_ANALOG2,	0x8400);
			snd_soc_write(codec,	DAC_ANALOG3,	0xaa0b);
		}
*/
		//snd_err("%s %d\n", __FILE__,__LINE__);
		snd_soc_update_bits(codec,
			AUDIOINOUT_CTL, 0x01 << 1, 0x01 << 1);
		snd_soc_update_bits(codec,
			DAC_ANALOG3, 0x03, 0x03);
#ifdef CONFIG_SND_UBUNTU
        atc2603c_playback_set_controls(codec);
		audio_set_output_mode(substream, O_MODE_I2S);
#endif
	} else {
		snd_soc_update_bits(codec,
			ADC_HPFCTL, 0x3 << 0, 0x3 << 0);//adc0 hpf disable
		//snd_err("%s %d\n", __FILE__,__LINE__);
		snd_soc_update_bits(codec,
			AUDIOINOUT_CTL, 0x01 << 8, 0x01 << 8);
#ifdef CONFIG_SND_UBUNTU
        atc2603c_capture_set_controls(codec);
        if(params_channels(params) == 1){
            snd_soc_update_bits(codec, ADC_CTL, 0x3 << 6, 0x2 << 6);
            snd_soc_update_bits(codec, ADC_CTL, 0x3 << 3, 0x2 << 3);
        }else{
            snd_soc_update_bits(codec, ADC_CTL, 0x3 << 6, 0x3 << 6);
            snd_soc_update_bits(codec, ADC_CTL, 0x3 << 3, 0x3 << 3);
        }
#endif
	}
	atc2603c_open_count++;

	return 0;
}

static int atc2603c_audio_hw_free(
	struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	snd_dbg("atc2603c_audio_hw_free\n");
	/* disable the atc2603c i2s input function */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		//printk("SNDRV_PCM_STREAM_PLAYBACK\n");
		snd_soc_update_bits(codec,
				AUDIOINOUT_CTL,
				0x01 << 1, 0);
	} else {
		//printk("SNDRV_PCM_STREAM_CAPTURE\n");
		snd_soc_update_bits(codec,
				AUDIOINOUT_CTL,
				0x01 << 8, 0);
	}

    if(atc2603c_open_count > 0)
        atc2603c_open_count--;
	return 0;
}

static int atc2603c_audio_prepare(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	/* nothing should to do here now */
	return 0;
}

static int atc2603c_audio_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = dai->codec;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		break;
	default:
		return -EINVAL;
	}

	snd_soc_update_bits(codec, DAC_DIGITALCTL, 0x3<<4, 0x2<<4);
	/* we set the i2s 2 channel-mode by default */
	//snd_soc_update_bits(codec, AUDIOINOUT_CTL, 0x03 << 2, 0);

	return 0;
}

static int atc2603c_audio_set_dai_sysclk(struct snd_soc_dai *dai,
		int clk_id, unsigned int freq, int dir)
{
	/* nothing should to do here now */
	return 0;
}

static void atc2603c_power_down(struct snd_soc_codec *codec)
{
	atc2603c_pa_down(codec);
}

static int atc2603c_set_bias_level(struct snd_soc_codec *codec,
		                        enum snd_soc_bias_level level)
{
#if 0
	int ret;
	switch (level) {
	case SND_SOC_BIAS_ON:
		snd_dbg("%s:  SND_SOC_BIAS_ON\n", __func__);
		break;

	case SND_SOC_BIAS_PREPARE:
		snd_dbg("%s:  SND_SOC_BIAS_PREPARE\n", __func__);
		break;

	case SND_SOC_BIAS_STANDBY:
		#ifdef SND_SOC_BIAS_DEBUG
		snd_dbg("%s:  SND_SOC_BIAS_STANDBY\n", __func__);
		#endif

		if (SND_SOC_BIAS_OFF == codec->dapm.bias_level) {
			codec->cache_only = false;
			codec->cache_sync = 1;
			//ret = snd_soc_cache_sync(codec);
		}
		break;

	case SND_SOC_BIAS_OFF:
		snd_dbg("%s:  SND_SOC_BIAS_OFF\n", __func__);
		break;

	default:
		break;
	}

	codec->dapm.bias_level = level;
#endif
	return 0;
}

static void reenable_audio_block(struct snd_soc_codec *codec)
{
	snd_soc_update_bits_pmu(codec,
		ATC2603C_CMU_DEVRST, 0x1 << 4, 0);//audio block reset

	snd_soc_update_bits_pmu(codec, ATC2603C_CMU_DEVRST,
		0x1 << 10, 0x1 << 10);//SCLK to Audio Clock Enable Control
	snd_soc_update_bits_pmu(codec,
		ATC2603C_CMU_DEVRST, 0x1 << 4, 0x1 << 4);

}

static int atc2603c_probe(struct snd_soc_codec *codec)
{
	snd_dbg("atc2603c_probe!\n");
	if (codec == NULL)
		snd_dbg("NULL codec \r\n");

	snd_dbg("codec->name = %s\r\n", codec->name);

	atc2603c_codec = codec;
	codec->read  = atc2603c_read;
	codec->write = atc2603c_write;


	hw_init_flag = false;
	reenable_audio_block(codec);
	//snd_err("check ADC_ANALOG1 0x%x\n", atc2603c_read(codec, 0xf));

	atc2603c_pa_up_all(codec);	
	hw_init_flag = true;


	//codec->dapm.bias_level = SND_SOC_BIAS_STANDBY;
/*
	ret = request_irq(my_irq, earphone_detect_irq_handler,
			  IRQF_TRIGGER_HIGH, "atc260x-audio", NULL);

	ret = devm_request_irq(&pdev->dev, my_irq, earphone_detect_irq_handler,
		 		IRQF_TRIGGER_HIGH, "atc260x-audio", NULL);
*/

	//if((earphone_gpio_num < 0) && (adc_detect_mode == 0) && (earphone_irq != -1))
	if((earphone_gpio_num < 0) && (adc_detect_mode == 0) && (earphone_irq != -1))
	{
		int ret = devm_request_threaded_irq(codec->dev, earphone_irq, NULL,
				earphone_detect_irq_handler, IRQF_TRIGGER_HIGH, "atc260x-audio",
				NULL);

		if(ret < 0)
		{
			snd_err("NO IRQ %d\n", ret);
		}
		//printk("requested IRQ %d!!!!", ret);
	}
	
	if(adc_detect_mode == 1)
	{
		//config adc detect 
		atc2603c_adckeypad_config(codec);

		old_adc_state = SPEAKER_ON;
		now_adc_state = HEADSET_NO_MIC;
		
		INIT_DELAYED_WORK(&dwork_adc_detect, adc_poll);
		schedule_delayed_work(&dwork_adc_detect, msecs_to_jiffies(500));		
		
	}

	snd_soc_update_bits(codec, AUDIOINOUT_CTL, 0x1 << 2, 0);
	//snd_err("%s %d\n", __FILE__,__LINE__);
	snd_soc_update_bits(codec, AUDIOINOUT_CTL, 0x1 << 3, 0);	
		

	return 0;
}

static int atc2603c_remove(struct snd_soc_codec *codec)
{
	if (earphone_irq != -1)
	{
		free_irq(earphone_irq,codec->dev);
		earphone_irq = -1;
	}
	
	atc2603c_pa_down(codec);
	
	if(adc_detect_mode == 1)
	{
		cancel_delayed_work_sync(&dwork_adc_detect);	
	}
	return 0;
}

static unsigned short audio_regs[ADC_ANALOG1+1];
static int audio_regs_stored = -1;
static int atc2603c_store_regs(void)
{
	int i;
	for(i=0;i<=ADC_ANALOG1;i++)
	{
		audio_regs[i] = (unsigned short)atc260x_reg_read(atc260x, i + REG_BASE);
		if(i<8)
		{
			//printk("[0xa%d]:0x%x\n",i, audio_regs[i]);
		}
	}
	
	
	audio_regs_stored = 1;
	return 0;
}

static int atc2603c_restore_regs(void)
{
	int i;
	int reg_now;
	
	if(audio_regs_stored < 0)
	{
		printk("no initial regs value yet\n");
		return 0;
	}
	for(i=0;i<=ADC_ANALOG1;i++)
	{		
		reg_now = atc260x_reg_read(atc260x, i + REG_BASE);
		if(reg_now != (int)audio_regs[i])
		{
			atc260x_reg_write(atc260x, i + REG_BASE, audio_regs[i]);
		}		
	}
	
	audio_regs_stored = -1;
	
	return 0;
}


static int atc2603c_suspend(struct snd_soc_codec *codec)
{
	snd_err("atc2603c_suspend\n");
    //atc2603c_store_regs();
#ifdef CONFIG_SND_UBUNTU
	atc2603c_power_down(codec);
	atc2603c_set_bias_level(codec, SND_SOC_BIAS_OFF);
#endif
	if(adc_detect_mode == 1)
	{
		cancel_delayed_work_sync(&dwork_adc_detect);
	}
	return 0;
}

static int atc2603c_resume(struct snd_soc_codec *codec)
{
	int i;
	snd_err("atc2603c_resume\n");	
	printk("atc2603c_resume\n");
	reenable_audio_block(codec);
    
#ifdef CONFIG_SND_UBUNTU
	atc2603c_pa_up_all(codec);
	hw_init_flag = true;
#endif
	//atc2603c_restore_regs();
	//atc2603c_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	for(i=0;i<8;i++)
	{
		unsigned short reg_now;
		reg_now = (unsigned short)atc260x_reg_read(atc260x, i + REG_BASE);
                printk("[0xa%d]:0x%x\n",i, reg_now);
	}

	if(adc_detect_mode == 1)
	{
		schedule_delayed_work(&dwork_adc_detect, msecs_to_jiffies(500));
		atc2603c_adckeypad_config(codec);	
	}
#ifdef CONFIG_SND_UBUNTU  //suspend/resume
    if(atc2603c_open_count > 0){
        atc2603c_playback_set_controls(codec);
        snd_soc_update_bits(codec, AUDIOINOUT_CTL, 0x01 << 1, 0x01 << 1);
    }
#endif
	return 0;
}

static void atc2603c_audio_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
#if 0
	struct snd_soc_codec *codec = dai->codec;

	atc2603c_power_down(codec);
	atc2603c_set_bias_level(codec, SND_SOC_BIAS_OFF);
#endif
	return;
}

#define ATC2603C_RATES SNDRV_PCM_RATE_8000_192000
#ifdef CONFIG_SND_UBUNTU
#define ATC2603C_FORMATS (SNDRV_PCM_FMTBIT_S32_LE |\
	SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE)
#else
#define ATC2603C_FORMATS (SNDRV_PCM_FMTBIT_S16_LE |\
	SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE)
#endif

struct snd_soc_dai_ops atc2603c_aif_dai_ops = {
	.shutdown = atc2603c_audio_shutdown,
	.hw_params = atc2603c_audio_hw_params,
	.hw_free = atc2603c_audio_hw_free,
	.prepare = atc2603c_audio_prepare,
	.set_fmt = atc2603c_audio_set_dai_fmt,
	.set_sysclk = atc2603c_audio_set_dai_sysclk,
	.digital_mute = atc2603c_audio_set_dai_digital_mute,
};

struct snd_soc_dai_driver codec_atc2603c_dai[] = {
	{
		.name = "atc2603c-dai",
		.id = ATC2603C_AIF,
		.playback = {
			.stream_name = "AIF Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ATC2603C_RATES,
			.formats = ATC2603C_FORMATS,
		},
		.capture = {
			.stream_name = "AIF Capture",
			.channels_min = 1,
			.channels_max = 4,
			.rates = ATC2603C_RATES,
			.formats = ATC2603C_FORMATS,
		},
		.ops = &atc2603c_aif_dai_ops,
	},
};

static struct snd_soc_codec_driver soc_codec_atc2603c = {
	.probe = atc2603c_probe,
	.remove = atc2603c_remove,

	.suspend = atc2603c_suspend,
	.resume = atc2603c_resume,
	//.set_bias_level = atc2603c_set_bias_level,
	.idle_bias_off = true,

	.reg_cache_size = (ADC_ANALOG1 + 1),
	.reg_word_size = sizeof(u16),
	//.reg_cache_default = atc2603c_reg,
	.volatile_register = atc2603c_volatile_register,
	.readable_register = atc2603c_readable_register,
	.reg_cache_step = 1,

	.controls = atc2603c_snd_controls,
	.num_controls = ARRAY_SIZE(atc2603c_snd_controls),
	.dapm_widgets = atc2603c_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(atc2603c_dapm_widgets),
	.dapm_routes = atc2603c_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(atc2603c_dapm_routes),
};

static ssize_t error_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int cnt;

	cnt = sprintf(buf, "%d\n(Note: 1: open, 0:close)\n", error_switch);
	return cnt;
}

static ssize_t error_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int cnt, tmp;
	cnt = sscanf(buf, "%d", &tmp);
	switch (tmp) {
	case 0:
	case 1:
		error_switch = tmp;
		break;
	default:
		printk(KERN_ERR"invalid input\n");
		break;
	}
	return count;
}

static ssize_t debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int cnt;

	cnt = sprintf(buf, "%d\n(Note: 1: open, 0:close)\n", debug_switch);
	return cnt;
}

static ssize_t debug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int cnt, tmp;
	cnt = sscanf(buf, "%d", &tmp);
	switch (tmp) {
	case 0:
	case 1:
		debug_switch = tmp;
		break;
	default:
		printk(KERN_INFO"invalid input\n");
		break;
	}
	return count;
}

static struct device_attribute atc2603c_attr[] = {
	__ATTR(error, S_IRUSR | S_IWUSR, error_show, error_store),
	__ATTR(debug, S_IRUSR | S_IWUSR, debug_show, debug_store),
};

int atc2603c_audio_get_pmu_status(void)
{
	return atc2603c_ictype;
}

EXPORT_SYMBOL_GPL(atc2603c_audio_get_pmu_status);

static int earphone_is_in_for_irq(void)
{
	int val = 0;
	val = atc260x_reg_read(atc260x, ADC_ANALOG0+REG_BASE);
	if((val & 0x1<<8) != 0)
	{
		return HEADSET_NO_MIC;
	}
	
	val = atc260x_reg_read(atc260x, AGC_CTL1+REG_BASE);
	if((val & 0x1<<13) != 0)
	{
		return HEADSET_MIC;
	}
	
	return SPEAKER_ON;
}

static int old_state = -1;
static irqreturn_t earphone_detect_irq_handler(int irq, void *data) 
{
    int state;

    printk("earphone in/out\n");
    state = earphone_is_in_for_irq();
    
    if (state == HEADSET_NO_MIC) {
    	printk("sndrv: speaker off \n");
    	/* 1:headset with mic;  2:headset without mic */
        switch_set_state(&headphone_sdev, HEADSET_NO_MIC);
        
        atc260x_reg_write(atc260x,AUDIOINOUT_CTL+REG_BASE, atc260x_reg_read(atc260x, AUDIOINOUT_CTL+REG_BASE) | 0x1<<3);
        atc260x_reg_write(atc260x,AUDIOINOUT_CTL+REG_BASE, atc260x_reg_read(atc260x, AUDIOINOUT_CTL+REG_BASE) & ~(0x1<<3));
        
    } 
    else if (state == HEADSET_MIC)
    {
    	printk("sndrv: speaker off, mic in \n");
    	/* 1:headset with mic;  2:headset without mic */
        switch_set_state(&headphone_sdev, HEADSET_MIC);  
        
        atc260x_reg_write(atc260x,AUDIOINOUT_CTL+REG_BASE, atc260x_reg_read(atc260x, AUDIOINOUT_CTL+REG_BASE) | 0x1<<3);
        atc260x_reg_write(atc260x,AUDIOINOUT_CTL+REG_BASE, atc260x_reg_read(atc260x, AUDIOINOUT_CTL+REG_BASE) & ~(0x1<<3));
          	
    }
    else
    {
    	printk("sndrv: speaker on \n");
        switch_set_state(&headphone_sdev, SPEAKER_ON);

        atc260x_reg_write(atc260x,AUDIOINOUT_CTL+REG_BASE, atc260x_reg_read(atc260x, AUDIOINOUT_CTL+REG_BASE) | 0x1<<2);
        atc260x_reg_write(atc260x,AUDIOINOUT_CTL+REG_BASE, atc260x_reg_read(atc260x, AUDIOINOUT_CTL+REG_BASE) & ~(0x1<<2));
    }
    
    if(old_state == -1)
    {
    	old_state = state;
    }
    
    return IRQ_HANDLED;
}


static int atc2603c_platform_probe(struct platform_device *pdev)
{
	int i;
	int ret = 0;
	
	snd_err("atc2603c_platform_probe\r\n");

/*
	dn = of_find_compatible_node(NULL, NULL, "actions,atm7039c-i2s");
	if (!dn) {
		snd_err("Fail to get device_node actions,atm7039c-i2s\r\n");
		//goto of_get_failed;
	}
*/
	
	/*FIXME: what if error in second or third loop*/
/*
	for(i=0; i<2; i++) 
	{
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			snd_err("no memory resource i=%d\n", i);
			return -ENODEV;
		}

		if (!devm_request_mem_region (&pdev->dev, res->start,
					resource_size(res), "gl5203-audio-i2s")) {
			snd_err("Unable to request register region\n");
			return -EBUSY;
		}

		codec_res.base[i] = devm_ioremap(&pdev->dev, res->start, resource_size(res));
		if (codec_res.base[i] == NULL) {
			snd_err("Unable to ioremap register region\n");
			return -ENXIO;
		}
		
		snd_err("it's ok %d\n", i);
	}
*/
	for (i = 0; i < ARRAY_SIZE(atc2603c_attr); i++) {
		//snd_err("add file!\r\n");
		ret = device_create_file(&pdev->dev, &atc2603c_attr[i]);
	}


	atc260x = dev_get_drvdata(pdev->dev.parent);
	platform_set_drvdata(pdev, atc260x);

	atc2603c_ictype = ATC260X_ICTYPE_2603C;
	pdev->dev.init_name = "atc260x-audio";
	
	//we use VMICEXT to detect earphone
	if((earphone_gpio_num<0)&&(adc_detect_mode == 0))
	{
		earphone_irq = platform_get_irq(pdev, 0); 
		snd_err("what's my lucky draw %d\n", earphone_irq);
	}	
	
	return snd_soc_register_codec(&pdev->dev, &soc_codec_atc2603c,
			codec_atc2603c_dai, ARRAY_SIZE(codec_atc2603c_dai));
}

static int atc2603c_platform_remove(struct platform_device *pdev)
{
	int i = 0;
	struct device *dev;

	dev = bus_find_device_by_name(&platform_bus_type, NULL, "atc260x-audio");
	if (dev) {
		for (i = 0; i < ARRAY_SIZE(atc2603c_attr); i++) {
			//snd_err("remove file!\r\n");
			device_remove_file(dev, &atc2603c_attr[i]);
		}
	} else {
		snd_err("Find platform device atc260x-audio failed!\r\n");
		return -ENODEV;
	}

	snd_soc_unregister_codec(&pdev->dev);
		
	return 0;
}

static void atc2603c_platform_shutdown(struct platform_device *pdev)
{	
	gpio_direction_output(speaker_gpio_num, !speaker_gpio_active);
/*
	snd_soc_write(atc2603c_codec, DAC_VOLUMECTL0, 0xBEBE);
	snd_soc_write(atc2603c_codec, DAC_ANALOG1, 0x0);
	snd_soc_write(atc2603c_codec, DAC_ANALOG3, 0x8B0B);
	snd_soc_write(atc2603c_codec, DAC_ANALOG4, 0x08C0);
	snd_soc_write(atc2603c_codec, DAC_ANALOG2, 0x0100);
	snd_soc_update_bits(atc2603c_codec,
			AUDIOINOUT_CTL,
			0x01 << 1, 0);
*/
	atc2603c_power_down(atc2603c_codec);
	return;
}

static const struct of_device_id atc2603c_audio_of_match[]= {
	{.compatible = "actions,atc2603c-audio",},
	{}
};
MODULE_DEVICE_TABLE(of, atc2603c_audio_of_match);

static struct platform_driver atc2603c_platform_driver = {
	.probe      = atc2603c_platform_probe,
	.remove     = atc2603c_platform_remove,
	.driver     = {
		.name   = "atc2603c-audio",
		.owner  = THIS_MODULE,
		.of_match_table = atc2603c_audio_of_match,
	},
	.shutdown	= atc2603c_platform_shutdown,
};

static int atc2603c_get_cfg(void)
{
	u32 ret = 1;
	struct device_node *dn;

	dn = of_find_compatible_node(NULL, NULL, audio_device_node);
	if (!dn) {
		snd_err("Fail to get device_node\r\n");
		goto of_get_failed;
	}

	ret = of_property_read_u32(dn, snd_earphone_output_mode,
		&audio_hw_cfg.earphone_output_mode);
	if (ret) {
		snd_err("Fail to get snd_earphone_output_mode\r\n");
		goto of_get_failed;
	}

	ret = of_property_read_u32(dn, snd_mic_num,
		&audio_hw_cfg.mic_num);
	if (ret) {
		snd_err("Fail to get snd_mic_num\r\n");
		goto of_get_failed;
	}

	ret = of_property_read_u32_array(dn, snd_mic0_gain,
		audio_hw_cfg.mic0_gain, 2);
	if (ret) {
		snd_err("Fail to get snd_mic_gain\r\n");
		goto of_get_failed;
	}

	ret = of_property_read_u32_array(dn, snd_speaker_gain,
		audio_hw_cfg.speaker_gain, 2);
	if (ret) {
		snd_err("Fail to get snd_speaker_gain\r\n");
		goto of_get_failed;
	}

	ret = of_property_read_u32_array(dn, snd_earphone_gain,
		audio_hw_cfg.earphone_gain, 2);
	if (ret) {
		snd_err("Fail to get snd_earphone_gain\r\n");
		goto of_get_failed;
	}

/*
	ret = of_property_read_u32(dn, snd_speaker_volume,
		&audio_hw_cfg.speaker_volume);
	if (ret) {
		snd_err("Fail to get snd_speaker_volume\r\n");
		goto of_get_failed;
	}

	ret = of_property_read_u32(dn, snd_earphone_volume,
		&audio_hw_cfg.earphone_volume);
	if (ret) {
		snd_err("Fail to get snd_earphone_volume\r\n");
		goto of_get_failed;
	}

	ret = of_property_read_u32(dn, snd_earphone_detect_mode,
		&audio_hw_cfg.earphone_detect_mode);
	if (ret) {
		snd_err("Fail to get snd_earphone_detect_mode\r\n");
		goto of_get_failed;
	}
*/
	audio_hw_cfg.speaker_volume = 0x28;
	audio_hw_cfg.earphone_volume = 0x28;
	audio_hw_cfg.earphone_detect_mode = 0;

	//20141202 change_code by yuchen : dont force fail on this one, there's a default value.
	//default to differential
	ret = of_property_read_u32(dn, snd_mic_mode,
		&audio_hw_cfg.mic_mode);
	if (ret) {
		printk("fail get snd_mic_mode\r\n");
		audio_hw_cfg.mic_mode = 1;
		//goto of_get_failed;
	}

	//20150104 change_code by yuchen : add earphone detect method config
	//0 for gpio, 1 for irq, 2 for adc
	ret = of_property_read_u32(dn, snd_earphone_detect_method,
		&audio_hw_cfg.earphone_detect_method);
	if (ret) {
		printk("fail get earphone_detect_method\r\n");
		audio_hw_cfg.earphone_detect_method = 0;
		//goto of_get_failed;
	}
	
	if(audio_hw_cfg.earphone_detect_method == 2)
	{
		ret = of_property_read_u32(dn, snd_adc_plugin_threshold,
			&audio_hw_cfg.adc_plugin_threshold);
		if (ret) {
			printk("fail get snd_adc_plugin_threshold\r\n");
			audio_hw_cfg.adc_plugin_threshold = 900;
			//goto of_get_failed;
		}		
	}			
	
	//fixme
	ret = of_property_read_u32(dn, snd_adc_level,
		&audio_hw_cfg.adc_level);
	if (ret) {
		printk("fail get adc level\r\n");
		audio_hw_cfg.adc_level = 1;
		//goto of_get_failed;
	}		

	speaker_gpio_num = of_get_named_gpio_flags(dn, speaker_ctrl_name, 0, &speaker_gpio_level);
	if (speaker_gpio_num < 0) {
		snd_err("get gpio[%s] fail\r\n", speaker_ctrl_name);
	}
	speaker_gpio_active = (speaker_gpio_level & OF_GPIO_ACTIVE_LOW);
			
	earphone_gpio_num = of_get_named_gpio_flags(dn, earphone_detect_gpio, 0, NULL);
	if (earphone_gpio_num < 0) {
		snd_dbg("no earphone detect gpio\r\n");
	}
	

	return 0;
of_get_failed:
	return ret;
}

static void atc2603c_dump_cfg(void)
{
#if 0
	printk(KERN_ERR"earphone_detect_mode = %d\r\n",audio_hw_cfg.earphone_detect_mode);
	printk(KERN_ERR"earphone_gain[0] = %d\r\n",audio_hw_cfg.earphone_gain[0]);
	printk(KERN_ERR"earphone_gain[1] = %d\r\n",audio_hw_cfg.earphone_gain[1]);
	printk(KERN_ERR"speaker_gain[0] = %d\r\n",audio_hw_cfg.speaker_gain[0]);
	printk(KERN_ERR"speaker_gain[1] = %d\r\n",audio_hw_cfg.speaker_gain[1]);
	printk(KERN_ERR"mic0_gain[0] = %d\r\n",audio_hw_cfg.mic0_gain[0]);
	printk(KERN_ERR"mic0_gain[1] = %d\r\n",audio_hw_cfg.mic0_gain[1]);
	printk(KERN_ERR"earphone_volume = %d\r\n",audio_hw_cfg.earphone_volume);
	printk(KERN_ERR"speaker_volume = %d\r\n",audio_hw_cfg.speaker_volume);
	printk(KERN_ERR"earphone_output_mode = %d\r\n",audio_hw_cfg.earphone_output_mode);
	printk(KERN_ERR"mic_num = %d\r\n",audio_hw_cfg.mic_num);
#endif
}

static int atc2603c_pa_down_notifier(struct notifier_block *notifier,
				       unsigned long pm_event, void *v)
{
	mutex_lock(&atc2603c_pa_down_lock);

	switch (pm_event) {

		case PM_SUSPEND_PREPARE:
			user_lock = 1;
			snd_soc_update_bits(atc2603c_codec,
				AUDIOINOUT_CTL,
				0x01 << 1, 0x01 << 1);
			atc2603c_power_down(atc2603c_codec);
			snd_soc_update_bits(atc2603c_codec,
				AUDIOINOUT_CTL,
				0x01 << 1, 0);
			break;

		case PM_POST_SUSPEND:
			user_lock = 0;
			break;
	}
	mutex_unlock(&atc2603c_pa_down_lock);

	return NOTIFY_OK;
}

static struct notifier_block atc2603c_pa_down_nb = {
	.notifier_call = atc2603c_pa_down_notifier,
};

static int dbgflag;


static ssize_t dbgflag_show_file (struct device *dev, struct device_attribute *attr,
        char *buf)
{
    return sprintf(buf, "%d\n", dbgflag);
}

static ssize_t dbgflag_store_file(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t count)
{
    char *endp;	
    size_t size;
    int flag = 0;

    if (count > 0) {

        flag = simple_strtoul(buf,&endp,0);
        size = endp - buf;

        if (*endp && (((*endp)==0x20)||((*endp)=='\n')||((*endp)=='\t')))	
            size++;	
        if (size != count)	
        {
            snd_err("%s %d %s, %d", __FUNCTION__, __LINE__, buf, flag);		
            return -EINVAL;
        }

    }

    snd_err("%s %d", __FUNCTION__, __LINE__);
    dbgflag = flag;
}

static DEVICE_ATTR(dbgflag, 0744, dbgflag_show_file, dbgflag_store_file);


static int __init atc2603c_init(void)
{
	u32 ret = 0;

	printk("atc2603c_init\n");

	ret = atc2603c_get_cfg();
	if (ret){
		snd_err("audio get cfg failed!\r\n");
		goto audio_get_cfg_failed;
	}

	atc2603c_dump_cfg();

	direct_drive_disable = audio_hw_cfg.earphone_output_mode;
	//snd_err("direct_drive_disable %d\n", direct_drive_disable);

	ret = platform_driver_register(&atc2603c_platform_driver);
	if(ret){
		snd_err("platform_driver_register failed!\r\n");
		goto platform_driver_register_failed;
	}

	//dev = bus_find_device_by_name(&platform_bus_type, NULL, "atc260x-audio");


	register_pm_notifier(&atc2603c_pa_down_nb);

	//20150103 change_code by yuchen: set adc_detect_mode by earphone detect method
	if(audio_hw_cfg.earphone_detect_method == 2)
	{
		adc_detect_mode = 1;
	}	
	else
	{
		adc_detect_mode = 0;
	}
	
	if((audio_hw_cfg.earphone_detect_method == 1) || (audio_hw_cfg.earphone_detect_method == 2))
	{
		headphone_sdev.name = "h2w";
		ret = switch_dev_register(&headphone_sdev);
		if (ret < 0) {
			snd_err("failed to register switch dev for h2w\n");
		}	
		
		dbgflag = 0;
    		ret = device_create_file(headphone_sdev.dev, &dev_attr_dbgflag);
    		if (ret)
    		{
    			snd_err("failed to device_create_file\n");
        	}
		
			
	}

	return 0;
platform_driver_register_failed:
audio_get_cfg_failed:
	return ret;
}

static void __exit atc2603c_exit(void)
{
	platform_driver_unregister(&atc2603c_platform_driver);
	switch_dev_unregister(&headphone_sdev);
}

module_init(atc2603c_init);
module_exit(atc2603c_exit);


MODULE_AUTHOR("sall.xie <sall.xie@actions-semi.com>");
MODULE_DESCRIPTION("ATC2603C AUDIO module");
MODULE_LICENSE("GPL");
