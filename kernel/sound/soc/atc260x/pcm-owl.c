/*
 * atm7059-pcm.c  --  ALSA PCM interface for the OMAP SoC
 *
 * Copyright (C) 2008 Nokia Corporation
 *
 * Contact: Jarkko Nikula <jarkko.nikula@bitmer.com>
 *          Peter Ujfalusi <peter.ujfalusi@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>
#include <linux/dma-mapping.h>

#include <mach/hardware.h>
#include "sndrv-owl.h"

static struct snd_pcm_hardware atm7059_playback_hw_info = {
	.info			= SNDRV_PCM_INFO_MMAP |
				  SNDRV_PCM_INFO_MMAP_VALID |
				  SNDRV_PCM_INFO_INTERLEAVED |
				  SNDRV_PCM_INFO_BLOCK_TRANSFER |
				  SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE |
				  SNDRV_PCM_FMTBIT_S32_LE,
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 2,
	.channels_max		= 8,
	#ifdef CONFIG_SND_UBUNTU
	.buffer_bytes_max	= 32 * 1024,
	#else
	.buffer_bytes_max	= 64 * 1024,
	#endif
	.period_bytes_min	= 256,
	.period_bytes_max	= 32*1024,
	.periods_min		= 2,
	.periods_max		= 16,
};

static struct snd_pcm_hardware atm7059_capture_hw_info = {
	.info			= SNDRV_PCM_INFO_MMAP |
				  SNDRV_PCM_INFO_MMAP_VALID |
				  SNDRV_PCM_INFO_INTERLEAVED |
				  SNDRV_PCM_INFO_BLOCK_TRANSFER |
				  SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE |
				  SNDRV_PCM_FMTBIT_S32_LE,
	.rate_min		= 8000,
	.rate_max		= 96000,
	.channels_min		= 1,
	.channels_max		= 4,
	.buffer_bytes_max	= 64 * 1024,
	.period_bytes_min	= 256,
	.period_bytes_max	= 32*1024,
	.periods_min		= 2,
	.periods_max		= PAGE_SIZE / 16,
};
#ifdef CONFIG_SND_UBUNTU
int audio_set_output_mode(struct snd_pcm_substream *substream, int value)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_platform *platform = rtd->platform;
    struct atm7059_pcm_priv *pcm_priv =
        snd_soc_platform_get_drvdata(platform);
	pcm_priv->output_mode = value;
	return 0;
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

static struct device_attribute pcm_attr[] = {
	__ATTR(error, S_IRUSR | S_IWUSR, error_show, error_store),
	__ATTR(debug, S_IRUSR | S_IWUSR, debug_show, debug_store),
};


static const char *const audio_output_mode[]
	= {"i2s", "hdmi", "spdif"};
static const SOC_ENUM_SINGLE_DECL(audio_output_enum,
	0, 0, audio_output_mode);

static int audio_output_mode_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform =
		snd_kcontrol_chip(kcontrol);
	struct atm7059_pcm_priv *pcm_priv =
		snd_soc_platform_get_drvdata(platform);
	ucontrol->value.integer.value[0] = pcm_priv->output_mode;
	return 0;
}

static int audio_output_mode_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform =
		snd_kcontrol_chip(kcontrol);
	struct atm7059_pcm_priv *pcm_priv =
		snd_soc_platform_get_drvdata(platform);

	pcm_priv->output_mode = ucontrol->value.integer.value[0];
	return 0;
}

static const struct snd_kcontrol_new atm7059_pcm_controls[] = {
	SOC_ENUM_EXT("audio output mode switch", audio_output_enum,
			audio_output_mode_get, audio_output_mode_put),
};

/* this may get called several times by oss emulation */
static int atm7059_pcm_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct dma_chan *chan = snd_dmaengine_pcm_get_chan(substream);
	//struct imx_pcm_dma_params *dma_params;
	struct dma_slave_config slave_config;
	int ret;

	struct owl_dma_slave *atslave = (struct owl_dma_slave *)chan->private;
	dma_addr_t dst_addr;
	enum dma_slave_buswidth dst_addr_width;


	//dma_params = snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);

	//direction, dst_addr_width or src_addr_width
	ret = snd_hwparams_to_dma_slave_config(substream, params, &slave_config);
	if (ret)
		return ret;

	slave_config.device_fc = false;

	atslave->dma_dev = chan->device->dev;
	atslave->trans_type = SLAVE;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		struct snd_soc_platform *platform = rtd->platform;
		struct atm7059_pcm_priv *pcm_priv =
			snd_soc_platform_get_drvdata(platform);

		switch (pcm_priv->output_mode) {
		case O_MODE_SPDIF:
			atslave->mode = PRIORITY_SEVEN| SRC_INCR |
			    DST_CONSTANT | SRC_DCU | DST_DEV | HDMIAUDIO | CRITICAL_BIT;
			dst_addr = SPDIF_DAT;
			break;
		case O_MODE_HDMI:
			atslave->mode = PRIORITY_SEVEN | SRC_INCR |
			    DST_CONSTANT | SRC_DCU | DST_DEV | HDMIAUDIO | CRITICAL_BIT;
			dst_addr = HDMI_DAT;
			break;
		case O_MODE_I2S:
		default:
			atslave->mode = PRIORITY_SEVEN | SRC_INCR |
			    DST_CONSTANT | SRC_DCU | DST_DEV | I2S_T | CRITICAL_BIT;
			dst_addr = I2STX_DAT;
			break;
		}

		slave_config.dst_addr = dst_addr;

		dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config.dst_addr_width = dst_addr_width;

	} else {
		atslave->mode = PRIORITY_SEVEN | SRC_CONSTANT |
		    DST_INCR | DST_DCU | SRC_DEV | I2S_R | CRITICAL_BIT;

		slave_config.src_addr = I2SRX_DAT;

		slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	}

	ret = dmaengine_slave_config(chan, &slave_config);
	if (ret)
		return ret;

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	/*
	 * Set DMA transfer frame size equal to ALSA period size and frame
	 * count as no. of ALSA periods. Then with DMA frame interrupt enabled,
	 * we can transfer the whole ALSA buffer with single DMA transfer but
	 * still can get an interrupt at each period bounary
	 */

	return 0;
}

static int atm7059_pcm_hw_free(struct snd_pcm_substream *substream)
{
	snd_pcm_set_runtime_buffer(substream, NULL);

	return 0;
}

static int atm7059_pcm_prepare(struct snd_pcm_substream *substream)
{
	return 0;
}

static int atm7059_pcm_open(struct snd_pcm_substream *substream)
{
	int ret;
	struct dma_chan *chan;
	struct owl_dma_slave *atslave = kzalloc(sizeof(*atslave), GFP_KERNEL);
	if (!atslave)
		return -ENOMEM;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_set_runtime_hwparams(substream, &atm7059_playback_hw_info);
	} else {
		snd_soc_set_runtime_hwparams(substream, &atm7059_capture_hw_info);
	}

	//snd_pcm_hw_constraint_integer&&分配运行时&&申请通道
	//ret = snd_dmaengine_pcm_open(substream, filter, dma_data);
	ret = snd_dmaengine_pcm_open_request_chan(substream, NULL, NULL);
	if (ret) {
		return 0;
	}

	//snd_dmaengine_pcm_set_data(substream, dma_data);

	chan = snd_dmaengine_pcm_get_chan(substream);
	chan->private = (void *)atslave;
	return 0;
}

static int atm7059_pcm_close(struct snd_pcm_substream *substream)
{
	//struct imx_dma_data *dma_data = snd_dmaengine_pcm_get_data(substream);
	//释放通道，释放运行时
	snd_dmaengine_pcm_close_release_chan(substream);
	//printk("%s %d\n", __FUNCTION__, __LINE__);

	//kfree(dma_data);

	return 0;
}

static int atm7059_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret= dma_mmap_coherent(substream->pcm->card->dev, vma,
				                            runtime->dma_area,
				                            runtime->dma_addr,
				                            runtime->dma_bytes);
	return ret;
}

static struct snd_pcm_ops atm7059_pcm_ops = {
	.open		= atm7059_pcm_open,
	.close		= atm7059_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= atm7059_pcm_hw_params,
	.hw_free	= atm7059_pcm_hw_free,
	.prepare	= atm7059_pcm_prepare,
	.trigger	= snd_dmaengine_pcm_trigger,
	.pointer	= snd_dmaengine_pcm_pointer_no_residue,
	.mmap		= atm7059_pcm_mmap,
};

static u64 atm7059_pcm_dmamask = DMA_BIT_MASK(32);

static int atm7059_pcm_preallocate_dma_buffer(struct snd_pcm *pcm,
	int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size;
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		size = atm7059_playback_hw_info.buffer_bytes_max;
	else
		size = atm7059_capture_hw_info.buffer_bytes_max;

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	buf->area = dma_alloc_coherent(pcm->card->dev, size,
					   &buf->addr, GFP_KERNEL);
	if (!buf->area)
		return -ENOMEM;

	buf->bytes = size;

	return 0;
}

static void atm7059_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		dma_free_coherent(pcm->card->dev, buf->bytes,
				      buf->area, buf->addr);

		buf->area = NULL;
	}
}

static int atm7059_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	int ret = 0;

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &atm7059_pcm_dmamask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
		ret = atm7059_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
		ret = atm7059_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}

out:
	/* free preallocated buffers in case of error */
	if (ret)
		atm7059_pcm_free_dma_buffers(pcm);

	return ret;
}
#ifdef CONFIG_SND_UBUNTU
static int stm7059_pcm_suspend(struct snd_soc_dai *dai)
{
    struct snd_soc_pcm_runtime *rtd = dai->card->rtd;
	struct snd_pcm *pcm = rtd->pcm;
    snd_pcm_suspend_all(pcm);

}

static int stm7059_pcm_resume(struct snd_soc_dai *dai)
{
}
#endif

static struct snd_soc_platform_driver atm7059_soc_platform = {
	.ops		= &atm7059_pcm_ops,
	.pcm_new	= atm7059_pcm_new,
	.pcm_free	= atm7059_pcm_free_dma_buffers,
#ifdef CONFIG_SND_UBUNTU
    .suspend = stm7059_pcm_suspend,
    .resume = stm7059_pcm_resume,
#endif
	.controls = atm7059_pcm_controls,
	.num_controls = ARRAY_SIZE(atm7059_pcm_controls),
};

static int atm7059_pcm_probe(struct platform_device *pdev)
{
	dev_info(&pdev->dev,
			"atm7059_pcm_probe!!\n");
	pdev->dev.init_name = "atm7059-pcm-audio";
	return snd_soc_register_platform(&pdev->dev,
			&atm7059_soc_platform);
}

static int atm7059_pcm_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static struct platform_driver atm7059_pcm_driver = {
	.driver = {
			.name = "atm7059-pcm-audio",
			.owner = THIS_MODULE,
	},

	.probe = atm7059_pcm_probe,
	.remove = atm7059_pcm_remove,
};

static struct platform_device *atm7059_pcm_device;
static int __init atm7059_pcm_init(void)
{
	int ret;
	int i = 0;
	struct atm7059_pcm_priv *pcm_priv;

	snd_dbg("atm7059_pcm_init!!\n");
	atm7059_pcm_device = platform_device_alloc("atm7059-pcm-audio", -1);
	if (!atm7059_pcm_device) {
		snd_dbg(
				"ASoC: Platform device atm7059-pcm-audio allocation failed\n");
		ret = -ENOMEM;
		goto err;
	}

	ret = platform_device_add(atm7059_pcm_device);
	if (ret) {
		snd_dbg(
				"ASoC: Platform device atm7059-pcm-audio add failed\n");
		goto err_device_add;
	}

	pcm_priv = kzalloc(sizeof(struct atm7059_pcm_priv), GFP_KERNEL);
	if (NULL == pcm_priv)
		return -ENOMEM;
	pcm_priv->output_mode = O_MODE_I2S;
	platform_set_drvdata(atm7059_pcm_device, pcm_priv);

	ret = platform_driver_register(&atm7059_pcm_driver);
	if (ret) {
		snd_dbg(
				"ASoC: Platform driver atm7059-pcm-audio register failed\n");
		goto err_driver_register;
	}

	for (i = 0; i < ARRAY_SIZE(pcm_attr); i++) {
		ret = device_create_file(
			&atm7059_pcm_device->dev, &pcm_attr[i]);
		if (ret) {
			snd_err("Add device file failed");
			goto device_create_file_failed;
		}
	}

	return 0;

device_create_file_failed:
err_driver_register:
	platform_device_unregister(atm7059_pcm_device);
err_device_add:
	platform_device_put(atm7059_pcm_device);
err:
	return ret;
}
static void __exit atm7059_pcm_exit(void)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(pcm_attr); i++) {
		device_remove_file(&atm7059_pcm_device->dev, &pcm_attr[i]);
	}

	platform_driver_unregister(&atm7059_pcm_driver);
	platform_device_unregister(atm7059_pcm_device);
	atm7059_pcm_device = NULL;
}

module_init(atm7059_pcm_init);
module_exit(atm7059_pcm_exit);

MODULE_AUTHOR("sall.xie <sall.xie@actions-semi.com>");
MODULE_DESCRIPTION("ATM7059 PCM module");
MODULE_LICENSE("GPL");
