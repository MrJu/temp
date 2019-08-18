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

#define CARD 0
#define DEVICE 0

static struct task_struct * play_task;

static int track_thread(void *data)
{
	char *buffer;
	struct pcm *pcm;
	int size;
	unsigned long i = 0;

	struct pcm_config config = {
			.channels 		= 2,
			.rate			= 48000,
			.period_size		= 1024,
			.period_count		= 4,
			.format			= PCM_FORMAT_S16_LE,
			.start_threshold	= 0,
			.stop_threshold		= 0,
			.silence_threshold 	= 0,
	};

	if (!sample_is_playable(
    				CARD, 
    				DEVICE, 
    				config.channels, 
    				config.rate, 
    				pcm_format_to_bits(config.format), 
    				config.period_size, 
    				config.period_count)){
    				
		return -EINVAL;
	}

	printk("\n\n%s(): in the thead function\n\n", __func__);
	
	pcm = dummy_pcm_open(CARD, DEVICE, PCM_OUT, &config);

	if (!pcm || !pcm_is_ready(pcm)) {
		printk("%s(): Unable to open PCM device", __func__);
	        return -EAGAIN;
	}

	size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    
	buffer = kzalloc(size, GFP_KERNEL);
    	memset(buffer, 0, size);
    
	for(i = 0; i < size; i++){
    		buffer[i] = i / 4;
	}

	printk("Playing sample: %u ch, %u hz, %u bit\n", 
						config.channels, 
						config.rate, 
						pcm_format_to_bits(config.format));

	do {
		if(dummy_pcm_write(pcm, buffer, size)){
			printk("%s(): Error playing sample\n", __func__);
			break;
		}
	} while (!kthread_should_stop());

	dummy_pcm_close(pcm);
	printk("\n\n%s(): dummy_pcm_close(pcm)\n\n", __func__);
	return 0;
}

static int __init track_init(void)
{
	int err;

	play_task = kthread_create(track_thread, NULL, "playback");
	if(IS_ERR(play_task)){
		printk("Unable to create play task.\n");
		err = PTR_ERR(play_task);
		play_task = NULL;
		return err;
	}

	wake_up_process(play_task);

	return 0;
}

static void __exit track_exit(void)
{
	if(play_task){
		kthread_stop(play_task);
	}

	return;
}

module_init(track_init);
module_exit(track_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("andrew@loostone.com");
