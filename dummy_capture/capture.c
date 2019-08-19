#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kthread.h> 
#include <linux/err.h> 

#include "track.h"
#include "kernel_io.h"

#define CARD 3
#define DEVICE 0

static struct task_struct * capture_task;

static int track_thread(void *data)
{
	int ret, size;
	char *buf;
	struct file *filp;
	struct pcm *pcm;

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

	filp = filp_open("/home/pi/Documents/dummy.pcm", O_RDWR | O_CREAT, 0666);
	if (IS_ERR(filp))
		return PTR_ERR(filp);

	pcm = dummy_pcm_open(CARD, DEVICE, PCM_IN, &config);

	if (!pcm || !pcm_is_ready(pcm)) {
		printk("%s(): Unable to open PCM device", __func__);
	        return -EAGAIN;
	}

	size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    
	buf = kzalloc(size, GFP_KERNEL);
    
	printk("Playing sample: %u ch, %u hz, %u bit\n", 
				config.channels, 
				config.rate, 
				pcm_format_to_bits(config.format));

	do {
		if(dummy_pcm_read(pcm, buf, size)){
			printk("%s(): Error playing sample\n", __func__);
			break;
		}

		ret = write_file(filp, buf, size);
		if (ret < 0)
			break;

	} while (!kthread_should_stop());

	dummy_pcm_close(pcm);
	kfree(buf);
	filp_close(filp, NULL);

	printk("\n\n%s(): dummy_pcm_close(pcm)\n\n", __func__);

	return 0;
}

static int __init track_init(void)
{
	int err;

	capture_task = kthread_create(track_thread, NULL, "capture");
	if(IS_ERR(capture_task)){
		printk("Unable to create capture task.\n");
		err = PTR_ERR(capture_task);
		capture_task = NULL;
		return err;
	}

	wake_up_process(capture_task);

	return 0;
}

static void __exit track_exit(void)
{
	if(capture_task){
		kthread_stop(capture_task);
	}

	return;
}

module_init(track_init);
module_exit(track_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("andrew@loostone.com");
