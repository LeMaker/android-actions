#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/switch.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/time.h>
#include <linux/delay.h>
#include "sndrv-owl.h"

#include <linux/mfd/atc260x/atc260x.h>

extern int atc2603a_audio_get_pmu_status(void);
extern int atc2603c_audio_get_pmu_status(void);



#define SOUND_MAJOR		14
#define SNDRV_MAJOR		SOUND_MAJOR
#define SNDRV_NAME		"sound"

const char *earphone_ctrl_link_name = "earphone_detect_gpios";
const char *speaker_ctrl_link_name = "speaker_en_gpios";
const char *audio_atc2603a_link_node = "actions,atc2603a-audio";
const char *audio_atc2603c_link_node = "actions,atc2603c-audio";

static int earphone_gpio_num = -1;
enum of_gpio_flags earphone_gpio_level;
static int speaker_gpio_num;
static enum of_gpio_flags speaker_gpio_level;
static int speaker_gpio_active;
static int flag = 0;
static bool speaker_exist=true;

typedef struct {
	struct switch_dev sdev;
	struct delayed_work dwork;
	struct workqueue_struct *wq;
} headphone_dev_t;

static headphone_dev_t headphone_sdev;

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

static struct device_attribute link_attr[] = {
	__ATTR(error, S_IRUSR | S_IWUSR, error_show, error_store),
	__ATTR(debug, S_IRUSR | S_IWUSR, debug_show, debug_store),
};

static void set_pa_onoff(int status)
{
	int ret;
	if(speaker_exist){
		if(status){
			ret = gpio_direction_output(speaker_gpio_num, speaker_gpio_active);
		}else{
			ret = gpio_direction_output(speaker_gpio_num, !speaker_gpio_active);		
		}	
	
	}

}

static int speaker_gpio_get(struct snd_kcontrol * kcontrol,
	struct snd_ctl_elem_value * ucontrol)
{

	int state = 0;
	if(speaker_exist) {
	state = !!(gpio_get_value_cansleep(speaker_gpio_num));
	ucontrol->value.bytes.data[0] = state;	
	}
	return 0;
}
static int speaker_gpio_put(struct snd_kcontrol * kcontrol,
	struct snd_ctl_elem_value * ucontrol)
{

	int state = 0;
	state = ucontrol->value.bytes.data[0];
	set_pa_onoff(state);
	return 0;
}

static const struct snd_kcontrol_new owl_outpa_controls[] = {
	SOC_SINGLE_BOOL_EXT("speaker on off switch", 
			0, speaker_gpio_get, speaker_gpio_put),
};

struct device_node *atm7059_audio_get_device_node(const char *name)
{
	struct device_node *dn;

	dn = of_find_compatible_node(NULL, NULL, name);
	if (!dn) {
		snd_err("Fail to get device_node\r\n");
		goto fail;
	}

	return dn;
fail:
	return NULL;
}

/*earphone gpio worked as a external interrupt */
static int atm7059_audio_gpio_init(struct device_node *dn,
			const char *name,enum of_gpio_flags *flags)
{
	int gpio;
	int ret = 0;

	if (!of_find_property(dn, name, NULL)) {
		snd_err("find %s property fail\n", name);
		goto fail;
	}

	gpio = of_get_named_gpio_flags(dn, name, 0, flags);
	if (gpio < 0) {
		snd_err("get gpio[%s] fail\r\n", name);
		goto fail;
	}

	ret = gpio_request(gpio, name);
	if (ret) {
		snd_err("GPIO[%d] request failed\r\n", gpio);
		goto fail;
	}
	return gpio;

fail:
	return -ENOMEM;
}

static int earphone_is_in(void)
{
	int state = 0;
	if (earphone_gpio_num < 0)
	{
		//use irq to detect earphone
	}
	else
	{
		//use gpio to detect earphone
		state = !!(gpio_get_value_cansleep(earphone_gpio_num));
		state ^= earphone_gpio_level;
	}
	return state;
}

static void gl5203_earphone_monitor(struct work_struct *work)
{
	if (earphone_is_in())
		switch_set_state(&headphone_sdev.sdev, SPEAKER_ON);
	else
		switch_set_state(&headphone_sdev.sdev, HEADSET_NO_MIC);

	if(flag == 0)
	queue_delayed_work(headphone_sdev.wq,
		&headphone_sdev.dwork, msecs_to_jiffies(200));
}

static int atm7059_set_gpio_ear_detect(void)
{
	headphone_sdev.wq = create_singlethread_workqueue("earphone_detect_wq");
    if ( !headphone_sdev.wq ) {
        snd_err("Create workqueue failed");
        goto create_workqueue_failed;
    }

	INIT_DELAYED_WORK(&headphone_sdev.dwork, gl5203_earphone_monitor);
	queue_delayed_work(headphone_sdev.wq,
		&headphone_sdev.dwork, msecs_to_jiffies(200));

	return 0;
create_workqueue_failed:
	return -ENODEV;
}

static int atm7059_link_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret;
	snd_dbg("###atm7059_link_hw_params\n");

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
		SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_ops atm7059_link_ops = {
	.hw_params = atm7059_link_hw_params,
};

/*
 * Logic for a link as connected on a atm7059 board.
 */
static int atm7059_link_snd_init(struct snd_soc_pcm_runtime *rtd)
{
	snd_dbg("atm7059_link_init() called\n");

	return 0;
}

static struct snd_soc_dai_link atm7059_atc2603a_link_dai[] = {
	{
		.name = "ATM7059 ATC2603A",
		.stream_name = "ATC2603A PCM",
		.cpu_dai_name = "owl-audio-i2s",
		.codec_dai_name = "atc2603a-dai",
		.init = atm7059_link_snd_init,
		.platform_name = "atm7059-pcm-audio",
		.codec_name = "atc260x-audio",
		.ops = &atm7059_link_ops,
	},

	{
		.name = "ATM7059 HDMI AUDIO",
		.stream_name = "HDMI PCM",
		.cpu_dai_name = "owl-audio-i2s",
		.codec_dai_name = "atm7059-hdmi-dai",
		.init = atm7059_link_snd_init,
		.platform_name = "atm7059-pcm-audio",
		.codec_name = "atm7059-hdmi-audio",
		.ops = &atm7059_link_ops,
	}
};

static struct snd_soc_dai_link atm7059_atc2603c_link_dai[] = {
	{
		.name = "ATM7059 ATC2603C",
		.stream_name = "ATC2603C PCM",
		.cpu_dai_name = "owl-audio-i2s",
		.codec_dai_name = "atc2603c-dai",
		.init = atm7059_link_snd_init,
		.platform_name = "atm7059-pcm-audio",
		.codec_name = "atc260x-audio",
		.ops = &atm7059_link_ops,
	},

	{
		.name = "ATM7059 HDMI AUDIO",
		.stream_name = "HDMI PCM",
		.cpu_dai_name = "owl-audio-i2s",
		.codec_dai_name = "atm7059-hdmi-dai",
		.init = atm7059_link_snd_init,
		.platform_name = "atm7059-pcm-audio",
		.codec_name = "atm7059-hdmi-audio",
		.ops = &atm7059_link_ops,
	}
};


static struct snd_soc_card snd_soc_atm7059_atc2603a_link = {
	.name = "atm7059_link",
	.owner = THIS_MODULE,
	.dai_link = atm7059_atc2603a_link_dai,
	.num_links = ARRAY_SIZE(atm7059_atc2603a_link_dai),
	.controls = owl_outpa_controls,
	.num_controls = ARRAY_SIZE(owl_outpa_controls),
};

static struct snd_soc_card snd_soc_atm7059_atc2603c_link = {
	.name = "atm7059_link",
	.owner = THIS_MODULE,
	.dai_link = atm7059_atc2603c_link_dai,
	.num_links = ARRAY_SIZE(atm7059_atc2603c_link_dai),
	.controls = owl_outpa_controls,
	.num_controls = ARRAY_SIZE(owl_outpa_controls),
};


static struct platform_device *atm7059_link_snd_device;
/*
static int compare_audio_device_name(struct device *dev, void *data)
{
	const char *name = (const char *)data;
	char *device_name = dev_name(dev);
	char *match_start = strrchr(device_name, '.');
	
	snd_err("device_name %s, match_start %x\n", device_name, match_start);
	if(match_start==NULL)
	{ 
		if(strcmp(name, device_name)==0)
		{
			//got a match
			return 1;
		}	
		else
		{
			return 0;
		}
	}
	
	if((int)(match_start-device_name+1) >= (int)strlen(device_name))
	{
		//should not happen but in case
		return 0;
	}
	
	snd_err("name %s to match %s\n", name, match_start+1);
	if(strcmp(name, match_start+1)==0)
	{
		//got a match
		return 1;
	}
	
	return 0;
}
*/

static int dbgflag;


static ssize_t dbgflag_show_file (struct device *dev, struct device_attribute *attr,
        char *buf)
{
    return sprintf(buf, "%d", dbgflag);
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

    dbgflag = flag;
}

static DEVICE_ATTR(dbgflag, 0777, dbgflag_show_file, dbgflag_store_file);

//david add for i2s switch.
static int i2s_switch_gpio_num = -1;
enum of_gpio_flags i2s_switch_gpio_level;
const char *i2s_switch_gpio_name = "i2s_switch_gpio";
static int i2s_switch_gpio_active;



static int __init atm7059_link_init(void)
{
	int i = 0;
	int ret = 0;
	struct device_node *dn = NULL;
	int pmu_type;

	snd_err("atm7059_link_init\n");


	//20141013 yuchen: check pmu type to select correct codec param
	pmu_type = atc2603a_audio_get_pmu_status();
	if(pmu_type == ATC260X_ICTYPE_2603A)
	{
		dn = atm7059_audio_get_device_node(audio_atc2603a_link_node);
		if (!dn)
			goto no_device_node;
	}
	
	if(pmu_type == PMU_NOT_USED)
	{
		pmu_type = atc2603c_audio_get_pmu_status();
		if(pmu_type == ATC260X_ICTYPE_2603C)
		{
			dn = atm7059_audio_get_device_node(audio_atc2603c_link_node);
			if (!dn)
				goto no_device_node;			
		}
	}

	if(pmu_type == PMU_NOT_USED)
	{
		snd_err("ASoC: No PMU type detected!\n");
		goto no_device_node;
	}

    /*****************20151012 david add***************/ 
    printk(KERN_ERR"%s,%d\n", __func__, __LINE__);
    i2s_switch_gpio_num =
        atm7059_audio_gpio_init(dn, i2s_switch_gpio_name, &i2s_switch_gpio_level);
    if(i2s_switch_gpio_num > 0){
        printk(KERN_ERR"%s,%d,num:%d\n", __func__, __LINE__, i2s_switch_gpio_num);
        i2s_switch_gpio_active = (i2s_switch_gpio_level & OF_GPIO_ACTIVE_LOW); 
	    gpio_direction_output(i2s_switch_gpio_num, i2s_switch_gpio_active);
    }
    /*************************************************/


	earphone_gpio_num =
		atm7059_audio_gpio_init(dn, earphone_ctrl_link_name, &earphone_gpio_level);
	if (earphone_gpio_num < 0)
	{
		//assume it use irq to detect earphone
		//goto request_earphone_gpio_num_failed;
	}
	else
	{
		gpio_direction_input(earphone_gpio_num);
	}


	speaker_gpio_num =
		atm7059_audio_gpio_init(dn, speaker_ctrl_link_name, &speaker_gpio_level);
    speaker_gpio_active = (speaker_gpio_level & OF_GPIO_ACTIVE_LOW); 		
/*	
	if (speaker_gpio_num < 0)
		goto request_speaker_gpio_num_failed;
*/
	if(speaker_gpio_num>0)
		speaker_exist=true;
  else
		speaker_exist=false;
	if(speaker_exist)  
	gpio_direction_output(speaker_gpio_num, !speaker_gpio_active);

	if (earphone_gpio_num > 0)
	{
		ret = atm7059_set_gpio_ear_detect();
		if(ret)
			goto set_earphone_detect_failed;
	}
	else
	{
		switch(pmu_type)
		{
		case ATC260X_ICTYPE_2603A:
			snd_err("CANT GET EARPHONE GPIO!!!\n");
			break;
		
		case ATC260X_ICTYPE_2603C:
			//atc2603c_set_irq_ear_detect();
			snd_err("maybe using irq");
			break;
		default:
			break;
		}
		
	}

	if (earphone_gpio_num > 0)
	{
		//FIXME: we register h2w in codec if using irq?
		headphone_sdev.sdev.name = "h2w";
		ret = switch_dev_register(&headphone_sdev.sdev);
		if (ret < 0) {
			snd_err("failed to register switch dev for "SNDRV_NAME"\n");
			goto switch_dev_register_failed;
		}
		dbgflag = 0;
    		ret = device_create_file(headphone_sdev.sdev.dev, &dev_attr_dbgflag);
    		if (ret)
    		{
    			snd_err("failed to device_create_file\n");
        	}
		
	}

	atm7059_link_snd_device = platform_device_alloc("soc-audio", -1);
	if (!atm7059_link_snd_device) {
		snd_err("ASoC: Platform device allocation failed\n");
		ret = -ENOMEM;
		goto platform_device_alloc_failed;
	}

	/* 2014.09.10 check the real device names*/

	//20141013 yuchen: check pmu type to select correct codec param
	switch(pmu_type)
	{
	case ATC260X_ICTYPE_2603A:
		platform_set_drvdata(atm7059_link_snd_device,
				&snd_soc_atm7059_atc2603a_link);
		break;
	
	case ATC260X_ICTYPE_2603C:
		platform_set_drvdata(atm7059_link_snd_device,
				&snd_soc_atm7059_atc2603c_link);
		break;
	default:
		break;
	}
	
	
//	platform_set_drvdata(atm7059_link_snd_device,
//			&snd_soc_atm7059_link);


	ret = platform_device_add(atm7059_link_snd_device);
	if (ret) {
		snd_err("ASoC: Platform device allocation failed\n");
		goto platform_device_add_failed;
	}

	for (i = 0; i < ARRAY_SIZE(link_attr); i++) {
		ret = device_create_file(
			&atm7059_link_snd_device->dev, &link_attr[i]);
		if (ret) {
			snd_err("Add device file failed");
			goto device_create_file_failed;
		}
	}
	

	return 0;

device_create_file_failed:
	platform_device_del(atm7059_link_snd_device);
platform_device_add_failed:
	platform_device_put(atm7059_link_snd_device);
platform_device_alloc_failed:
	if (earphone_gpio_num > 0)
	{
		switch_dev_unregister(&headphone_sdev.sdev);
	}
switch_dev_register_failed:
	if (earphone_gpio_num > 0)
	{
		destroy_workqueue(headphone_sdev.wq);
	}
set_earphone_detect_failed:
	if(speaker_exist)
	gpio_free(speaker_gpio_num);
request_speaker_gpio_num_failed:
	if (earphone_gpio_num > 0)
	{
		gpio_free(earphone_gpio_num);
	}
no_device_node:
	return ret;
}

static void __exit atm7059_link_exit(void)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(link_attr); i++) {
		device_remove_file(&atm7059_link_snd_device->dev, &link_attr[i]);
	}
	if (headphone_sdev.wq) {
		flag = 1;
		msleep(500);/*clear the queue,ensure no cpu write to it*/
		flush_delayed_work(&headphone_sdev.dwork);
		destroy_workqueue(headphone_sdev.wq);
		headphone_sdev.wq = NULL;
	}

	if(earphone_gpio_num > 0)
	{
		gpio_free(earphone_gpio_num);
		earphone_gpio_num = -1;
		switch_dev_unregister(&headphone_sdev.sdev);
	}
	if(speaker_exist){
		gpio_free(speaker_gpio_num);
	}	
	speaker_gpio_num = -1;

    /*---------------david add-----------------------*/
    if(i2s_switch_gpio_num > 0)
    {
		gpio_free(i2s_switch_gpio_num);
		i2s_switch_gpio_num = -1;
    }
    /*----------------------------------------------*/

	platform_device_unregister(atm7059_link_snd_device);
}

module_init(atm7059_link_init);
module_exit(atm7059_link_exit);

/* Module information */
MODULE_AUTHOR("sall.xie <sall.xie@actions-semi.com>");
MODULE_LICENSE("GPL");
