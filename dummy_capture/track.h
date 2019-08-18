#ifndef __TRACK
#define __TRACK

#include <sound/asound.h>

#define PCM_OUT        0x00000000
#define PCM_IN         0x10000000
#define PCM_MMAP       0x00000001
#define PCM_NOIRQ      0x00000002
#define PCM_NORESTART  0x00000004 /* PCM_NORESTART - when set, calls to
                                   * pcm_write for a playback stream will not
                                   * attempt to restart the stream in the case
                                   * of an underflow, but will return -EPIPE
                                   * instead.  After the first -EPIPE error, the
                                   * stream is considered to be stopped, and a
                                   * second call to pcm_write will attempt to
                                   * restart the stream.
                                   */
#define PCM_MONOTONIC  0x00000008 /* see pcm_get_htimestamp */


#define SNDRV_PCM_IOCTL_READI_FRAMES_M	_IOR('A', 0x51, (struct snd_xferi))

/* Bit formats */
enum pcm_format {
    PCM_FORMAT_INVALID = -1,
    PCM_FORMAT_S16_LE = 0,  /* 16-bit signed */
    PCM_FORMAT_S32_LE,      /* 32-bit signed */
    PCM_FORMAT_S8,          /* 8-bit signed */
    PCM_FORMAT_S24_LE,      /* 24-bits in 4-bytes */
    PCM_FORMAT_S24_3LE,     /* 24-bits in 3-bytes */

    PCM_FORMAT_MAX,
};

/* PCM parameters */
enum pcm_param
{
    /* mask parameters */
    PCM_PARAM_ACCESS,
    PCM_PARAM_FORMAT,
    PCM_PARAM_SUBFORMAT,
    /* interval parameters */
    PCM_PARAM_SAMPLE_BITS,
    PCM_PARAM_FRAME_BITS,
    PCM_PARAM_CHANNELS,
    PCM_PARAM_RATE,
    PCM_PARAM_PERIOD_TIME,
    PCM_PARAM_PERIOD_SIZE,
    PCM_PARAM_PERIOD_BYTES,
    PCM_PARAM_PERIODS,
    PCM_PARAM_BUFFER_TIME,
    PCM_PARAM_BUFFER_SIZE,
    PCM_PARAM_BUFFER_BYTES,
    PCM_PARAM_TICK_TIME,
};

/* Configuration for a stream */
struct pcm_config {
    unsigned int channels;
    unsigned int rate;
    unsigned int period_size;
    unsigned int period_count;
    enum pcm_format format;

    /* Values to use for the ALSA start, stop and silence thresholds, and
     * silence size.  Setting any one of these values to 0 will cause the
     * default tinyalsa values to be used instead.
     * Tinyalsa defaults are as follows.
     *
     * start_threshold   : period_count * period_size
     * stop_threshold    : period_count * period_size
     * silence_threshold : 0
     * silence_size      : 0
     */
    unsigned int start_threshold;
    unsigned int stop_threshold;
    unsigned int silence_threshold;
    unsigned int silence_size;

    /* Minimum number of frames available before pcm_mmap_write() will actually
     * write into the kernel buffer. Only used if the stream is opened in mmap mode
     * (pcm_open() called with PCM_MMAP flag set).   Use 0 for default.
     */
    int avail_min;
};

struct pcm {
    /* int fd; */
    struct file *fp; /* add */
    unsigned int flags;
    int running:1;
    int prepared:1;
    int underruns;
    unsigned int buffer_size;
    unsigned int boundary;
    /* char error[PCM_ERROR_MAX]; */
    struct pcm_config config;
    /* struct snd_pcm_mmap_status *mmap_status; */
    /* struct snd_pcm_mmap_control *mmap_control; */
    /* struct snd_pcm_sync_ptr *sync_ptr; */
    /* void *mmap_buffer; */
    /* unsigned int noirq_frames_per_msec; */
    int wait_for_avail_min;
};

int sample_is_playable(unsigned int card, unsigned int device, unsigned int channels,
                        unsigned int rate, unsigned int bits, unsigned int period_size,
                        unsigned int period_count);

unsigned int pcm_format_to_bits(enum pcm_format format);

struct pcm *dummy_pcm_open(unsigned int card, unsigned int device,
                     unsigned int flags, struct pcm_config *config);

int pcm_is_ready(struct pcm *pcm);

unsigned int pcm_frames_to_bytes(struct pcm *pcm, unsigned int frames);

unsigned int pcm_get_buffer_size(struct pcm *pcm);

int dummy_pcm_read(struct pcm *pcm, void *data, unsigned int count);

int dummy_pcm_write(struct pcm *pcm, const void *data, unsigned int count);

int dummy_pcm_close(struct pcm *pcm);

#endif
