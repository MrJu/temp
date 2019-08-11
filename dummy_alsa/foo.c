/*
 * Copyright (C) 2019 Andrew <mrju.email@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 */

#include <linux/module.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/pcm.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>

#define STR(x) _STR(x)
#define _STR(x) #x

#define VERSION_PREFIX dummy
#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define PATCH_VERSION 0

#define VERSION STR(VERSION_PREFIX-MAJOR_VERSION.MINOR_VERSION.PATCH_VERSION)

#define DEVICE_NAME "dummy-card"

#define MS_TO_NS(x) (x * 1E6L)
#define DELAY_MS 1L

static struct hrtimer timer;
ktime_t kt;
unsigned long int pos;
struct snd_pcm_substream *subs;

static enum hrtimer_restart hrtimer_handler(struct hrtimer *timer)
{
	struct snd_pcm_substream *substream = subs;
	unsigned long int delta;

	printk("%s(): %d\n", __func__, __LINE__);

	pos += snd_pcm_lib_period_bytes(substream);
	if (pos >= snd_pcm_lib_buffer_bytes(substream))
		pos = 0;

	if (substream)
		snd_pcm_period_elapsed(substream);

	hrtimer_forward_now(timer, kt);
	return HRTIMER_RESTART;
}

static int timer_start(void)
{
	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_SOFT);
	timer.function = hrtimer_handler;
	hrtimer_start(&timer, kt, HRTIMER_MODE_REL_SOFT);

	return 0;
}

static void timer_stop(void)
{
	hrtimer_cancel(&timer);
}


static const struct snd_pcm_hardware dummy_pcm_hardware = {
	.info =			SNDRV_PCM_INFO_MMAP |
					SNDRV_PCM_INFO_INTERLEAVED |
					SNDRV_PCM_INFO_RESUME |
					SNDRV_PCM_INFO_MMAP_VALID,

	.formats =		SNDRV_PCM_FMTBIT_U8 |
					SNDRV_PCM_FMTBIT_S16_LE,

	.rates =		SNDRV_PCM_RATE_CONTINUOUS |
					SNDRV_PCM_RATE_8000_48000,

	.rate_min =		5500,
	.rate_max =		48000,
	.channels_min =		1,
	.channels_max =		2,
	.buffer_bytes_max =	64 * 1024,
	.period_bytes_min =	64,
	.period_bytes_max =	64 * 1024,
	.periods_min =		1,
	.periods_max =		1024,
	.fifo_size =		0,
};

static int dummy_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	subs = substream;

	runtime->hw = dummy_pcm_hardware;

	if (substream->pcm->device & 1) {
		runtime->hw.info &= ~SNDRV_PCM_INFO_INTERLEAVED;
		runtime->hw.info |= SNDRV_PCM_INFO_NONINTERLEAVED;
	}
	if (substream->pcm->device & 2)
		runtime->hw.info &= ~(SNDRV_PCM_INFO_MMAP |
				      SNDRV_PCM_INFO_MMAP_VALID);

#if 0
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (model->playback_constraints)
			err = model->playback_constraints(substream->runtime);
	} else {
		if (model->capture_constraints)
			err = model->capture_constraints(substream->runtime);
	}
#endif

	printk("%s(): %d\n", __func__, __LINE__);

	return 0;
}

static int dummy_pcm_close(struct snd_pcm_substream *substream)
{
	subs = NULL;
	printk("%s(): %d\n", __func__, __LINE__);
	return 0;
}

static int dummy_pcm_hw_params(struct snd_pcm_substream *substream,
			struct snd_pcm_hw_params *hw_params)
{
	int ret;

	if (PCM_RUNTIME_CHECK(substream)) {
		printk("%s(): %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (snd_BUG_ON(substream->dma_buffer.dev.type ==
		       SNDRV_DMA_TYPE_UNKNOWN)) {
		printk("%s(): %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	ret = snd_pcm_lib_malloc_pages(substream,
			params_buffer_bytes(hw_params));

	printk("%s(): %d %d\n", __func__, __LINE__, ret);

	return ret;
}

static int dummy_pcm_hw_free(struct snd_pcm_substream *substream)
{
	printk("%s(): %d\n", __func__, __LINE__);
	return snd_pcm_lib_free_pages(substream);
}

static int dummy_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_pcm_uframes_t period_size = runtime->period_size;
	unsigned int rate = runtime->rate;
	unsigned long secs, nsecs;

	secs = period_size / rate;
	period_size %= rate;
	nsecs = div_u64((u64)period_size * 1000000000UL + rate - 1, rate);
	kt = ktime_set(secs, nsecs);

	printk("%s(): %d\n", __func__, __LINE__);
	return 0;
}

static int dummy_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		timer_start();
		return 0;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		timer_stop();
		return 0;
	}

	return -EINVAL;
}

static snd_pcm_uframes_t dummy_pcm_pointer(struct snd_pcm_substream *substream)
{
	printk("%s(): %d\n", __func__, __LINE__);
	return bytes_to_frames(substream->runtime, pos);
}

static struct snd_pcm_ops dummy_pcm_ops = {
	.open =		dummy_pcm_open,
	.close =	dummy_pcm_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	dummy_pcm_hw_params,
	.hw_free =	dummy_pcm_hw_free,
	.prepare =	dummy_pcm_prepare,
	.trigger =	dummy_pcm_trigger,
	.pointer =	dummy_pcm_pointer,
};

static int dummy_probe(struct platform_device *pdev)
{
	int ret;
	struct snd_card *card;
	struct snd_pcm *pcm;

	ret = snd_card_new(&pdev->dev, SNDRV_DEFAULT_IDX1,
			SNDRV_DEFAULT_STR1, THIS_MODULE, 0, &card);
	if (ret < 0)
		return ret;

 	ret = snd_pcm_new(card, "Dummy PCM", 0, 1, 1, &pcm);
	if (ret < 0)
		return ret;

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &dummy_pcm_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &dummy_pcm_ops);

	pcm->info_flags = 0;
	strcpy(pcm->name, "Dummy PCM");
	strcpy(card->driver, "Dummy");
	strcpy(card->shortname, "Dummy");
	sprintf(card->longname, "Dummy 0");

	snd_pcm_lib_preallocate_pages_for_all(pcm,
		SNDRV_DMA_TYPE_CONTINUOUS,
		snd_dma_continuous_data(GFP_KERNEL),
		0, 64*1024);

	ret = snd_card_register(card);
	if (ret) {
		printk("%s(): %d\n", __func__, __LINE__);
		return ret;
	}

	platform_set_drvdata(pdev, card);

	printk("%s(): %d\n", __func__, __LINE__);
	return 0;
}

static int dummy_remove(struct platform_device *pdev)
{
	struct snd_card *card = platform_get_drvdata(pdev);

	snd_card_free(card);

	printk("%s(): %d\n", __func__, __LINE__);

	return 0;
}

static struct platform_driver dummy_drv = {
	.probe	= dummy_probe,
	.remove	= dummy_remove,
	.driver	= {
		.name = DEVICE_NAME,
	}
};

static int __init dummy_init(void)
{
	int ret;
	struct platform_device *pdev;

	pdev = platform_device_register_simple(DEVICE_NAME, -1, NULL, 0);
	if (IS_ERR(pdev))
		return PTR_ERR(pdev);

	ret = platform_driver_register(&dummy_drv);
	if (ret) {
		platform_device_unregister(pdev);
		return ret;
	}

	return 0;
}

static void __exit dummy_exit(void)
{
	struct device *dev;

	dev = bus_find_device_by_name(dummy_drv.driver.bus, NULL, DEVICE_NAME);
	if (dev)
		platform_device_unregister(to_platform_device(dev));

	platform_driver_unregister(&dummy_drv);
}

module_init(dummy_init);
module_exit(dummy_exit);

MODULE_ALIAS("foo-driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(VERSION);
MODULE_DESCRIPTION("Linux is not Unix");
MODULE_AUTHOR("andrew, mrju.email@gmail.com");
