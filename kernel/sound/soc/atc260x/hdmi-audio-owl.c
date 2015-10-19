#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h> 
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include <linux/clk.h>			/* clk_enable */
#include "sndrv-owl.h"
#include <mach/module-owl.h>
#include "common-regs-owl.h"

#include <linux/io.h> 
#include <linux/ioport.h>

#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>

#if defined(CONFIG_HAS_EARLYSUSPEND)&&defined(CONFIG_SND_UBUNTU)
#include <linux/earlysuspend.h>
	struct early_suspend early_suspend;
    static int is_suspended = 0;
#endif

static int audio_clk_enable;

#define Audio60958	1	/*1--IEC60958; 0--IEC61937*/
#define HDMI_RAMPKT_AUDIO_SLOT	1
#define HDMI_RAMPKT_PERIOD	1

static int Speaker;			//Speaker Placement, stereo

/* for register io remap
struct asoc_hdmi_resource {
    void __iomem    *base[MAX_RES_NUM];//virtual base for every resource
    void __iomem    *baseptr; //pointer to every virtual base
    struct clk      *clk;
    int             irq;
    unsigned int    setting;
};

static struct asoc_hdmi_resource hdmi_res;

static void set_hdmi_reg_base(int num)
{
	hdmi_res.baseptr = hdmi_res.base[num];
}

static u32 snd_hdmi_readl(u32 reg)
{
	return readl(hdmi_res.baseptr + reg);
}
	 
static void snd_hdmi_writel(u32 val, u32 reg)
{
	writel(val, hdmi_res.baseptr + reg);
}
*/
#if defined(CONFIG_HAS_EARLYSUSPEND)&&defined(CONFIG_SND_UBUNTU)
static void hdmi_audio_early_suspend(struct early_suspend *h)
{
    is_suspended = 1;
}

static void hdmi_audio_late_resume(struct early_suspend *h)
{
    is_suspended = 0;

}
#endif

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

static struct device_attribute hdmi_attr[] = {
	__ATTR(error, S_IRUSR | S_IWUSR, error_show, error_store),
	__ATTR(debug, S_IRUSR | S_IWUSR, debug_show, debug_store),
};

static int hdmi_EnableWriteRamPacket(void)
{
	int i;
	//set_hdmi_reg_base(0);
	hdmihw_write_reg(hdmihw_read_reg((u16)HDMI_OPCR) | (0x1 << 31), (u16)HDMI_OPCR);
	while ((hdmihw_read_reg((u16)HDMI_OPCR) & (0x1 << 31)) != 0) {
		for (i = 0; i < 10; i++)
			;
	}
	return 0;
}

static int hdmi_SetRamPacket(unsigned no, unsigned char *pkt)
{
	unsigned char tpkt[36];
	unsigned int *reg = (unsigned int *) tpkt;
	unsigned int addr = 126 + no * 14;

	if (no > 5)
		return -EINVAL;

	/**
	 * according to change by genganan 2008-09-24
	 */
	/* Packet Header */
	tpkt[0] = pkt[0];
	tpkt[1] = pkt[1];
	tpkt[2] = pkt[2];
	tpkt[3] = 0;
	/* Packet Word0 */
	tpkt[4] = pkt[3];
	tpkt[5] = pkt[4];
	tpkt[6] = pkt[5];
	tpkt[7] = pkt[6];
	/* Packet Word1 */
	tpkt[8] = pkt[7];
	tpkt[9] = pkt[8];
	tpkt[10] = pkt[9];
	tpkt[11] = 0;
	/* Packet Word2 */
	tpkt[12] = pkt[10];
	tpkt[13] = pkt[11];
	tpkt[14] = pkt[12];
	tpkt[15] = pkt[13];
	/* Packet Word3 */
	tpkt[16] = pkt[14];
	tpkt[17] = pkt[15];
	tpkt[18] = pkt[16];
	tpkt[19] = 0;
	/* Packet Word4 */
	tpkt[20] = pkt[17];
	tpkt[21] = pkt[18];
	tpkt[22] = pkt[19];
	tpkt[23] = pkt[20];
	/* Packet Word5 */
	tpkt[24] = pkt[21];
	tpkt[25] = pkt[22];
	tpkt[26] = pkt[23];
	tpkt[27] = 0;
	/* Packet Word6 */
	tpkt[28] = pkt[24];
	tpkt[29] = pkt[25];
	tpkt[30] = pkt[26];
	tpkt[31] = pkt[27];
	/* Packet Word7 */
	tpkt[32] = pkt[28];
	tpkt[33] = pkt[29];
	tpkt[34] = pkt[30];
	tpkt[35] = 0;

	/* write mode */
	//set_hdmi_reg_base(0);
	hdmihw_write_reg((1 << 8) | (((addr) & 0xFF) << 0), (u16)HDMI_OPCR);
	hdmihw_write_reg(reg[0], (u16)HDMI_ORP6PH);
	hdmihw_write_reg(reg[1], (u16)HDMI_ORSP6W0);
	hdmihw_write_reg(reg[2], (u16)HDMI_ORSP6W1);
	hdmihw_write_reg(reg[3], (u16)HDMI_ORSP6W2);
	hdmihw_write_reg(reg[4], (u16)HDMI_ORSP6W3);
	hdmihw_write_reg(reg[5], (u16)HDMI_ORSP6W4);
	hdmihw_write_reg(reg[6], (u16)HDMI_ORSP6W5);
	hdmihw_write_reg(reg[7], (u16)HDMI_ORSP6W6);
	hdmihw_write_reg(reg[8], (u16)HDMI_ORSP6W7);

	hdmi_EnableWriteRamPacket();

	return 0;
}

static int hdmi_SetRamPacketPeriod(unsigned int no, int period)
{
	if (no > 5)
		return -EINVAL;

	if ((period > 0xf) || (period < 0))
		return -EINVAL;

	/* disable */
	//set_hdmi_reg_base(0);
	hdmihw_write_reg(hdmihw_read_reg((u16)HDMI_RPCR) & (unsigned int) (~(1 << no)),
	(u16)HDMI_RPCR);
	hdmihw_write_reg(hdmihw_read_reg((u16)HDMI_RPCR) &
	(unsigned int) (~(0xf << (no * 4 + 8))),
			(u16)HDMI_RPCR);

	if (period != 0) {
		/* enable and set period */
		hdmihw_write_reg(hdmihw_read_reg((u16)HDMI_RPCR) |
		(unsigned int) (period << (no * 4 + 8)),
				(u16)HDMI_RPCR);
		hdmihw_write_reg(hdmihw_read_reg((u16)HDMI_RPCR) | (unsigned int) (1 << no),
		(u16)HDMI_RPCR);
	}
	return 0;
}

static int hdmi_gen_audio_infoframe(int audio_channel)
{
		unsigned char pkt[32];
		unsigned int  checksum = 0;
		int i;
		memset(pkt, 0, 32);
		/* header */
		pkt[0] = 0x80 | 0x04;
		pkt[1] = 1;
		pkt[2] = 0x1f & 10;
		pkt[3] = 0x00;
		pkt[4] = audio_channel & 0x7;
		pkt[5] = 0x0;
		pkt[6] = 0x0;
		pkt[7] = Speaker;
		pkt[8] = (0x0 << 7) | (0x0 << 3);

		/* count checksum */
		for (i = 0; i < 31; i++)
			checksum += pkt[i];

		pkt[3] = (unsigned char)((~checksum + 1) & 0xff);
    /* set to RAM Packet */
		hdmi_SetRamPacket(HDMI_RAMPKT_AUDIO_SLOT, pkt);
		hdmi_SetRamPacketPeriod(HDMI_RAMPKT_AUDIO_SLOT,
		HDMI_RAMPKT_PERIOD);
		return 0;
}
void set_hdmi_audio_interface(int channel, int samplerate)
{
	unsigned int tmp03;
	unsigned int tmp47;
	unsigned int CRP_N = 0;
	unsigned int ASPCR = 0;
	unsigned int ACACR = 0;

	//改变音频相关参数时需要首先disable audio, 配置完成后enable
	//snd_err("HDMI_ICR 0x%x", hdmihw_read_reg((u16)HDMI_ICR));
	hdmihw_write_reg(hdmihw_read_reg((u16)HDMI_ICR) & ~(0x1 << 25), (u16)HDMI_ICR);
	hdmihw_write_reg(hdmihw_read_reg((u16)HDMI_ACRPCR) | (0x1 << 31), (u16)HDMI_ACRPCR);
	hdmihw_read_reg((u16)HDMI_ACRPCR); /*flush write buffer effect*/
	
	//set_hdmi_reg_base(0);
	tmp03 = hdmihw_read_reg((u16)HDMI_AICHSTABYTE0TO3);
	tmp03 &= (~(0xf << 24));

	tmp47 = hdmihw_read_reg((u16)HDMI_AICHSTABYTE4TO7);
	tmp47 &= (~(0xf << 4));
	tmp47 |= 0xb;

	switch (samplerate) {
	/* 32000, 44100, 48000, 88200, 96000,
	176400, 192000, 352.8kHz, 384kHz */
	case 1:
		tmp03 |= (0x3 << 24);
		tmp47 |= (0xc << 4);
		CRP_N = 4096;
		break;

	case 2:
		tmp03 |= (0x0 << 24);
		tmp47 |= (0xf << 4);
		CRP_N = 6272;
		break;

	case 3:
		tmp03 |= (0x2 << 24);
		tmp47 |= (0xd << 4);
		CRP_N = 6144;
		break;

	case 4:
		tmp03 |= (0x8 << 24);
		tmp47 |= (0x7 << 4);
		CRP_N = 12544;
		break;

	case 5:
		tmp03 |= (0xa << 24);
		tmp47 |= (0x5 << 4);
		CRP_N = 12288;
		break;

	case 6:
		tmp03 |= (0xc << 24);
		tmp47 |= (0x3 << 4);
		CRP_N = 12288;
		break;

	case 7:
		tmp03 |= (0xe << 24);
		tmp47 |= (0x1 << 4);
		CRP_N = 24576;
		break;

	case 8:
		tmp03 |= (0x1 << 24);
		CRP_N = 12544;
		break;

	case 9:
		tmp03 |= (0x1 << 24);
		CRP_N = 12288;
		break;

	default:
		break;
	}
	hdmihw_write_reg(tmp03, (u16)HDMI_AICHSTABYTE0TO3);
	hdmihw_write_reg(tmp47, (u16)HDMI_AICHSTABYTE4TO7);

	hdmihw_write_reg(0x0, (u16)HDMI_AICHSTABYTE8TO11);
	hdmihw_write_reg(0x0, (u16)HDMI_AICHSTABYTE12TO15);
	hdmihw_write_reg(0x0, (u16)HDMI_AICHSTABYTE16TO19);
	hdmihw_write_reg(0x0, (u16)HDMI_AICHSTABYTE20TO23);

	switch (channel) {
	case 2:
		hdmihw_write_reg(0x20001, (u16)HDMI_AICHSTASCN);
		break;

	case 3:
		hdmihw_write_reg(0x121, (u16)HDMI_AICHSTASCN);
		break;

	case 4:
		hdmihw_write_reg(0x2121, (u16)HDMI_AICHSTASCN);
		break;

	case 5:
		hdmihw_write_reg(0x12121, (u16)HDMI_AICHSTASCN);
		break;

	case 6:
		hdmihw_write_reg(0x212121, (u16)HDMI_AICHSTASCN);
		break;

	case 7:
		hdmihw_write_reg(0x1212121, (u16)HDMI_AICHSTASCN);
		break;

	case 8:
		hdmihw_write_reg(0x21212121, (u16)HDMI_AICHSTASCN);
		break;

	default:
		break;
	}
	/* TODO samplesize 16bit, 20bit */
	/* 24 bit */
	hdmihw_write_reg((hdmihw_read_reg((u16)HDMI_AICHSTABYTE4TO7) & ~0xf),
	(u16)HDMI_AICHSTABYTE4TO7);
	hdmihw_write_reg(hdmihw_read_reg((u16)HDMI_AICHSTABYTE4TO7) | 0xb, (u16)HDMI_AICHSTABYTE4TO7);
	if (Audio60958 == 1) {
		switch (channel) {
		case 2:
			ASPCR = 0x00000011;
			ACACR = 0xfac688;
			Speaker = 0x0;
			break;

		case 3:
			ASPCR = 0x0002d713;
			ACACR = 0x4008;
			Speaker = 0x1;
			break;

		case 4:
			ASPCR = 0x0003df1b;
			ACACR = 0x4608;
			Speaker = 0x3;
			break;

		case 5:
			ASPCR = 0x0003df3b;
			ACACR = 0x2c608;
			Speaker = 0x7;
			break;

		case 6:
			ASPCR = 0x0003df3f;
			ACACR = 0x2c688;
			Speaker = 0xb;
			break;

		case 7:
			ASPCR = 0x0007ff7f;
			ACACR = 0x1ac688;
			Speaker = 0xf;
			break;

		case 8:
			ASPCR = 0x0007ffff;
			ACACR = 0xfac688;
			Speaker = 0x13;
			break;

		default:
			break;
		}
	} else {
		ASPCR = 0x7f87c003;
		ACACR = 0xfac688;

		tmp03 = hdmihw_read_reg((u16)HDMI_AICHSTABYTE0TO3);
		tmp03 |= 0x1;
		hdmihw_write_reg(tmp03, (u16)HDMI_AICHSTABYTE0TO3);
		hdmihw_write_reg(0x21, (u16)HDMI_AICHSTASCN);
	}

	/* enable Audio FIFO_FILL  disable wait cycle */
	hdmihw_write_reg(hdmihw_read_reg((u16)HDMI_CR) | 0x50, (u16)HDMI_CR);

	if (samplerate > 7)
		ASPCR |= (0x1 << 31);

	hdmihw_write_reg(ASPCR, (u16)HDMI_ASPCR);
	hdmihw_write_reg(ACACR, (u16)HDMI_ACACR);
    /*非压缩格式23~30位写0
    * 如果针对压缩码流,
    则HDMI_AICHSTABYTE0TO3的bit[1:0]=0x2（5005新加）;
    * 如果针对线性PCM码流，则HDMI_AICHSTABYTE0TO3
    的bit[1:0]=0x0（同227A）;
    */
		hdmihw_write_reg(hdmihw_read_reg((u16)HDMI_AICHSTABYTE0TO3) & ~0x3,
		(u16)HDMI_AICHSTABYTE0TO3);

    /* 如果针对压缩码流，则
    HDMI_ASPCR的bit[30:23]=0xff（5005新加）;
     *  如果针对线性PCM码流,
     则HDMI_ASPCR的bit[30:23]=0x0（同227A）;
     */
		hdmihw_write_reg(hdmihw_read_reg((u16)HDMI_ASPCR) & ~(0xff << 23), (u16)HDMI_ASPCR);

		hdmihw_write_reg(CRP_N | (0x1 << 31), (u16)HDMI_ACRPCR);
		hdmi_gen_audio_infoframe(channel - 1);

    /*****配置完音频相关参数后enable audio*/
    /* enable CRP */
		hdmihw_write_reg(hdmihw_read_reg((u16)HDMI_ACRPCR) & ~(0x1 << 31), (u16)HDMI_ACRPCR);

    /* enable Audio Interface */
		hdmihw_write_reg(hdmihw_read_reg((u16)HDMI_ICR) | (0x1 << 25), (u16)HDMI_ICR);
}

static int get_hdmi_audio_fs(int sample_rate)
{
	int AudioFS;
	int fs = sample_rate / 1000;

	/* for O_MODE_HDMI */
	/* 32000, 44100, 48000, 88200, 96000, 176400,
	192000, 352.8kHz, 384kHz */
	switch (fs) {
	case 32:
		AudioFS = 1;
		break;
	case 44:
		AudioFS = 2;
		break;
	case 48:
		AudioFS = 3;
		break;
	case 88:
		AudioFS = 4;
		break;
	case 96:
		AudioFS = 5;
		break;
	case 176:
		AudioFS = 6;
		break;
	case 192:
		AudioFS = 7;
		break;
	case 352:
		AudioFS = 8;
		break;
	case 384:
		AudioFS = 9;
		break;
	default:
		AudioFS = 2;
		break;
	}
	return AudioFS;
}

static int hdmi_audio_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	int rate = params_rate(params);
	int audio_fs = get_hdmi_audio_fs(rate);

#if defined(CONFIG_HAS_EARLYSUSPEND)&&defined(CONFIG_SND_UBUNTU)
    if(is_suspended == 1){
        printk(KERN_ERR"system has suspended!\n");
        return 0;
    }
#endif
	if (audio_clk_enable == 0) {
		module_clk_enable(MOD_ID_HDMIA);
		audio_clk_enable = 1;
	}
	/* we set the hdmi audio channels stereo now*/
	set_hdmi_audio_interface(2, audio_fs);
	#ifdef CONFIG_SND_UBUNTU
	audio_set_output_mode(substream, O_MODE_HDMI);
	#endif
	return 0;
}
static int hdmi_dai_hw_free(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{

	if (audio_clk_enable == 1) {
		module_clk_disable(MOD_ID_HDMIA);
		audio_clk_enable = 0;
	}

	return 0;
}

static int hdmi_audio_prepare(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	/* nothing should to do here now */
	return 0;
}

static int hdmi_audio_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	/* nothing should to do here now */
	return 0;
}

static int hdmi_audio_set_dai_sysclk(struct snd_soc_dai *dai,
		int clk_id, unsigned int freq, int dir)
{
	/* nothing should to do here now */
	return 0;
}

#define atm7059_HDMI_RATES SNDRV_PCM_RATE_8000_192000
#ifdef CONFIG_SND_UBUNTU
#define atm7059_HDMI_FORMATS (SNDRV_PCM_FMTBIT_S32_LE \
| SNDRV_PCM_FMTBIT_S20_3LE | \
		SNDRV_PCM_FMTBIT_S24_LE)
#else
#define atm7059_HDMI_FORMATS (SNDRV_PCM_FMTBIT_S16_LE \
| SNDRV_PCM_FMTBIT_S20_3LE | \
		SNDRV_PCM_FMTBIT_S24_LE)
#endif

struct snd_soc_dai_ops hdmi_aif_dai_ops = {
	.hw_params = hdmi_audio_hw_params,
	.prepare = hdmi_audio_prepare,
	.set_fmt = hdmi_audio_set_dai_fmt,
	.set_sysclk = hdmi_audio_set_dai_sysclk,
	.hw_free = hdmi_dai_hw_free,
};

struct snd_soc_dai_driver codec_hdmi_dai[] = {
	{
		.name = "atm7059-hdmi-dai",
		.id = ATM7059_AIF_HDMI,
		.playback = {
			.stream_name = "atm7059 hdmi Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = atm7059_HDMI_RATES,
			.formats = atm7059_HDMI_FORMATS,
		},
		.ops = &hdmi_aif_dai_ops,
	},
};



static int codec_hdmi_probe(struct snd_soc_codec *codec)
{
	/* nothing should to do here now */
	snd_dbg("codec_hdmi_probe!\n");
	return 0;
}

static int codec_hdmi_remove(struct snd_soc_codec *codec)
{
	/* nothing should to do here now */
	return 0;
}



static struct snd_soc_codec_driver soc_codec_hdmi = {
	.probe = codec_hdmi_probe,
	.remove = codec_hdmi_remove,
};

static int atm7059_hdmi_probe(struct platform_device *pdev)
{
/*
	struct resource *res;
	int i;
	int ret;

	struct device_node *dn;

	dn = of_find_compatible_node(NULL, NULL, "actions,gl5203-audio-hdmi");
	if (!dn) {
		snd_err("Fail to get device_node actions,atm7039c-hdmi\r\n");
		//goto of_get_failed;
	}
	
	for(i=0; i<1; i++) 
	{
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			snd_err("no memory resource\n");
			return -ENODEV;
		}

		if (!devm_request_mem_region (&pdev->dev, res->start,
					resource_size(res), "gl5203-audio-hdmi")) {
			snd_err("Unable to request register region\n");
			return -EBUSY;
		}

		hdmi_res.base[i] = devm_ioremap(&pdev->dev, res->start, resource_size(res));
		if (hdmi_res.base[i] == NULL) {
			snd_err("Unable to ioremap register region\n");
			return -ENXIO;
		}
	}

	if (1)
	{
		for (i = 0; i < ARRAY_SIZE(hdmi_attr); i++) 
		{
			ret = device_create_file(&pdev->dev, &hdmi_attr[i]);
			if (ret) {
				snd_err("Add device file failed");
				//goto device_create_file_failed;
			}
		}
	}
	else
	{
		snd_err("Find device failed");
		//goto err_bus_find_device;	
	}
*/	
#if defined(CONFIG_HAS_EARLYSUSPEND)&&defined(CONFIG_SND_UBUNTU)
        early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
        //ts->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
        early_suspend.suspend = hdmi_audio_early_suspend;
        early_suspend.resume = hdmi_audio_late_resume;
        register_early_suspend(&early_suspend);
#endif
	
	dev_warn(&pdev->dev,
			"atm7059_hdmi_probe!!\n");
						
	return snd_soc_register_codec(&pdev->dev, &soc_codec_hdmi,
			codec_hdmi_dai, ARRAY_SIZE(codec_hdmi_dai));
}

static int atm7059_hdmi_remove(struct platform_device *pdev)
{
#if defined(CONFIG_HAS_EARLYSUSPEND)&&defined(CONFIG_SND_UBUNTU)
        unregister_early_suspend(&early_suspend);
#endif
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static const struct of_device_id gl5203_hdmi_of_match[] = {
	{.compatible = "actions,gl5203-audio-hdmi",},
	{}
};

MODULE_DEVICE_TABLE(of, gl5203_hdmi_of_match);


static struct platform_driver atm7059_hdmi_driver = {
	.driver = {
			.name = "atm7059-hdmi-audio",
			.owner = THIS_MODULE,
	},

	.probe = atm7059_hdmi_probe,
	.remove = atm7059_hdmi_remove,
};
 
static struct platform_device *atm7059_hdmi_device;
static int __init atm7059_hdmi_init(void)
{
	int ret;
	int i = 0;

	atm7059_hdmi_device = platform_device_alloc("atm7059-hdmi-audio", -1);
	if (!atm7059_hdmi_device) {
		snd_err(
				"ASoC: Platform device atm7059-hdmi-audio allocation failed\n");
		ret = -ENOMEM;
		goto err;
	}

	ret = platform_device_add(atm7059_hdmi_device);
	if (ret) {
		snd_err(
				"ASoC: Platform device atm7059-hdmi-audio add failed\n");
		goto err_device_add;
	}

	ret = platform_driver_register(&atm7059_hdmi_driver);
	if (ret) {
		snd_err(
				"ASoC: Platform driver atm7059-hdmi-audio register failed\n");
		goto err_driver_register;
	}

	for (i = 0; i < ARRAY_SIZE(hdmi_attr); i++) {
		ret = device_create_file(
			&atm7059_hdmi_device->dev, &hdmi_attr[i]);
		if (ret) {
			snd_err("Add device file failed");
			goto device_create_file_failed;
		}
	}

	return 0;

device_create_file_failed:
err_driver_register:
	platform_device_unregister(atm7059_hdmi_device);

err_device_add:
	platform_device_put(atm7059_hdmi_device);
	
err:
	return ret;
}
static void __exit atm7059_hdmi_exit(void)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(hdmi_attr); i++) {
		device_remove_file(&atm7059_hdmi_device->dev, &hdmi_attr[i]);
	}

	platform_driver_unregister(&atm7059_hdmi_driver);
	platform_device_unregister(atm7059_hdmi_device);
	atm7059_hdmi_device = NULL;
}

module_init(atm7059_hdmi_init);
module_exit(atm7059_hdmi_exit);

MODULE_AUTHOR("sall.xie <sall.xie@actions-semi.com>");
MODULE_DESCRIPTION("atm7059 HDMI AUDIO module");
MODULE_LICENSE("GPL");
