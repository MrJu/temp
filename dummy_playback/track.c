#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h> 
#include <linux/err.h> 
#include <linux/fs.h>
#include <linux/slab.h>

/* *********************************************************** */

#include <linux/init.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>

#include "track.h"

static struct pcm bad_pcm = {
    .fp = NULL,
};

static inline int param_is_mask(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_MASK) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_MASK);
}

static inline int param_is_interval(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_INTERVAL) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL);
}

static inline struct snd_mask *param_to_mask(struct snd_pcm_hw_params *p, int n)
{
    return &(p->masks[n - SNDRV_PCM_HW_PARAM_FIRST_MASK]);
}

static inline struct snd_interval *param_to_interval(struct snd_pcm_hw_params *p, int n)
{
    return &(p->intervals[n - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL]);
}

static void param_init(struct snd_pcm_hw_params *p)
{
    int n;

    memset(p, 0, sizeof(*p));
    for (n = SNDRV_PCM_HW_PARAM_FIRST_MASK;
         n <= SNDRV_PCM_HW_PARAM_LAST_MASK; n++) {
            struct snd_mask *m = param_to_mask(p, n);
            m->bits[0] = ~0;
            m->bits[1] = ~0;
    }
    for (n = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;
         n <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; n++) {
            struct snd_interval *i = param_to_interval(p, n);
            i->min = 0;
            i->max = ~0;
    }
    p->rmask = ~0U;
    p->cmask = 0;
    p->info = ~0U;
}

static void param_set_mask(struct snd_pcm_hw_params *p, int n, unsigned int bit)
{
    if (bit >= SNDRV_MASK_MAX)
        return;
    if (param_is_mask(n)) {
        struct snd_mask *m = param_to_mask(p, n);
        m->bits[0] = 0;
        m->bits[1] = 0;
        m->bits[bit >> 5] |= (1 << (bit & 31));
    }
}

static unsigned int param_get_max(struct snd_pcm_hw_params *p, int n)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        return i->max;
    }
    return 0;
}

static unsigned int param_get_min(struct snd_pcm_hw_params *p, int n)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        return i->min;
    }
    return 0;
}

static void param_set_min(struct snd_pcm_hw_params *p, int n, unsigned int val)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        i->min = val;
    }
}

static void dummy_param_set_int(struct snd_pcm_hw_params *p, int n, unsigned int val)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        i->min = val;
        i->max = val;
        i->integer = 1;
    }
}

static unsigned int dummy_param_get_int(struct snd_pcm_hw_params *p, int n)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        if (i->integer)
            return i->max;
    }
    return 0;
}

static int pcm_prepare(struct pcm *pcm)
{
	int err;
	
    if (pcm->prepared)
        return 0;

    if ((err = pcm->fp->f_op->unlocked_ioctl(pcm->fp, SNDRV_PCM_IOCTL_PREPARE, (unsigned long)NULL))){
    	printk("%s(): cannot prepare channel\n", __func__);
    	return err;
    }

    pcm->prepared = 1;
    return 0;
}

static int pcm_start(struct pcm *pcm)
{
	int err;
    int prepare_error = pcm_prepare(pcm);
    if (prepare_error)
        return prepare_error;

    if((err = pcm->fp->f_op->unlocked_ioctl(pcm->fp, SNDRV_PCM_IOCTL_START, (unsigned long)NULL))){
    	printk("%s(): cannot start channel\n", __func__);
        return err;
    }

    pcm->running = 1;
    return 0;
}

static struct pcm_params *pcm_params_get(unsigned int card, unsigned int device,
                                  unsigned int flags)
{
    struct snd_pcm_hw_params *params;
    char fn[256];
    struct file *fp;

    snprintf(fn, sizeof(fn), "/dev/snd/pcmC%uD%u%c", card, device,
             flags & PCM_IN ? 'c' : 'p');

    fp = filp_open(fn, O_RDWR, 0666);
    if (!fp) {
        printk("%s(): cannot open device\n", __func__);
        goto err_open;
    }

    params = kzalloc(sizeof(struct snd_pcm_hw_params), GFP_KERNEL);
    if (!params)
        goto err_kzalloc;

    param_init(params);
    if (fp->f_op->unlocked_ioctl(fp, SNDRV_PCM_IOCTL_HW_REFINE, (unsigned long)params)) {
        printk("%s(): SNDRV_PCM_IOCTL_HW_REFINE error\n", __func__);
        goto err_hw_refine;
    }

    filp_close(fp, NULL);

    return (struct pcm_params *)params;

err_hw_refine:
    kfree(params);
err_kzalloc:
    filp_close(fp, NULL);
err_open:
    return NULL;
}

static void pcm_params_free(struct pcm_params *pcm_params)
{
    struct snd_pcm_hw_params *params = (struct snd_pcm_hw_params *)pcm_params;

    if (params)
        kfree(params);
}

static int pcm_param_to_alsa(enum pcm_param param)
{
    switch (param) {
    case PCM_PARAM_ACCESS:
        return SNDRV_PCM_HW_PARAM_ACCESS;
    case PCM_PARAM_FORMAT:
        return SNDRV_PCM_HW_PARAM_FORMAT;
    case PCM_PARAM_SUBFORMAT:
        return SNDRV_PCM_HW_PARAM_SUBFORMAT;
    case PCM_PARAM_SAMPLE_BITS:
        return SNDRV_PCM_HW_PARAM_SAMPLE_BITS;
        break;
    case PCM_PARAM_FRAME_BITS:
        return SNDRV_PCM_HW_PARAM_FRAME_BITS;
        break;
    case PCM_PARAM_CHANNELS:
        return SNDRV_PCM_HW_PARAM_CHANNELS;
        break;
    case PCM_PARAM_RATE:
        return SNDRV_PCM_HW_PARAM_RATE;
        break;
    case PCM_PARAM_PERIOD_TIME:
        return SNDRV_PCM_HW_PARAM_PERIOD_TIME;
        break;
    case PCM_PARAM_PERIOD_SIZE:
        return SNDRV_PCM_HW_PARAM_PERIOD_SIZE;
        break;
    case PCM_PARAM_PERIOD_BYTES:
        return SNDRV_PCM_HW_PARAM_PERIOD_BYTES;
        break;
    case PCM_PARAM_PERIODS:
        return SNDRV_PCM_HW_PARAM_PERIODS;
        break;
    case PCM_PARAM_BUFFER_TIME:
        return SNDRV_PCM_HW_PARAM_BUFFER_TIME;
        break;
    case PCM_PARAM_BUFFER_SIZE:
        return SNDRV_PCM_HW_PARAM_BUFFER_SIZE;
        break;
    case PCM_PARAM_BUFFER_BYTES:
        return SNDRV_PCM_HW_PARAM_BUFFER_BYTES;
        break;
    case PCM_PARAM_TICK_TIME:
        return SNDRV_PCM_HW_PARAM_TICK_TIME;
        break;

    default:
        return -1;
    }
}

static unsigned int pcm_format_to_alsa(enum pcm_format format)
{
    switch (format) {
    case PCM_FORMAT_S32_LE:
        return SNDRV_PCM_FORMAT_S32_LE;
    case PCM_FORMAT_S8:
        return SNDRV_PCM_FORMAT_S8;
    case PCM_FORMAT_S24_3LE:
        return SNDRV_PCM_FORMAT_S24_3LE;
    case PCM_FORMAT_S24_LE:
        return SNDRV_PCM_FORMAT_S24_LE;
    default:
    case PCM_FORMAT_S16_LE:
        return SNDRV_PCM_FORMAT_S16_LE;
    };
}

static unsigned int pcm_params_get_min(struct pcm_params *pcm_params,
                                enum pcm_param param)
{
    struct snd_pcm_hw_params *params = (struct snd_pcm_hw_params *)pcm_params;
    int p;

    if (!params)
        return 0;

    p = pcm_param_to_alsa(param);
    if (p < 0)
        return 0;

    return param_get_min(params, p);
}

static unsigned int pcm_params_get_max(struct pcm_params *pcm_params,
                                enum pcm_param param)
{
    struct snd_pcm_hw_params *params = (struct snd_pcm_hw_params *)pcm_params;
    int p;

    if (!params)
        return 0;

    p = pcm_param_to_alsa(param);
    if (p < 0)
        return 0;

    return param_get_max(params, p);
}

static int check_param(struct pcm_params *params, unsigned int param, unsigned int value,
                 char *param_name, char *param_unit)
{
    unsigned int min;
    unsigned int max;
    int is_within_bounds = 1;

    min = pcm_params_get_min(params, param);
    if (value < min) {
        printk("%s(): %s is %u%s, device only supports >= %u%s\n", __func__, param_name, value,
                param_unit, min, param_unit);
        is_within_bounds = 0;
    }

    max = pcm_params_get_max(params, param);
    if (value > max) {
        printk("%s(): %s is %u%s, device only supports <= %u%s\n", __func__, param_name, value,
                param_unit, max, param_unit);
        is_within_bounds = 0;
    }

    return is_within_bounds;
}

int sample_is_playable(unsigned int card, unsigned int device, unsigned int channels,
                        unsigned int rate, unsigned int bits, unsigned int period_size,
                        unsigned int period_count)
{
    struct pcm_params *params;
    int can_play;

    params = pcm_params_get(card, device, PCM_OUT);
    if (params == NULL) {
        printk("%s(): Unable to open PCM device.\n", __func__);
        return 0;
    }

    can_play = check_param(params, PCM_PARAM_RATE, rate, "Sample rate", "Hz");
    can_play &= check_param(params, PCM_PARAM_CHANNELS, channels, "Sample", " channels");
    can_play &= check_param(params, PCM_PARAM_SAMPLE_BITS, bits, "Bitrate", " bits");
    can_play &= check_param(params, PCM_PARAM_PERIOD_SIZE, period_size, "Period size", "Hz");
    can_play &= check_param(params, PCM_PARAM_PERIODS, period_count, "Period count", "Hz");

    pcm_params_free(params);

    return can_play;
}

EXPORT_SYMBOL(sample_is_playable);

unsigned int pcm_get_buffer_size(struct pcm *pcm)
{
    return pcm->buffer_size;
}

EXPORT_SYMBOL(pcm_get_buffer_size);

unsigned int pcm_format_to_bits(enum pcm_format format)
{
    switch (format) {
    case PCM_FORMAT_S32_LE:
    case PCM_FORMAT_S24_LE:
        return 32;
    case PCM_FORMAT_S24_3LE:
        return 24;
    default:
    case PCM_FORMAT_S16_LE:
        return 16;
    };
}

EXPORT_SYMBOL(pcm_format_to_bits);

unsigned int pcm_frames_to_bytes(struct pcm *pcm, unsigned int frames)
{
    return frames * pcm->config.channels *
        (pcm_format_to_bits(pcm->config.format) >> 3);
}

EXPORT_SYMBOL(pcm_frames_to_bytes);

int pcm_is_ready(struct pcm *pcm)
{
    return pcm->fp != NULL;
}

EXPORT_SYMBOL(pcm_is_ready);

struct pcm *dummy_pcm_open(unsigned int card, unsigned int device,
                     unsigned int flags, struct pcm_config *config)
{
    struct pcm *pcm;
    struct snd_pcm_info info;
    struct snd_pcm_hw_params params;
    struct snd_pcm_sw_params sparams;
    char fn[256];
    int rc;

	pcm = kzalloc(sizeof(*pcm), GFP_KERNEL);
	if (!pcm || !config) {
		return &bad_pcm;
	}


    pcm->config = *config;

    snprintf(fn, sizeof(fn), "/dev/snd/pcmC%uD%u%c", card, device,
             flags & PCM_IN ? 'c' : 'p');

    pcm->flags = flags;
    pcm->fp = filp_open(fn, O_RDWR, 0666);
    if (pcm->fp == NULL) {
        printk("%s(): cannot open device '%s'\n", __func__, fn);
        return pcm;
    }

    if (pcm->fp->f_op->unlocked_ioctl(pcm->fp, SNDRV_PCM_IOCTL_INFO, (unsigned long)&info)) {
        printk("%s(): cannot get info\n", __func__);
        goto fail_close;
    }

    param_init(&params);
    param_set_mask(&params, SNDRV_PCM_HW_PARAM_FORMAT,
                   pcm_format_to_alsa(config->format));
    param_set_mask(&params, SNDRV_PCM_HW_PARAM_SUBFORMAT,
                   SNDRV_PCM_SUBFORMAT_STD);
    param_set_min(&params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, config->period_size);
    dummy_param_set_int(&params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS,
                  pcm_format_to_bits(config->format));
    dummy_param_set_int(&params, SNDRV_PCM_HW_PARAM_FRAME_BITS,
                  pcm_format_to_bits(config->format) * config->channels);
    dummy_param_set_int(&params, SNDRV_PCM_HW_PARAM_CHANNELS,
                  config->channels);
    dummy_param_set_int(&params, SNDRV_PCM_HW_PARAM_PERIODS, config->period_count);
    dummy_param_set_int(&params, SNDRV_PCM_HW_PARAM_RATE, config->rate);

    if (flags & PCM_NOIRQ) {
		printk("%s(): noirq is not supported in kernel\n", __func__);
    }

    if (flags & PCM_MMAP){
        printk("%s(): mmap is not supported in kernel\n", __func__);
    }else{
        param_set_mask(&params, SNDRV_PCM_HW_PARAM_ACCESS,
                       SNDRV_PCM_ACCESS_RW_INTERLEAVED);
    }

    if (pcm->fp->f_op->unlocked_ioctl(pcm->fp, SNDRV_PCM_IOCTL_HW_PARAMS, (unsigned long)&params)){
        printk("%s(): cannot set hw params\n", __func__);
        goto fail_close;
    }

    /* get our refined hw_params */
    config->period_size = dummy_param_get_int(&params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE);
    config->period_count = dummy_param_get_int(&params, SNDRV_PCM_HW_PARAM_PERIODS);
    pcm->buffer_size = config->period_count * config->period_size;

    memset(&sparams, 0, sizeof(sparams));
    sparams.tstamp_mode = SNDRV_PCM_TSTAMP_ENABLE;
    sparams.period_step = 1;

    if (!config->start_threshold) {
        if (pcm->flags & PCM_IN){
            pcm->config.start_threshold = sparams.start_threshold = 1;
        }else{
            pcm->config.start_threshold = sparams.start_threshold =
                config->period_count * config->period_size / 2;
		}
    } else{
        sparams.start_threshold = config->start_threshold;
	}

	/* pick a high stop threshold - todo: does this need further tuning */
	if (!config->stop_threshold) {
        if (pcm->flags & PCM_IN){
            pcm->config.stop_threshold = sparams.stop_threshold =
                config->period_count * config->period_size * 10;
        }else{
			pcm->config.stop_threshold = sparams.stop_threshold =
				config->period_count * config->period_size;
		}
	}else{
		sparams.stop_threshold = config->stop_threshold;
	}

	if (!pcm->config.avail_min) {
		pcm->config.avail_min = sparams.avail_min = 1;
	}else{
		sparams.avail_min = config->avail_min;
	}

    sparams.xfer_align = config->period_size / 2; /* needed for old kernels */
    sparams.silence_threshold = config->silence_threshold;
    sparams.silence_size = config->silence_size;
    pcm->boundary = sparams.boundary = pcm->buffer_size;

    while (pcm->boundary * 2 <= INT_MAX - pcm->buffer_size){ // ?
        pcm->boundary *= 2;
	}
	
    if (pcm->fp->f_op->unlocked_ioctl(pcm->fp, SNDRV_PCM_IOCTL_SW_PARAMS, (unsigned long)&sparams)) {
        printk("%s(): cannot set sw params", __func__);
        goto fail_close;
    }

#ifdef SNDRV_PCM_IOCTL_TTSTAMP
    if (pcm->flags & PCM_MONOTONIC) {
        int arg = SNDRV_PCM_TSTAMP_TYPE_MONOTONIC;
        rc = pcm->fp->f_op->unlocked_ioctl(pcm->fp, SNDRV_PCM_IOCTL_TTSTAMP, (unsigned long)&arg);
        if (rc < 0) {
            printk("%s(): cannot set timestamp type", __func__);
            goto fail_close;
        }
    }
#endif

    pcm->underruns = 0;
    return pcm;

fail_close:
    filp_close(pcm->fp, NULL);
    pcm->fp = NULL;
    return pcm;
}

EXPORT_SYMBOL(dummy_pcm_open);

int dummy_pcm_close(struct pcm *pcm)
{
    if (pcm == &bad_pcm)
        return 0;

    if (pcm->fp)
        filp_close(pcm->fp, NULL);
    pcm->prepared = 0;
    pcm->running = 0;
    pcm->buffer_size = 0;
    pcm->fp = NULL;
    kfree(pcm);
    return 0;
}

EXPORT_SYMBOL(dummy_pcm_close);

int dummy_pcm_write(struct pcm *pcm, const void *data, unsigned int count)
{
    struct snd_xferi x;
	int err;

    if (pcm->flags & PCM_IN)
        return -EINVAL;

    x.buf = (void*)data;
    x.frames = count / (pcm->config.channels *
                        pcm_format_to_bits(pcm->config.format) / 8);

    for (;;) {
        if (!pcm->running) {
            int prepare_error = pcm_prepare(pcm);
            if (prepare_error){
                return prepare_error;
            }
            if ((err = pcm->fp->f_op->unlocked_ioctl(pcm->fp, SNDRV_PCM_IOCTL_WRITEI_FRAMES, (unsigned long)&x))){
                printk("%s(): cannot write initial data\n", __func__);
                return err;
            }
            pcm->running = 1;
            return 0;
        }
        if ((err = pcm->fp->f_op->unlocked_ioctl(pcm->fp, SNDRV_PCM_IOCTL_WRITEI_FRAMES, (unsigned long)&x))) {
            pcm->prepared = 0;
            pcm->running = 0;
            if (err == -EPIPE) {
                /* we failed to make our window -- try to restart if we are
                 * allowed to do so.  Otherwise, simply allow the EPIPE error to
                 * propagate up to the app level */
                pcm->underruns++;
                if (pcm->flags & PCM_NORESTART)
                    return -EPIPE;
                continue;
            }
            
            printk("%s(): cannot write stream data\n", __func__);
            return err;
        }
        return 0;
    }
}

EXPORT_SYMBOL(dummy_pcm_write);

int dummy_pcm_read(struct pcm *pcm, void *data, unsigned int count)
{
    struct snd_xferi x;
	int err;
	
    if (!(pcm->flags & PCM_IN))
        return -EINVAL;

    x.buf = data;
    x.frames = count / (pcm->config.channels *
                        pcm_format_to_bits(pcm->config.format) / 8);

    for (;;) {
        if (!pcm->running) {
            if ((err = pcm_start(pcm))) {
                printk("%s(): start error", __func__);
                return err;
            }
        }
        if ((err = pcm->fp->f_op->unlocked_ioctl(pcm->fp, SNDRV_PCM_IOCTL_READI_FRAMES, (unsigned long)&x))) {
            pcm->prepared = 0;
            pcm->running = 0;
            if (err == -EPIPE) {
                    /* we failed to make our window -- try to restart */
                pcm->underruns++;
                continue;
            }
            printk("%s(): cannot read stream data\n", __func__);
            return err;
        }
        return 0;
    }
}
EXPORT_SYMBOL(dummy_pcm_read);
