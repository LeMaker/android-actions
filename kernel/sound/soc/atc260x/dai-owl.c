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
#include <sound/initval.h>

#include <linux/clk.h>			/* clk_enable */
#include <mach/clkname.h>
#include <linux/cpufreq.h>
#include <linux/io.h> 
#include <linux/ioport.h>
#include "sndrv-owl.h"
#include "common-regs-owl.h"
#include <mach/module-owl.h>

#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>

static int dai_clk_i2s_count;
static int dai_clk_hdmi_count;
static int dai_clk_spdif_count;
static int dai_mode_i2s_count;
static int dai_mode_hdmi_count;

#ifdef CONFIG_SND_UBUNTU
static int is_i2s_playback;
static int is_i2s_record;
#endif

struct asoc_dai_resource {
    void __iomem    *base[MAX_RES_NUM];/*virtual base for every resource*/
    void __iomem    *baseptr; /*pointer to every virtual base*/
    struct clk      *clk;
    int             irq;
    unsigned int    setting;
};

//dai resources
static struct asoc_dai_resource dai_res;
 
void set_dai_reg_base(int num)
{
	dai_res.baseptr = dai_res.base[num];
}

EXPORT_SYMBOL_GPL(set_dai_reg_base);

u32 snd_dai_readl(u32 reg)
{
	return readl(dai_res.baseptr + reg);
}

EXPORT_SYMBOL_GPL(snd_dai_readl);
	 
void snd_dai_writel(u32 val, u32 reg)
{
	writel(val, dai_res.baseptr + reg);
}

EXPORT_SYMBOL_GPL(snd_dai_writel);



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

static struct device_attribute dai_attr[] = {
	__ATTR(error, S_IRUSR | S_IWUSR, error_show, error_store),
	__ATTR(debug, S_IRUSR | S_IWUSR, debug_show, debug_store),
};

/******************************************************************************/
/*!
 * \par  Description:
 *    将采样率转换成矊 *¬件寄存器设置所需的索引值
 * \param[in]    sample_rate  采样率
 * \param[in]    mode 输入输出模
 *式 （如果是spdif或者hdmi，index不同）
 * \return       索引值
 * \retval           -1 failed
 * \ingroup      sndrv
 *****************************************/
static int get_sf_index(int sample_rate, int mode)
{
	int i = 0;
	char fs = sample_rate / 1000;
	// 44k 在基础上寄存器bit位基础上加16 
	static fs_t fs_list[] = {
		{ 384, { -1, 0} },
		{ 352, { -1, 16} },
		{ 192, { 0,  1} },
		{ 176, { 16, 17} },
		{ 96,  { 1,  3} },
		{ 88,  { 17, 19} },
		{ 64,  { 2, -1} },
		{ 48,  { 3,  5} },
		{ 44,  { 19, 21} },
		{ 32,  { 4,  6} },
		{ 24,  { 5, -1} },
		{ 22,  {21, -1} },
		{ 16,  { 6, -1} },
		{ 12,  { 7, -1} },
		{ 11,  {23, -1} },
		{ 8,   { 8, -1} },
		{ -1,  {-1, -1} }
	};

	while ((fs_list[i].sample_rate > 0) && (fs_list[i].sample_rate != fs))
		i++;

	if ((mode == O_MODE_HDMI) || (mode == O_MODE_SPDIF))
		return fs_list[i].index[1];
	else
		return fs_list[i].index[0];
}

static int atm7059_dai_record_clk_set(int mode, int rate)
{
	struct clk *apll_clk;
	unsigned long reg_val;
	int sf_index, ret;

	if (dai_clk_i2s_count > 0) {
		dai_clk_i2s_count++;
		return 0;
	}
	module_clk_enable(MOD_ID_I2SRX);
	module_clk_enable(MOD_ID_I2STX);


	sf_index = get_sf_index(rate, mode);
	if (sf_index & 0x10)
		reg_val = 45158400;
	else
		reg_val = 49152000;

	apll_clk = clk_get(NULL, CLKNAME_AUDIOPLL);
	clk_prepare(apll_clk);
	ret = clk_set_rate(apll_clk, reg_val);
	if (ret < 0) {
		snd_dbg("audiopll set error!\n");
		return ret;
	}

	apll_clk = clk_get(NULL, CLKNAME_I2STX_CLK);
	ret = clk_set_rate(apll_clk, rate << 8);
	if (ret) {
		snd_dbg("i2stx clk rate set error!!\n");
		return ret;
	}
	apll_clk = clk_get(NULL, CLKNAME_I2SRX_CLK);
	ret = clk_set_rate(apll_clk, rate << 8);
	if (ret) {
		snd_dbg("i2srx clk rate set error!!\n");
		return ret;
	}
	dai_clk_i2s_count++;

	return 0;
}

static int atm7059_dai_clk_set(int mode, int rate)
{
	struct clk *apll_clk;
	unsigned long reg_val;
	int sf_index, ret;

	module_clk_enable(MOD_ID_I2SRX);
	module_clk_enable(MOD_ID_I2STX);

	sf_index = get_sf_index(rate, mode);
	if (sf_index & 0x10)
		reg_val = 45158400;
	else
		reg_val = 49152000;

	apll_clk = clk_get(NULL, CLKNAME_AUDIOPLL);
	clk_prepare(apll_clk);
	ret = clk_set_rate(apll_clk, reg_val);
	if (ret < 0) {
		snd_dbg("audiopll set error!\n");
		return ret;
	}

	switch (mode) {
	case O_MODE_I2S:
	if (dai_clk_i2s_count == 0) {
		apll_clk = clk_get(NULL, CLKNAME_I2STX_CLK);
		ret = clk_set_rate(apll_clk, rate << 8);
		if (ret) {
			snd_dbg("i2stx clk rate set error!!\n");
			return ret;
		}
		apll_clk = clk_get(NULL, CLKNAME_I2SRX_CLK);
		ret = clk_set_rate(apll_clk, rate << 8);
		if (ret) {
			snd_dbg("i2srx clk rate set error!!\n");
			return ret;
		}
	}
	dai_clk_i2s_count++;
	break;

	case O_MODE_HDMI:
	if (dai_clk_hdmi_count == 0) {
		apll_clk = clk_get(NULL, CLKNAME_HDMIA_CLK);
		ret = clk_set_rate(apll_clk, rate << 7);
		if (ret) {
			snd_dbg("hdmi clk rate set error!!\n");
			return ret;
		}
	}
	dai_clk_hdmi_count++;
	break;

	case O_MODE_SPDIF:
	if (dai_clk_spdif_count == 0) {
		apll_clk = clk_get(NULL, CLKNAME_SPDIF_CLK);
		ret = clk_set_rate(apll_clk, rate << 7);
		if (ret) {
			snd_dbg("spdif clk rate set error!!\n");
			return ret;
		}
	}
	dai_clk_spdif_count++;
	break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int atm7059_dai_record_clk_disable(void)
{
    if(dai_clk_i2s_count > 0)
	{
		dai_clk_i2s_count--;
		return 0;
	}

	//module_clk_disable(MOD_ID_I2SRX);
	//module_clk_disable(MOD_ID_I2STX);

	return 0;
}

static int atm7059_dai_clk_disable(int mode)
{
	switch (mode) {
	case O_MODE_I2S:
	/* we disable the i2s_clk in another place */
	/*
	apll_clk = clk_get_sys(CLK_NAME_I2STX_CLK, NULL);
	clk_disable(apll_clk);
	apll_clk = clk_get_sys(CLK_NAME_I2SRX_CLK, NULL);
	clk_disable(apll_clk);
	*/
	if(dai_clk_i2s_count > 0)
		dai_clk_i2s_count--;
	break;

	case O_MODE_HDMI:
    if(dai_clk_hdmi_count > 0)
	    dai_clk_hdmi_count--;
	break;

	case O_MODE_SPDIF:
    if(dai_clk_spdif_count > 0)    
	dai_clk_spdif_count--;
	break;
	default:
		return -EINVAL;
	}
	return 0;
}

#if 0
static int atm7059_i2s_4wire_config(struct atm7059_pcm_priv *pcm_priv,
		struct snd_soc_dai *dai)
{
	int ret;

	pcm_priv->pc = pinctrl_get(dai->dev);
	if (IS_ERR(pcm_priv->pc) || (pcm_priv->pc == NULL)) {
		snd_dbg("i2s pin control failed!\n");
		return -EAGAIN;
	}

	pcm_priv->ps = pinctrl_lookup_state(pcm_priv->pc, "default");
	if (IS_ERR(pcm_priv->ps) || (pcm_priv->ps == NULL)) {
		snd_dbg("i2s pin state get failed!\n");
		return -EAGAIN;
	}

	ret = pinctrl_select_state(pcm_priv->pc, pcm_priv->ps);
	if (ret) {
		snd_dbg("i2s pin state set failed!\n");
		return -EAGAIN;
	}

	/*
	 * set 4wire mode
	snd_dai_writel(snd_dai_readl(PAD_CTL) | (0x1 << 1), PAD_CTL);
	snd_dai_writel(snd_dai_readl(MFP_CTL0) & ~(0x1 << 2), MFP_CTL0);
	snd_dai_writel(snd_dai_readl(MFP_CTL0) & ~(0x1 << 5), MFP_CTL0);
	 */

	/* disable i2s tx&rx */
	snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x3 << 0), I2S_CTL);

	/* reset i2s rx&&tx fifo, avoid left & right channel wrong */
	snd_dai_writel(snd_dai_readl(I2S_FIFOCTL) & ~(0x3 << 9) & ~0x3, I2S_FIFOCTL);
	snd_dai_writel(snd_dai_readl(I2S_FIFOCTL) | (0x3 << 9) | 0x3, I2S_FIFOCTL);

	/* this should before enable rx/tx,
	or after suspend, data may be corrupt */
	snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x3 << 11), I2S_CTL);
	snd_dai_writel(snd_dai_readl(I2S_CTL) | (0x1 << 11), I2S_CTL);
	/* set i2s mode I2S_RX_ClkSel==1 */
	snd_dai_writel(snd_dai_readl(I2S_CTL) | (0x1 << 10), I2S_CTL);

	/* enable i2s rx/tx at the same time */
	snd_dai_writel(snd_dai_readl(I2S_CTL) | 0x3, I2S_CTL);

	/* i2s rx 00: 2.0-Channel Mode */
	snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x3 << 8), I2S_CTL);
	snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x7 << 4), I2S_CTL);

	return 0;
}
#endif

static int atm7059_dai_record_mode_set(struct atm7059_pcm_priv *pcm_priv,
		struct snd_soc_dai *dai)
{
	/*ret = atm7059_i2s_4wire_config(pcm_priv, dai);*/
	/*snd_dai_writel(snd_dai_readl(PAD_CTL) | (0x1 << 1), PAD_CTL);*/
	if (dai_mode_i2s_count == 0) {
//		set_dai_reg_base(GPIO_MFP_NUM);
//		snd_dai_writel(snd_dai_readl(MFP_CTL0) & ~(0x1 << 2), MFP_CTL0);
//		snd_dai_writel(snd_dai_readl(MFP_CTL0) & ~(0x3 << 3), MFP_CTL0);

		/* disable i2s tx&rx */
		set_dai_reg_base(I2S_SPDIF_NUM);
		snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x3 << 0), I2S_CTL);

		/* reset i2s rx&&tx fifo, avoid left & right channel wrong */
		snd_dai_writel(snd_dai_readl(I2S_FIFOCTL)
		& ~(0x3 << 9) & ~0x3, I2S_FIFOCTL);
		snd_dai_writel(snd_dai_readl(I2S_FIFOCTL)
			| (0x3 << 9) | 0x3, I2S_FIFOCTL);

		/* this should before enable rx/tx,
		or after suspend, data may be corrupt */
		snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x3 << 11), I2S_CTL);
		snd_dai_writel(snd_dai_readl(I2S_CTL) | (0x1 << 11), I2S_CTL);
		/* set i2s mode I2S_RX_ClkSel==1 */
		snd_dai_writel(snd_dai_readl(I2S_CTL) | (0x1 << 10), I2S_CTL);

		/* enable i2s rx/tx at the same time */
		snd_dai_writel(snd_dai_readl(I2S_CTL) | 0x3, I2S_CTL);

		/* i2s rx 00: 2.0-Channel Mode */
		snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x3 << 8), I2S_CTL);
		snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x7 << 4), I2S_CTL);
	}
	dai_mode_i2s_count++;

	return 0;
}


static int atm7059_dai_mode_set(struct atm7059_pcm_priv *pcm_priv,
		struct snd_soc_dai *dai)
{
	int ret;

	switch (pcm_priv->output_mode) {
	case O_MODE_I2S:
		/*ret = atm7059_i2s_4wire_config(pcm_priv, dai);*/
		/*snd_dai_writel(snd_dai_readl(PAD_CTL) | (0x1 << 1), PAD_CTL);*/
		if (dai_mode_i2s_count == 0) {
//			set_dai_reg_base(GPIO_MFP_NUM);
//			snd_dai_writel(snd_dai_readl(MFP_CTL0) & ~(0x1 << 2), MFP_CTL0);
//			snd_dai_writel(snd_dai_readl(MFP_CTL0) & ~(0x3 << 3), MFP_CTL0);

			/* disable i2s tx&rx */
			set_dai_reg_base(I2S_SPDIF_NUM);
			snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x3 << 0), I2S_CTL);

			/* reset i2s rx&&tx fifo, avoid left & right channel wrong */
			snd_dai_writel(snd_dai_readl(I2S_FIFOCTL)
				& ~(0x3 << 9) & ~0x3, I2S_FIFOCTL);
			snd_dai_writel(snd_dai_readl(I2S_FIFOCTL)
				| (0x3 << 9) | 0x3, I2S_FIFOCTL);

			/* this should before enable rx/tx,
			or after suspend, data may be corrupt */
			snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x3 << 11), I2S_CTL);
			snd_dai_writel(snd_dai_readl(I2S_CTL) | (0x1 << 11), I2S_CTL);
			/* set i2s mode I2S_RX_ClkSel==1 */
			snd_dai_writel(snd_dai_readl(I2S_CTL) | (0x1 << 10), I2S_CTL);

			/* enable i2s rx/tx at the same time */
			snd_dai_writel(snd_dai_readl(I2S_CTL) | 0x3, I2S_CTL);

			/* i2s rx 00: 2.0-Channel Mode */
			snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x3 << 8), I2S_CTL);
			snd_dai_writel(snd_dai_readl(I2S_CTL) & ~(0x7 << 4), I2S_CTL);
		}
		dai_mode_i2s_count++;
		break;

	case O_MODE_HDMI:
		/* HDMI&SPDIF fifo reset */
		if (dai_mode_hdmi_count == 0) {   
            set_dai_reg_base(I2S_SPDIF_NUM);
			snd_dai_writel(snd_dai_readl(SPDIF_HDMI_CTL) & ~0x3,
				SPDIF_HDMI_CTL);
			/* HDMI fifo enable,DRQ enable */
			snd_dai_writel(snd_dai_readl(SPDIF_HDMI_CTL) |
				0x102, SPDIF_HDMI_CTL);
		}
		dai_mode_hdmi_count++;
		break;
	case O_MODE_SPDIF:
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static int atm7059_dai_record_mode_unset(void)
{
    if(dai_mode_i2s_count > 0)
	    dai_mode_i2s_count--;
	if (dai_mode_i2s_count == 0) {
		set_dai_reg_base(I2S_SPDIF_NUM);
		snd_dai_writel(snd_dai_readl(I2S_CTL) & ~0x3, I2S_CTL);
		snd_dai_writel(snd_dai_readl(I2S_FIFOCTL) & ~0x3, I2S_FIFOCTL);
		snd_dai_writel(snd_dai_readl(I2S_FIFOCTL) & ~(0x3 << 9), I2S_FIFOCTL);
#ifdef CONFIG_SND_UBUNTU		
		is_i2s_record = 0;
#endif
		/*pinctrl_put(pcm_priv->pc);*/
	}
	return 0;
}

static int atm7059_dai_mode_unset(struct atm7059_pcm_priv *pcm_priv)
{
	switch (pcm_priv->output_mode) {
	case O_MODE_I2S:
		if(dai_mode_i2s_count > 0)
		dai_mode_i2s_count--;
		if (dai_mode_i2s_count == 0) {
			set_dai_reg_base(I2S_SPDIF_NUM);
			snd_dai_writel(snd_dai_readl(I2S_CTL) & ~0x3, I2S_CTL);
			snd_dai_writel(snd_dai_readl(I2S_FIFOCTL) & ~0x3, I2S_FIFOCTL);
			snd_dai_writel(snd_dai_readl(I2S_FIFOCTL) & ~(0x3 << 9), I2S_FIFOCTL);
			/*pinctrl_put(pcm_priv->pc);*/
#ifdef CONFIG_SND_UBUNTU			
			is_i2s_playback = 0;
#endif
		}
		break;
	case O_MODE_HDMI:
		/* HDMI fifo disable */
        if(dai_mode_hdmi_count > 0)
		dai_mode_hdmi_count--;
		if (dai_mode_hdmi_count == 0) {
			set_dai_reg_base(I2S_SPDIF_NUM);
			snd_dai_writel(snd_dai_readl(SPDIF_HDMI_CTL) & ~0x2, SPDIF_HDMI_CTL);
		}
		break;
	case O_MODE_SPDIF:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int atm7059_dai_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct atm7059_pcm_priv *pcm_priv
		= snd_soc_platform_get_drvdata(platform);

#ifdef CONFIG_SND_UBUNTU		
	if(((pcm_priv->output_mode == O_MODE_I2S)&&(is_i2s_playback == 1)) || 
		(is_i2s_record == 1))
	{
		//snd_err("param setted\n");
		return 0;
	}	
#endif

//	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
//		//snd_err("playback hw param\n");
//	}
//	else
//	{
//		//snd_err("record hw param\n");
//	}

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
        #ifdef CONFIG_SND_UBUNTU
        printk(KERN_ERR"%s,SNDRV_PCM_FORMAT_S16_LE\n", __func__);
        break;
        #endif
	case SNDRV_PCM_FORMAT_S32_LE:
        #ifdef CONFIG_SND_UBUNTU
        printk(KERN_ERR"%s,SNDRV_PCM_FORMAT_S32_LE\n", __func__);
        #endif
		break;
	default:
		return -EINVAL;
	}
	if (SNDRV_PCM_STREAM_CAPTURE == substream->stream ) {
		atm7059_dai_record_clk_set(pcm_priv->output_mode, params_rate(params));
		atm7059_dai_record_mode_set(pcm_priv, dai);
#ifdef CONFIG_SND_UBUNTU		
		is_i2s_record = 1;
#endif
	}

	if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream ) {
		atm7059_dai_clk_set(pcm_priv->output_mode, params_rate(params));
		atm7059_dai_mode_set(pcm_priv, dai);
#ifdef CONFIG_SND_UBUNTU		
		if(pcm_priv->output_mode == O_MODE_I2S)
		{
			is_i2s_playback = 1;
		}
#endif
	}
	return 0;
}

static int atm7059_dai_hw_free(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct atm7059_pcm_priv *pcm_priv =
		snd_soc_platform_get_drvdata(platform);
		
		

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		//snd_err("playback free\n");
	}
	else
	{
		//snd_err("record free\n");
	}
#ifdef CONFIG_SND_UBUNTU	
	if (SNDRV_PCM_STREAM_CAPTURE == substream->stream && is_i2s_playback == 0 )
#else
	if (SNDRV_PCM_STREAM_CAPTURE == substream->stream)
#endif
	{
		atm7059_dai_record_clk_disable();
		atm7059_dai_record_mode_unset();
	}

#ifdef CONFIG_SND_UBUNTU
	if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream && is_i2s_record == 0 )
#else
	if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream)
#endif
	{
		atm7059_dai_clk_disable(pcm_priv->output_mode);
		atm7059_dai_mode_unset(pcm_priv);
	}
	

	return 0;
}

struct snd_soc_dai_ops atm7059_dai_dai_ops = {
	.hw_params = atm7059_dai_hw_params,
	.hw_free = atm7059_dai_hw_free,
};

#ifdef CONFIG_SND_UBUNTU
static u32 i2s_ctl_reg;
static u32 i2s_fifoctl_reg;
static u32 hdmi_ctl_reg;
static int dai_regs_stored = -1;
static int atm7059_dai_store_regs(void)
{
    set_dai_reg_base(I2S_SPDIF_NUM);

	i2s_ctl_reg = snd_dai_readl(I2S_CTL);
    i2s_fifoctl_reg = snd_dai_readl(I2S_FIFOCTL);
    hdmi_ctl_reg = snd_dai_readl(SPDIF_HDMI_CTL);
	dai_regs_stored = 1;
	return 0;
}

static int atm7059_dai_restore_regs(void)
{
	int i;
	int reg_now;

	if(dai_regs_stored < 0)
	{
		printk("no initial regs value yet\n");
		return 0;
	}
    set_dai_reg_base(I2S_SPDIF_NUM);

	snd_dai_writel(i2s_ctl_reg, I2S_CTL);
    snd_dai_writel(i2s_fifoctl_reg, I2S_FIFOCTL);
    snd_dai_writel(hdmi_ctl_reg, SPDIF_HDMI_CTL);	
	dai_regs_stored = -1;

	return 0;
}

static int atm7059_dai_suspend(struct snd_soc_dai *dai)
{
    atm7059_dai_store_regs();
}
static int atm7059_dai_resume(struct snd_soc_dai *dai)
{
    atm7059_dai_restore_regs();
}
#endif

#define ATM7059_STEREO_CAPTURE_RATES SNDRV_PCM_RATE_8000_96000
#define ATM7059_STEREO_PLAYBACK_RATES SNDRV_PCM_RATE_8000_192000
#define ATM7059_FORMATS SNDRV_PCM_FMTBIT_S16_LE

struct snd_soc_dai_driver atm7059_dai = {
	.name = "gl5203-audio-i2s",
	.id = ATM7059_AIF_I2S,
	.playback = {
		.stream_name = "atm7059 dai Playback",
		.channels_min = 1,
		.channels_max = 8,
		.rates = ATM7059_STEREO_PLAYBACK_RATES,
		#ifdef CONFIG_SND_UBUNTU
		.formats = SNDRV_PCM_FMTBIT_S32_LE,
		#else
		.formats = SNDRV_PCM_FMTBIT_S16_LE,		
		#endif
	},
	.capture = {
		.stream_name = "atm7059 dai Capture",
		.channels_min = 1,
		.channels_max = 4,
		.rates = ATM7059_STEREO_CAPTURE_RATES,
		#ifdef CONFIG_SND_UBUNTU
		.formats = SNDRV_PCM_FMTBIT_S32_LE,
		#else
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
		#endif
	},
#ifdef CONFIG_SND_UBUNTU
	.suspend = atm7059_dai_suspend,
    .resume = atm7059_dai_resume,
#endif
	.ops = &atm7059_dai_dai_ops,
};

static const struct snd_soc_component_driver atm7059_component = {
	.name		= "atm7059ac97c",
};

static const struct of_device_id owl_i2s_of_match[] = {
	{.compatible = "actions,owl-audio-i2s",},
	{}
};

MODULE_DEVICE_TABLE(of, owl_i2s_of_match);

static int atm7059_dai_probe(struct platform_device *pdev)
{
	struct resource *res;
	int i;
	int ret = 0;

	struct device_node *dn;

	dn = of_find_compatible_node(NULL, NULL, "actions,owl-audio-i2s");
	if (!dn) {
		snd_err("Fail to get device_node actions,owl-audio-i2s\r\n");
		//goto of_get_failed;
	}
	
	/*FIXME: what if error in second or third loop*/
	//for(i=0; i<MAX_RES_NUM; i++) 
	for(i=0; i<1; i++) 
	{
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			snd_err("no memory resource i=%d\n", i);
			return -ENODEV;
		}

		if (!devm_request_mem_region (&pdev->dev, res->start,
					resource_size(res), "owl-audio-i2s")) {
			snd_err("Unable to request register region\n");
			return -EBUSY;
		}

		dai_res.base[i] = devm_ioremap(&pdev->dev, res->start, resource_size(res));
		if (dai_res.base[i] == NULL) {
			snd_err("Unable to ioremap register region\n");
			return -ENXIO;
		}
		
		snd_err("it's ok %d\n", i);
	}


	if (1)
	{
		for (i = 0; i < ARRAY_SIZE(dai_attr); i++) 
		{
			ret = device_create_file(&pdev->dev, &dai_attr[i]);
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

	
	dev_warn(&pdev->dev, "atm7059_dai_probe\n");
	//snd_err("dai probe fine\n");
	
	pdev->dev.init_name = "owl-audio-i2s";
	
	return snd_soc_register_component(&pdev->dev, &atm7059_component,
					 &atm7059_dai, 1);

}

static int atm7059_dai_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}



static struct platform_driver atm7059_dai_driver = {
	.driver = {
		.name = "owl-audio-i2s",
		.owner = THIS_MODULE,
		.of_match_table = owl_i2s_of_match,
	},

	.probe = atm7059_dai_probe,
	.remove = atm7059_dai_remove,
};

//static struct platform_device *atm7059_dai_device;

static int __init atm7059_dai_init(void)
{
	int ret;

	ret = platform_driver_register(&atm7059_dai_driver);
	if (ret) {
		snd_err(
			"ASoC: Platform driver atm7059-dai register failed\n");
		goto err_driver_register;
	}


	return 0;

err_driver_register:
	return ret;
}

static void __exit atm7059_dai_exit(void)
{
	int i = 0;
	struct device *dev = NULL;

    dev = bus_find_device_by_name(&platform_bus_type, NULL, "owl-audio-i2s");	

	if (dev)
	{
		for (i = 0; i < ARRAY_SIZE(dai_attr); i++)
		{
			device_remove_file(dev, &dai_attr[i]);
		}
	}

	platform_driver_unregister(&atm7059_dai_driver);
	//platform_device_unregister(atm7059_dai_device);
	//atm7059_dai_device = NULL;
}

module_init(atm7059_dai_init);
module_exit(atm7059_dai_exit);


/* Module information */
MODULE_AUTHOR("sall.xie <sall.xie@actions-semi.com>");
MODULE_DESCRIPTION("ATM7059 I2S Interface");
MODULE_LICENSE("GPL");
