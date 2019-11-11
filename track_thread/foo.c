#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "tinyalsa/asoundlib.h"
#include "tinyalsa/pcm.h"
#include "wav_reverb.h"

#define container_of(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	((type *)(__mptr - offsetof(type, member))); })

#define DELAY_TIME      (120)  // delay time in ms
#define DELAY_REPEAT    0.6 // (0.50)
#define DELAY_BUF_SIZE  (48000 * DELAY_TIME / 1000 * 2)

static short *delay_buf_l, *delay_buf_r;
static int delay_buf_idx = 0;

void reverb_init(int *param)
{
    int i;

    delay_buf_l = (short *)malloc(DELAY_BUF_SIZE * sizeof(short));
    delay_buf_r = (short *)malloc(DELAY_BUF_SIZE * sizeof(short));

    for (i = 0; i < DELAY_BUF_SIZE; i++) {
        delay_buf_l[i] = 0;
        delay_buf_r[i] = 0;
    }
}

void reverb_exit(void)
{
    free(delay_buf_l);
    free(delay_buf_r);
}

void reverb_main(short (*inbuf)[2], short (*outbuf)[2], int frames)
{
    short *inbuf_l = inbuf[0];
    short *inbuf_r = inbuf[1];
    short *outbuf_l = outbuf[0];
    short *outbuf_r = outbuf[1];
    int delay;
    int i;

    for (i = 0; i < frames; i++) {
        delay = (short)(float)(inbuf_l[i] * (1 - DELAY_REPEAT)) +
            (short)((float)delay_buf_l[delay_buf_idx] * DELAY_REPEAT);
        delay_buf_l[delay_buf_idx] = delay;
        outbuf_l[i] = delay;

        delay = (short)(float)(inbuf_r[i] * (1 - DELAY_REPEAT)) +
            (short)((float)delay_buf_r[delay_buf_idx] * DELAY_REPEAT);
        delay_buf_r[delay_buf_idx] = delay;
        outbuf_r[i] = delay;

        delay_buf_idx = (delay_buf_idx + 1) % DELAY_BUF_SIZE;
    }
}

struct pcm_config config[2] = {
	{
		.channels = 2,
		.rate = 48000,
		.period_size = 256,
		.period_count = 8,
		.format = PCM_FORMAT_S16_LE,
		.start_threshold = 0,
		.stop_threshold = 0,
		.silence_threshold = 0,
		// .silence_size = 0,
		// .avail_min = 0,
	},
	{
		.channels = 2,
		.rate = 48000,
		.period_size = 256,
		.period_count = 8,
		.format = PCM_FORMAT_S16_LE,
		.start_threshold = 0,
		.stop_threshold = 0,
		.silence_threshold = 0,
		// .silence_size = 0,
		// .avail_min = 0,
	},
};

struct task_info {
	char *name;
	pthread_t id;
	void *(*func)(void *);
	void *priv;
};

#define BUFFER_SIZE 4096

void *func(void *data)
{
	int ret, i, delay, frames;
	struct task_info *task;
	struct pcm *_pcm[2];
	char ibuf[BUFFER_SIZE], obuf[BUFFER_SIZE];
	short **__ibuf = ibuf, **__obuf = obuf;
	// short (*__ibuf)[2] = ibuf, (*__obuf)[2] = obuf;

	task = (struct task_info *)data;

	ret = prctl(PR_SET_NAME, task->name);
	if (ret < 0)
		return NULL;

	setpriority(PRIO_PROCESS, getpid(), -19);

	reverb_init(NULL);

	_pcm[0] = pcm_open(0, 0, PCM_OUT, &config[0]);
	if (pcm_get_file_descriptor(_pcm[0]) < 0)
		perror("error to open pcm 0");

	_pcm[1] = pcm_open(1, 0, PCM_IN, &config[1]);
	if (pcm_get_file_descriptor(_pcm[1]) < 0)
		perror("error to open pcm 1");

	while (1) {
		frames = pcm_readi(_pcm[1], ibuf,
				pcm_bytes_to_frames(_pcm[1], BUFFER_SIZE));
#if 0
		for (i = 0; i < frames * 2; i++) {
			delay = __ibuf[0][i] / 2 +
				(short)((float)delay_buf_l[delay_buf_idx]
				* DELAY_REPEAT);
			delay_buf_l[delay_buf_idx] = delay;
			__obuf[0][i] = delay;

			delay_buf_idx = (delay_buf_idx + 1) % DELAY_BUF_SIZE;
		}
#endif
		reverb_main(__ibuf, __obuf, frames * 2);
		// memcpy(obuf, ibuf, pcm_frames_to_bytes(_pcm[1], frames));
		pcm_writei(_pcm[0], obuf, frames);
	}

        pcm_close(_pcm[0]);
        pcm_close(_pcm[1]);
	reverb_exit();

	return NULL;
}

int main(int argc, char **argv)
{
	struct task_info *info;

	info = (struct task_info *)malloc(sizeof(*info));
	info->name = "dummy";
	info->func = func;

	pthread_create(&info->id, NULL, info->func, (void *)info);
	pthread_join(info->id, NULL);

	free(info);

	return 0;
}
