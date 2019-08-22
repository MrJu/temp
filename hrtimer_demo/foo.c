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
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/hrtimer.h>

#define STR(x) _STR(x)
#define _STR(x) #x

#define VERSION_PREFIX Foo
#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define PATCH_VERSION 0

#define VERSION STR(VERSION_PREFIX-MAJOR_VERSION.MINOR_VERSION.PATCH_VERSION)

#define msecs_to_nsecs(x) (x * 1000000UL)
#define DELAY_MS 1UL

struct time {
	struct timespec this_ts;
	struct timespec last_ts;
	unsigned long int diff_ts;
	struct timeval this_val;
	struct timeval last_val;
	unsigned long int diff_val;
	unsigned int valid;
};

struct __hrtimer {
        struct hrtimer timer;
        struct timer_ops *ops;
        ktime_t interval;
        ktime_t delay;
	struct time time;
        struct snd_pcm_substream *substream;
        void *priv;
        spinlock_t lock;
	atomic_t running;
};

struct timer_ops {
        int (*start)(struct hrtimer *timer);
        int (*stop)(struct hrtimer *timer);
	int (*set)(struct hrtimer *timer,
		const unsigned long delay_secs,
		const unsigned long delay_nsecs,
		const unsigned long interval_secs,
		const unsigned long interval_nsecs);
};

static struct __hrtimer *hrtimer_create(
                enum hrtimer_restart (*handler)(struct hrtimer *),
                struct timer_ops *ops)
{
        struct __hrtimer *__timer = kzalloc(sizeof(*__timer), GFP_KERNEL);
        if (!__timer)
                return ERR_PTR(-ENOMEM);

        hrtimer_init(&__timer->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        __timer->timer.function = handler;
        __timer->ops = ops;
        spin_lock_init(&__timer->lock);

        return __timer;
}

static void hrtimer_destroy(struct hrtimer *timer)
{
        struct __hrtimer *__timer
                        = container_of(timer, struct __hrtimer, timer);
        kfree(__timer);
}

static struct __hrtimer *__timer;

static int timer_start(struct hrtimer *timer)
{
        struct __hrtimer *__timer
                = container_of(timer, struct __hrtimer, timer);

        hrtimer_start(timer, __timer->delay, HRTIMER_MODE_REL);

        return 0;
}

static int timer_stop(struct hrtimer *timer)
{
        hrtimer_cancel(timer);
        return 0;
}

static int timer_set(struct hrtimer *timer,
		const unsigned long delay_secs,
		const unsigned long delay_nsecs,
		const unsigned long interval_secs,
		const unsigned long interval_nsecs)
{
        struct __hrtimer *__timer
                        = container_of(timer, struct __hrtimer, timer);

	__timer->delay = ktime_set(delay_secs, delay_nsecs);
	__timer->interval = ktime_set(interval_secs, interval_nsecs);
	
        return 0;
}

struct timer_ops timer_ops = {
	.start = timer_start,
	.stop = timer_stop,
	.set = timer_set,
};

static enum hrtimer_restart hrtimer_handler(struct hrtimer *timer)
{
        struct __hrtimer *__timer
                        = container_of(timer, struct __hrtimer, timer);

	__timer->time.last_val = __timer->time.this_val;
	do_gettimeofday(&__timer->time.this_val);
	__timer->time.diff_val
		= (__timer->time.this_val.tv_sec
				- __timer->time.last_val.tv_sec)*1000000
		+ (__timer->time.this_val.tv_usec
				- __timer->time.last_val.tv_usec);

	__timer->time.last_ts = __timer->time.this_ts;
	getnstimeofday(&__timer->time.this_ts);
	__timer->time.diff_ts
		= (__timer->time.this_ts.tv_sec
				- __timer->time.last_ts.tv_sec)*1000000000
		+ (__timer->time.this_ts.tv_nsec
				- __timer->time.last_ts.tv_nsec);

	printk("Andrew : %s %s %d %luus %luns\n",
		__FILE__, __func__, __LINE__,
		__timer->time.diff_val,
		__timer->time.diff_ts);

	hrtimer_forward_now(timer, __timer->interval);
	return HRTIMER_RESTART;
}

static int __init foo_init(void)
{
	__timer = hrtimer_create(hrtimer_handler, &timer_ops);
	if (IS_ERR(__timer))
		return PTR_ERR(__timer);
	__timer->ops->set(&__timer->timer, 0, 0, 0, msecs_to_nsecs(DELAY_MS));
	__timer->ops->start(&__timer->timer);

	return 0;
}

static void __exit foo_exit(void)
{
	__timer->ops->stop(&__timer->timer);
	hrtimer_destroy(&__timer->timer);
	__timer = NULL;
}

module_init(foo_init);
module_exit(foo_exit);

MODULE_ALIAS("foo-driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(VERSION);
MODULE_DESCRIPTION("Linux is not Unix");
MODULE_AUTHOR("andrew, mrju.email@gmail.com");
